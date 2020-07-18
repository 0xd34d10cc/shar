extern crate rtsparse;

use std::io::Write;
use std::marker::PhantomData;
use std::net::{Shutdown, SocketAddr};

use anyhow::Result;
use bytes::{Buf, BytesMut};
use futures::future::FutureExt;
use futures::select;
use futures::stream::{Stream, StreamExt};
use http::{Response, StatusCode};
use rtsparse::{Header, Request, Status, EMPTY_HEADER};
use thiserror::Error;
use tokio::io::{AsyncRead, AsyncReadExt, AsyncWriteExt};
use tokio::net::{TcpListener, TcpStream};

use crate::codec::Unit;

#[derive(Error, Debug)]
pub enum RequestReceivingError {
    #[error("Invalid request: {0}")]
    InvalidRequest(#[from] rtsparse::Error),
    #[error("Buffer overflow")]
    BufferOverflow,
    #[error("IO error: {0}")]
    IOError(#[from] std::io::Error),
}

struct Client {
    id: usize,
    stream: TcpStream,
    buffer_in: BytesMut,
    buffer_out: Vec<u8>,
    session: Option<Session>,
}

impl Client {
    pub fn new(stream: TcpStream, id: usize) -> Self {
        Client {
            id,
            stream,
            buffer_in: BytesMut::with_capacity(4096),
            buffer_out: vec![0u8; 4096],
            session: None,
        }
    }

    fn shutdown(&mut self) {
        match self.stream.shutdown(Shutdown::Both) {
            Ok(_) => (),
            Err(e) => log::error!("Error occured TCPStream shutdown. Client: {}. Error: {}", self.id, e),
        }
    }

    async fn write_response<B>(&mut self, response: &Response<B>) -> std::io::Result<()>
    where
        B: AsRef<[u8]>,
    {
        self.buffer_out.clear();
        match write_response(response, &mut self.buffer_out) {
            Err(e) => log::error!("Error occured on writing response. Client: {}. Error: {}, Response: {:?}", self.id, e, self.buffer_out),
            _ => (),
        };
        self.stream.write_all(&self.buffer_out).await?;
        Ok(())
    }

    async fn process(mut self) {
        loop {
            let mut headers = [EMPTY_HEADER; 16];
            let request = match receive_request(&mut self.stream, &mut headers, &mut self.buffer_in).await {
                Ok(request) => request,
                Err(e) => {
                    log::error!(
                        "Failed to read request. Client: {}. Error: {}. ",
                        self.id,
                        e
                    );
                    self.shutdown();
                    return;
                }
            };
            let peer_addr = match self.stream.peer_addr() {
                Ok(addr) => addr,
                Err(e) => {
                    log::error!("Error on getting peer address. Client: {}. Error: {}", self.id, e);
                    return;
                }
            };
            let response = match process_request(&request, self.session, self.id, peer_addr) {
                Ok((response, session)) => {
                    self.session = session;
                    response
                },
                Err(e) => {
                    log::error!(
                        "Failed to process request. Client: {}. Error: {}",
                        self.id,
                        e
                    );
                    self.shutdown();
                    return;
                }
            };
            match self.write_response(&response).await {
                Err(e) => {
                    log::error!(
                        "Failed to write response. Client: {}. Error: {}",
                        self.id,
                        e
                    );
                    self.shutdown();
                    return;
                }
                Ok(_) => {}
            }
        }
    }
}

#[derive(Clone, Copy)]
struct Session {
    number: usize,
    address: SocketAddr,
    rtp: u16,
    rtcp: u16,
}

pub struct Server<S, U> {
    units: S,
    client_id: usize,

    _unit: PhantomData<U>,
}

impl<S, U> Server<S, U>
where
    S: Stream<Item = U> + Unpin,
    U: Unit,
{
    pub fn new(units: S) -> Self {
        Server {
            units,
            client_id: 0,
            _unit: PhantomData,
        }
    }

    pub async fn start_accepting(
        &mut self,
        address: SocketAddr,
    ) -> Result<(), RequestReceivingError> {
        let mut listener = TcpListener::bind(address).await?;
        loop {
            select! {
                client = listener.next().fuse() => {
                    match client {
                         Some(Ok(client)) => {
                             self.client_id += 1;
                             let client = Client::new(client, self.client_id);
                             log::info!("Client {} connected to RTSP server", self.client_id);
                             tokio::spawn(client.process());
                         },
                         Some(Err(e)) => {
                             log::error!("tcp accept failed: {}", e);
                             break Err(e.into());
                         },
                         None => {
                             log::warn!("tcp listener closed");
                             break Ok(());
                         },
                     }
                },
                unit = self.units.next().fuse() => {
                    //  match unit {
                    //      Some(unit) => self.send(unit).await,
                    //      None => {
                    //          log::info!("tcp sender stop: no more frames");
                    //          break Ok(());
                    //      }
                    // }
                },
            }
        }
    }

    async fn send(&mut self, unit: U) {}
}

fn find_header<'r, 'b>(request: &'r Request<'_, 'b>, to_find: &str) -> Option<&'r Header<'b>> {
    request
        .headers
        .iter()
        .find(|&header| header.name == to_find)
}

fn is_session_exist(
    session_header: &Header,
    client_session: &Session,
    cseq_header: &Header,
) -> Result<(), Response<String>> {
    let session_number = match std::str::from_utf8(session_header.value)
        .ok()
        .and_then(|session| session.parse::<usize>().ok())
    {
        Some(session_number) => session_number,
        None => {
            return Err(Response::builder()
                .status(StatusCode::BAD_REQUEST)
                .header("Reason", "Invalid session number")
                .body("".to_string())
                .unwrap())
        }
    };
    if session_number != client_session.number {
        return Err(Response::builder()
            .status(StatusCode::SESSION_NOT_FOUND)
            .header(cseq_header.name, cseq_header.value)
            .body("".to_string())
            .unwrap());
    };

    Ok(())
}

fn parse_transport(transport: &Header) -> Option<(u16, u16)> {
    let transport = match std::str::from_utf8(transport.value) {
        Ok(transport) => transport,
        Err(_) => return None,
    };

    const TRANSPORT_PREFIX: &str = "RTP/AVP;unicast;client_port=";

    if !transport.starts_with(TRANSPORT_PREFIX) {
        return None;
    }

    let ports: Vec<&str> = transport[TRANSPORT_PREFIX.len()..].split('-').collect();
    if ports.len() != 2 {
        return None;
    }

    match (ports[0].parse::<u16>(), ports[1].parse::<u16>()) {
        (Ok(rtp), Ok(rtcp)) => return Some((rtp, rtcp)),
        _ => return None,
    }
}

fn write_response<W, B>(response: &Response<B>, w: &mut W) -> std::io::Result<()>
where
    W: Write,
    B: AsRef<[u8]>,
{
    writeln!(w, "RTSP/1.0 {}", response.status().as_str())?;

    for (name, value) in response.headers().iter() {
        writeln!(w, "{}: {}", name, value.to_str().unwrap())?;
    }

    w.write_all(b"\n\r")?; // extra newline before the body
    w.write_all(response.body().as_ref())?;

    Ok(())
}

unsafe fn transmute_lifetime<'a, 'b>(src: &'a [u8]) -> &'b [u8] {
    std::mem::transmute(src)
}

async fn receive_request<'h, 'b, R>(
    reader: &mut R,
    headers: &'h mut [Header<'b>],
    buffer: &'b mut BytesMut,
) -> Result<Request<'h, 'b>, RequestReceivingError> 
where
    R: AsyncRead + Unpin,{
    let mut request = rtsparse::Request::new(headers);
    loop {
        if !buffer.has_remaining() {
            return Err(RequestReceivingError::BufferOverflow);
        }

        reader
            .read_buf(buffer)
            .await?;
        match request.parse(unsafe { transmute_lifetime(buffer.bytes()) }) {
            Ok(Status::Partial) => continue,
            Ok(Status::Complete(_)) => {
                return Ok(request);
            }
            Err(e) => {
                log::error!("Invalid request received: {:?}", buffer);
                return Err(RequestReceivingError::InvalidRequest(e));
            }
        }
    }
}

fn process_request(
    request: &Request,
    c_session: Option<Session>,
    id: usize,
    addr: SocketAddr,
) -> Result<(Response<String>, Option<Session>), anyhow::Error> {
    let cseq = find_header(request, "CSeq");

    if cseq.is_none() {
        Response::builder()
            .status(StatusCode::BAD_REQUEST)
            .header("Reason", "No CSeq header");
    }

    let cseq = cseq.unwrap();

    match request
        .method
        .ok_or(anyhow::anyhow!("No method in request"))?
    {
        "OPTIONS" => {
            return Ok((Response::builder()
                .status(StatusCode::OK)
                .header(cseq.name, cseq.value)
                .header("Public", "DESCRIBE, SETUP, TEARDOWN, PLAY")
                .body("".to_string())
                .unwrap(), c_session));
        }
        "DESCRIBE" => {
            let simple_sdp = String::from(
                r#"o=- 1815849 0 IN IP4 127.0.0.1\r\n"
                                                    "c=IN IP4 127.0.0.1\r\n"
                                                    "m=video 1336 RTP/AVP 96\r\n"
                                                    "a=rtpmap:96 H264/90000\r\n"
                                                    "a=fmtp:96 packetization-mode=1\r\n"#,
            );
            return Ok((Response::builder()
                .status(StatusCode::OK)
                .header(cseq.name, cseq.value)
                .header("Content-Type", "application/sdp")
                .header("Content-Length", simple_sdp.len())
                .body(simple_sdp)
                .unwrap(), c_session));
        }
        "TEARDOWN" => {
            let session = match find_header(request, "Session") {
                Some(session) => session,
                None => {
                    return Ok((Response::builder()
                        .status(StatusCode::SESSION_NOT_FOUND)
                        .header(cseq.name, cseq.value)
                        .body("".to_string())
                        .unwrap(), None))
                }
            };

            let client_session = match &c_session {
                Some(session) => session,
                None => {
                    return Ok((Response::builder()
                        .status(StatusCode::BAD_REQUEST)
                        .header("Reason", "SETUP request was not received")
                        .body("".to_string())
                        .unwrap(), c_session))
                }
            };

            if let Err(error_response) = is_session_exist(session, client_session, cseq) {
                return Ok((error_response, c_session));
            }
            return Ok((Response::builder()
                .status(StatusCode::OK)
                .header(cseq.name, cseq.value)
                .body("".to_string())
                .unwrap(), None));
        }
        "SETUP" => {
            let transport_str = "Transport";
            let transport = match find_header(request, transport_str) {
                Some(header) => header,
                None => {
                    return Ok((Response::builder()
                        .status(StatusCode::PAYMENT_REQUIRED)
                        .header(cseq.name, cseq.value)
                        .body("".to_string())?, c_session))
                }
            };
            let (rtp, rtcp) = match parse_transport(transport) {
                Some(ports) => ports,
                None => {
                    return Ok((Response::builder()
                        .status(StatusCode::UNSUPPORTED_TRANSPORT)
                        .header(cseq.name, cseq.value)
                        .body("".to_string())
                        .unwrap(), c_session))
                }
            };
            let new_session = Some(Session {
                number: id,
                address: addr,
                rtp,
                rtcp,
            });
            let mut buffer = Vec::new();
            match write!(
                buffer,
                "RTP/AVP;unicast;client_port={}-{};server_port=1336-1337;ssrc=D34D10CC",
                new_session.unwrap().rtp,
                new_session.unwrap().rtcp
            ) {
                Ok(_) => (),
                Err(e) => log::error!("Error occured on writing transport prefix. Cliend: {}. Error: {}", id, e),
            }
            return Ok((Response::builder()
                .status(StatusCode::OK)
                .header(cseq.name, cseq.value)
                .header(transport_str, buffer)
                .header("Session", c_session.unwrap().number.to_string()) // FIX THIS
                .header(
                    "Media-Properties",
                    "No-Seeking, Time-Processing, Time-Duration=0.0",
                )
                .body("".to_string())
                .unwrap(), new_session));
        }
        "PLAY" => match find_header(request, "Session") {
            Some(session) => {
                let client_session = match &c_session {
                    Some(session) => session,
                    None => {
                        return Ok((Response::builder()
                            .status(StatusCode::BAD_REQUEST)
                            .header("Reason", "SETUP request was not received")
                            .body("".to_string())
                            .unwrap(), c_session))
                    }
                };

                if let Err(error_response) = is_session_exist(session, client_session, cseq) {
                    return Ok((error_response, c_session));
                }

                return Ok((Response::builder()
                    .status(StatusCode::OK)
                    .header(cseq.name, cseq.value)
                    .header(session.name, session.value)
                    .body("".to_string())
                    .unwrap(), c_session));
            }
            None => {
                return Ok((Response::builder()
                    .status(StatusCode::SESSION_NOT_FOUND)
                    .header(cseq.name, cseq.value)
                    .body("".to_string())
                    .unwrap(), c_session))
            }
        },
        _ => {
            return Ok((Response::builder()
                .status(StatusCode::NOT_IMPLEMENTED)
                .header(cseq.name, cseq.value)
                .body("".to_string())
                .unwrap(), c_session))
        }
    }
}
