use anyhow::Result;
use iced::{Application, Settings};
use std::sync::Arc;
use tokio::sync::Mutex;

mod capture;
mod codec;
mod config;
mod net;
mod resolve;
mod state;
mod stream;
mod ui;
mod view;

use config::Config;
use state::AppState;
use ui::App;

fn main() -> Result<()> {
    // Enable logging by default
    // TODO: remove for release
    std::env::set_var("RUST_LOG", "shar=debug");
    dotenv::dotenv().ok();
    env_logger::init();
    let config = config::load().unwrap_or_else(|e| {
        log::error!("Failed to read configuration file: {}", e);
        Config::default()
    });

    let state = AppState {
        config,
        selune: Mutex::new(None),
    };

    App::run(Settings::with_flags(Arc::new(state)))?;
    Ok(())
}
