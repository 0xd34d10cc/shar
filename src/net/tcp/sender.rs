use std::collections::HashMap;
use std::marker::PhantomData;
use std::net::SocketAddr;

use anyhow::Result;
use futures::future::FutureExt;
use futures::stream::{Stream, StreamExt};
use tokio::io::AsyncWriteExt;
use tokio::net::{TcpListener, TcpStream};

use crate::codec::Unit;

struct Client<U> {
    stream: TcpStream,
    idr_received: bool,

    _unit: PhantomData<U>,
}

impl<U> Client<U>
where
    U: Unit,
{
    fn new(stream: TcpStream) -> Self {
        Client {
            stream,
            idr_received: false,

            _unit: PhantomData,
        }
    }

    async fn send(&mut self, data: &[u8], _is_idr: bool) -> std::io::Result<()> {
        // FIXME: this flag is not correct for nvenc (and maybe other codecs)
        // if !self.idr_received && !is_idr {
        //     return Ok(());
        // }

        self.idr_received = true;

        let length = (data.len() as u32).to_le_bytes();
        self.stream.write_all(&length).await?;
        self.stream.write_all(data).await?;
        Ok(())
    }
}

pub struct Sender<S, U> {
    units: S,
    client_id: usize,
    clients: HashMap<usize, Client<U>>,

    _unit: PhantomData<U>,
}

impl<S, U> Sender<S, U>
where
    S: Stream<Item = U> + Unpin,
    U: Unit,
{
    pub fn new(units: S) -> Self {
        Sender {
            units,
            client_id: 0,
            clients: HashMap::new(),

            _unit: PhantomData,
        }
    }

    pub async fn stream_on(&mut self, address: SocketAddr) -> Result<()> {
        let listener = TcpListener::bind(address).await?;
        loop {
            futures::select! {
                client = listener.accept().fuse() => {
                    match client {
                        Ok((client, _address)) => self.accept(client),
                        Err(e) => {
                            log::error!("tcp accept failed: {}", e);
                            break Err(e.into());
                        },
                    }
                },
                unit = self.units.next().fuse() => {
                    match unit {
                        Some(unit) => self.send(unit).await,
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
