use anyhow::Result;
use iced::image::{self, Image};
use iced::{
    button, executor, Application, Button, Column, Command, Element, Settings, Subscription, Text,
};

mod capture;

enum App {
    Waiting {
        start: button::State,
    },
    Capturing {
        stop: button::State,
        display_id: capture::DisplayID,
        current_frame: Option<image::Handle>,
    },
}

#[derive(Debug, Clone)]
enum Message {
    StartCapturing,
    StopCapturing,
    UpdateFrame(image::Handle),
}

impl Application for App {
    type Flags = ();
    type Message = Message;
    type Executor = executor::Default;

    fn new(_flags: Self::Flags) -> (Self, Command<Self::Message>) {
        let app = App::Waiting {
            start: button::State::default(),
        };

        (app, Command::none())
    }

    fn title(&self) -> String {
        String::from("shar")
    }

    fn subscription(&self) -> Subscription<Self::Message> {
        match self {
            App::Capturing { display_id, .. } => {
                capture::capture(*display_id, 30).map(Message::UpdateFrame)
            }
            App::Waiting { .. } => Subscription::none(),
        }
    }

    fn update(&mut self, message: Message) -> Command<Self::Message> {
        match message {
            Message::StartCapturing => {
                *self = App::Capturing {
                    stop: button::State::default(),
                    display_id: capture::DisplayID::Primary,
                    current_frame: None,
                };
            }
            Message::StopCapturing => {
                *self = App::Waiting {
                    start: button::State::default(),
                };
            }
            Message::UpdateFrame(frame) => {
                if let App::Capturing { current_frame, .. } = self {
                    *current_frame = Some(frame);
                }
            }
        }

        Command::none()
    }

    fn view(&mut self) -> Element<Self::Message> {
        match self {
            App::Waiting { start } => {
                let start =
                    Button::new(start, Text::new("start")).on_press(Message::StartCapturing);
                Element::from(start)
            }
            App::Capturing {
                stop,
                current_frame,
                ..
            } => {
                let stop = Button::new(stop, Text::new("stop")).on_press(Message::StopCapturing);

                let layout = Column::new().push(stop);
                let layout = if let Some(frame) = current_frame {
                    layout.push(Image::new(frame.clone()))
                } else {
                    layout
                };

                Element::from(layout)
            }
        }
    }
}

fn main() -> Result<()> {
    dotenv::dotenv().ok();
    env_logger::init();
    App::run(Settings::with_flags(()));
    Ok(())
}
