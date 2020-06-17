use anyhow::{anyhow, Result};
use futures::stream::{Stream, StreamExt};
use iced::image;
use tokio::sync::mpsc;
use url::Url;

use crate::codec::Encoder;
use crate::net::TcpSender;
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
                while let Some(frame) = frames.next().await {
                    let packet = match encoder.encode(frame) {
                        Ok(packet) => packet,
                        Err(e) => {
                            log::error!("Failed to encode a packet: {}", e);
                            break;
                        }
                    };

                    if let Err(_) = sender.send(packet).await {
                        break;
                    }
                }
            });

            TcpSender::new(receiver.map(|packet| packet.freeze()))
                .stream_on(address)
                .await?;
            Ok(())
        }
        scheme => Err(anyhow!("Unsupported sender protocol: {}", scheme)),
    }
}
