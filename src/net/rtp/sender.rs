use std::time::Instant;
use std::net::SocketAddr;

use anyhow::Result;
use tokio::net::UdpSocket;
use futures::stream::{Stream, StreamExt};

use crate::codec::Unit;
use super::packetizer::Packetizer;
use super::packet::Packet;

// ticks per second for h264 rtp clock, see RFC 6184 Section 8.2.1
const CLOCK_RATE: usize = 90000;
// ticks per ms
const CLOCK_RATE_MS: usize = CLOCK_RATE / 1000;

pub struct Sender<S> {
    units: S,

    sequence: u16,
    timestamp: u32,
}

impl<S, U> Sender<S> where S: Stream<Item=U>  + Unpin, U: Unit {
    pub fn new(units: S) -> Self {
        Sender {
            units,

            sequence: 0,
            timestamp: 0,
        }
    }

    pub async fn stream_to(&mut self, remote: SocketAddr) -> Result<()> {
        let local: SocketAddr = ([0, 0, 0, 0], 0).into();
        // ...why the heck are these async? -_-
        let mut socket = UdpSocket::bind(local).await?;
        socket.connect(remote).await?;

        let mut last_unit_time = Instant::now();
        let mut buffer = [0u8; 2048];
        while let Some(unit) = self.units.next().await {

            let now = Instant::now();
            let dt = (now - last_unit_time).as_millis() as usize * CLOCK_RATE_MS;
            self.timestamp = self.timestamp.wrapping_add(dt as u32);
            last_unit_time = now;

            let data = unit.data();
            let mtu = 1400;
            let mut packetizer = Packetizer::with_mtu(mtu, data);

            while let Some(fragment) = packetizer.next() {
                const HEADER_SIZE: usize = 12;
                let packet_size = HEADER_SIZE + 2 + fragment.payload.len();;
                let mut packet = Packet::new(&mut buffer[..packet_size]).unwrap();

                let header = packet.header_mut();
                header.set_version(2);
                header.set_payload_type(96);
                header.set_sequence(self.sequence);
                header.set_timestamp(self.timestamp);

                let payload = packet.payload_mut();
                payload[0] = fragment.indicator;
                payload[1] = fragment.header;
                payload[2..].copy_from_slice(fragment.payload);

                self.sequence = self.sequence.wrapping_add(1);

                socket.send(&buffer[..packet_size]).await?;
            }
        }

        Ok(())
    }
}