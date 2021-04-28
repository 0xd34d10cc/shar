use std::fs::File;
use std::io::ErrorKind;
use std::path::PathBuf;
use std::sync::Arc;

use anyhow::Result;
use arc_swap::ArcSwap;
use serde::{Deserialize, Serialize};
use url::Url;

use crate::ui::style::Theme;

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct ConfigData {
    pub theme: Theme,
    pub selune_server: Url,
    pub stun_server: Vec<String>,
}

impl Default for ConfigData {
    fn default() -> Self {
        Self {
            theme: Theme::Dark,
            selune_server: Url::parse("ws://127.0.0.1:8088/streams").unwrap(),
            stun_server: vec![
                "stun1.l.google.com:19302".to_owned(),
                "stun2.l.google.com:19302".to_owned(),
                "stun3.l.google.com:19302".to_owned(),
                "stun4.l.google.com:19302".to_owned(),
            ],
        }
    }
}

pub type Config = Arc<ArcSwap<ConfigData>>;

const CONFIG_FILENAME: &str = "shar.json";

pub fn path() -> Result<PathBuf> {
    let dir = if let Some(dir) = dirs::config_dir() {
        dir
    } else {
        std::env::current_dir()?
    };

    Ok(dir.join(CONFIG_FILENAME))
}

pub fn save(config: &ConfigData) -> Result<()> {
    let mut file = File::create(path()?)?;
    serde_json::to_writer_pretty(&mut file, config)?;
    Ok(())
}

pub fn load() -> Result<Config> {
    let config_file = match File::open(path()?) {
        Ok(file) => file,
        Err(e) if e.kind() == ErrorKind::NotFound => return Ok(Config::default()),
        Err(e) => return Err(e.into()),
    };

    let config: ConfigData = serde_json::from_reader(config_file)?;
    Ok(Arc::from(ArcSwap::from_pointee(config)))
}
