use anyhow::Result;
use iced::{Application, Settings};

mod capture;
mod codec;
mod config;
mod net;
mod resolve;
mod stream;
mod ui;
mod view;

use config::Config;
use ui::App;

fn main() -> Result<()> {
    dotenv::dotenv().ok();
    env_logger::init();
    let config = config::load().unwrap_or_else(|e| {
        log::error!("Failed to read configuration file: {}", e);
        Config::default()
    });

    App::run(Settings::with_flags(config))?;
    Ok(())
}
