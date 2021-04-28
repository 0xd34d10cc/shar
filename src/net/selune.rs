use std::collections::HashMap;
use std::fmt::{self, Debug};

use anyhow::{anyhow, Result};
use futures::{SinkExt, StreamExt};
use serde::{de::DeserializeOwned, Deserialize, Serialize};
use tokio::net::TcpStream;
use tokio_tungstenite::{connect_async, tungstenite::Message, MaybeTlsStream, WebSocketStream};

pub type StreamId = String;

#[derive(Deserialize)]
#[serde(tag = "status")]
#[serde(rename_all = "snake_case")]
enum Response<T> {
    Success(T),
    Fail { message: String },
}

#[derive(Debug, Deserialize, Clone)]
pub struct StreamDescription {
    pub address: String,
    pub description: String,
}

pub struct Client {
    stream: WebSocketStream<MaybeTlsStream<TcpStream>>,
}

impl Debug for Client {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "SeluneClient")
    }
}

impl Client {
    pub async fn connect(address: &str) -> Result<Self> {
        let (stream, _response) = connect_async(address).await?;
        Ok(Client { stream })
    }

    pub async fn list_streams(&mut self) -> Result<HashMap<StreamId, StreamDescription>> {
        #[derive(Deserialize)]
        struct StreamsResponse {
            streams: HashMap<String, StreamDescription>,
        }

        #[derive(Serialize)]
        struct StreamsRequest {
            #[serde(rename = "type")]
            type_: &'static str,
        }

        self.send(&StreamsRequest {
            type_: "get_streams",
        })
        .await?;

        let response: Response<StreamsResponse> = self.receive().await?;
        match response {
            Response::Success(r) => Ok(r.streams),
            Response::Fail { message } => Err(anyhow!("Streams request failed: {}", message)),
        }
    }

    async fn send<R>(&mut self, request: &R) -> Result<()>
    where
        R: Serialize,
    {
        let data = serde_json::to_vec(request)?;
        let message = Message::Binary(data);
        self.stream.send(message).await?;
        Ok(())
    }

    async fn receive<R>(&mut self) -> Result<R>
    where
        R: DeserializeOwned,
    {
        loop {
            let message = self
                .stream
                .next()
                .await
                .transpose()?
                .ok_or_else(|| anyhow!("Unexpected end of stream"))?;

            match message {
                Message::Binary(data) => return Ok(serde_json::from_slice(&data)?),
                Message::Text(data) => return Ok(serde_json::from_slice(data.as_bytes())?),
                Message::Ping(data) => self.stream.send(Message::Pong(data)).await?,
                m => {
                    log::warn!("Unhandled message: {:?}", m);
                }
            }
        }
    }
}
