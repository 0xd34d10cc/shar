use iced::{Command, Element, Subscription};

mod app_config;
mod main;

pub use app_config::AppConfigView;
pub use main::MainView;

#[derive(Debug)]
pub enum ViewUpdate {
    Main(main::Update),
    AppConfig(app_config::Update),
}

pub enum View {
    Main(MainView),
    AppConfig(AppConfigView),
}

impl View {
    pub fn subscription(&self) -> Subscription<ViewUpdate> {
        match self {
            View::Main(view) => view.subscription().map(ViewUpdate::Main),
            View::AppConfig(config) => config.subscription().map(ViewUpdate::AppConfig),
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
        }
    }
}
