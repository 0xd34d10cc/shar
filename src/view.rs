use std::hash::{Hash, Hasher};

use anyhow::{anyhow, Result};
use futures::channel::mpsc::{self, Sender};
use futures::sink::SinkExt;
use futures::stream::{BoxStream, StreamExt};
use iced::image;
use iced_native::Subscription;
use url::Url;

use crate::codec::{self, Decoder};
use crate::net::tcp;
use crate::resolve;

// NOTE: assumes zero configuration, i.e. Decoder implements Default
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub enum DecoderId {
    Ffmpeg,
}

type Unit = codec::ffmpeg::Unit;

impl DecoderId {
    fn build_decoder(&self) -> Box<dyn Decoder<Frame = image::Handle, Unit = Unit>> {
        match self {
            DecoderId::Ffmpeg => Box::new(
                // codec::Null
                codec::ffmpeg::Decoder::new(
                    // FIXME: unhardcode
                    codec::Config {
                        bitrate: 5000,
                        fps: 30,
                        gop: 3,
                        width: 1920,
                        height: 1080,
                    },
                )
                .unwrap(),
            ),
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
        decoder_id: DecoderId::Ffmpeg,
    })
}

async fn receive_from<D>(
    url: Url,
    mut consumer: Sender<image::Handle>,
    mut decoder: D,
) -> Result<()>
where
    D: Decoder<Frame = image::Handle, Unit = Unit>,
{
    match url.scheme() {
        "tcp" => {
            let address = resolve::address_of(url.clone()).await?;
            log::trace!("{} resolved to {}", url, address);
            let (sender, mut receiver) = mpsc::channel(5);
            tokio::spawn(async move {
                let mut frames = Vec::new();
                while let Some(unit) = receiver.next().await {
                    if let Err(e) = decoder.decode(unit, &mut frames) {
                        log::error!("Failed to decode a packet: {}", e);
                        break;
                    };

                    for frame in frames.drain(..) {
                        if consumer.send(frame).await.is_err() {
                            return;
                        }
                    }
                }
            });

            tcp::Receiver::new(sender).receive(address).await;
            Ok(())
        }
        scheme => Err(anyhow!("Unsupported receiver protocol: {}", scheme)),
    }
}
