use std::hash::{Hash, Hasher};

use anyhow::{anyhow, Result};
use bytes::BytesMut;
use futures::stream::{BoxStream, StreamExt};
use iced::image;
use iced_native::Subscription;
use tokio::sync::mpsc::{self, Sender};
use url::Url;

use crate::codec::{self, Decoder};
use crate::net::TcpReceiver;
use crate::resolve;

// NOTE: assumes zero configuration, i.e. Decoder implements Default
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub enum DecoderId {
    Null,
}

impl DecoderId {
    fn build_decoder(&self) -> Box<dyn Decoder<Frame = image::Handle>> {
        match self {
            DecoderId::Null => Box::new(codec::Null),
        }
    }
}

struct ViewStream {
    url: Url,
    decoder_id: DecoderId,
}

impl<H, I> iced_native::subscription::Recipe<H, I> for ViewStream
where
    H: Hasher,
{
    type Output = image::Handle;

    fn hash(&self, state: &mut H) {
        self.url.hash(state);
        self.decoder_id.hash(state);
    }

    fn stream(self: Box<Self>, _input: BoxStream<'static, I>) -> BoxStream<'static, Self::Output> {
        let (frames, receiver) = mpsc::channel(5);
        tokio::spawn(receive_from(
            self.url,
            frames,
            self.decoder_id.build_decoder(),
        ));
        receiver.boxed()
    }
}

pub fn view(url: Url) -> Subscription<image::Handle> {
    Subscription::from_recipe(ViewStream {
        url,
        decoder_id: DecoderId::Null,
    })
}

async fn receive_from<D>(url: Url, mut frames: Sender<image::Handle>, mut decoder: D) -> Result<()>
where
    D: Decoder<Frame = image::Handle>,
{
    match url.scheme() {
        "tcp" => {
            let address = resolve::address_of(url.clone()).await?;
            log::trace!("{} resolved to {}", url, address);
            let (sender, mut receiver) = mpsc::channel::<BytesMut>(5);
            tokio::spawn(async move {
                while let Some(buffer) = receiver.recv().await {
                    let packets = match decoder.decode(buffer.freeze()) {
                        Ok(packets) => packets,
                        Err(e) => {
                            log::error!("Failed to decode a packet: {}", e);
                            break;
                        }
                    };

                    for packet in packets {
                        if let Err(_) = frames.send(packet).await {
                            break;
                        }
                    }
                }
            });

            TcpReceiver::new(sender).receive(address).await;
            Ok(())
        }
        scheme => Err(anyhow!("Unsupported receiver protocol: {}", scheme)),
    }
}
