use std::net::SocketAddr;
use std::time::Duration;

use bytes::BytesMut;
use tokio::io::AsyncReadExt;
use tokio::net::TcpStream;
use tokio::sync::mpsc::Sender;

pub struct TcpReceiver {
    sink: Sender<BytesMut>, // does not implement Sink
}

impl TcpReceiver {
    pub fn new(sink: Sender<BytesMut>) -> Self {
        TcpReceiver { sink }
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

            let mut buffer = BytesMut::with_capacity(4096);
            loop {
                if buffer.capacity() < 4096 {
                    buffer.reserve(4096);
                }

                let n = match stream.read_buf(&mut buffer).await {
                    Ok(n) => n,
                    Err(e) => {
                        log::error!("tcp recv failed: {}", e);
                        break;
                    }
                };

                if n < 8 {
                    continue;
                }

                let width = u32::from_le_bytes([buffer[0], buffer[1], buffer[2], buffer[3]]);
                let height = u32::from_le_bytes([buffer[4], buffer[5], buffer[6], buffer[7]]);
                let total_bytes = (width * height * 4) as usize;
                if total_bytes > 1024 * 1024 * 8 {
                    // 8 mb
                    log::error!(
                        "Received too big frame: {}x{} ({} total bytes)",
                        width,
                        height,
                        total_bytes
                    );
                    break;
                }

                if buffer.len() < total_bytes + 8 {
                    buffer.reserve(total_bytes + 8 - buffer.len());
                }

                unsafe { buffer.set_len(total_bytes + 8) };
                if let Err(e) = stream.read_exact(&mut buffer[n..]).await {
                    log::error!("Failed to receive entire frame: {}", e);
                    break;
                }

                if let Err(_) = self.sink.send(buffer.split()).await {
                    log::error!("tcp receiver closed: consumer dropped the channel");
                    return;
                }

                buffer.reserve(total_bytes + 8);
            }
        }
    }
}
