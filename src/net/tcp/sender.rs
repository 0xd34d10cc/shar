use std::collections::HashMap;
use std::net::SocketAddr;
use std::ops::Deref;
use std::marker::PhantomData;

use anyhow::Result;
use futures::future::FutureExt;
use futures::select;
use futures::stream::{Stream, StreamExt};
use tokio::io::AsyncWriteExt;
use tokio::net::{TcpListener, TcpStream};

pub struct TcpSender<S, U> {
    frames: S,
    client_id: usize,
    clients: HashMap<usize, TcpStream>,

    _unit: PhantomData<U>,
}

impl<S, U> TcpSender<S, U>
where
    S: Stream<Item = U> + Unpin,
    U: Deref<Target=[u8]>,
{
    pub fn new(frames: S) -> Self {
        TcpSender {
            frames,
            client_id: 0,
            clients: HashMap::new(),

            _unit: PhantomData,
        }
    }

    pub async fn stream_on(&mut self, address: SocketAddr) -> Result<()> {
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
                    match frame {
                        Some(frame) => self.send(frame).await,
                        None => {
                            log::info!("tcp sender stop: no more frames");
                            break Ok(())
                        }
                    }
                },
            }
        }
    }

    async fn send(&mut self, unit: U) {
        let buffer = unit.deref();

        let mut ids = Vec::new();
        for (id, sender) in self.clients.iter_mut() {
            // FIXME: single slow user could block this whole task
            if let Err(e) = sender.write_all(buffer).await {
                log::error!("tcp send failed: {}", e);
                ids.push(*id);
            }
        }

        for id in ids {
            log::info!("Client with ID={} was disconnected", id);
            self.clients.remove(&id);
        }
    }

    fn accept(&mut self, client: TcpStream) {
        self.client_id += 1;
        self.clients.insert(self.client_id, client);
        log::info!("Client {} connected", self.client_id);
    }
}
