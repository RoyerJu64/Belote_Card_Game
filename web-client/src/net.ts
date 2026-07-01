// Thin frontend wrapper over the Voie B transport bridge (see src-tauri/src/net.rs).
// The frontend only ever speaks JSON events — it never touches the binary codec.

import { invoke } from "@tauri-apps/api/core";
import { listen, type UnlistenFn } from "@tauri-apps/api/event";

/** A server event: `{ t: "hello", ... }`. `t` is the discriminator. */
export type ServerEvent = { t: string; [k: string]: unknown };

/** A client message sent to the server, same shape. */
export type ClientMessage = { t: string; [k: string]: unknown };

export type ConnStatus = "connected" | "disconnected" | string;

export async function connect(host: string, port: number): Promise<void> {
  await invoke("net_connect", { host, port });
}

export async function send(msg: ClientMessage): Promise<void> {
  await invoke("net_send", { data: JSON.stringify(msg) });
}

export async function disconnect(): Promise<void> {
  await invoke("net_disconnect");
}

/** Subscribe to server-pushed JSON events. Returns an unlisten function. */
export async function onEvent(cb: (ev: ServerEvent) => void): Promise<UnlistenFn> {
  return listen<string>("net://frame", (e) => {
    try {
      cb(JSON.parse(e.payload) as ServerEvent);
    } catch {
      cb({ t: "raw", payload: e.payload });
    }
  });
}

/** Subscribe to transport status changes. Returns an unlisten function. */
export async function onStatus(cb: (s: ConnStatus) => void): Promise<UnlistenFn> {
  return listen<string>("net://status", (e) => cb(e.payload));
}
