use ::image::{load_from_memory_with_format, ImageFormat};
use anyhow::Result;
use iced::image::{self, Image};
use iced::{
    button, executor, text_input, Align, Application, Button, Column, Command, Container, Element,
    Length, Radio, Row, Settings, Subscription, Text, TextInput,
};
use once_cell::sync::OnceCell;
use tokio::sync::mpsc::{self, Sender};
use url::Url;

use crate::capture::DisplayID;
use crate::stream::stream;
use crate::ui::style::Theme;
use crate::view::view;

mod capture;
mod codec;
mod net;
mod resolve;
mod stream;
mod ui;
mod view;

fn background() -> image::Handle {
    const BACKGROUND_IMAGE: &[u8] = include_bytes!("../resources/background.png");
    static BACKGROUND: OnceCell<image::Handle> = OnceCell::new();

    BACKGROUND
        .get_or_init(|| {
            let background = load_from_memory_with_format(BACKGROUND_IMAGE, ImageFormat::PNG)
                .unwrap()
                .to_bgra();
            let (width, height) = background.dimensions();
            let pixels = background.into_raw();
            image::Handle::from_pixels(width, height, pixels)
        })
        .clone()
}

#[derive(Default)]
struct UIState {
    stop: button::State,
    stream: button::State,
    view: button::State,

    url_text: String,
    url: text_input::State,
}

#[derive(Debug)]
enum StreamState {
    Streaming(Url),
    Viewing(Url),
}

struct App {
    state: Option<StreamState>,
    current_frame: image::Handle,
    stream_sender: Option<Sender<image::Handle>>,
    url: Option<Url>,
    num_monitors: usize,
    selected_monitor: Option<DisplayID>,
    theme: Theme,

    ui: UIState,
}

impl Default for App {
    fn default() -> Self {
        let mut app = App {
            state: None,
            current_frame: background(),
            stream_sender: None,
            num_monitors: scrap::Display::all()
                .map(|monitors| monitors.len())
                .unwrap_or(0),
            selected_monitor: None,
            theme: Theme::Dark,

            url: None,
            ui: UIState::default(),
        };

        app.set_url("tcp://127.0.0.1:1337".into());
        app
    }
}

impl App {
    fn set_url(&mut self, url: String) {
        self.url = url.parse().ok();
        self.ui.url_text = url;
    }
}

#[derive(Debug, Clone)]
enum Message {
    Stop,
    Stream,
    View,

    StreamFinished(Url, Option<String>),
    UpdateUrlText(String),
    UpdateFrame(image::Handle),
    UpdateMonitor(DisplayID),
    UpdateTheme(Theme),
}

impl Application for App {
    type Flags = ();
    type Message = Message;
    type Executor = executor::Default;

    fn new(_flags: Self::Flags) -> (Self, Command<Self::Message>) {
        let app = App::default();
        (app, Command::none())
    }

    fn title(&self) -> String {
        String::from("shar")
    }

    fn subscription(&self) -> Subscription<Self::Message> {
        match &self.state {
            None => Subscription::none(),
            Some(StreamState::Streaming(_)) => {
                let handles =
                    capture::capture(self.selected_monitor.unwrap_or(DisplayID::Primary), 30);
                handles.map(Message::UpdateFrame)
            }
            Some(StreamState::Viewing(url)) => view(url.clone()).map(Message::UpdateFrame),
        }
    }

    fn update(&mut self, message: Message) -> Command<Self::Message> {
        match message {
            Message::Stop => {
                self.state = None;
                self.stream_sender.take();
                log::info!("Stopped");
                Command::from(async { Message::UpdateFrame(background()) })
            }
            Message::View => {
                if let Some(ref url) = self.url {
                    self.state = Some(StreamState::Viewing(url.clone()));
                    log::info!("Start viewing from {}", url);
                }
                Command::none()
            }
            Message::Stream => {
                if let Some(ref url) = self.url {
                    self.state = Some(StreamState::Streaming(url.clone()));
                    log::info!("Start streaming to {}", url);

                    let (sender, receiver) = mpsc::channel(5);
                    let url = url.clone();

                    self.stream_sender = Some(sender);
                    return Command::from(async move {
                        // TODO: unhardcode
                        let config = codec::Config {
                            bitrate: 5000 * 1024,
                            fps: 30,
                            gop: 3,
                            width: 1920,
                            height: 1080,
                        };
                        match codec::ffmpeg::Encoder::new(config) {
                            Ok(encoder) => {
                                let status = stream(url.clone(), encoder, receiver).await;
                                Message::StreamFinished(url, status.err().map(|e| e.to_string()))
                            }
                            Err(e) => {
                                log::error!("Failed to create encoder: {}", e);
                                Message::StreamFinished(url, Some(e.to_string()))
                            }
                        }
                    });
                }
                Command::none()
            }
            Message::StreamFinished(url, status) => {
                match status {
                    None => {
                        log::info!("Stream to {} finished successfully", url,);
                    }
                    Some(err) => {
                        log::error!("Stream to {} closed with error: {}", url, err);
                    }
                }
                Command::none()
            }
            Message::UpdateUrlText(url) => {
                self.set_url(url);
                log::info!("Updated url to {:?}", self.url);
                Command::none()
            }
            Message::UpdateFrame(frame) => {
                if let Some(sender) = self.stream_sender.as_mut() {
                    let _ = sender.try_send(frame.clone());
                }

                self.current_frame = frame;
                Command::none()
            }
            Message::UpdateMonitor(id) => {
                self.selected_monitor = Some(id);
                Command::none()
            }
            Message::UpdateTheme(theme) => {
                self.theme = theme;
                Command::none()
            }
        }
    }

    fn view(&mut self) -> Element<Self::Message> {
        let choose_theme = Theme::ALL.iter().fold(
            Row::new().push(Text::new("Choose a theme:")),
            |row, theme| {
                row.push(
                    Radio::new(
                        *theme,
                        &format!("{:?}", theme),
                        Some(self.theme),
                        Message::UpdateTheme,
                    )
                    .style(self.theme),
                )
            },
        );

        let stop = Button::new(&mut self.ui.stop, Text::new("stop"))
            .on_press(Message::Stop)
            .style(self.theme);

        let stream = Button::new(&mut self.ui.stream, Text::new("stream"))
            .on_press(Message::Stream)
            .style(self.theme);

        let view = Button::new(&mut self.ui.view, Text::new("view"))
            .on_press(Message::View)
            .style(self.theme);

        let buttons = Row::new().push(stop).push(stream).push(view);

        let url = TextInput::new(
            &mut self.ui.url,
            "stream url",
            &self.ui.url_text,
            Message::UpdateUrlText,
        )
        .style(self.theme);
        let url = Row::new().push(url);

        let stream_state = Text::new(format!("{:?}", self.state));
        let video_frame = Image::new(self.current_frame.clone());
        let mut monitors = Row::new().push(
            Radio::new(
                DisplayID::Primary,
                "Primary",
                self.selected_monitor,
                Message::UpdateMonitor,
            )
            .style(self.theme),
        );

        for i in 0..self.num_monitors {
            monitors = monitors.push(
                Radio::new(
                    DisplayID::Index(i),
                    i.to_string(),
                    self.selected_monitor,
                    Message::UpdateMonitor,
                )
                .style(self.theme),
            );
        }

        let layout = Column::new()
            .align_items(Align::Center)
            .push(choose_theme)
            .push(buttons)
            .push(url)
            .push(stream_state)
            .push(monitors)
            .push(video_frame);

        let content = Container::new(layout)
            .width(Length::Fill)
            .height(Length::Fill)
            .center_x()
            .center_y()
            .style(self.theme);

        Element::from(content)
    }
}

fn main() -> Result<()> {
    dotenv::dotenv().ok();
    env_logger::init();
    App::run(Settings::with_flags(()));
    Ok(())
}
