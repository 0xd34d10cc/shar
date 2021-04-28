use crate::net::selune::{StreamDescription, StreamId};
use crate::state::AppState;
use iced::{button, Button, Column, Command, Element, Row, Subscription, Text};
use std::collections::HashMap;
use std::sync::Arc;

#[derive(Debug, Clone)]
pub enum Update {
    RequestStreams,
    Disconnected,
    ListStreams(HashMap<String, StreamDescription>),
}

#[derive(Default)]
struct Widgets {
    update: button::State,
    view_buttons: Vec<button::State>,
}

pub struct StreamsView {
    state: Arc<AppState>,
    streams: Vec<(StreamId, StreamDescription)>,
    ui: Widgets,
}

impl StreamsView {
    pub fn new(state: Arc<AppState>) -> (Self, Command<Update>) {
        let view = StreamsView {
            state,
            streams: Vec::new(),
            ui: Widgets::default(),
        };

        let c = Command::from(list_streams(view.state.clone()));
        (view, c)
    }

    pub fn update(&mut self, update: Update) -> Command<Update> {
        match update {
            Update::RequestStreams => return Command::from(list_streams(self.state.clone())),
            Update::ListStreams(streams) => {
                self.streams = streams.into_iter().collect();
            }
            Update::Disconnected => {
                log::warn!("Disconnected");
            }
        }

        Command::none()
    }

    pub fn subscription(&self) -> Subscription<Update> {
        Subscription::none()
    }

    pub fn view(&mut self) -> Element<Update> {
        let style = self.state.config.load().theme;
        self.ui
            .view_buttons
            .resize_with(self.streams.len(), button::State::default);

        let mut list = Column::new();
        for (button, (id, stream)) in self.ui.view_buttons.iter_mut().zip(self.streams.iter()) {
            let line = Row::new()
                .push(Button::new(button, Text::new("View")).style(style))
                .push(Text::new(id))
                .push(Text::new(&stream.address))
                .push(Text::new(&stream.description))
                .spacing(10);

            list = list.push(line);
        }

        list = list.push(
            Button::new(&mut self.ui.update, Text::new("Update"))
                .on_press(Update::RequestStreams)
                .style(style),
        );

        Element::new(list)
    }
}

async fn list_streams(state: Arc<AppState>) -> Update {
    let mut client = state.selune.lock().await;
    if let Some(client) = client.as_mut() {
        let response = client.list_streams().await;
        Update::ListStreams(response.unwrap()) // FIXME
    } else {
        Update::Disconnected
    }
}
