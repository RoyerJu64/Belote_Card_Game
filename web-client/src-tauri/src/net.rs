//! Voie B transport bridge — a **dumb pipe** between the PixiJS frontend and the
//! C++ server's TCP socket.
//!
//! It understands exactly one thing: the wire framing `[u32 big-endian length]
//! [payload bytes]`. It never parses the payload — the payload is opaque JSON that
//! only the C++ server and the frontend understand. This keeps the protocol schema
//! in a single place (C++) and makes a future pure-browser client a matter of
//! swapping this pipe for a WebSocket, with the frontend unchanged.
//!
//! Frontend contract (Tauri IPC):
//!   - command `net_connect(host, port)` / `net_send(data)` / `net_disconnect()`
//!   - event `net://frame`  : payload = one JSON string pushed by the server
//!   - event `net://status` : "connected" | "disconnected" | "error: <msg>"

use std::io::{Read, Write};
use std::net::TcpStream;
use std::sync::Mutex;

use tauri::{AppHandle, Emitter, State};

/// Holds the write half of the active connection (the reader thread owns its own
/// cloned handle, so writes never contend with reads).
#[derive(Default)]
pub struct NetState {
    writer: Mutex<Option<TcpStream>>,
}

const MAX_FRAME: u32 = 1 << 20; // 1 MiB guard, mirrors the C++ side

fn frame(payload: &[u8]) -> Vec<u8> {
    let n = payload.len() as u32;
    let mut out = Vec::with_capacity(4 + payload.len());
    out.extend_from_slice(&n.to_be_bytes());
    out.extend_from_slice(payload);
    out
}

#[tauri::command]
pub fn net_connect(
    app: AppHandle,
    state: State<'_, NetState>,
    host: String,
    port: u16,
) -> Result<(), String> {
    // Replace any previous connection.
    if let Ok(mut guard) = state.writer.lock() {
        if let Some(old) = guard.take() {
            let _ = old.shutdown(std::net::Shutdown::Both);
        }
    }

    let stream = TcpStream::connect((host.as_str(), port)).map_err(|e| e.to_string())?;
    stream.set_nodelay(true).ok();
    let reader = stream.try_clone().map_err(|e| e.to_string())?;

    *state.writer.lock().unwrap() = Some(stream);
    let _ = app.emit("net://status", "connected");

    // Reader thread: peel frames off the socket and forward each payload as one
    // `net://frame` event. Owns its own cloned handle, so it can block on reads.
    std::thread::spawn(move || {
        let mut sock = reader;
        let mut header = [0u8; 4];
        loop {
            if let Err(_) = sock.read_exact(&mut header) {
                break; // peer closed or error
            }
            let len = u32::from_be_bytes(header);
            if len > MAX_FRAME {
                let _ = app.emit("net://status", "error: frame too large");
                break;
            }
            let mut body = vec![0u8; len as usize];
            if let Err(_) = sock.read_exact(&mut body) {
                break;
            }
            let text = String::from_utf8_lossy(&body).into_owned();
            let _ = app.emit("net://frame", text);
        }
        let _ = app.emit("net://status", "disconnected");
    });

    Ok(())
}

#[tauri::command]
pub fn net_send(state: State<'_, NetState>, data: String) -> Result<(), String> {
    let mut guard = state.writer.lock().map_err(|e| e.to_string())?;
    let stream = guard.as_mut().ok_or("not connected")?;
    stream.write_all(&frame(data.as_bytes())).map_err(|e| e.to_string())?;
    stream.flush().map_err(|e| e.to_string())?;
    Ok(())
}

#[tauri::command]
pub fn net_disconnect(state: State<'_, NetState>) -> Result<(), String> {
    if let Ok(mut guard) = state.writer.lock() {
        if let Some(stream) = guard.take() {
            let _ = stream.shutdown(std::net::Shutdown::Both);
        }
    }
    Ok(())
}
