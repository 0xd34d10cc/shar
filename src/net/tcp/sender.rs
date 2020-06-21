use std::collections::HashMap;
use std::net::SocketAddr;
use std::marker::PhantomData;

use anyhow::Result;
use futures::future::FutureExt;
use futures::select;
use futures::stream::{Stream, StreamExt};
use tokio::io::AsyncWriteExt;
use tokio::net::{TcpListener, TcpStream};
use bytes::Buf;
use bytes::buf::ext::BufExt;

use crate::codec::Unit;

struct Client<U> {
    stream: TcpStream,
    idr_received: bool,

    _unit: PhantomData<U>,
}

impl<U> Client<U> where U: Unit {
    fn new(stream: TcpStream) -> Self {
        Client {
            stream,
            idr_received: false,

            _unit: PhantomData,
        }
    }

    async fn send(&mut self, data: &[u8], is_idr: bool) -> std::io::Result<()> {
        if !self.idr_received && !is_idr {
            return Ok(());
        }

        self.idr_received = true;


        let length = (data.len() as u32).to_le_bytes();
        let mut packet = length.chain(data);

        log::debug!("Sending packet of size {}", data.len());
        while packet.has_remaining() {
            self.stream.write_buf(&mut packet).await?;
        }

        Ok(())
    }
}


pub struct TcpSender<S, U> {
    frames: S,
    client_id: usize,
    clients: HashMap<usize, Client<U>>,

    _unit: PhantomData<U>,
}

impl<S, U> TcpSender<S, U>
where
    S: Stream<Item = U> + Unpin,
    U: Unit,
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
        let mut ids = Vec::new();
        let data = unit.data();
        let is_idr = unit.is_idr();
        for (id, client) in self.clients.iter_mut() {
            // FIXME: single slow user could block this whole task
            if let Err(e) = client.send(data, is_idr).await {
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
        self.clients.insert(self.client_id, Client::new(client));
        log::info!("Client {} connected", self.client_id);
    }
}