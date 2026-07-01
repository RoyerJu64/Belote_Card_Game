mod net;

#[cfg_attr(mobile, tauri::mobile_entry_point)]
pub fn run() {
    tauri::Builder::default()
        .plugin(tauri_plugin_opener::init())
        .manage(net::NetState::default())
        .invoke_handler(tauri::generate_handler![
            net::net_connect,
            net::net_send,
            net::net_disconnect
        ])
        .run(tauri::generate_context!())
        .expect("error while running tauri application");
}
