window.__BENCHMARK_DATA = {"timestamp": "2026-02-03T06:09:23.894533+00:00", "scenarios": [{"name": "Sensor Fusion (N=16 streams)", "atomik_ops_sec": 4079761.7908676183, "conventional_ops_sec": 2781273.2975091506, "atomik_bytes": 800000, "conventional_bytes": 1600000, "speedup": 1.466868356490305, "n_streams": 16, "n_updates": 100000}, {"name": "Rollback (10K deltas)", "atomik_undo_ops": 1, "conventional_undo_ops": 100, "atomik_latency_us": 3.456000001733628, "conventional_latency_us": 52.368000012847915, "n_deltas": 10000, "rollback_steps": 1}, {"name": "Distributed Sync (N nodes)", "atomik_messages": 8, "conventional_messages": 10000, "atomik_correct": true, "conventional_correct": true, "atomik_time_us": 1957.7550000065003, "conventional_time_us": 1937554.7270000197, "n_nodes": 8, "n_updates": 10000}, {"name": "Memory Traffic", "atomik_bytes_written": 135284, "conventional_bytes_written": 800000, "atomik_parallel_bytes": 135284, "reduction_pct": 83.08949999999999, "n_updates": 100000, "state_width": 64, "sparsity": 0.95}]};
// Demo launcher — run scenarios and display results
const scenarioNames = {
    sensor_fusion: 'Sensor Fusion (N=16 streams)',
    rollback: 'Rollback (10K deltas)',
    distributed_sync: 'Distributed Sync (N nodes)',
    memory_traffic: 'Memory Traffic',
};

async function runScenario(name) {
    const btn = event.target;
    const resultsDiv = document.getElementById('result-' + name);
    if (!resultsDiv) return;

    btn.disabled = true;
    btn.textContent = 'Running...';
    resultsDiv.classList.add('visible');
    resultsDiv.innerHTML = '<em>Running benchmark...</em>';

    try {
        // Try to fetch from API (dynamic mode)
        const resp = await Promise.resolve({ok:true,json:()=>window.__BENCHMARK_DATA});
        if (!resp.ok) throw new Error('API unavailable');
        const data = await resp.json();

        if (data.error) {
            resultsDiv.innerHTML = '<em>No results cached. Run from terminal first.</em>';
            btn.disabled = false;
            btn.textContent = 'Run Scenario';
            return;
        }

        // Find matching scenario
        const displayName = scenarioNames[name] || name;
        const scenario = (data.scenarios || []).find(s => s.name === displayName);

        if (!scenario) {
            resultsDiv.innerHTML = '<em>Scenario not found in results.</em>';
        } else {
            resultsDiv.innerHTML = formatResult(scenario);
        }
    } catch (e) {
        resultsDiv.innerHTML = '<em>Running in static mode. Use terminal to run benchmarks.</em>';
    }

    btn.disabled = false;
    btn.textContent = 'Run Scenario';
}

function formatResult(s) {
    let html = '<table style="width:100%; font-size:0.85rem;">';

    if (s.atomik_ops_sec) {
        html += row('ATOMiK ops/s', Math.round(s.atomik_ops_sec).toLocaleString());
        html += row('Conventional ops/s', Math.round(s.conventional_ops_sec).toLocaleString());
        html += row('Speedup', (s.speedup || 1).toFixed(2) + 'x');
    }
    if (s.atomik_undo_ops !== undefined) {
        html += row('ATOMiK undo ops', s.atomik_undo_ops);
        html += row('Conventional undo ops', s.conventional_undo_ops);
        html += row('ATOMiK latency', s.atomik_latency_us.toFixed(2) + ' us');
        html += row('Conventional latency', s.conventional_latency_us.toFixed(2) + ' us');
    }
    if (s.atomik_messages !== undefined) {
        html += row('ATOMiK messages', s.atomik_messages);
        html += row('Conventional messages', s.conventional_messages);
        html += row('Both correct?', s.atomik_correct && s.conventional_correct ? 'Yes' : 'No');
    }
    if (s.reduction_pct !== undefined) {
        html += row('ATOMiK bytes', s.atomik_bytes_written.toLocaleString());
        html += row('Conventional bytes', s.conventional_bytes_written.toLocaleString());
        html += row('Reduction', s.reduction_pct.toFixed(1) + '%');
    }

    html += '</table>';
    return html;
}

function row(label, value) {
    return `<tr><td style="color:var(--subtext0)">${label}</td><td style="color:var(--text);font-weight:600">${value}</td></tr>`;
}
