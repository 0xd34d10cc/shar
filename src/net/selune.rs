use std::collections::HashMap;
use std::fmt::{self, Debug};

use anyhow::{anyhow, Result};
use futures::{SinkExt, StreamExt};
use serde::{de::DeserializeOwned, Deserialize, Serialize};
use tokio::net::TcpStream;
use tokio::sync::mpsc::Sender;
use tokio_tungstenite::{connect_async, tungstenite::Message, MaybeTlsStream, WebSocketStream};
use url::Url;

pub type StreamId = String;

#[derive(Debug, Deserialize)]
pub struct NewViewer {
    pub stream_id: StreamId,
    pub destination: String, // url
}

#[derive(Deserialize)]
#[serde(tag = "status")]
#[serde(rename_all = "snake_case")]
enum Response<T> {
    Success(T),
    NewViewer(NewViewer),
    Fail { message: String },
}

#[derive(Debug, Deserialize, Clone)]
pub struct StreamDescription {
    pub address: Url,
    pub description: String,
}

pub struct Client {
    stream: WebSocketStream<MaybeTlsStream<TcpStream>>,
    notifications: Sender<NewViewer>,
}

impl Debug for Client {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "SeluneClient")
    }
}

impl Client {
    pub async fn connect(address: &str, notifications: Sender<NewViewer>) -> Result<Self> {
        let (stream, _response) = connect_async(address).await?;
        Ok(Client { stream, notifications })
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

        let response: StreamsResponse = self.receive().await?;
        Ok(response.streams)
    }

    pub async fn watch(&mut self, stream_id: String, destination: String) -> Result<()> {
        #[derive(Serialize)]
        struct WatchRequest {
            #[serde(rename = "type")]
            type_: &'static str,
            stream_id: String,
            destination: String,
        }

        self.send(&WatchRequest {
            type_: "watch",
            stream_id,
            destination,
        })
        .await?;

        #[derive(Deserialize)]
        struct Empty;
        let _e: Empty = self.receive().await?;
        Ok(())
    }

    // returns stream id on success
    pub async fn add_stream(&mut self, address: String, description: String) -> Result<String> {
        #[derive(Serialize)]
        struct Stream {
            address: String,
            description: String,
        }

        #[derive(Serialize)]
        struct AddStreamRequest {
            #[serde(rename = "type")]
            type_: &'static str,
            stream: Stream,
        }

        self.send(&AddStreamRequest {
            type_: "add_stream",
            stream: Stream {
                address,
                description,
            },
        })
        .await?;

        #[derive(Deserialize)]
        struct StreamId {
            stream_id: String,
        }

        let response: StreamId = self.receive().await?;
        Ok(response.stream_id)
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

    pub async fn receive<R>(&mut self) -> Result<R>
    where
        R: DeserializeOwned,
    {
        loop {
            let r: Response<R> = self.receive_raw().await?;
            match r {
                Response::Success(r) => return Ok(r),
                Response::NewViewer(v) => self
                    .notifications
                    .send(v)
                    .await
                    .map_err(|e| anyhow!("{:?}", e))?,
                Response::Fail { message } => return Err(anyhow!("Failure: {}", message)),
            }
        }
    }

    async fn receive_raw<R>(&mut self) -> Result<R>
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

            log::info!("message in: {}", message);

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
