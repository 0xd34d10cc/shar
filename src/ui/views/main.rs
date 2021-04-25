use ::image::{load_from_memory_with_format, ImageFormat};
use futures::channel::mpsc::{self, Sender};
use iced::image::{self, Image};
use iced::{
    button, pick_list, text_input, Align, Button, Column, Command, Container, Element, Length, Row,
    Subscription, Text, TextInput,
};
use once_cell::sync::OnceCell;
use url::Url;

use crate::capture::DisplayID;
use crate::config::Config;
use crate::stream::stream;
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

    monitors: pick_list::State<DisplayID>,
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
    monitors: Vec<DisplayID>,
    selected_monitor: DisplayID,
    config: Config,

    ui: UIState,
}

impl MainView {
    pub fn new(config: Config) -> Self {
        let n_monitors = scrap::Display::all()
            .map(|monitors| monitors.len())
            .unwrap_or(0);

        let mut app = MainView {
            state: None,
            current_frame: background(),
            stream_sender: None,
            monitors: Some(DisplayID::Primary)
                .into_iter()
                .chain((0..n_monitors).map(DisplayID::Index))
                .collect(),
            selected_monitor: DisplayID::Primary,
            config,

            url: None,
            ui: UIState::default(),
        };

        app.set_url("tcp://127.0.0.1:1337".into());
        app
    }

    fn set_url(&mut self, url: String) {
        self.url = url.parse().ok();
        self.ui.url_text = url;
    }

    pub fn subscription(&self) -> Subscription<Update> {
        match &self.state {
            None => Subscription::none(),
            Some(StreamState::Streaming(_)) => {
                let handles = crate::capture::capture(
                    self.selected_monitor,
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
                            fps: 60,
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
                self.selected_monitor = id;
                Command::none()
            }
        }
    }

    pub fn view(&mut self) -> Element<Update> {
        let style = self.config.load().theme;

        let stop = Button::new(&mut self.ui.stop, Text::new("stop"))
            .on_press(Update::Stop)
            .style(style);

        let stream = Button::new(&mut self.ui.stream, Text::new("stream"))
            .on_press(Update::Stream)
            .style(style);

        let view = Button::new(&mut self.ui.view, Text::new("view"))
            .on_press(Update::View)
            .style(style);

        let url = TextInput::new(
            &mut self.ui.url,
            "stream url",
            &self.ui.url_text,
            Update::SetUrlText,
        )
        .style(style);

        let monitors = pick_list::PickList::new(
            &mut self.ui.monitors,
            self.monitors.as_slice(),
            Some(self.selected_monitor),
            Update::SetMonitor,
        )
        .style(style);

        let url = Row::new()
            .push(url)
            .push(monitors)
            .push(stop)
            .push(stream)
            .push(view);

        let stream_state = Text::new(format!("{:?}", self.state));
        let video_frame = Image::new(self.current_frame.clone());

        let layout = Column::new()
            .align_items(Align::Center)
            .push(url)
            .push(stream_state)
            .push(video_frame);

        let content = Container::new(layout)
            .width(Length::Fill)
            .height(Length::Fill)
            .center_x()
            .style(style);

        Element::from(content)
    }
}
