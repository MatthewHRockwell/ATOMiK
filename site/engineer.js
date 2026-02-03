window.__BENCHMARK_DATA = {"timestamp": "2026-02-03T19:08:18.583960+00:00", "scenarios": [{"name": "Sensor Fusion (N=16 streams)", "atomik_ops_sec": 4063653.061556514, "conventional_ops_sec": 2773059.8016724726, "atomik_bytes": 800000, "conventional_bytes": 1600000, "speedup": 1.4654040490239935, "n_streams": 16, "n_updates": 100000}, {"name": "Rollback (10K deltas)", "atomik_undo_ops": 1, "conventional_undo_ops": 100, "atomik_latency_us": 3.0960000003688037, "conventional_latency_us": 49.70199999831948, "n_deltas": 10000, "rollback_steps": 1}, {"name": "Distributed Sync (N nodes)", "atomik_messages": 8, "conventional_messages": 10000, "atomik_correct": true, "conventional_correct": true, "atomik_time_us": 2049.9560000004635, "conventional_time_us": 1941339.7329999995, "n_nodes": 8, "n_updates": 10000}, {"name": "Memory Traffic", "atomik_bytes_written": 135284, "conventional_bytes_written": 800000, "atomik_parallel_bytes": 135284, "reduction_pct": 83.08949999999999, "n_updates": 100000, "state_width": 64, "sparsity": 0.95}]};
// Engineer dashboard — load benchmark results if available
(async function() {
    try {
        const resp = await Promise.resolve({ok:true,json:()=>window.__BENCHMARK_DATA});
        if (!resp.ok) return;
        const data = await resp.json();
        if (data.error) return;

        const container = document.getElementById('benchResults');
        if (!container || !data.scenarios) return;

        let html = '<table><tr><th>Scenario</th><th>ATOMiK</th><th>Conventional</th><th>Advantage</th></tr>';
        for (const s of data.scenarios) {
            const name = s.name || 'Unknown';
            let atomik = '', conv = '', advantage = '';

            if (s.atomik_ops_sec) {
                atomik = Math.round(s.atomik_ops_sec).toLocaleString() + ' ops/s';
                conv = Math.round(s.conventional_ops_sec).toLocaleString() + ' ops/s';
                advantage = (s.speedup || 1).toFixed(1) + 'x faster';
            } else if (s.atomik_undo_ops !== undefined) {
                atomik = s.atomik_undo_ops + ' op(s)';
                conv = s.conventional_undo_ops + ' ops';
                advantage = 'Self-inverse';
            } else if (s.atomik_messages !== undefined) {
                atomik = s.atomik_messages + ' msgs';
                conv = s.conventional_messages + ' msgs';
                advantage = 'Order-independent';
            } else if (s.reduction_pct !== undefined) {
                atomik = s.atomik_bytes_written.toLocaleString() + ' bytes';
                conv = s.conventional_bytes_written.toLocaleString() + ' bytes';
                advantage = s.reduction_pct.toFixed(1) + '% reduction';
            }
            html += `<tr><td>${name}</td><td style="color:var(--green)">${atomik}</td><td style="color:var(--red)">${conv}</td><td>${advantage}</td></tr>`;
        }
        html += '</table>';
        container.innerHTML = html;
    } catch (e) {
        console.log('Benchmark data not available (static mode)');
    }
})();
