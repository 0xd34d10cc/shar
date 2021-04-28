use std::ops::Deref;
use std::sync::Arc;

use iced::{text_input, Column, Command, Element, Radio, Row, Subscription, Text, TextInput};
use url::Url;

use crate::state::AppState;
use crate::ui::style::Theme;

#[derive(Debug, Clone)]
pub enum Update {
    SetTheme(Theme),
    SetSeluneServer(String),
}

#[derive(Default)]
struct Widgets {
    selune_server: text_input::State,
}

pub struct AppConfigView {
    state: Arc<AppState>,
    selune_server: String,
    ui: Widgets,
}

impl AppConfigView {
    pub fn new(state: Arc<AppState>) -> Self {
        let c = state.config.load();
        let selune_server = c.selune_server.as_str().to_owned();
        AppConfigView {
            state,
            selune_server,

            ui: Widgets::default(),
        }
    }

    pub fn update(&mut self, update: Update) -> Command<Update> {
        match update {
            Update::SetTheme(theme) => {
                self.state.config.rcu(|c| {
                    let mut data = c.deref().clone();
                    data.theme = theme;
                    data
                });
            }
            Update::SetSeluneServer(address) => {
                self.selune_server = address;
                if let Ok(address) = Url::parse(&self.selune_server) {
                    self.state.config.rcu(move |c| {
                        let mut data = c.deref().clone();
                        data.selune_server = address.clone();
                        data
                    });
                }
            }
        }

        Command::none()
    }

    pub fn subscription(&self) -> Subscription<Update> {
        Subscription::none()
    }

    pub fn view(&mut self) -> Element<Update> {
        let config = self.state.config.load();
        let style = config.theme;

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

        let selune_server = Row::new().push(Text::new("Selune server")).push(
            TextInput::new(
                &mut self.ui.selune_server,
                "address",
                &self.selune_server,
                Update::SetSeluneServer,
            )
            .style(style),
        );

        let stun_servers = Row::new();

        let view = Column::new()
            .push(choose_theme)
            .push(selune_server)
            .push(stun_servers);

        Element::new(view)
    }
}
