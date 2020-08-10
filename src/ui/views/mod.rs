use iced::Command;
use iced::image;
use url::Url;

mod main;

use super::style::Theme;
use crate::capture::DisplayID;

pub use main::MainView;


#[derive(Debug, Clone)]
pub enum SubMessage {
    Stop,
    Stream,
    View,

    StreamFinished(Url, Option<String>),
    UpdateUrlText(String),
    UpdateFrame(image::Handle),
    UpdateMonitor(DisplayID),
    UpdateTheme(Theme),
}

pub enum View {
    Main(MainView)
}

impl View {
    pub fn update(&mut self, message: SubMessage) -> Command<SubMessage> {
        match self {
            View::Main(view) => view.update(message)
        }
    }
}