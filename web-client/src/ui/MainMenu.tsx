import { useState } from "react";
import { connect } from "../net";
import { api } from "../protocol";

export function MainMenu({ status }: { status: string }) {
  const [name, setName] = useState("Joueur");
  const [host, setHost] = useState("127.0.0.1");
  const [port, setPort] = useState(5556);
  const [busy, setBusy] = useState(false);
  const [err, setErr] = useState<string | null>(null);

  const doConnect = async () => {
    setBusy(true);
    setErr(null);
    try {
      await connect(host, Number(port));
      api.login(name.trim() || "Joueur");
    } catch (e) {
      setErr(String(e));
      setBusy(false);
    }
  };

  return (
    <div className="menu">
      <h1>Belote / Coinche</h1>
      <p className="hint">Connecte-toi au serveur qui héberge la partie.</p>
      <label>
        Pseudo
        <input value={name} onChange={(e) => setName(e.target.value)} />
      </label>
      <label>
        Hôte
        <input value={host} onChange={(e) => setHost(e.target.value)} />
      </label>
      <label>
        Port
        <input
          value={port}
          onChange={(e) => setPort(Number(e.target.value) || 0)}
        />
      </label>
      <button className="primary" onClick={doConnect} disabled={busy}>
        {busy ? "Connexion…" : "Se connecter"}
      </button>
      {err && <p className="error">{err}</p>}
      <p className="status">Statut : {status}</p>
    </div>
  );
}
