import { redirect } from 'next/navigation';

export default function Home() {
  // Serve the static landing page
  redirect('/landing.html');
}
