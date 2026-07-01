// Shared types for the JSON event protocol (mirror of server/src/WebServer.cpp)
// plus the client -> server senders. The frontend speaks JSON only.

import { send } from "./net";

export type SuitCode = "C" | "D" | "H" | "S" | "T" | "-";
export interface CardT {
  s: SuitCode;
  r: number;
}

export type Phase = "lobby" | "bidding" | "playing" | "roundOver" | "matchOver";

export interface ContractT {
  taker: number;
  suit: SuitCode;
  value: number;
  mult: number;
}

export interface AuctionT {
  bidder: number;
  round: number;
  upcard: CardT | null;
  contract: ContractT | null;
}

/** The authoritative per-seat snapshot (server `state` event). */
export interface GameState {
  phase: Phase;
  yourSeat: number;
  mode: "belote" | "coinche";
  trump: SuitCode;
  dealer: number;
  currentPlayer: number;
  scoreA: number;
  scoreB: number;
  yourHand: CardT[];
  handCounts: number[];
  names: string[];
  trick: { seat: number; card: CardT }[];
  yourLegal?: CardT[];
  auction?: AuctionT;
  contract?: ContractT;
}

export interface LobbySeat {
  occupied: boolean;
  name: string;
}

// Auction kinds, matching BidAction::Kind on the server.
export const BidKind = { Pass: 0, Take: 1, Coinche: 2, Surcoinche: 3 } as const;

// ---- Client -> Server senders ----

export const api = {
  login: (name: string) => send({ t: "login", name }),
  start: (mode: "belote" | "coinche") => send({ t: "start", mode }),
  playCard: (card: CardT) => send({ t: "playCard", card }),
  bid: (kind: "pass" | "take" | "coinche" | "surcoinche", suit?: SuitCode, value?: number) =>
    send({ t: "bid", kind, ...(suit ? { suit } : {}), ...(value ? { value } : {}) }),
  chat: (text: string) => send({ t: "chat", text }),
};

// ---- Display helpers ----

export const SUIT_SYMBOL: Record<SuitCode, string> = {
  C: "♣",
  D: "♦",
  H: "♥",
  S: "♠",
  T: "T",
  "-": "—",
};

export const SUIT_NAME: Record<SuitCode, string> = {
  C: "Trèfle",
  D: "Carreau",
  H: "Cœur",
  S: "Pique",
  T: "Atout",
  "-": "—",
};

export const ORDINARY_SUITS: SuitCode[] = ["C", "D", "H", "S"];

/** French rank label for belote/coinche (7..As). */
export function rankLabel(r: number): string {
  switch (r) {
    case 1:
    case 14:
      return "A";
    case 13:
      return "R";
    case 12:
      return "D";
    case 11:
      return "V";
    default:
      return String(r);
  }
}

export function cardKey(c: CardT): string {
  return `${c.s}${c.r}`;
}

export function sameCard(a: CardT, b: CardT): boolean {
  return a.s === b.s && a.r === b.r;
}
