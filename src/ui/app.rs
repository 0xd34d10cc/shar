use iced::pane_grid::{self, Axis, Direction, DragEvent, PaneGrid, ResizeEvent};
use iced::{executor, Application, Command, Container, Element, Length, Subscription};

use super::views::{MainView, View, ViewUpdate};

type ViewID = pane_grid::Pane;

#[derive(Debug)]
pub enum ViewsGridUpdate {
    Open(Axis), // TODO: add params
    SplitFocused(Axis),
    FocusAdjacent(Direction),
    Dragged(DragEvent),
    Resized(ResizeEvent),
    // Close(ViewID),
    CloseFocused,
}

#[derive(Debug)]
pub enum Message {
    ViewsGridUpdate(ViewsGridUpdate),
    ViewUpdate(ViewID, ViewUpdate),
}

pub struct App {
    views: pane_grid::State<View>,
}

impl App {
    fn update_views(&mut self, update: ViewsGridUpdate) -> Command<ViewsGridUpdate> {
        match update {
            ViewsGridUpdate::Open(axis) => {
                if let Some(view_id) = self.views.active() {
                    let _ = self
                        .views
                        .split(axis, &view_id, View::Main(MainView::default()));
                }
            }
            ViewsGridUpdate::SplitFocused(axis) => {
                if let Some(view_id) = self.views.active() {
                    let _ = self
                        .views
                        .split(axis, &view_id, View::Main(MainView::default()));
                }
            }
            ViewsGridUpdate::FocusAdjacent(direction) => {
                if let Some(view_id) = self.views.active() {
                    if let Some(adjacent) = self.views.adjacent(&view_id, direction) {
                        self.views.focus(&adjacent);
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
            // ViewsGridUpdate::Close(view_id) => {
            //     let _ = self.views.close(&view_id);
            // }
            ViewsGridUpdate::CloseFocused => {
                if let Some(view_id) = self.views.active() {
                    let _ = self.views.close(&view_id);
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
        let (views, _) = pane_grid::State::new(first_view);
        let app = App { views };
        let startup = Command::none();
        (app, startup)
    }

    fn title(&self) -> String {
        String::from("shar")
    }

    fn subscription(&self) -> Subscription<Self::Message> {
        Subscription::batch(self.views.iter().map(|(id, view)| {
            let id = id.clone();
            view.subscription()
                .map(move |update| Message::ViewUpdate(id, update))
        }))
    }

    fn update(&mut self, message: Message) -> Command<Self::Message> {
        match message {
            Message::ViewsGridUpdate(update) => {
                self.update_views(update).map(Message::ViewsGridUpdate)
            }
            Message::ViewUpdate(view_id, message) => {
                if let Some(view) = self.views.get_mut(&view_id) {
                    let id = view_id.clone();
                    view.update(message)
                        .map(move |message| Message::ViewUpdate(id, message))
                } else {
                    Command::none()
                }
            }
        }
    }

    fn view(&mut self) -> Element<Self::Message> {
        let grid = PaneGrid::new(&mut self.views, |id, view, _focus| {
            // TODO: style differentely depending on |focus|
            view.view()
                .map(move |update| Message::ViewUpdate(id, update))
        })
        .width(Length::Fill)
        .height(Length::Fill)
        .spacing(5)
        .on_drag(|event| Message::ViewsGridUpdate(ViewsGridUpdate::Dragged(event)))
        .on_resize(|event| Message::ViewsGridUpdate(ViewsGridUpdate::Resized(event)))
        .on_key_press(handle_hotkey);

        Container::new(grid)
            .width(Length::Fill)
            .height(Length::Fill)
            .padding(5)
            .into()
    }
}

fn handle_hotkey(event: pane_grid::KeyPressEvent) -> Option<Message> {
    use iced::keyboard::KeyCode;

    let update = match event.key_code {
        KeyCode::Up => Some(ViewsGridUpdate::FocusAdjacent(Direction::Up)),
        KeyCode::Down => Some(ViewsGridUpdate::FocusAdjacent(Direction::Down)),
        KeyCode::Left => Some(ViewsGridUpdate::FocusAdjacent(Direction::Left)),
        KeyCode::Right => Some(ViewsGridUpdate::FocusAdjacent(Direction::Right)),
        KeyCode::V => Some(ViewsGridUpdate::SplitFocused(Axis::Vertical)),
        KeyCode::H => Some(ViewsGridUpdate::SplitFocused(Axis::Horizontal)),
        KeyCode::W => Some(ViewsGridUpdate::CloseFocused),
        KeyCode::D => Some(ViewsGridUpdate::Open(Axis::Horizontal)),
        _ => None,
    };

    update.map(Message::ViewsGridUpdate)
}
