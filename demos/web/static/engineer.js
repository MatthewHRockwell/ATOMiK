// Engineer dashboard — load benchmark results if available
(async function() {
    try {
        const resp = await fetch('/api/benchmark');
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
