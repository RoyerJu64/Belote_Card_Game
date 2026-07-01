// A tiny observable store: folds the server's JSON events into an immutable
// snapshot that React reads via useSyncExternalStore. No heavy state management —
// just what the UI needs. The Pixi table renders from `game`; the DOM chrome
// renders from `status`/`seat`/`lobby`/`notices`.

import type { GameState, LobbySeat } from "./protocol";
import { onEvent, onStatus } from "./net";

export interface AppState {
  status: string;
  connected: boolean;
  seat: number | null;
  lobby: LobbySeat[];
  game: GameState | null;
  notices: string[];
  lastRejection: string | null;
}

let state: AppState = {
  status: "déconnecté",
  connected: false,
  seat: null,
  lobby: [],
  game: null,
  notices: [],
  lastRejection: null,
};

const listeners = new Set<() => void>();

function set(patch: Partial<AppState>) {
  state = { ...state, ...patch };
  listeners.forEach((l) => l());
}

function notice(line: string) {
  const stamped = `${new Date().toLocaleTimeString()}  ${line}`;
  set({ notices: [stamped, ...state.notices].slice(0, 40) });
}

export const store = {
  subscribe(cb: () => void): () => void {
    listeners.add(cb);
    return () => listeners.delete(cb);
  },
  getSnapshot(): AppState {
    return state;
  },
  reset() {
    set({ seat: null, lobby: [], game: null, notices: [], lastRejection: null });
  },
};

let started = false;

/** Wire the transport once. Safe to call repeatedly. */
export async function initStore(): Promise<void> {
  if (started) return;
  started = true;

  await onStatus((s) => {
    set({ status: s, connected: s === "connected" });
    notice(`● ${s}`);
  });

  await onEvent((ev) => {
    switch (ev.t) {
      case "welcome":
        set({ seat: ev.seat as number });
        break;
      case "lobby":
        set({ lobby: ev.seats as LobbySeat[] });
        break;
      case "state":
        set({ game: ev as unknown as GameState, lastRejection: null });
        break;
      case "rejected":
        set({ lastRejection: String(ev.reason) });
        notice(`✗ refusé : ${ev.reason}`);
        break;
      case "error":
        notice(`⚠ serveur : ${ev.reason}`);
        break;
      case "roundEnd":
        notice(
          `Donne : A+${ev.deltaA} B+${ev.deltaB}  (total ${ev.totalA} / ${ev.totalB})`,
        );
        break;
      case "matchEnd":
        notice(`🏆 Match : équipe ${ev.winner === 0 ? "A" : "B"} gagne`);
        break;
      case "chat":
        notice(`💬 ${ev.from} : ${ev.text}`);
        break;
      default:
        // bid / cardPlayed / trickWon / dealt: the `state` snapshot already
        // reflects them for the static render. They drive animation in Phase 3.
        break;
    }
  });
}
