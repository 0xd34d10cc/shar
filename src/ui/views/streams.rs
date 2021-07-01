use std::net::SocketAddr;
use std::sync::Arc;
use std::collections::HashMap;

use anyhow::Result;
use iced::{button, Button, Column, Command, Element, Row, Subscription, Text};
use tokio::net::{lookup_host, UdpSocket};
use tokio::time::{Instant, Duration, sleep, sleep_until};
use url::Url;

use crate::net::selune::{StreamDescription, StreamId};
use crate::net::stun;
use crate::resolve;
use crate::state::AppState;

#[derive(Debug, Clone)]
pub enum Update {
    RequestStreams,
    StartNewStream,
    Disconnected,
    ListStreams(HashMap<String, StreamDescription>),
    View(usize /* stream index */),
    Failure(String),
}

#[derive(Default)]
struct Widgets {
    stream: button::State,
    update: button::State,
    view_buttons: Vec<button::State>,
}

pub struct StreamsView {
    state: Arc<AppState>,
    streams: Vec<(StreamId, StreamDescription)>,
    ui: Widgets,
}

impl StreamsView {
    pub fn new(state: Arc<AppState>) -> (Self, Command<Update>) {
        let view = StreamsView {
            state,
            streams: Vec::new(),
            ui: Widgets::default(),
        };

        let c = Command::from(list_streams(view.state.clone()));
        (view, c)
    }

    pub fn update(&mut self, update: Update) -> Command<Update> {
        match update {
            Update::Failure(_message) => {
                // ignore, for now
            }
            Update::RequestStreams => return Command::from(list_streams(self.state.clone())),
            Update::StartNewStream => return Command::from(start_stream(self.state.clone())),
            Update::ListStreams(streams) => {
                self.streams = streams.into_iter().collect();
            }
            Update::Disconnected => {
                log::warn!("Disconnected");
            }
            Update::View(i) => {
                if let Some((id, stream)) = self.streams.get(i) {
                    // TODO: cancellation
                    let state = self.state.clone();
                    let stream_id = id.clone();
                    let description = stream.clone();

                    return Command::from(async move {
                        match receive_messages(state, stream_id, description).await {
                            Ok(update) => update,
                            Err(e) => {
                                log::error!("Connection failed: {}", e);
                                Update::Failure(e.to_string())
                            }
                        }
                    });
                }
            }
        }

        Command::none()
    }

    pub fn subscription(&self) -> Subscription<Update> {
        Subscription::none()
    }

    pub fn view(&mut self) -> Element<Update> {
        let style = self.state.config.load().theme;
        self.ui
            .view_buttons
            .resize_with(self.streams.len(), button::State::default);

        let mut list = Column::new();
        for (i, (button, (id, stream))) in self
            .ui
            .view_buttons
            .iter_mut()
            .zip(self.streams.iter())
            .enumerate()
        {
            let b = Button::new(button, Text::new("View"))
                .style(style)
                .on_press(Update::View(i));
            let line = Row::new()
                .push(b)
                .push(Text::new(id))
                .push(Text::new(stream.address.as_str()))
                .push(Text::new(&stream.description))
                .spacing(10);

            list = list.push(line);
        }

        list = list.push(
            Button::new(&mut self.ui.stream, Text::new("Stream"))
                .on_press(Update::StartNewStream)
                .style(style),
        );
        list = list.push(
            Button::new(&mut self.ui.update, Text::new("Update"))
                .on_press(Update::RequestStreams)
                .style(style),
        );

        Element::new(list)
    }
}

async fn process_client(destination: String, socket: &mut UdpSocket) -> Result<()> {
    let destination = Url::parse(&destination).unwrap();
    let client_address = resolve::address_of(destination).await.unwrap();
    log::info!("New viewer {}", client_address);

    // lets send some shit
    let mut buffer = [0u8; 1024];
    // punch a hole
    for _ in 0..3 {
        socket
            .send_to(b"Punching a hole (server)", client_address)
            .await
            .unwrap();
        sleep(Duration::from_millis(200)).await;
    }

    // send "Hello from streamer" 5 times and print client messages, if any
    let mut i = 0;
    let mut deadline = Instant::now();
    loop {
        tokio::select! {
            _ = sleep_until(deadline) => {
                socket.send_to(b"Hello from streamer", client_address).await?;
                deadline = Instant::now() + Duration::from_secs(1);
                i += 1;
                if i >= 5 {
                    break;
                }
            },
            res = socket.recv_from(&mut buffer) => {
                let (n, sender) = res?;
                log::info!("{} says: {:?}", sender, std::str::from_utf8(&buffer[..n]));
            }
        }
    }

    Ok(())
}

async fn start_stream(state: Arc<AppState>) -> Update {
    let mut socket = UdpSocket::bind("0.0.0.0:0").await.unwrap();

    let stun_server = state.config.load().stun_server.get(0).unwrap().clone();
    let stun_addr = lookup_host(&stun_server).await.unwrap().next().unwrap();
    let public_addr = public_address(stun_addr, &mut socket).await.unwrap();
    log::info!("public address: {}", public_addr);

    let mut client = state.selune.lock().await;
    let (client, notifications) = match *client {
        None => return Update::Disconnected,
        Some((ref mut client, ref mut notifications)) => (client, notifications),
    };

    let stream_id = client
        .add_stream(format!("rtp://{}", public_addr), "ololo".to_string())
        .await
        .unwrap(); // FIXME

    // FIXME: this design is fucked. Some task should call client.receive() repeatedly for notifications to work
    loop {
        tokio::select! {
            viewer = notifications.recv() => {
                if let Some(viewer) = viewer {
                    if stream_id != viewer.stream_id {
                        log::warn!("Received new_viewer notification on stream {}, but my stream id is {}", viewer.stream_id, stream_id);
                        continue;
                    }

                    process_client(viewer.destination, &mut socket).await.unwrap();
                }
            },
            message = client.receive::<()>() => {
                log::warn!("Message dropped: {:?}", message);
            },
        }
    }
}

async fn list_streams(state: Arc<AppState>) -> Update {
    let mut client = state.selune.lock().await;
    match *client {
        Some((ref mut client, ref mut _n)) => {
            let response = client.list_streams().await;
            Update::ListStreams(response.unwrap()) // FIXME
        }
        None => Update::Disconnected,
    }
}

async fn public_address(stun_addr: SocketAddr, socket: &mut UdpSocket) -> Result<SocketAddr> {
    // TODO: add a timeout
    // TODO: generate a random id
    let id = [1, 2, 3, 4, 5, 6, 6, 5, 4, 3, 2, 1];
    let message = stun::bind_request(id);
    socket.send_to(&message, stun_addr).await?;

    let mut buffer = [0u8; 256];
    loop {
        let (n, sender) = socket.recv_from(&mut buffer).await?;
        if sender != stun_addr {
            log::warn!("Unexpected packet from {}", sender);
            continue;
        }

        if let Some(packet) = stun::Packet::parse(&buffer[..n]) {
            if *packet.header().id() != id {
                log::warn!("Unexpected response id: {:?}", packet.header().id());
                continue;
            }

            if let Some(attr) = packet.attributes().next() {
                match attr {
                    stun::Attribute::MappedAddress(addr)
                    | stun::Attribute::XorMappedIpAddr(addr) => break Ok(addr),
                    stun::Attribute::Unknown(type_, data) => {
                        log::warn!(
                            "Received unknown attribute. type={} data={:x?}",
                            type_,
                            data
                        );
                    }
                }
            }
        } else {
            log::warn!("Invalid response from stun server");
        }
    }
}

async fn receive_messages(
    state: Arc<AppState>,
    stream_id: String,
    description: StreamDescription,
) -> Result<Update> {
    let mut socket = UdpSocket::bind("0.0.0.0:0").await?;
    let streamer_addr = resolve::address_of(description.address).await?;

    // TODO: pick a random stun server
    let stun_server = state.config.load().stun_server.get(0).unwrap().clone();
    let stun_addr = lookup_host(&stun_server).await?.next().unwrap();
    let public_addr = public_address(stun_addr, &mut socket).await?;
    log::info!("public address: {}", public_addr);
    log::info!("awaiting messages from: {}", streamer_addr);

    let mut client = state.selune.lock().await;
    let client = &mut client.as_mut().unwrap().0;
    let destination = format!("rtp://{}", public_addr);
    client.watch(stream_id, destination).await?;

    // punch a hole
    for _ in 0..3 {
        socket.send_to(b"Punching a hole (viewer)", streamer_addr).await?;
        sleep(Duration::from_millis(200)).await;
    }

    let mut buffer = [0u8; 1024];
    let mut deadline = Instant::now();
    loop {
        tokio::select! {
            _ = sleep_until(deadline) => {
                socket.send_to(b"Hello from viewer", streamer_addr).await?;
                deadline = Instant::now() + Duration::from_secs(1);
            },
            res = socket.recv_from(&mut buffer) => {
                let (n, sender) = res?;
                log::info!("[{}] says: {:?}", sender, std::str::from_utf8(&buffer[..n]));
            }
        }
    }
}
