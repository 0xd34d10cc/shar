use crate::config::Config;
use crate::net::SeluneClient;
use tokio::sync::Mutex;

// Global application state
pub struct AppState {
    pub config: Config,
    pub selune: Mutex<Option<SeluneClient>>,
}
