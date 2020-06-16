use std::hash::{Hash, Hasher};
use std::net::{IpAddr, Ipv4Addr, SocketAddr};

use anyhow::{anyhow, Context, Result};
use bytes::BytesMut;
use futures::stream::{BoxStream, Stream, StreamExt};
use iced::{image, Subscription};
use tokio::sync::mpsc::{self, Sender};
use url::{Host, Url};

use crate::tcp::{TcpReceiver, TcpSender};

fn localhost() -> IpAddr {
    IpAddr::V4(Ipv4Addr::new(127, 0, 0, 1))
}

async fn resolve(hostname: &str) -> Result<IpAddr> {
    log::debug!("Resolving: {}", hostname);

    let mut addresses = tokio::net::lookup_host(&hostname).await?;
    let address = addresses
        .next()
        .ok_or_else(|| anyhow!("DNS returned 0 ip addresses for {}", hostname))?;
    let ip = address.ip();
    Ok(ip)
}

async fn address_of(url: Url) -> Result<SocketAddr> {
    let ip = match url.host() {
        None => localhost(),
        Some(Host::Domain(name)) => {
            // see https://github.com/servo/rust-url/issues/606
            if let Ok(ip) = name.parse::<IpAddr>() {
                ip
            } else {
                resolve(name).await.context("Hostname resolution failed")?
            }
        },
        Some(Host::Ipv4(ip)) => IpAddr::V4(ip),
        Some(Host::Ipv6(ip)) => IpAddr::V6(ip),
    };

    let port = url.port_or_known_default().unwrap_or(1337);
    let address = SocketAddr::new(ip, port);
    Ok(address)
}

// p2p sender
pub async fn on<S>(url: Url, frames: S) -> Result<()>
where
    S: Stream<Item = image::Handle> + Unpin,
{
    match url.scheme() {
        "tcp" => {
            let address = address_of(url).await?;
            let frames = frames.map(crate::codec::encode).map(|data| data.freeze());
            TcpSender::new(frames).stream_on(address).await?;
            Ok(())
        }
        scheme => Err(anyhow!("Unsupported sender protocol: {}", scheme)),
    }
}

struct ViewStream {
    url: Url,
}

impl<H, I> iced_native::subscription::Recipe<H, I> for ViewStream
where
    H: Hasher,
{
    type Output = image::Handle;

    fn hash(&self, state: &mut H) {
        self.url.hash(state);
    }

    fn stream(self: Box<Self>, _input: BoxStream<'static, I>) -> BoxStream<'static, Self::Output> {
        let (sender, receiver) = mpsc::channel(5);
        tokio::spawn(receive_from(self.url, sender));
        receiver.boxed()
    }
}

pub fn view(url: Url) -> Subscription<image::Handle> {
    Subscription::from_recipe(ViewStream { url })
}

async fn receive_from(url: Url, mut frames: Sender<image::Handle>) -> Result<()> {
    match url.scheme() {
        "tcp" => {
            let address = address_of(url.clone()).await?;
            log::trace!("{} resolved to {}", url, address);
            let (sender, mut receiver) = mpsc::channel::<BytesMut>(5);
            tokio::spawn(async move {
                while let Some(buffer) = receiver.recv().await {
                    if let Err(_) = frames.send(crate::codec::decode(buffer.freeze())).await {
                        break;
                    }
                }
            });

            log::debug!("creating TcpReceiver");
            TcpReceiver::new(sender).receive(address).await;
            Ok(())
        }
        scheme => Err(anyhow!("Unsupported receiver protocol: {}", scheme)),
    }
}
