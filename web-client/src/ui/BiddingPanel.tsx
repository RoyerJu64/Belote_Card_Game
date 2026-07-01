import { useState } from "react";
import {
  api,
  type GameState,
  type SuitCode,
  ORDINARY_SUITS,
  SUIT_NAME,
} from "../protocol";

const CAPOT = 250;

export function BiddingPanel({ game }: { game: GameState }) {
  const a = game.auction!;
  const mine = a.bidder === game.yourSeat;
  const [bidSuit, setBidSuit] = useState<SuitCode | null>(null);
  const [bidValue, setBidValue] = useState(80);

  const contract = a.contract;
  const title = game.mode === "coinche" ? "Enchères — Coinche" : "Enchères — Belote";

  return (
    <div className="bidding">
      <h2>{title}</h2>
      {contract ? (
        <p className="contract">
          Contrat : {contract.value > 0 ? `${contract.value} ` : ""}
          {SUIT_NAME[contract.suit]} par S{contract.taker}
          {contract.mult === 4 ? " ×4" : contract.mult === 2 ? " ×2" : ""}
        </p>
      ) : (
        <p className="contract dim">Personne n'a encore pris</p>
      )}

      <p className="turn">
        {mine ? "À toi d'enchérir" : `Au tour de ${game.names[a.bidder] ?? `S${a.bidder}`}`}
      </p>

      {!mine ? null : game.mode === "belote" ? (
        <BeloteBids round={a.round} upcard={a.upcard?.s ?? null} />
      ) : (
        <CoincheBids
          contractValue={contract?.value ?? 0}
          contractMult={contract?.mult ?? 1}
          contractTaker={contract?.taker ?? -1}
          yourSeat={game.yourSeat}
          bidSuit={bidSuit}
          setBidSuit={setBidSuit}
          bidValue={bidValue}
          setBidValue={setBidValue}
        />
      )}
    </div>
  );
}

function BeloteBids({ round, upcard }: { round: number; upcard: SuitCode | null }) {
  if (round === 0) {
    return (
      <div className="bidrow">
        <button className="primary" onClick={() => upcard && api.bid("take", upcard)}>
          Prendre {upcard ? SUIT_NAME[upcard] : ""}
        </button>
        <button onClick={() => api.bid("pass")}>Passer</button>
      </div>
    );
  }
  return (
    <div className="bidrow">
      {ORDINARY_SUITS.filter((s) => s !== upcard).map((s) => (
        <button key={s} onClick={() => api.bid("take", s)}>
          {SUIT_NAME[s]}
        </button>
      ))}
      <button onClick={() => api.bid("pass")}>Passer</button>
    </div>
  );
}

function CoincheBids(props: {
  contractValue: number;
  contractMult: number;
  contractTaker: number;
  yourSeat: number;
  bidSuit: SuitCode | null;
  setBidSuit: (s: SuitCode) => void;
  bidValue: number;
  setBidValue: (v: number) => void;
}) {
  const { contractValue, contractMult, contractTaker, yourSeat } = props;
  const hasContract = contractTaker >= 0;
  const higher = !hasContract || props.bidValue > contractValue;
  const canAnnounce = props.bidSuit !== null && contractMult === 1 && higher;
  const opposing = hasContract && contractTaker % 2 !== yourSeat % 2;
  const sameTeam = hasContract && contractTaker % 2 === yourSeat % 2;
  const canCoinche = hasContract && contractMult === 1 && opposing;
  const canSur = hasContract && contractMult === 2 && sameTeam;

  return (
    <div className="coinche">
      <div className="bidrow">
        {ORDINARY_SUITS.map((s) => (
          <button
            key={s}
            className={props.bidSuit === s ? "on" : ""}
            onClick={() => props.setBidSuit(s)}
          >
            {SUIT_NAME[s]}
          </button>
        ))}
      </div>
      <div className="bidrow">
        <button onClick={() => props.setBidValue(Math.max(80, (props.bidValue >= CAPOT ? 160 : props.bidValue) - 10))}>
          −
        </button>
        <span className="value">{props.bidValue >= CAPOT ? "Capot" : props.bidValue}</span>
        <button onClick={() => props.setBidValue(Math.min(160, (props.bidValue >= CAPOT ? 160 : props.bidValue) + 10))}>
          +
        </button>
        <button onClick={() => props.setBidValue(CAPOT)}>Capot</button>
      </div>
      <div className="bidrow">
        <button
          className="primary"
          disabled={!canAnnounce}
          onClick={() => props.bidSuit && api.bid("take", props.bidSuit, props.bidValue)}
        >
          Annoncer
        </button>
        <button onClick={() => api.bid("pass")}>Passer</button>
        {canCoinche && <button onClick={() => api.bid("coinche")}>Coincher</button>}
        {canSur && <button onClick={() => api.bid("surcoinche")}>Surcoincher</button>}
      </div>
    </div>
  );
}
