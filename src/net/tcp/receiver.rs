use std::marker::PhantomData;
use std::net::SocketAddr;
use std::time::Duration;

use futures::sink::{Sink, SinkExt};
use tokio::io::AsyncReadExt;
use tokio::net::TcpStream;

use crate::codec::Unit;

pub struct Receiver<S, U> {
    sink: S,

    _unit: PhantomData<U>,
}

impl<S, U> Receiver<S, U>
where
    S: Sink<U> + Unpin,
    U: Unit,
{
    pub fn new(sink: S) -> Self {
        Receiver {
            sink,

            _unit: PhantomData,
        }
    }

    pub async fn receive(&mut self, address: SocketAddr) {
        loop {
            log::info!("Connecting to {}", address);
            let mut stream = match TcpStream::connect(address).await {
                Ok(stream) => stream,
                Err(e) => {
                    log::error!("Failed to connect to {}: {}", address, e);
                    tokio::time::delay_for(Duration::from_secs(1)).await;
                    continue;
                }
            };

            let mut packet_len: [u8; 4] = [0u8; 4];
            let mut buffer = Vec::with_capacity(4096);
            loop {
                buffer.clear();

                if let Err(e) = stream.read_exact(&mut packet_len).await {
                    log::error!("tcp recv failed: {}", e);
                    break;
                };

                let size = u32::from_le_bytes(packet_len) as usize;
                if size > 1024 * 1024 * 8 {
                    // 8 mb
                    log::error!("Packet size is too big: {} total bytes", size);
                    break;
                }

                buffer.resize(size, 0);
                assert_eq!(buffer.len(), size);
                if let Err(e) = stream.read_exact(&mut buffer[0..size]).await {
                    log::error!("Failed to receive entire packet: {}", e);
                    break;
                }

                let unit = U::from_packet(&buffer[0..size]);
                if self.sink.send(unit).await.is_err() {
                    log::error!("tcp receiver closed: consumer dropped the channel");
                    break;
                }
            }
        }
    }
}
