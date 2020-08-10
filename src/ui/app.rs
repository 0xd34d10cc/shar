use iced::{executor, image, Application, Command, Element, Subscription};
use iced::pane_grid::{self, Axis, Direction, DragEvent, ResizeEvent};

use super::views::{MainView, View, SubMessage};

type ViewID = pane_grid::Pane;

pub struct App {
    views: pane_grid::State<View>,
    views_created: usize,
}

#[derive(Debug)]
pub enum Message {
    Split(ViewID, Axis),
    SplitFocused(Axis),
    FocusAdjacent(Direction),
    Dragged(DragEvent),
    Resized(ResizeEvent),
    Close(ViewID),
    CloseFocused,

    SubMessage(ViewID, SubMessage)
}

impl Application for App {
    type Flags = ();
    type Message = Message;
    type Executor = executor::Default;

    fn new(_flags: ()) -> (Self, Command<Self::Message>) {
        let first_view = View::Main(MainView::default());
        let (views, _) = pane_grid::State::new(first_view);
        let app = App {
            views,
            views_created: 1,
        };
        let startup = Command::none();
        (app, startup)
    }

    fn title(&self) -> String {
        String::from("shar")
    }

    fn subscription(&self) -> Subscription<Self::Message> {
        Subscription::none()
    }

    fn update(&mut self, message: Message) -> Command<Self::Message> {
        match message {
            Message::Split(view_id, axis) => {
                let _ = self
                    .views
                    .split(axis, &view_id, View::Main(MainView::default()));

                self.views_created += 1;
            }
            Message::SplitFocused(axis) => {
                if let Some(view_id) = self.views.active() {
                    let _ = self
                        .views
                        .split(axis, &view_id, View::Main(MainView::default()));

                    self.views_created += 1;
                }
            }
            Message::FocusAdjacent(direction) => {
                if let Some(view_id) = self.views.active() {
                    if let Some(adjacent) = self.views.adjacent(&view_id, direction) {
                        self.views.focus(&adjacent);
                    }
                }
            }
            Message::Resized(ResizeEvent { split, ratio }) => {
                self.views.resize(&split, ratio);
            }
            Message::Dragged(DragEvent::Dropped { pane, target }) => {
                self.views.swap(&pane, &target);
            }
            Message::Dragged(_) => {}
            Message::Close(view_id) => {
                let _ = self.views.close(&view_id);
            }
            Message::CloseFocused => {
                if let Some(view_id) = self.views.active() {
                    let _ = self.views.close(&view_id);
                }
            }
            Message::SubMessage(view_id, message) => {
                if let Some(view) = self.views.get_mut(&view_id) {
                    let id = view_id.clone();
                    return view.update(message)
                        .map(move |message| Message::SubMessage(id, message));
                }
            }
        }

        Command::none()
    }

    fn view(&mut self) -> Element<Self::Message> {
        todo!()
    }
}
