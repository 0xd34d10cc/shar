use ::image::{load_from_memory_with_format, ImageFormat};
use futures::channel::mpsc::{self, Sender};
use iced::image::{self, Image};
use iced::{
    button, text_input, Align, Button, Column, Command, Container, Element, Length, Radio, Row,
    Subscription, Text, TextInput,
};
use once_cell::sync::OnceCell;
use url::Url;

use crate::capture::DisplayID;
use crate::stream::stream;
use crate::ui::style::Theme;
use crate::view::view;

#[derive(Debug, Clone)]
pub enum Update {
    Stop,
    Stream,
    View,

    StreamFinished(Url, Option<String>),
    SetUrlText(String),
    SetCurrentFrame(image::Handle),
    SetMonitor(DisplayID),
    SetTheme(Theme),
}

fn background() -> image::Handle {
    const BACKGROUND_IMAGE: &[u8] = include_bytes!(concat!(
        env!("CARGO_MANIFEST_DIR"),
        "/resources/background.png"
    ));
    static BACKGROUND: OnceCell<image::Handle> = OnceCell::new();

    BACKGROUND
        .get_or_init(|| {
            let background = load_from_memory_with_format(BACKGROUND_IMAGE, ImageFormat::Png)
                .unwrap()
                .to_bgra8();
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

pub struct MainView {
    state: Option<StreamState>,
    current_frame: image::Handle,
    stream_sender: Option<Sender<image::Handle>>,
    url: Option<Url>,
    num_monitors: usize,
    selected_monitor: Option<DisplayID>,
    theme: Theme,

    ui: UIState,
}

impl Default for MainView {
    fn default() -> Self {
        let mut app = MainView {
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

impl MainView {
    fn set_url(&mut self, url: String) {
        self.url = url.parse().ok();
        self.ui.url_text = url;
    }

    pub fn subscription(&self) -> Subscription<Update> {
        match &self.state {
            None => Subscription::none(),
            Some(StreamState::Streaming(_)) => {
                let handles = crate::capture::capture(
                    self.selected_monitor.unwrap_or(DisplayID::Primary),
                    // FIXME: unhardcode fps
                    60,
                );
                handles.map(Update::SetCurrentFrame)
            }
            Some(StreamState::Viewing(url)) => view(url.clone()).map(Update::SetCurrentFrame),
        }
    }

    pub fn update(&mut self, message: Update) -> Command<Update> {
        match message {
            Update::Stop => {
                self.state = None;
                self.stream_sender.take();
                log::info!("Stopped");
                Command::from(async { Update::SetCurrentFrame(background()) })
            }
            Update::View => {
                if let Some(ref url) = self.url {
                    self.state = Some(StreamState::Viewing(url.clone()));
                    log::info!("Start viewing from {}", url);
                }
                Command::none()
            }
            Update::Stream => {
                if let Some(ref url) = self.url {
                    self.state = Some(StreamState::Streaming(url.clone()));
                    log::info!("Start streaming to {}", url);

                    let (sender, receiver) = mpsc::channel(5);
                    let url = url.clone();

                    self.stream_sender = Some(sender);
                    return Command::from(async move {
                        // TODO: unhardcode
                        let config = crate::codec::Config {
                            bitrate: 5000 * 1024,
                            fps: 30,
                            gop: 3,
                            width: 1920,
                            height: 1080,
                        };
                        match crate::codec::ffmpeg::Encoder::new(config) {
                            Ok(encoder) => {
                                let status = stream(url.clone(), encoder, receiver).await;
                                Update::StreamFinished(url, status.err().map(|e| e.to_string()))
                            }
                            Err(e) => {
                                log::error!("Failed to create encoder: {}", e);
                                Update::StreamFinished(url, Some(e.to_string()))
                            }
                        }
                    });
                }
                Command::none()
            }
            Update::StreamFinished(url, status) => {
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
            Update::SetUrlText(url) => {
                self.set_url(url);
                log::info!("Updated url to {:?}", self.url);
                Command::none()
            }
            Update::SetCurrentFrame(frame) => {
                if let Some(sender) = self.stream_sender.as_mut() {
                    let _ = sender.try_send(frame.clone());
                }

                self.current_frame = frame;
                Command::none()
            }
            Update::SetMonitor(id) => {
                self.selected_monitor = Some(id);
                Command::none()
            }
            Update::SetTheme(theme) => {
                self.theme = theme;
                Command::none()
            }
        }
    }

    pub fn view(&mut self) -> Element<Update> {
        let choose_theme = Theme::ALL.iter().fold(
            Row::new().push(Text::new("Choose a theme:")),
            |row, theme| {
                row.push(
                    Radio::new(
                        *theme,
                        &format!("{:?}", theme),
                        Some(self.theme),
                        Update::SetTheme,
                    )
                    .style(self.theme),
                )
            },
        );

        let stop = Button::new(&mut self.ui.stop, Text::new("stop"))
            .on_press(Update::Stop)
            .style(self.theme);

        let stream = Button::new(&mut self.ui.stream, Text::new("stream"))
            .on_press(Update::Stream)
            .style(self.theme);

        let view = Button::new(&mut self.ui.view, Text::new("view"))
            .on_press(Update::View)
            .style(self.theme);

        let buttons = Row::new().push(stop).push(stream).push(view);

        let url = TextInput::new(
            &mut self.ui.url,
            "stream url",
            &self.ui.url_text,
            Update::SetUrlText,
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
                Update::SetMonitor,
            )
            .style(self.theme),
        );

        for i in 0..self.num_monitors {
            monitors = monitors.push(
                Radio::new(
                    DisplayID::Index(i),
                    i.to_string(),
                    self.selected_monitor,
                    Update::SetMonitor,
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
