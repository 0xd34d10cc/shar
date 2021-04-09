use anyhow::Result;
use iced::{Application, Settings};

mod capture;
mod codec;
mod net;
mod resolve;
mod stream;
mod ui;
mod view;

use ui::App;

fn main() -> Result<()> {
    dotenv::dotenv().ok();
    env_logger::init();
    App::run(Settings::with_flags(()))?;
    Ok(())
}
