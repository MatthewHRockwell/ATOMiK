'use client';

import { useSearchParams } from 'next/navigation';
import { useEffect, useState, Suspense } from 'react';

interface LicenseInfo {
  licenseKey: string;
  tier: string;
  email: string;
  downloads: {
    linux: string;
    windows: string;
    macos: string;
  };
  install: {
    linux: string;
    pip: string;
  };
}

function SuccessContent() {
  const searchParams = useSearchParams();
  const sessionId = searchParams.get('session_id');
  const [license, setLicense] = useState<LicenseInfo | null>(null);
  const [error, setError] = useState<string | null>(null);
  const [copied, setCopied] = useState(false);
  const [copiedInstall, setCopiedInstall] = useState(false);
  const [retryAttempt, setRetryAttempt] = useState(0);

  useEffect(() => {
    if (!sessionId) return;
    let attempt = 0;
    const maxAttempts = 4;
    const delays = [0, 2000, 4000, 8000];

    function tryFetch() {
      setRetryAttempt(attempt);
      fetch(`/api/license?session_id=${sessionId}`)
        .then(res => res.json())
        .then(data => {
          if (data.error && attempt < maxAttempts - 1) {
            attempt++;
            setTimeout(tryFetch, delays[attempt]);
            return;
          }
          if (data.error) setError(data.error);
          else setLicense(data);
        })
        .catch(() => {
          if (attempt < maxAttempts - 1) {
            attempt++;
            setTimeout(tryFetch, delays[attempt]);
          } else {
            setError('Failed to retrieve license');
          }
        });
    }
    tryFetch();
  }, [sessionId]);

  if (!sessionId) {
    return <div style={styles.container}><p>No session found.</p></div>;
  }

  if (error) {
    return (
      <div style={styles.container}>
        <h1 style={styles.title}>Something went wrong</h1>
        <p style={styles.error}>{error}</p>
        <a href="mailto:mrockwell@atomik.tech" style={styles.link}>Contact support</a>
      </div>
    );
  }

  if (!license) {
    return (
      <div style={styles.container}>
        <div style={styles.spinner} />
        <p style={styles.loading}>
          Generating your license...{retryAttempt > 0 ? ` (attempt ${retryAttempt + 1}/4)` : ''}
        </p>
      </div>
    );
  }

  const copyKey = () => {
    navigator.clipboard.writeText(license.licenseKey);
    setCopied(true);
    setTimeout(() => setCopied(false), 2000);
  };

  const installCommand = `git clone https://github.com/MatthewHRockwell/ATOMiK.git\ncd ATOMiK/software/atomik_kmod\nsudo ./install.sh --license ${license.licenseKey}`;

  const copyInstallCommand = () => {
    navigator.clipboard.writeText(installCommand);
    setCopiedInstall(true);
    setTimeout(() => setCopiedInstall(false), 2000);
  };

  return (
    <div style={styles.container}>
      <div style={styles.checkmark}>&#10003;</div>
      <h1 style={styles.title}>Welcome to ATOMiK {license.tier === 'enterprise' ? 'Enterprise' : license.tier === 'team' ? 'Team' : 'Pro'}</h1>
      <p style={styles.subtitle}>Your license has been activated for {license.email}</p>

      <div style={styles.card}>
        <h2 style={styles.cardTitle}>Your License Key</h2>
        <div style={styles.keyBox} onClick={copyKey}>
          <code style={styles.key}>{license.licenseKey}</code>
          <span style={styles.copyHint}>{copied ? 'Copied!' : 'Click to copy'}</span>
        </div>
      </div>

      <div style={styles.card}>
        <h2 style={styles.cardTitle}>Install Kernel Module</h2>
        <p style={styles.installNote}>ATOMiK kernel module is Linux-only (x86_64 / arm64).</p>

        <div style={styles.installBox} onClick={copyInstallCommand}>
          <pre style={styles.installPre}>{installCommand}</pre>
          <span style={styles.copyHint}>{copiedInstall ? 'Copied!' : 'Click to copy'}</span>
        </div>

        <div style={styles.cliSection}>
          <h3 style={styles.cliTitle}>Python SDK (all platforms)</h3>
          <div style={styles.cliBox}>
            <code>{license.install.pip}</code>
          </div>
        </div>
      </div>

      <div style={styles.card}>
        <h2 style={styles.cardTitle}>Quick Start</h2>
        <ol style={styles.stepsList}>
          <li style={styles.step}>
            <strong>Install the kernel module</strong> using the command above. The installer will register your license key and build the module for your kernel version.
          </li>
          <li style={styles.step}>
            <strong>Verify the installation</strong> by running <code style={styles.inlineCode}>lsmod | grep atomik</code> to confirm the module is loaded.
          </li>
          <li style={styles.step}>
            <strong>Install the Python SDK</strong> with <code style={styles.inlineCode}>pip install atomik-core</code> to start using ATOMiK in your applications.
          </li>
          <li style={styles.step}>
            <strong>Run the example</strong> to confirm everything works:
            <div style={{ ...styles.cliBox, marginTop: 8 }}>
              <code>python -c &quot;import atomik_core; print(atomik_core.status())&quot;</code>
            </div>
          </li>
        </ol>
      </div>

      <div style={styles.card}>
        <h2 style={styles.cardTitle}>Need Help?</h2>
        <p style={styles.helpText}>
          Email <a href="mailto:support@atomik.tech" style={styles.link}>support@atomik.tech</a> for
          {license.tier === 'enterprise' ? ' 4-hour response SLA' : ' priority support (48hr SLA)'}.
        </p>
      </div>

      <div style={styles.card}>
        <h2 style={styles.cardTitle}>What&apos;s Next</h2>
        <div style={styles.nextGrid}>
          <a href="/docs/kernel-module" style={styles.nextItem}>
            <span style={styles.nextIcon}>&#128214;</span>
            <span style={styles.nextLabel}>Read the Kernel Module Guide</span>
            <span style={styles.nextArrow}>&rarr;</span>
          </a>
          <a href="/docs/quickstart" style={styles.nextItem}>
            <span style={styles.nextIcon}>&#9889;</span>
            <span style={styles.nextLabel}>Run Your First Benchmark</span>
            <span style={styles.nextArrow}>&rarr;</span>
          </a>
          <a href="/dashboard" style={styles.nextItem}>
            <span style={styles.nextIcon}>&#128202;</span>
            <span style={styles.nextLabel}>Explore the Dashboard</span>
            <span style={styles.nextArrow}>&rarr;</span>
          </a>
          <a href="https://github.com/MatthewHRockwell/ATOMiK/discussions" style={styles.nextItem} target="_blank" rel="noopener noreferrer">
            <span style={styles.nextIcon}>&#128101;</span>
            <span style={styles.nextLabel}>Join the Community</span>
            <span style={styles.nextArrow}>&rarr;</span>
          </a>
        </div>
        <p style={styles.nextHelp}>
          Need help? Reach out at{' '}
          <a href="mailto:support@atomik.tech" style={styles.link}>support@atomik.tech</a>
          {' '}&mdash; we&apos;re here to get you up and running.
        </p>
      </div>

      <a href="/" style={styles.backLink}>&larr; Back to atomik.tech</a>
    </div>
  );
}

export default function SuccessPage() {
  return (
    <Suspense fallback={
      <div style={styles.container}>
        <div style={styles.spinner} />
        <p style={styles.loading}>Loading...</p>
      </div>
    }>
      <SuccessContent />
    </Suspense>
  );
}

const styles: Record<string, React.CSSProperties> = {
  container: {
    maxWidth: 680,
    margin: '0 auto',
    padding: '60px 24px',
    fontFamily: "-apple-system, BlinkMacSystemFont, 'Segoe UI', system-ui, sans-serif",
    color: '#e0e0e8',
    background: '#0a0a0f',
    minHeight: '100vh',
  },
  checkmark: {
    width: 64, height: 64, borderRadius: '50%',
    background: 'rgba(34,197,94,0.15)', color: '#22c55e',
    display: 'flex', alignItems: 'center', justifyContent: 'center',
    fontSize: 32, margin: '0 auto 24px', fontWeight: 700,
  },
  title: {
    fontSize: 32, fontWeight: 700, textAlign: 'center' as const,
    marginBottom: 8, letterSpacing: -1,
  },
  subtitle: {
    textAlign: 'center' as const, color: '#8888a0', fontSize: 16,
    marginBottom: 40,
  },
  card: {
    background: '#12121a', border: '1px solid #1e1e2e',
    borderRadius: 12, padding: 32, marginBottom: 24,
  },
  cardTitle: {
    fontSize: 18, fontWeight: 600, marginBottom: 16,
  },
  keyBox: {
    background: '#0a0a0f', border: '1px solid #1e1e2e',
    borderRadius: 8, padding: '16px 20px',
    cursor: 'pointer', textAlign: 'center' as const,
    transition: 'border-color 0.2s',
  },
  key: {
    fontSize: 20, fontWeight: 700, color: '#22d3ee',
    fontFamily: "'SF Mono', 'Fira Code', monospace",
    letterSpacing: 1,
  },
  copyHint: {
    display: 'block', fontSize: 12, color: '#8888a0', marginTop: 8,
  },
  installNote: {
    fontSize: 13, color: '#8888a0', marginBottom: 16,
  },
  installBox: {
    background: '#0a0a0f', border: '1px solid #1e1e2e',
    borderRadius: 8, padding: '16px 20px',
    cursor: 'pointer', marginBottom: 20,
    transition: 'border-color 0.2s',
  },
  installPre: {
    margin: 0, whiteSpace: 'pre-wrap' as const, wordBreak: 'break-all' as const,
    fontFamily: "'SF Mono', 'Fira Code', monospace",
    fontSize: 13, color: '#22d3ee', lineHeight: 1.7,
  },
  cliSection: { borderTop: '1px solid #1e1e2e', paddingTop: 20 },
  cliTitle: { fontSize: 14, color: '#8888a0', marginBottom: 12 },
  cliBox: {
    background: '#0a0a0f', border: '1px solid #1e1e2e',
    borderRadius: 6, padding: '10px 14px', marginBottom: 8,
    fontFamily: "'SF Mono', 'Fira Code', monospace",
    fontSize: 12, color: '#22d3ee', overflowX: 'auto' as const,
  },
  stepsList: {
    margin: 0, paddingLeft: 20, listStyleType: 'decimal' as const,
  },
  step: {
    fontSize: 14, color: '#c0c0d0', marginBottom: 16, lineHeight: 1.6,
  },
  inlineCode: {
    fontFamily: "'SF Mono', 'Fira Code', monospace",
    fontSize: 12, color: '#22d3ee',
    background: '#0a0a0f', border: '1px solid #1e1e2e',
    borderRadius: 4, padding: '2px 6px',
  },
  helpText: { color: '#8888a0', fontSize: 14 },
  link: { color: '#4f8fff', textDecoration: 'none' },
  error: { color: '#ef4444', textAlign: 'center' as const, marginBottom: 16 },
  loading: { textAlign: 'center' as const, color: '#8888a0' },
  spinner: {
    width: 40, height: 40, margin: '40px auto 16px',
    border: '3px solid #1e1e2e', borderTopColor: '#4f8fff',
    borderRadius: '50%',
    animation: 'spin 0.8s linear infinite',
  },
  nextGrid: {
    display: 'flex', flexDirection: 'column' as const, gap: 12,
    marginBottom: 20,
  },
  nextItem: {
    display: 'flex', alignItems: 'center', gap: 12,
    padding: '14px 16px', borderRadius: 8,
    background: '#0a0a0f', border: '1px solid #1e1e2e',
    textDecoration: 'none', color: '#e0e0e8',
    transition: 'border-color 0.2s, background 0.2s',
    cursor: 'pointer',
  },
  nextIcon: {
    fontSize: 20, width: 32, textAlign: 'center' as const, flexShrink: 0,
  },
  nextLabel: {
    fontSize: 14, fontWeight: 500, flex: 1,
  },
  nextArrow: {
    fontSize: 14, color: '#4f8fff', flexShrink: 0,
  },
  nextHelp: {
    fontSize: 13, color: '#8888a0', textAlign: 'center' as const, margin: 0,
  },
  backLink: {
    display: 'block', textAlign: 'center' as const,
    color: '#8888a0', textDecoration: 'none', fontSize: 14,
    marginTop: 32,
  },
};
