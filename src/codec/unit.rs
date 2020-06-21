pub trait Unit: Send + 'static {
    fn from_packet(data: &[u8]) -> Self;

    fn data(&self) -> &[u8];
    fn is_idr(&self) -> bool;
}