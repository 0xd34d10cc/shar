use std::sync::Arc;

use arc_swap::ArcSwap;

use super::style::Theme;

#[derive(Default, Clone)]
pub struct ConfigData {
    pub theme: Theme,
}

pub type Config = Arc<ArcSwap<ConfigData>>;
