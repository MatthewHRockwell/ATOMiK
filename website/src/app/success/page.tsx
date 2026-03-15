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

  useEffect(() => {
    if (!sessionId) return;

    fetch(`/api/license?session_id=${sessionId}`)
      .then(res => res.json())
      .then(data => {
        if (data.error) setError(data.error);
        else setLicense(data);
      })
      .catch(() => setError('Failed to retrieve license information'));
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
        <p style={styles.loading}>Generating your license...</p>
      </div>
    );
  }

  const copyKey = () => {
    navigator.clipboard.writeText(license.licenseKey);
    setCopied(true);
    setTimeout(() => setCopied(false), 2000);
  };

  return (
    <div style={styles.container}>
      <div style={styles.checkmark}>&#10003;</div>
      <h1 style={styles.title}>Welcome to ATOMiK {license.tier === 'enterprise' ? 'Enterprise' : 'Professional'}</h1>
      <p style={styles.subtitle}>Your license has been activated for {license.email}</p>

      <div style={styles.card}>
        <h2 style={styles.cardTitle}>Your License Key</h2>
        <div style={styles.keyBox} onClick={copyKey}>
          <code style={styles.key}>{license.licenseKey}</code>
          <span style={styles.copyHint}>{copied ? 'Copied!' : 'Click to copy'}</span>
        </div>
      </div>

      <div style={styles.card}>
        <h2 style={styles.cardTitle}>Download &amp; Install</h2>

        <div style={styles.platformGrid}>
          <a href={license.downloads.linux} style={styles.platformBtn}>
            <span style={styles.platformIcon}>&#128039;</span>
            <span>Linux</span>
            <span style={styles.platformSub}>x86_64 / arm64</span>
          </a>
          <a href={license.downloads.windows} style={styles.platformBtn}>
            <span style={styles.platformIcon}>&#9638;</span>
            <span>Windows</span>
            <span style={styles.platformSub}>.exe installer</span>
          </a>
          <a href={license.downloads.macos} style={styles.platformBtn}>
            <span style={styles.platformIcon}>&#63743;</span>
            <span>macOS</span>
            <span style={styles.platformSub}>Universal binary</span>
          </a>
        </div>

        <div style={styles.cliSection}>
          <h3 style={styles.cliTitle}>Or install via CLI</h3>
          <div style={styles.cliBox}>
            <code>{license.install.linux}</code>
          </div>
          <div style={styles.cliBox}>
            <code>{license.install.pip}</code>
          </div>
        </div>
      </div>

      <div style={styles.card}>
        <h2 style={styles.cardTitle}>Need Help?</h2>
        <p style={styles.helpText}>
          Email <a href="mailto:support@atomik.tech" style={styles.link}>support@atomik.tech</a> for
          {license.tier === 'enterprise' ? ' 4-hour response SLA' : ' priority support (48hr SLA)'}.
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
  platformGrid: {
    display: 'grid', gridTemplateColumns: 'repeat(3, 1fr)', gap: 12,
    marginBottom: 24,
  },
  platformBtn: {
    display: 'flex', flexDirection: 'column' as const,
    alignItems: 'center', gap: 4,
    background: '#0a0a0f', border: '1px solid #1e1e2e',
    borderRadius: 8, padding: '20px 12px',
    color: '#e0e0e8', textDecoration: 'none',
    fontSize: 14, fontWeight: 600,
    transition: 'border-color 0.2s',
  },
  platformIcon: { fontSize: 28 },
  platformSub: { fontSize: 11, color: '#8888a0', fontWeight: 400 },
  cliSection: { borderTop: '1px solid #1e1e2e', paddingTop: 20 },
  cliTitle: { fontSize: 14, color: '#8888a0', marginBottom: 12 },
  cliBox: {
    background: '#0a0a0f', border: '1px solid #1e1e2e',
    borderRadius: 6, padding: '10px 14px', marginBottom: 8,
    fontFamily: "'SF Mono', 'Fira Code', monospace",
    fontSize: 12, color: '#22d3ee', overflowX: 'auto' as const,
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
  backLink: {
    display: 'block', textAlign: 'center' as const,
    color: '#8888a0', textDecoration: 'none', fontSize: 14,
    marginTop: 32,
  },
};
