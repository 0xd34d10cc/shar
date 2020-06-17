use std::net::{IpAddr, Ipv4Addr, SocketAddr};

use anyhow::{anyhow, Result, Context};
use url::{Url, Host};


fn localhost() -> IpAddr {
    IpAddr::V4(Ipv4Addr::new(127, 0, 0, 1))
}

async fn resolve(hostname: &str) -> Result<IpAddr> {
    let mut addresses = tokio::net::lookup_host(&hostname).await?;
    let address = addresses
        .next()
        .ok_or_else(|| anyhow!("DNS returned 0 ip addresses for {}", hostname))?;
    let ip = address.ip();
    Ok(ip)
}

pub async fn address_of(url: Url) -> Result<SocketAddr> {
    let ip = match url.host() {
        None => localhost(),
        Some(Host::Domain(name)) => {
            // see https://github.com/servo/rust-url/issues/606
            if let Ok(ip) = name.parse::<IpAddr>() {
                ip
            } else {
                resolve(name).await.context("Hostname resolution failed")?
            }
        },
        Some(Host::Ipv4(ip)) => IpAddr::V4(ip),
        Some(Host::Ipv6(ip)) => IpAddr::V6(ip),
    };

    let port = url.port_or_known_default().unwrap_or(1337);
    let address = SocketAddr::new(ip, port);

    log::info!("Resolved {} to {}", url, address);
    Ok(address)
}
