import { useState } from "react";
import { api, type LobbySeat } from "../protocol";

export function Lobby({ seat, seats }: { seat: number; seats: LobbySeat[] }) {
  const [mode, setMode] = useState<"belote" | "coinche">("belote");
  const filled: LobbySeat[] = Array.from(
    { length: 4 },
    (_, i) => seats[i] ?? { occupied: false, name: "" },
  );

  return (
    <div className="menu">
      <h1>Salon</h1>
      <p className="hint">
        Tu es assis au siège <b>{seat}</b>. Les sièges libres seront tenus par des bots.
      </p>

      <ul className="seats">
        {filled.map((s, i) => (
          <li key={i} className={i === seat ? "me" : ""}>
            <span className="seatnum">S{i}</span>
            <span>{s.occupied ? s.name || "(occupé)" : "— libre —"}</span>
            {i === seat && <span className="tag">toi</span>}
          </li>
        ))}
      </ul>

      <div className="modes">
        <label className={mode === "belote" ? "on" : ""}>
          <input
            type="radio"
            checked={mode === "belote"}
            onChange={() => setMode("belote")}
          />
          Belote
        </label>
        <label className={mode === "coinche" ? "on" : ""}>
          <input
            type="radio"
            checked={mode === "coinche"}
            onChange={() => setMode("coinche")}
          />
          Coinche
        </label>
      </div>

      <button className="primary" onClick={() => api.start(mode)}>
        Démarrer la partie
      </button>
    </div>
  );
}
