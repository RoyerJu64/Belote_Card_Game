import { useEffect, useRef } from "react";
import { PixiTable } from "../game/PixiTable";
import { api, type GameState, SUIT_NAME, SUIT_SYMBOL } from "../protocol";
import { BiddingPanel } from "./BiddingPanel";

export function GameView({
  game,
  notices,
  onLeave,
}: {
  game: GameState;
  notices: string[];
  onLeave: () => void;
}) {
  const mountRef = useRef<HTMLDivElement>(null);
  const tableRef = useRef<PixiTable | null>(null);

  // Boot the Pixi table once.
  useEffect(() => {
    let disposed = false;
    (async () => {
      if (mountRef.current && !tableRef.current) {
        const t = await PixiTable.create(mountRef.current);
        if (disposed) {
          t.destroy();
          return;
        }
        t.setPlayHandler((card) => api.playCard(card));
        tableRef.current = t;
        t.update(game);
      }
    })();
    return () => {
      disposed = true;
      tableRef.current?.destroy();
      tableRef.current = null;
    };
    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, []);

  // Push every new snapshot into the table.
  useEffect(() => {
    tableRef.current?.update(game);
  }, [game]);

  const bidding = game.phase === "bidding";
  const playing = game.phase === "playing";
  const myTurn = playing && game.currentPlayer === game.yourSeat;
  const contract = game.contract;

  return (
    <div className="game">
      <div className="felt" ref={mountRef} />

      <header className="hud">
        <div className="scores">
          <span className="teamA">Équipe A : {game.scoreA}</span>
          <span className="teamB">Équipe B : {game.scoreB}</span>
        </div>
        {playing && (
          <div className="trump">
            Atout : <b>{SUIT_SYMBOL[game.trump]}</b> {SUIT_NAME[game.trump]}
            {contract && (
              <span className="contract">
                {" "}
                · Contrat {contract.value > 0 ? `${contract.value} ` : ""}
                par S{contract.taker}
                {contract.mult === 4 ? " ×4" : contract.mult === 2 ? " ×2" : ""}
              </span>
            )}
          </div>
        )}
        <button className="leave" onClick={onLeave}>
          Quitter
        </button>
      </header>

      {playing && (
        <div className={`banner ${myTurn ? "you" : ""}`}>
          {myTurn ? "À toi de jouer" : "En attente des autres joueurs…"}
        </div>
      )}

      {bidding && (
        <div className="overlay-panel">
          <BiddingPanel game={game} />
        </div>
      )}

      {game.phase === "matchOver" && (
        <div className="modal">
          <h1>Équipe {game.scoreA >= game.scoreB ? "A" : "B"} remporte la partie !</h1>
          <p className="finalscore">
            {game.scoreA} / {game.scoreB}
          </p>
          <button className="primary" onClick={onLeave}>
            Retour au menu
          </button>
        </div>
      )}

      <ol className="notices">
        {notices.slice(0, 6).map((n, i) => (
          <li key={i}>{n}</li>
        ))}
      </ol>
    </div>
  );
}
