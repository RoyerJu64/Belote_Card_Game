import { useEffect, useSyncExternalStore } from "react";
import "./App.css";
import { disconnect } from "./net";
import { initStore, store } from "./store";
import { MainMenu } from "./ui/MainMenu";
import { Lobby } from "./ui/Lobby";
import { GameView } from "./ui/GameView";

export default function App() {
  const st = useSyncExternalStore(store.subscribe, store.getSnapshot);

  useEffect(() => {
    initStore();
  }, []);

  const onLeave = async () => {
    await disconnect();
    store.reset();
  };

  if (st.seat === null) {
    return <MainMenu status={st.status} />;
  }
  if (!st.game || st.game.phase === "lobby") {
    return <Lobby seat={st.seat} seats={st.lobby} />;
  }
  return <GameView game={st.game} notices={st.notices} onLeave={onLeave} />;
}
