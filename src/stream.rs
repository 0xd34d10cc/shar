use anyhow::{anyhow, Result};
use futures::stream::{Stream, StreamExt};
use iced::image;
use futures::channel::mpsc;
use futures::SinkExt;
use url::Url;

use crate::codec::Encoder;
use crate::net::{rtp, tcp};
use crate::resolve;

// p2p sender
pub async fn stream<S, E>(url: Url, mut encoder: E, mut frames: S) -> Result<()>
where
    S: Stream<Item = image::Handle> + Unpin + Send + 'static,
    E: Encoder<Frame = image::Handle>,
{
    match url.scheme() {
        "tcp" => {
            let address = resolve::address_of(url).await?;
            let (mut sender, receiver) = mpsc::channel(5);

            // encoder thread
            tokio::spawn(async move {
                let mut units = Vec::new();
                while let Some(frame) = frames.next().await {
                    if let Err(e) = encoder.encode(frame, &mut units) {
                        log::error!("Failed to encode a frame: {}", e);
                        break;
                    };

                    for unit in units.drain(..) {
                        if sender.send(unit).await.is_err() {
                            return;
                        }
                    }
                }
            });

            tcp::Sender::new(receiver).stream_on(address).await?;
            Ok(())
        }
        "rtp" => {
            let address = resolve::address_of(url).await?;
            let (mut sender, receiver) = mpsc::channel(5);

            // encoder thread
            // FIXME: copy-paste of above
            tokio::spawn(async move {
                let mut units = Vec::new();
                while let Some(frame) = frames.next().await {
                    if let Err(e) = encoder.encode(frame, &mut units) {
                        log::error!("Failed to encode a frame: {}", e);
                        break;
                    };

                    for unit in units.drain(..) {
                        if sender.send(unit).await.is_err() {
                            return;
                        }
                    }
                }
            });

            rtp::Sender::new(receiver).stream_to(address).await?;
            Ok(())
        }
        scheme => Err(anyhow!("Unsupported sender protocol: {}", scheme)),
    }
}
