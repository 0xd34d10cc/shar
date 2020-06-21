use std::marker::PhantomData;
use std::net::SocketAddr;
use std::time::Duration;

use futures::sink::{Sink, SinkExt};
use tokio::io::AsyncReadExt;
use tokio::net::TcpStream;

use crate::codec::Unit;

pub struct TcpReceiver<S, U> {
    sink: S,

    _unit: PhantomData<U>,
}

impl<S, U> TcpReceiver<S, U>
where
    S: Sink<U> + Unpin,
    U: Unit,
{
    pub fn new(sink: S) -> Self {
        TcpReceiver {
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

            let mut packet_len = [0u8; 4];
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
                    log::error!(
                        "Packet size is too big: {} total bytes",
                        size
                    );
                    break;
                }

                log::debug!("Receiving packet of size {}", size);
                if size > buffer.capacity() {
                    let additional = size - buffer.capacity();
                    buffer.reserve(additional);
                }

                unsafe { buffer.set_len(size) };
                if let Err(e) = stream.read_exact(&mut buffer).await {
                    log::error!("Failed to receive entire packet: {}", e);
                    break;
                }

                let unit = U::from_packet(&buffer);
                if let Err(_) = self.sink.send(unit).await {
                    log::error!("tcp receiver closed: consumer dropped the channel");
                    break;
                }
            }
        }
    }
}
