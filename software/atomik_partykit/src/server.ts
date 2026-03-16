/**
 * ATOMiK Live Demo — PartyKit Relay Server
 *
 * Stateless relay: broadcasts messages from any peer to all other peers.
 * The server does NO XOR computation — ATOMiK algebra runs client-side.
 * Each message is a small JSON payload (type + value, ~30 bytes).
 *
 * Room ID determines the sync group. Default room: "lobby".
 * Peers in the same room converge; different rooms are isolated.
 */

import type * as Party from "partykit/server";

export default class AtomikRelay implements Party.Server {
  constructor(readonly room: Party.Room) {}

  onConnect(conn: Party.Connection) {
    // Notify all peers of updated count
    this.broadcastPeerCount();
  }

  onClose(conn: Party.Connection) {
    this.broadcastPeerCount();
  }

  onMessage(message: string, sender: Party.Connection) {
    // Relay to all OTHER connections in this room
    this.room.broadcast(message, [sender.id]);
  }

  private broadcastPeerCount() {
    const count = [...this.room.getConnections()].length;
    this.room.broadcast(JSON.stringify({ type: "peers", count }));
  }
}

AtomikRelay satisfies Party.Worker;
