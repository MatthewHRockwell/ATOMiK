// ATOMiK Demo Dashboard — WebSocket client + Chart.js

let ws = null;
let throughputChart = null;
let reconnectAttempts = 0;
const MAX_RECONNECT_ATTEMPTS = 10;

// Catppuccin Mocha colors
const COLORS = {
    node1: '#f9e2af',  // yellow
    node2: '#89b4fa',  // blue
    node3: '#a6e3a1',  // green
    red: '#f38ba8',
    text: '#cdd6f4',
    subtext0: '#a6adc8',
    surface0: '#313244',
    surface1: '#45475a',
    mantle: '#181825',
};

// ═══════════════════════════════════════════════════════════════════════
// WebSocket Connection
// ═══════════════════════════════════════════════════════════════════════

function connect() {
    const wsUrl = `ws://${location.host}/ws`;
    console.log('Connecting to WebSocket:', wsUrl);

    try {
        ws = new WebSocket(wsUrl);
    } catch (e) {
        console.error('WebSocket creation failed:', e);
        scheduleReconnect();
        return;
    }

    ws.onopen = () => {
        console.log('WebSocket connected');
        reconnectAttempts = 0;
        setConnectionStatus(true);
        setNarration('Connected to demo server. Ready to run acts.');
    };

    ws.onmessage = (evt) => {
        try {
            const msg = JSON.parse(evt.data);
            handleMessage(msg);
        } catch (e) {
            console.error('Failed to parse message:', e);
        }
    };

    ws.onclose = (evt) => {
        console.log('WebSocket closed:', evt.code, evt.reason);
        setConnectionStatus(false);
        scheduleReconnect();
    };

    ws.onerror = (err) => {
        console.error('WebSocket error:', err);
    };
}

function scheduleReconnect() {
    if (reconnectAttempts < MAX_RECONNECT_ATTEMPTS) {
        reconnectAttempts++;
        const delay = Math.min(1000 * Math.pow(2, reconnectAttempts - 1), 10000);
        setNarration(`Disconnected. Reconnecting in ${delay/1000}s... (attempt ${reconnectAttempts})`);
        setTimeout(connect, delay);
    } else {
        setNarration('Connection lost. Please refresh the page.');
    }
}

function send(obj) {
    if (ws && ws.readyState === WebSocket.OPEN) {
        ws.send(JSON.stringify(obj));
        return true;
    }
    console.warn('WebSocket not connected');
    return false;
}

function setConnectionStatus(connected) {
    const el = document.getElementById('connection-status');
    if (connected) {
        el.textContent = 'Connected';
        el.className = 'status-indicator connected';
    } else {
        el.textContent = 'Disconnected';
        el.className = 'status-indicator disconnected';
    }
}

// ═══════════════════════════════════════════════════════════════════════
// Message Handling
// ═══════════════════════════════════════════════════════════════════════

function handleMessage(msg) {
    console.log('Received:', msg.type, msg);

    switch (msg.type) {
        case 'init':
            updateNodes(msg.snapshots || []);
            updateSummary(msg.state || {}, msg.snapshots || []);
            break;

        case 'snapshots':
            updateNodes(msg.snapshots || []);
            updateSummary({}, msg.snapshots || []);
            break;

        case 'act_start':
            setNarration(`Running Act ${msg.number}: ${msg.title}...`);
            setButtonRunning(msg.number, true);
            break;

        case 'act_complete':
            addActResult(msg.number, msg.title, msg.passed, msg.summary);
            setNarration(`Act ${msg.number}: ${msg.title} — ${msg.passed ? 'PASS' : 'FAIL'}`);
            setButtonRunning(msg.number, false);
            send({ action: 'refresh' });
            break;

        case 'demo_start':
            clearActResults();
            setNarration('Running all 5 acts...');
            break;

        case 'demo_complete':
            setNarration(`Demo complete: ${msg.passed}/${msg.total} acts passed.`);
            document.querySelectorAll('.act-btn').forEach(btn => btn.classList.remove('running'));
            send({ action: 'refresh' });
            break;
    }
}

// ═══════════════════════════════════════════════════════════════════════
// UI Updates
// ═══════════════════════════════════════════════════════════════════════

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
            badge.className = `node-badge ${s.is_hardware ? 'hw' : 'sim'}`;
        }
    });

    updateChart(snapshots);
}

function updateSummary(state, snapshots) {
    const totalTp = snapshots.reduce((sum, s) => sum + (s.throughput_mops || 0), 0);
    const hwCount = state.hw_count ?? snapshots.filter(s => s.is_hardware).length;
    const simCount = state.sim_count ?? snapshots.filter(s => !s.is_hardware).length;

    document.getElementById('total-throughput').textContent = totalTp.toLocaleString();
    document.getElementById('total-nodes').textContent = snapshots.length;
    document.getElementById('hw-count').textContent = hwCount;
    document.getElementById('sim-count').textContent = simCount;
}

function setNarration(text) {
    document.getElementById('narration').textContent = text;
}

function setButtonRunning(actNum, running) {
    const btn = document.getElementById(`btn-act${actNum}`);
    if (btn) {
        if (running) {
            btn.classList.add('running');
        } else {
            btn.classList.remove('running');
        }
    }
}

function addActResult(num, title, passed, summary) {
    const container = document.getElementById('act-results');

    // Remove "no results" placeholder
    const noResults = container.querySelector('.no-results');
    if (noResults) noResults.remove();

    const div = document.createElement('div');
    div.className = `result-item ${passed ? 'pass' : 'fail'}`;
    div.innerHTML = `
        <div class="result-header">
            <span class="result-title">Act ${num}: ${title}</span>
            <span class="result-status ${passed ? 'pass' : 'fail'}">${passed ? 'PASS' : 'FAIL'}</span>
        </div>
        <div class="result-summary">${summary}</div>
    `;
    container.appendChild(div);

    // Scroll to bottom
    container.scrollTop = container.scrollHeight;
}

function clearActResults() {
    const container = document.getElementById('act-results');
    container.innerHTML = '<div class="no-results">Running...</div>';
}

// ═══════════════════════════════════════════════════════════════════════
// Chart.js — Vertical bars side by side, fixed height, rescaling axis
// ═══════════════════════════════════════════════════════════════════════

function initChart() {
    const ctx = document.getElementById('throughput-chart').getContext('2d');

    throughputChart = new Chart(ctx, {
        type: 'bar',
        data: {
            labels: ['N=4 Finance', 'N=8 Sensor', 'N=16 Peak'],
            datasets: [{
                label: 'Throughput (Mops/s)',
                data: [324, 540, 1070],
                backgroundColor: [COLORS.node1, COLORS.node2, COLORS.node3],
                borderColor: [COLORS.node1, COLORS.node2, COLORS.node3],
                borderWidth: 1,
                borderRadius: 4,
                barPercentage: 0.7,
                categoryPercentage: 0.8,
            }]
        },
        options: {
            responsive: true,
            maintainAspectRatio: false,
            animation: {
                duration: 300
            },
            plugins: {
                legend: {
                    display: false
                },
                tooltip: {
                    backgroundColor: COLORS.surface0,
                    titleColor: COLORS.text,
                    bodyColor: COLORS.subtext0,
                    borderColor: COLORS.surface1,
                    borderWidth: 1,
                    callbacks: {
                        label: function(context) {
                            const value = context.parsed.y;
                            if (value >= 1000) {
                                return `${(value / 1000).toFixed(2)} Gops/s`;
                            }
                            return `${value.toLocaleString()} Mops/s`;
                        }
                    }
                },
                // 1 Gops/s target line annotation
                annotation: {
                    annotations: {
                        targetLine: {
                            type: 'line',
                            yMin: 1000,
                            yMax: 1000,
                            borderColor: COLORS.red,
                            borderWidth: 2,
                            borderDash: [6, 4],
                            label: {
                                display: true,
                                content: '1 Gops/s',
                                position: 'end',
                                backgroundColor: 'transparent',
                                color: COLORS.red,
                                font: { size: 11, weight: 'bold' }
                            }
                        }
                    }
                }
            },
            scales: {
                x: {
                    grid: {
                        display: false
                    },
                    ticks: {
                        color: COLORS.subtext0,
                        font: { size: 11 }
                    }
                },
                y: {
                    beginAtZero: true,
                    suggestedMax: 1200,  // Initial max, will auto-scale if needed
                    grid: {
                        color: COLORS.surface1,
                        lineWidth: 1
                    },
                    ticks: {
                        color: COLORS.subtext0,
                        font: { size: 11 },
                        callback: function(value) {
                            if (value >= 1000) {
                                return (value / 1000).toFixed(1) + ' G';
                            }
                            return value;
                        }
                    },
                    title: {
                        display: true,
                        text: 'Mops/s',
                        color: COLORS.subtext0,
                        font: { size: 11 }
                    }
                }
            }
        }
    });
}

function updateChart(snapshots) {
    if (!throughputChart || !snapshots.length) return;

    // Update labels and data
    throughputChart.data.labels = snapshots.map(s => `N=${s.n_banks} ${s.domain}`);
    throughputChart.data.datasets[0].data = snapshots.map(s => s.throughput_mops);

    // Auto-scale Y axis: always show at least up to 1200, but expand if needed
    const maxValue = Math.max(...snapshots.map(s => s.throughput_mops));
    const suggestedMax = Math.max(1200, Math.ceil(maxValue * 1.15 / 100) * 100);
    throughputChart.options.scales.y.suggestedMax = suggestedMax;

    throughputChart.update('none');  // No animation for data updates
}

// ═══════════════════════════════════════════════════════════════════════
// Button Handlers
// ═══════════════════════════════════════════════════════════════════════

function setupButtonHandlers() {
    // Individual act buttons
    document.querySelectorAll('.act-btn').forEach(btn => {
        btn.addEventListener('click', () => {
            const actNum = parseInt(btn.dataset.act);
            if (actNum && send({ action: 'run_act', act: actNum })) {
                setNarration(`Starting Act ${actNum}...`);
            }
        });
    });

    // Run All button
    document.getElementById('btn-run-all').addEventListener('click', () => {
        if (send({ action: 'run_all' })) {
            setNarration('Starting all acts...');
        }
    });
}

// ═══════════════════════════════════════════════════════════════════════
// Initialization
// ═══════════════════════════════════════════════════════════════════════

document.addEventListener('DOMContentLoaded', () => {
    initChart();
    setupButtonHandlers();
    connect();
});
