import { NextRequest, NextResponse } from 'next/server';
import { readFile, writeFile, mkdir } from 'fs/promises';
import path from 'path';

const EMAIL_RE = /^[^\s@]+@[^\s@]+\.[^\s@]+$/;
const DATA_DIR = path.join(process.cwd(), 'data');
const SUBSCRIBERS_FILE = path.join(DATA_DIR, 'subscribers.json');

interface Subscriber {
  email: string;
  subscribedAt: string;
}

async function readSubscribers(): Promise<Subscriber[]> {
  try {
    const data = await readFile(SUBSCRIBERS_FILE, 'utf-8');
    return JSON.parse(data);
  } catch {
    return [];
  }
}

async function writeSubscribers(subscribers: Subscriber[]): Promise<void> {
  await mkdir(DATA_DIR, { recursive: true });
  await writeFile(SUBSCRIBERS_FILE, JSON.stringify(subscribers, null, 2));
}

export async function POST(req: NextRequest) {
  try {
    const { email } = await req.json();

    if (!email || typeof email !== 'string') {
      return NextResponse.json({ error: 'Email is required' }, { status: 400 });
    }

    const normalized = email.trim().toLowerCase();

    if (!EMAIL_RE.test(normalized)) {
      return NextResponse.json({ error: 'Invalid email format' }, { status: 400 });
    }

    const subscribers = await readSubscribers();

    if (subscribers.some((s) => s.email === normalized)) {
      return NextResponse.json({ success: true });
    }

    subscribers.push({ email: normalized, subscribedAt: new Date().toISOString() });
    await writeSubscribers(subscribers);

    return NextResponse.json({ success: true });
  } catch (error) {
    console.error('Subscribe error:', error);
    return NextResponse.json(
      { error: 'Failed to subscribe' },
      { status: 500 }
    );
  }
}
