use iced::{Command, Element, Subscription};

mod app_config;
mod main;
mod streams;

pub use app_config::AppConfigView;
pub use main::MainView;
pub use streams::StreamsView;

#[derive(Debug)]
pub enum ViewUpdate {
    Main(main::Update),
    AppConfig(app_config::Update),
    Streams(streams::Update),
}

pub enum View {
    Main(MainView),
    AppConfig(AppConfigView),
    Streams(StreamsView),
}

impl View {
    pub fn subscription(&self) -> Subscription<ViewUpdate> {
        match self {
            View::Main(view) => view.subscription().map(ViewUpdate::Main),
            View::AppConfig(config) => config.subscription().map(ViewUpdate::AppConfig),
            View::Streams(streams) => streams.subscription().map(ViewUpdate::Streams),
        }
    }

    pub fn update(&mut self, message: ViewUpdate) -> Command<ViewUpdate> {
        match (self, message) {
            (View::Main(view), ViewUpdate::Main(update)) => {
                view.update(update).map(ViewUpdate::Main)
            }
            (View::AppConfig(view), ViewUpdate::AppConfig(update)) => {
                view.update(update).map(ViewUpdate::AppConfig)
            }
            (View::Streams(view), ViewUpdate::Streams(update)) => {
                view.update(update).map(ViewUpdate::Streams)
            }
            (view, update) => {
                panic!(
                    "Unmatched view & update: view_id={:?} and {:?}",
                    std::mem::discriminant(&*view),
                    update
                );
            }
        }
    }

    pub fn view(&mut self) -> Element<ViewUpdate> {
        match self {
            View::Main(view) => view.view().map(ViewUpdate::Main),
            View::AppConfig(view) => view.view().map(ViewUpdate::AppConfig),
            View::Streams(view) => view.view().map(ViewUpdate::Streams),
        }
    }
}
