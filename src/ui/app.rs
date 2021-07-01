use iced::keyboard::{self, KeyCode};
use iced::pane_grid::{self, Axis, Content, Direction, DragEvent, PaneGrid, ResizeEvent};
use iced::{executor, Application, Clipboard, Command, Container, Element, Length, Subscription};
use iced_native::{subscription, Event};
use std::sync::Arc;

use super::views::{AppConfigView, MainView, StreamsView, View, ViewUpdate};
use crate::config;
use crate::net::SeluneClient;
use crate::state::AppState;

type ViewId = pane_grid::Pane;

#[derive(Debug)]
pub enum ViewsGridUpdate {
    OpenStreams,
    ToggleConfig,
    Open(Axis), // TODO: add params
    SplitFocused(Axis),
    FocusAdjacent(Direction),
    Dragged(DragEvent),
    Resized(ResizeEvent),
    Clicked(ViewId),
    CloseFocused,
    Connected(Box<(SeluneClient, tokio::sync::mpsc::Receiver<crate::net::selune::NewViewer>)>),

    Nop,
}

#[derive(Debug)]
pub enum Message {
    ViewsGridUpdate(ViewsGridUpdate),
    ViewUpdate(ViewId, ViewUpdate),
}

pub struct App {
    state: Arc<AppState>,
    views: pane_grid::State<View>,
    active_view: ViewId,
    config_view: Option<ViewId>,
}

impl Drop for App {
    fn drop(&mut self) {
        let c = self.state.config.load();
        if let Err(e) = config::save(&*c) {
            log::error!("Failed to save config file: {}", e);
        }
    }
}

impl App {
    fn update_views(&mut self, update: ViewsGridUpdate) -> Command<Message> {
        match update {
            ViewsGridUpdate::OpenStreams => {
                let (view, startup) = StreamsView::new(self.state.clone());
                let view = View::Streams(view);
                let (id, _split) = self
                    .views
                    .split(Axis::Horizontal, &self.active_view, view)
                    .unwrap();
                return startup
                    .map(move |update| Message::ViewUpdate(id, ViewUpdate::Streams(update)));
            }
            ViewsGridUpdate::ToggleConfig => {
                if let Some(id) = self.config_view {
                    let (_view, adjasent) = self.views.close(&id).unwrap();
                    if self.active_view == id {
                        self.active_view = adjasent;
                    }
                    self.config_view = None;
                } else {
                    let view = View::AppConfig(AppConfigView::new(self.state.clone()));
                    let (id, _split) = self
                        .views
                        .split(Axis::Horizontal, &self.active_view, view)
                        .unwrap();
                    self.active_view = id;
                    self.config_view = Some(id);
                }
            }
            ViewsGridUpdate::Open(axis) => {
                let view = View::Main(MainView::new(self.state.clone()));
                let (id, _split) = self.views.split(axis, &self.active_view, view).unwrap();
                self.active_view = id;
            }
            ViewsGridUpdate::SplitFocused(axis) => {
                let view = View::Main(MainView::new(self.state.clone()));
                let (id, _split) = self.views.split(axis, &self.active_view, view).unwrap();
                self.active_view = id;
            }
            ViewsGridUpdate::FocusAdjacent(direction) => {
                if let Some(adjacent) = self.views.adjacent(&self.active_view, direction) {
                    self.active_view = adjacent;
                }
            }
            ViewsGridUpdate::Resized(ResizeEvent { split, ratio }) => {
                self.views.resize(&split, ratio);
            }
            ViewsGridUpdate::Dragged(DragEvent::Dropped { pane, target }) => {
                self.views.swap(&pane, &target);
            }
            ViewsGridUpdate::Dragged(_) => {}
            ViewsGridUpdate::Clicked(id) => {
                self.active_view = id;
            }
            ViewsGridUpdate::CloseFocused => {
                if self.views.len() > 1 {
                    let (_view, id) = self.views.close(&self.active_view).unwrap();
                    self.active_view = id;
                }
            }
            ViewsGridUpdate::Connected(b) => {
                let (client, receiver) = *b;
                let state = self.state.clone();
                return Command::from(async move {
                    let mut selune = state.selune.lock().await;
                    *selune = Some((client, receiver));
                    Message::ViewsGridUpdate(ViewsGridUpdate::Nop)
                });
            }
            ViewsGridUpdate::Nop => {}
        }

        Command::none()
    }
}

impl Application for App {
    type Flags = Arc<AppState>;
    type Message = Message;
    type Executor = executor::Default;

    fn new(state: Arc<AppState>) -> (Self, Command<Self::Message>) {
        let first_view = View::Main(MainView::new(state.clone()));
        let (views, id) = pane_grid::State::new(first_view);
        let app = App {
            views,
            active_view: id,
            state,
            config_view: None,
        };
        let address = app.state.config.load().selune_server.clone();
        let startup = Command::from(async move {
            let (sender, receiver) = tokio::sync::mpsc::channel(10);
            let client = SeluneClient::connect(address.as_str(), sender).await.unwrap();
            Message::ViewsGridUpdate(ViewsGridUpdate::Connected(Box::new((client, receiver))))
        });
        (app, startup)
    }

    fn title(&self) -> String {
        String::from("shar")
    }

    fn subscription(&self) -> Subscription<Self::Message> {
        let views_events = self.views.iter().map(|(id, view)| {
            let id = *id;
            view.subscription()
                .with(id)
                .map(|(id, update)| Message::ViewUpdate(id, update))
        });

        let keyboard_events = subscription::events_with(|event, _status| match event {
            Event::Keyboard(keyboard::Event::KeyPressed {
                modifiers,
                key_code,
            }) => key_to_message(modifiers, key_code),
            _ => None,
        });

        Subscription::batch(Some(keyboard_events).into_iter().chain(views_events))
    }

    fn update(&mut self, message: Message, _clipboard: &mut Clipboard) -> Command<Self::Message> {
        match message {
            Message::ViewsGridUpdate(update) => self.update_views(update),
            Message::ViewUpdate(view_id, message) => {
                if let Some(view) = self.views.get_mut(&view_id) {
                    let id = view_id;
                    view.update(message)
                        .map(move |message| Message::ViewUpdate(id, message))
                } else {
                    Command::none()
                }
            }
        }
    }

    fn view(&mut self) -> Element<Self::Message> {
        let style = self.state.config.load().theme;
        let grid = PaneGrid::new(&mut self.views, |id, view| {
            let content = view
                .view()
                .map(move |update| Message::ViewUpdate(id, update));

            Content::new(content)
        })
        // .style(style)
        .width(Length::Fill)
        .height(Length::Fill)
        .spacing(5)
        .on_drag(|event| Message::ViewsGridUpdate(ViewsGridUpdate::Dragged(event)))
        .on_click(|id| Message::ViewsGridUpdate(ViewsGridUpdate::Clicked(id)))
        .on_resize(10, |event| {
            Message::ViewsGridUpdate(ViewsGridUpdate::Resized(event))
        });

        Container::new(grid)
            .style(style)
            .width(Length::Fill)
            .height(Length::Fill)
            .padding(5)
            .into()
    }
}

fn key_to_message(modifiers: keyboard::Modifiers, code: KeyCode) -> Option<Message> {
    let cmd = modifiers.is_command_pressed();
    let update = match code {
        KeyCode::Up if cmd => ViewsGridUpdate::FocusAdjacent(Direction::Up),
        KeyCode::Down if cmd => ViewsGridUpdate::FocusAdjacent(Direction::Down),
        KeyCode::Left if cmd => ViewsGridUpdate::FocusAdjacent(Direction::Left),
        KeyCode::Right if cmd => ViewsGridUpdate::FocusAdjacent(Direction::Right),
        KeyCode::V if cmd => ViewsGridUpdate::SplitFocused(Axis::Vertical),
        KeyCode::H if cmd => ViewsGridUpdate::SplitFocused(Axis::Horizontal),
        KeyCode::W if cmd => ViewsGridUpdate::CloseFocused,
        KeyCode::D if cmd => ViewsGridUpdate::Open(Axis::Horizontal),
        KeyCode::F1 => ViewsGridUpdate::ToggleConfig,
        KeyCode::F2 => ViewsGridUpdate::OpenStreams,
        _ => return None,
    };

    Some(Message::ViewsGridUpdate(update))
}
