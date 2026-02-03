// Investor dashboard — fetch live metrics from API
(async function() {
    try {
        const resp = await fetch('/api/metrics');
        if (!resp.ok) return;
        const data = await resp.json();
        console.log('Metrics loaded:', data);
    } catch (e) {
        console.log('Running in static mode (no API server)');
    }
})();
