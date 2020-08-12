use iced::{Command, Element, Subscription};

mod main;

pub use main::MainView;

#[derive(Debug)]
pub enum ViewUpdate {
    Main(main::Update),
}

pub enum View {
    Main(MainView),
}

impl View {
    pub fn subscription(&self) -> Subscription<ViewUpdate> {
        match self {
            View::Main(view) => view.subscription().map(ViewUpdate::Main),
        }
    }

    pub fn update(&mut self, message: ViewUpdate) -> Command<ViewUpdate> {
        match self {
            View::Main(view) => {
                let ViewUpdate::Main(update) = message;
                view.update(update).map(ViewUpdate::Main)
            }
        }
    }

    pub fn view(&mut self) -> Element<ViewUpdate> {
        match self {
            View::Main(view) => view.view().map(ViewUpdate::Main),
        }
    }
}
