// Imperative PixiJS v8 renderer for the card table. React owns the DOM chrome
// (menu, lobby, bidding panel, scoreboard); this owns the felt, the hands, the
// trick and (Phase 3) the animations. For now cards are drawn procedurally — the
// Bellot sprite atlas replaces `drawCard` later without touching the layout.

import { Application, Container, Graphics, Text } from "pixi.js";
import { type CardT, type GameState, rankLabel, sameCard, SUIT_SYMBOL } from "../protocol";

const CARD_W = 74;
const CARD_H = 104;
const RED = 0xd23b3b;
const BLACK = 0x1c1c22;

type PlayHandler = (card: CardT) => void;

export class PixiTable {
  app: Application;
  private felt = new Container();
  private tableLayer = new Container();
  private handLayer = new Container();
  private last: GameState | null = null;
  private onPlay: PlayHandler = () => {};

  private constructor(app: Application) {
    this.app = app;
  }

  static async create(mount: HTMLElement): Promise<PixiTable> {
    const app = new Application();
    await app.init({
      background: "#0f5132",
      antialias: true,
      resizeTo: mount,
      autoDensity: true,
      resolution: window.devicePixelRatio || 1,
    });
    mount.appendChild(app.canvas);

    const t = new PixiTable(app);
    app.stage.addChild(t.felt, t.tableLayer, t.handLayer);
    app.renderer.on("resize", () => t.redraw());
    return t;
  }

  setPlayHandler(fn: PlayHandler) {
    this.onPlay = fn;
  }

  update(state: GameState) {
    this.last = state;
    this.redraw();
  }

  destroy() {
    this.app.destroy(true, { children: true });
  }

  // ---- rendering ----

  private redraw() {
    this.tableLayer.removeChildren();
    this.handLayer.removeChildren();
    const s = this.last;
    if (!s) return;

    const W = this.app.screen.width;
    const H = this.app.screen.height;
    const cx = W / 2;
    const cy = H / 2;

    this.drawOpponents(s, W, H, cy);
    this.drawTrick(s, cx, cy);
    this.drawHand(s, W, H);
  }

  private drawOpponents(s: GameState, W: number, _H: number, cy: number) {
    // relative seats: 1 = left, 2 = top, 3 = right (0 = you, bottom)
    for (let rel = 1; rel <= 3; rel++) {
      const seat = (s.yourSeat + rel) % 4;
      const active = isActive(s, seat);
      const name = s.names[seat] || "—";
      const count = s.handCounts[seat] ?? 0;

      let x = 0;
      let y = 0;
      if (rel === 1) {
        x = 40;
        y = cy - 120;
      } else if (rel === 2) {
        x = W / 2 - 60;
        y = 40;
      } else {
        x = W - 200;
        y = cy - 120;
      }

      const label = new Text({
        text: `${name}${s.dealer === seat ? "  (D)" : ""}`,
        style: { fill: active ? 0xffd166 : 0xe6f4ea, fontSize: 18, fontFamily: "sans-serif" },
      });
      label.position.set(x, y);
      this.tableLayer.addChild(label);

      for (let k = 0; k < count; k++) {
        this.tableLayer.addChild(cardBack(x + k * 14, y + 28));
      }
    }
  }

  private drawTrick(s: GameState, cx: number, cy: number) {
    for (const pc of s.trick) {
      const rel = (pc.seat - s.yourSeat + 4) % 4;
      const { x, y } = trickSlot(rel, cx, cy);
      const c = cardFace(pc.card, s.trump === pc.card.s);
      c.position.set(x, y);
      this.tableLayer.addChild(c);
    }
  }

  private drawHand(s: GameState, W: number, H: number) {
    const count = s.yourHand.length;
    if (count === 0) return;
    const legal = s.yourLegal ?? [];
    const isLegal = (c: CardT) => legal.some((l) => sameCard(l, c));

    const spacing = Math.min(CARD_W + 14, (W - 160) / Math.max(1, count));
    const totalW = spacing * (count - 1) + CARD_W;
    const baseX = W / 2 - totalW / 2;
    const baseY = H - CARD_H - 24;

    s.yourHand.forEach((card, i) => {
      const ok = isLegal(card);
      const c = cardFace(card, s.trump === card.s);
      c.position.set(baseX + i * spacing, ok ? baseY - 18 : baseY);
      if (ok) {
        c.eventMode = "static";
        c.cursor = "pointer";
        c.on("pointertap", () => this.onPlay(card));
        highlight(c);
      } else {
        c.alpha = 0.85;
      }
      this.handLayer.addChild(c);
    });
  }
}

// ---- card primitives ----

function isActive(s: GameState, seat: number): boolean {
  if (s.phase === "playing") return s.currentPlayer === seat;
  if (s.phase === "bidding") return s.auction?.bidder === seat;
  return false;
}

function trickSlot(rel: number, cx: number, cy: number): { x: number; y: number } {
  switch (rel) {
    case 0:
      return { x: cx - CARD_W / 2, y: cy + 40 }; // you (bottom)
    case 1:
      return { x: cx - 150, y: cy - CARD_H / 2 }; // left
    case 2:
      return { x: cx - CARD_W / 2, y: cy - 150 }; // top
    default:
      return { x: cx + 80, y: cy - CARD_H / 2 }; // right
  }
}

function cardBack(x: number, y: number): Graphics {
  const g = new Graphics();
  g.roundRect(0, 0, 34, 50, 5).fill(0x2a3f6b).stroke({ width: 1.5, color: 0x8fb0ff });
  g.roundRect(5, 5, 24, 40, 3).stroke({ width: 1, color: 0x557 });
  g.position.set(x, y);
  return g;
}

/** A face-up procedural card as a Container (rounded rect + rank + suit). */
function cardFace(card: CardT, isTrump: boolean): Container {
  const c = new Container();
  const red = card.s === "D" || card.s === "H";
  const ink = red ? RED : BLACK;

  const bg = new Graphics();
  bg.roundRect(0, 0, CARD_W, CARD_H, 8).fill(0xfbfbf5);
  bg.roundRect(0, 0, CARD_W, CARD_H, 8).stroke({
    width: isTrump ? 3 : 1.5,
    color: isTrump ? 0xffb703 : 0x999,
  });
  c.addChild(bg);

  const corner = new Text({
    text: `${rankLabel(card.r)}\n${SUIT_SYMBOL[card.s]}`,
    style: { fill: ink, fontSize: 18, fontFamily: "serif", lineHeight: 18, align: "center" },
  });
  corner.position.set(6, 5);
  c.addChild(corner);

  const pip = new Text({
    text: SUIT_SYMBOL[card.s],
    style: { fill: ink, fontSize: 40, fontFamily: "serif" },
  });
  pip.anchor.set(0.5);
  pip.position.set(CARD_W / 2, CARD_H / 2 + 6);
  c.addChild(pip);

  return c;
}

function highlight(c: Container) {
  const glow = new Graphics();
  glow.roundRect(-3, -3, CARD_W + 6, CARD_H + 6, 10).stroke({ width: 3, color: 0x6bd08a });
  c.addChildAt(glow, 0);
}
