use std::sync::Arc;

use iced::{Command, Element, Radio, Row, Subscription, Text};

use crate::ui::config::Config;
use crate::ui::style::Theme;

#[derive(Debug, Clone)]
pub enum Update {
    SetTheme(Theme),
}

pub struct AppConfigView {
    config: Config,
}

impl AppConfigView {
    pub fn new(config: Config) -> Self {
        AppConfigView { config }
    }
    pub fn update(&mut self, update: Update) -> Command<Update> {
        match update {
            Update::SetTheme(theme) => {
                let config = &**self.config.load();
                let mut config = config.clone();
                config.theme = theme;
                self.config.store(Arc::new(config));
            }
        }

        Command::none()
    }

    pub fn subscription(&self) -> Subscription<Update> {
        Subscription::none()
    }

    pub fn view(&mut self) -> Element<Update> {
        let style = self.config.load().theme;
        let choose_theme = Theme::ALL.iter().fold(
            Row::new().push(Text::new("Choose a theme:")),
            |row, theme| {
                row.push(
                    Radio::new(
                        *theme,
                        &format!("{:?}", theme),
                        Some(style),
                        Update::SetTheme,
                    )
                    .style(style),
                )
            },
        );

        Element::new(choose_theme)
    }
}
