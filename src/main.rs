use anyhow::{Context, Result};
use iced::image::{Handle, Image};
use iced::{button, executor, Application, Button, Column, Command, Element, Settings, Text};
use scrap::{Capturer, Display};

struct App {
    capturer: Capturer,
    screenshot: button::State,
    current_frame: Option<Handle>,
}

#[derive(Debug, Clone)]
enum Message {
    Screenshot,
}

impl Application for App {
    type Flags = Capturer;
    type Message = Message;
    type Executor = executor::Default;

    fn new(capturer: Capturer) -> (Self, Command<Self::Message>) {
        let app = App {
            capturer,
            screenshot: button::State::default(),
            current_frame: None,
        };

        (app, Command::none())
    }

    fn title(&self) -> String {
        String::from("shar")
    }

    fn update(&mut self, message: Message) -> Command<Self::Message> {
        match message {
            Message::Screenshot => {
                match self.capturer.frame() {
                    Ok(frame) => {
                        let pixels = frame.to_vec();
                        let width = self.capturer.width() as u32;
                        let height = self.capturer.height() as u32;

                        self.current_frame = Some(Handle::from_pixels(width, height, pixels));
                    },
                    // frame is not yet ready, resend message after 10ms
                    Err(e) if e.kind() == std::io::ErrorKind::WouldBlock => {
                        return Command::from(async {
                            tokio::time::delay_for(std::time::Duration::from_millis(10)).await;
                            Message::Screenshot
                        });
                    },
                    Err(e) => panic!("Failed to capture a frame: {}", e),
                }
            }
        }

        Command::none()
    }

    fn view(&mut self) -> Element<Self::Message> {
        let column = Column::new().push(
            Button::new(&mut self.screenshot, Text::new("Take screenshot"))
                .on_press(Message::Screenshot),
        );

        if let Some(frame) = self.current_frame.as_ref() {
            column.push(Image::new(frame.clone())).into()
        } else {
            column.into()
        }
    }
}

fn main() -> Result<()> {
    let display = Display::primary().context("Unable to get primary display")?;
    let capturer = Capturer::new(display).context("Unable to create display capturer")?;
    App::run(Settings::with_flags(capturer));
    Ok(())
}
