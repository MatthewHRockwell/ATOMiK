"""HeyGen API integration for AI avatar video generation."""

from __future__ import annotations

from typing import Any

try:
    import httpx
except ImportError:
    httpx = None  # type: ignore[assignment]


HEYGEN_API_BASE = "https://api.heygen.com/v2"

# Mandatory disclosure line for all generated videos
AI_DISCLOSURE = (
    "This presentation was created using AI avatar technology. "
    "All content has been reviewed and approved by me personally."
)


class VideoGenerator:
    """Generate founder presentation videos via HeyGen REST API.

    Requires a HeyGen Pro plan ($99/mo for 100 credits).
    """

    def __init__(self, api_key: str) -> None:
        if httpx is None:
            raise ImportError(
                "httpx is required for video generation. "
                "Install with: pip install httpx"
            )
        self.api_key = api_key
        self._client = httpx.Client(
            base_url=HEYGEN_API_BASE,
            headers={
                "X-Api-Key": api_key,
                "Content-Type": "application/json",
            },
            timeout=60.0,
        )

    def create_avatar(self, photo_path: str) -> str:
        """Upload a photo to create a talking avatar.

        Returns the avatar_id for use in video generation.
        """
        with open(photo_path, "rb") as f:
            resp = self._client.post(
                "/photo_avatar",
                files={"file": (photo_path, f, "image/jpeg")},
            )
        resp.raise_for_status()
        data = resp.json()
        return data.get("data", {}).get("photo_avatar_id", "")

    def generate_video(
        self,
        avatar_id: str,
        script: str,
        output_path: str = "",
    ) -> str:
        """Generate a video from a script using the specified avatar.

        Returns the video_id for status polling.
        """
        payload = {
            "video_inputs": [
                {
                    "character": {
                        "type": "avatar",
                        "avatar_id": avatar_id,
                    },
                    "voice": {
                        "type": "text",
                        "input_text": script,
                    },
                }
            ],
            "dimension": {"width": 1920, "height": 1080},
        }
        resp = self._client.post("/video/generate", json=payload)
        resp.raise_for_status()
        data = resp.json()
        return data.get("data", {}).get("video_id", "")

    def get_status(self, video_id: str) -> dict[str, Any]:
        """Poll video generation status.

        Returns dict with keys: status, video_url (when complete).
        """
        resp = self._client.get(f"/video_status.get?video_id={video_id}")
        resp.raise_for_status()
        data = resp.json().get("data", {})
        return {
            "status": data.get("status", "unknown"),
            "video_url": data.get("video_url", ""),
            "duration": data.get("duration", 0),
        }

    def close(self) -> None:
        self._client.close()
