// ATOMiK Demo Dashboard — WebSocket client + Chart.js

const WS_URL = `ws://${location.host}/ws`;
let ws = null;
let throughputChart = null;

// Catppuccin Mocha colours
const COLORS = {
    yellow: '#f9e2af',
    blue: '#89b4fa',
    green: '#a6e3a1',
    red: '#f38ba8',
    text: '#cdd6f4',
    subtext0: '#a6adc8',
    surface0: '#313244',
    surface1: '#45475a',
    mantle: '#181825',
};

// ── WebSocket ─────────────────────────────────────────────────────────

function connect() {
    ws = new WebSocket(WS_URL);

    ws.onopen = () => {
        console.log('WebSocket connected');
        setNarration('Connected to demo server.');
    };

    ws.onmessage = (evt) => {
        const msg = JSON.parse(evt.data);
        handleMessage(msg);
    };

    ws.onclose = () => {
        console.log('WebSocket disconnected, reconnecting in 2s...');
        setNarration('Disconnected. Reconnecting...');
        setTimeout(connect, 2000);
    };

    ws.onerror = (err) => {
        console.error('WebSocket error:', err);
    };
}

function send(obj) {
    if (ws && ws.readyState === WebSocket.OPEN) {
        ws.send(JSON.stringify(obj));
    }
}

// ── Message Handling ──────────────────────────────────────────────────

function handleMessage(msg) {
    switch (msg.type) {
        case 'init':
        case 'snapshots':
            updateNodes(msg.snapshots || []);
            if (msg.state) {
                updateModeBadge(msg.state.hw_count, msg.state.sim_count);
            }
            break;
        case 'act_start':
            setNarration(`Act ${msg.number}: ${msg.title} — ${msg.narration || 'Running...'}`);
            break;
        case 'act_complete':
            addActResult(msg.number, msg.title, msg.passed, msg.summary);
            setNarration(`Act ${msg.number}: ${msg.title} — ${msg.passed ? 'PASS' : 'FAIL'}: ${msg.summary}`);
            // Refresh snapshots
            send({ action: 'refresh' });
            break;
        case 'demo_start':
            clearActResults();
            setNarration('Running all 5 acts...');
            break;
        case 'demo_complete':
            setNarration(`Demo complete: ${msg.passed}/${msg.total} acts passed.`);
            send({ action: 'refresh' });
            break;
        default:
            // Other orchestrator events
            break;
    }
}

// ── UI Updates ────────────────────────────────────────────────────────

function updateNodes(snapshots) {
    snapshots.forEach((s, i) => {
        const el = (id) => document.getElementById(`${id}-${i}`);
        if (el('domain')) el('domain').textContent = s.domain;
        if (el('banks')) el('banks').textContent = s.n_banks;
        if (el('freq')) el('freq').textContent = `${s.freq_mhz.toFixed(1)} MHz`;
        if (el('tp')) el('tp').textContent = `${s.throughput_mops.toLocaleString()} Mops/s`;
        if (el('state')) el('state').textContent = s.state_hex;
        if (el('acc')) el('acc').textContent = s.accumulator_zero ? 'ZERO' : 'NON-ZERO';
        if (el('deltas')) el('deltas').textContent = s.delta_count;

        const badge = el('badge');
        if (badge) {
            badge.textContent = s.is_hardware ? 'HW' : 'SIM';
            badge.className = `status-badge ${s.is_hardware ? 'badge-hw' : 'badge-sim'}`;
        }
    });

    updateChart(snapshots);
}

function updateModeBadge(hw, sim) {
    const badge = document.getElementById('mode-badge');
    if (hw > 0 && sim > 0) {
        badge.textContent = `${hw} HW + ${sim} SIM`;
        badge.className = 'status-badge badge-hw';
    } else if (hw > 0) {
        badge.textContent = `${hw} HW`;
        badge.className = 'status-badge badge-hw';
    } else {
        badge.textContent = `${sim} SIM`;
        badge.className = 'status-badge badge-sim';
    }
}

function setNarration(text) {
    document.getElementById('narration').textContent = text;
}

function addActResult(num, title, passed, summary) {
    const container = document.getElementById('act-results');
    // Clear placeholder
    if (container.querySelector('p')) container.innerHTML = '';

    const div = document.createElement('div');
    div.className = 'act-result';
    const cls = passed ? 'pass' : 'fail';
    const icon = passed ? '+' : 'X';
    div.innerHTML = `<span class="${cls}">[${icon}]</span> Act ${num}: ${title} — ${summary}`;
    container.appendChild(div);
}

function clearActResults() {
    document.getElementById('act-results').innerHTML =
        '<p style="color: var(--subtext0); font-size: 0.85rem;">Running...</p>';
}

// ── Chart.js ──────────────────────────────────────────────────────────

function initChart() {
    const ctx = document.getElementById('throughput-chart').getContext('2d');
    throughputChart = new Chart(ctx, {
        type: 'bar',
        data: {
            labels: ['N=4 Finance', 'N=8 Sensor', 'N=16 Peak'],
            datasets: [{
                label: 'Throughput (Mops/s)',
                data: [324, 540, 1070],
                backgroundColor: [COLORS.yellow, COLORS.blue, COLORS.green],
                borderColor: [COLORS.yellow, COLORS.blue, COLORS.green],
                borderWidth: 1,
                borderRadius: 4,
            }]
        },
        options: {
            indexAxis: 'y',
            responsive: true,
            maintainAspectRatio: false,
            plugins: {
                legend: { display: false },
                annotation: {
                    annotations: {
                        gopsLine: {
                            type: 'line',
                            xMin: 1000,
                            xMax: 1000,
                            borderColor: COLORS.red,
                            borderWidth: 2,
                            borderDash: [6, 4],
                            label: {
                                content: '1 Gops/s',
                                enabled: true,
                                color: COLORS.red,
                                font: { size: 11 },
                            }
                        }
                    }
                }
            },
            scales: {
                x: {
                    beginAtZero: true,
                    max: 1200,
                    ticks: { color: COLORS.subtext0 },
                    grid: { color: COLORS.surface1 },
                },
                y: {
                    ticks: { color: COLORS.text },
                    grid: { display: false },
                }
            }
        }
    });
}

function updateChart(snapshots) {
    if (!throughputChart || !snapshots.length) return;
    throughputChart.data.labels = snapshots.map(
        s => `N=${s.n_banks} ${s.domain}`
    );
    throughputChart.data.datasets[0].data = snapshots.map(s => s.throughput_mops);
    throughputChart.update();
}

// ── Controls ──────────────────────────────────────────────────────────

function runAct(n) {
    send({ action: 'run_act', act: n });
    setNarration(`Starting Act ${n}...`);
}

function runAll() {
    send({ action: 'run_all' });
    setNarration('Running all acts...');
}

// ── Init ──────────────────────────────────────────────────────────────

document.addEventListener('DOMContentLoaded', () => {
    initChart();
    connect();
});
