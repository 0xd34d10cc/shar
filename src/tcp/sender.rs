use std::collections::HashMap;
use std::net::SocketAddr;

use anyhow::Result;
use bytes::Bytes;
use futures::future::FutureExt;
use futures::select;
use futures::stream::{Stream, StreamExt};
use tokio::io::AsyncWriteExt;
use tokio::net::{TcpListener, TcpStream};
use tokio::sync::mpsc::{self, Sender};

pub struct TcpSender<S> {
    frames: S,
    client_id: usize,
    clients: HashMap<usize, Sender<Bytes>>,
}

impl<S> TcpSender<S>
where
    S: Stream<Item = Bytes> + Unpin,
{
    pub fn new(frames: S) -> Self {
        TcpSender {
            frames,
            client_id: 0,
            clients: HashMap::new(),
        }
    }

    pub async fn stream_on(&mut self, address: SocketAddr) -> Result<()> {
        log::debug!("tcp sender stream on");
        let mut listener = TcpListener::bind(address).await?;
        loop {
            select! {
                client = listener.next().fuse() => {
                    match client {
                        Some(Ok(client)) => self.accept(client),
                        Some(Err(e)) => {
                            log::error!("tcp accept failed: {}", e);
                            break Err(e.into());
                        },
                        None => {
                            log::warn!("tcp listener closed");
                            break Ok(())
                        },
                    }
                },
                frame = self.frames.next().fuse() => {
                    log::debug!("tcp sender received frame");
                    match frame {
                        Some(frame) => self.send(frame).await, // FIXME: blocks current task from receiving connections
                        None => {
                            log::info!("tcp sender stop: no more frames");
                            break Ok(())
                        }
                    }
                },
            }
        }
    }

    async fn send(&mut self, frame: Bytes) {
        let mut ids = Vec::new();
        for (id, sender) in self.clients.iter_mut() {
            log::debug!("Sending frame to {}", id);
            if let Err(_) = sender.send(frame.clone()).await {
                ids.push(*id);
            }
        }

        for id in ids {
            self.clients.remove(&id);
        }
    }

    fn accept(&mut self, mut client: TcpStream) {
        let (sender, mut receiver) = mpsc::channel::<Bytes>(5);

        self.client_id += 1;
        self.clients.insert(self.client_id, sender);

        log::info!("Client {} connected", self.client_id);
        tokio::spawn(async move {
            while let Some(buffer) = receiver.next().await {
                if let Err(e) = client.write_all(&buffer).await {
                    log::error!("tcp send failed: {}", e);
                    break;
                }
            }
        });
    }
}
