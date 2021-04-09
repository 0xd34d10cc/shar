use iced::keyboard::{self, KeyCode};
use iced::pane_grid::{self, Axis, Content, Direction, DragEvent, PaneGrid, ResizeEvent};
use iced::{executor, Application, Clipboard, Command, Container, Element, Length, Subscription};
use iced_native::{subscription, Event};

use super::views::{MainView, View, ViewUpdate};

type ViewID = pane_grid::Pane;

#[derive(Debug)]
pub enum ViewsGridUpdate {
    Open(Axis), // TODO: add params
    SplitFocused(Axis),
    FocusAdjacent(Direction),
    Dragged(DragEvent),
    Resized(ResizeEvent),
    Clicked(ViewID),
    CloseFocused,
}

#[derive(Debug)]
pub enum Message {
    ViewsGridUpdate(ViewsGridUpdate),
    ViewUpdate(ViewID, ViewUpdate),
}

pub struct App {
    views: pane_grid::State<View>,
    active_view: Option<ViewID>,
}

impl App {
    fn update_views(&mut self, update: ViewsGridUpdate) -> Command<ViewsGridUpdate> {
        match update {
            ViewsGridUpdate::Open(axis) => {
                if let Some(view_id) = self.active_view {
                    let (id, _split) = self
                        .views
                        .split(axis, &view_id, View::Main(MainView::default()))
                        .unwrap();
                    self.active_view = Some(id);
                }
            }
            ViewsGridUpdate::SplitFocused(axis) => {
                if let Some(view_id) = self.active_view {
                    let (id, _split) = self
                        .views
                        .split(axis, &view_id, View::Main(MainView::default()))
                        .unwrap();
                    self.active_view = Some(id);
                }
            }
            ViewsGridUpdate::FocusAdjacent(direction) => {
                if let Some(view_id) = self.active_view {
                    if let Some(adjacent) = self.views.adjacent(&view_id, direction) {
                        self.active_view = Some(adjacent);
                    }
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
                self.active_view = Some(id);
            }
            ViewsGridUpdate::CloseFocused => {
                if let Some(view_id) = self.active_view.take() {
                    let (_view, id) = self.views.close(&view_id).unwrap();
                    self.active_view = Some(id);
                }
            }
        }

        Command::none()
    }
}

impl Application for App {
    type Flags = ();
    type Message = Message;
    type Executor = executor::Default;

    fn new(_flags: ()) -> (Self, Command<Self::Message>) {
        let first_view = View::Main(MainView::default());
        let (views, id) = pane_grid::State::new(first_view);
        let app = App {
            active_view: Some(id),
            views,
        };
        let startup = Command::none();
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
            }) if modifiers.is_command_pressed() => key_to_message(key_code),
            _ => None,
        });

        Subscription::batch(Some(keyboard_events).into_iter().chain(views_events))
    }

    fn update(&mut self, message: Message, _clipboard: &mut Clipboard) -> Command<Self::Message> {
        match message {
            Message::ViewsGridUpdate(update) => {
                self.update_views(update).map(Message::ViewsGridUpdate)
            }
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
        let grid = PaneGrid::new(&mut self.views, |id, view| {
            let content = view
                .view()
                .map(move |update| Message::ViewUpdate(id, update));

            Content::new(content)
        })
        .width(Length::Fill)
        .height(Length::Fill)
        .spacing(5)
        .on_drag(|event| Message::ViewsGridUpdate(ViewsGridUpdate::Dragged(event)))
        .on_click(|id| Message::ViewsGridUpdate(ViewsGridUpdate::Clicked(id)))
        .on_resize(10, |event| {
            Message::ViewsGridUpdate(ViewsGridUpdate::Resized(event))
        });

        Container::new(grid)
            .width(Length::Fill)
            .height(Length::Fill)
            .padding(5)
            .into()
    }
}

fn key_to_message(code: KeyCode) -> Option<Message> {
    let update = match code {
        KeyCode::Up => ViewsGridUpdate::FocusAdjacent(Direction::Up),
        KeyCode::Down => ViewsGridUpdate::FocusAdjacent(Direction::Down),
        KeyCode::Left => ViewsGridUpdate::FocusAdjacent(Direction::Left),
        KeyCode::Right => ViewsGridUpdate::FocusAdjacent(Direction::Right),
        KeyCode::V => ViewsGridUpdate::SplitFocused(Axis::Vertical),
        KeyCode::H => ViewsGridUpdate::SplitFocused(Axis::Horizontal),
        KeyCode::W => ViewsGridUpdate::CloseFocused,
        KeyCode::D => ViewsGridUpdate::Open(Axis::Horizontal),
        _ => return None,
    };

    Some(Message::ViewsGridUpdate(update))
}
