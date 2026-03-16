"""ATOMiK FastAPI middleware — auto-deduplicate WebSocket frames and track response redundancy.

Usage:
    pip install atomik-core[fastapi]

    from atomik_core.integrations.fastapi_middleware import AtomikMiddleware
    app = FastAPI()
    app.add_middleware(AtomikMiddleware)
    # Every response is fingerprinted. Redundant responses get 304 Not Modified.
"""

from __future__ import annotations

import json
import time
from collections import OrderedDict

from atomik_core.fingerprint import Fingerprint


class _LRUDict:
    """Bounded dict that evicts the least-recently-used entry on overflow.

    Backed by ``collections.OrderedDict`` for O(1) move-to-end on access.
    """

    __slots__ = ("_data", "_max")

    def __init__(self, maxsize: int = 10000):
        self._data: OrderedDict[str, Fingerprint] = OrderedDict()
        self._max = maxsize

    def get(self, key: str) -> Fingerprint | None:
        """Look up *key*, promoting it to most-recently-used."""
        if key in self._data:
            self._data.move_to_end(key)
            return self._data[key]
        return None

    def put(self, key: str, value: Fingerprint) -> None:
        """Insert or update *key*, evicting LRU if at capacity."""
        if key in self._data:
            self._data.move_to_end(key)
            self._data[key] = value
        else:
            if len(self._data) >= self._max:
                self._data.popitem(last=False)
            self._data[key] = value

    def __len__(self) -> int:
        return len(self._data)


class AtomikMiddleware:
    """ASGI middleware that deduplicates identical responses with 304 Not Modified.

    For every non-304 response, the body is fingerprinted using ATOMiK's
    XOR-reduce algebra.  If the fingerprint matches the previous response
    for the same ``(method, path)`` pair, the client receives a ``304``
    with no body, saving bandwidth and serialization cost.

    The middleware also serves an optional ``/atomik/stats`` JSON endpoint
    that reports deduplication statistics.

    Args:
        app: The ASGI application to wrap.
        max_entries: Maximum number of ``(method, path)`` fingerprints to
            keep in memory (LRU eviction).  Default 10 000.
        stats_endpoint: Path for the stats JSON endpoint.  Set to ``None``
            to disable.
        fingerprint_width: Bit width of the fingerprint (default 64).

    Example::

        from starlette.applications import Starlette
        from atomik_core.integrations.fastapi_middleware import AtomikMiddleware

        app = Starlette()
        app.add_middleware(AtomikMiddleware, max_entries=5000)
    """

    __slots__ = (
        "_app",
        "_cache",
        "_stats_endpoint",
        "_fp_width",
        "_total_requests",
        "_deduplicated",
        "_bytes_saved",
        "_start_time",
    )

    def __init__(
        self,
        app,
        max_entries: int = 10000,
        stats_endpoint: str | None = "/atomik/stats",
        fingerprint_width: int = 64,
    ):
        self._app = app
        self._cache = _LRUDict(maxsize=max_entries)
        self._stats_endpoint = stats_endpoint
        self._fp_width = fingerprint_width
        self._total_requests = 0
        self._deduplicated = 0
        self._bytes_saved = 0
        self._start_time = time.monotonic()

    # ------------------------------------------------------------------
    # Stats
    # ------------------------------------------------------------------

    def stats(self) -> dict:
        """Return cumulative deduplication statistics.

        Returns:
            A dict with keys ``total_requests``, ``deduplicated``,
            ``bytes_saved``, ``dedup_rate``, and ``uptime_seconds``.
        """
        total = self._total_requests
        rate = (self._deduplicated / total * 100) if total > 0 else 0.0
        return {
            "total_requests": total,
            "deduplicated": self._deduplicated,
            "bytes_saved": self._bytes_saved,
            "dedup_rate": f"{rate:.1f}%",
            "cached_paths": len(self._cache),
            "uptime_seconds": round(time.monotonic() - self._start_time, 1),
        }

    # ------------------------------------------------------------------
    # ASGI entry point
    # ------------------------------------------------------------------

    async def __call__(self, scope, receive, send):
        """ASGI interface."""
        if scope["type"] != "http":
            await self._app(scope, receive, send)
            return

        path = scope.get("path", "/")
        method = scope.get("method", "GET")

        # Serve the built-in stats endpoint
        if self._stats_endpoint and path == self._stats_endpoint:
            await self._send_stats(send)
            return

        self._total_requests += 1
        cache_key = f"{method}:{path}"

        # Collect the response produced by the downstream app
        response_started = False
        status_code = 200
        response_headers: list[tuple[bytes, bytes]] = []
        body_parts: list[bytes] = []

        async def capture_send(message):
            nonlocal response_started, status_code, response_headers
            if message["type"] == "http.response.start":
                response_started = True
                status_code = message.get("status", 200)
                response_headers = list(message.get("headers", []))
            elif message["type"] == "http.response.body":
                body_parts.append(message.get("body", b""))

        await self._app(scope, receive, capture_send)

        if not response_started:
            return

        body = b"".join(body_parts)

        # Only deduplicate successful responses with a body
        if 200 <= status_code < 300 and body:
            fp = self._cache.get(cache_key)
            if fp is not None:
                fp.update(body)
                if not fp.changed:
                    # Fingerprint matches -- send 304
                    self._deduplicated += 1
                    self._bytes_saved += len(body)
                    await send({
                        "type": "http.response.start",
                        "status": 304,
                        "headers": [
                            (k, v)
                            for k, v in response_headers
                            if k.lower() not in (b"content-length", b"content-type")
                        ],
                    })
                    await send({
                        "type": "http.response.body",
                        "body": b"",
                    })
                    return
                # Data changed -- re-baseline
                fp.load(body)
            else:
                fp = Fingerprint(width=self._fp_width)
                fp.load(body)
                self._cache.put(cache_key, fp)

        # Forward the original response
        await send({
            "type": "http.response.start",
            "status": status_code,
            "headers": response_headers,
        })
        await send({
            "type": "http.response.body",
            "body": body,
        })

    # ------------------------------------------------------------------
    # Internal helpers
    # ------------------------------------------------------------------

    async def _send_stats(self, send):
        """Respond with JSON stats."""
        payload = json.dumps(self.stats(), indent=2).encode("utf-8")
        await send({
            "type": "http.response.start",
            "status": 200,
            "headers": [
                (b"content-type", b"application/json"),
                (b"content-length", str(len(payload)).encode()),
            ],
        })
        await send({
            "type": "http.response.body",
            "body": payload,
        })

    def __repr__(self) -> str:
        s = self.stats()
        return (
            f"AtomikMiddleware(requests={s['total_requests']}, "
            f"dedup={s['deduplicated']}, saved={s['bytes_saved']}B)"
        )
