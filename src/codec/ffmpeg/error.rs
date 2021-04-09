use std::fmt::{self, Debug, Display, Formatter};

use ffmpeg_sys as ff;
use std::os::raw::c_int;

#[derive(Debug, Clone, Copy)]
pub struct Error {
    code: c_int,
}

#[derive(Debug, PartialEq, Eq, Clone, Copy)]
pub enum ErrorKind {
    Success,
    WouldBlock,
    Other,
}

impl Error {
    pub fn from_code(mut code: c_int) -> Self {
        if code < 0 {
            code = ff::AVERROR(code);
        }

        Error { code }
    }

    pub fn kind(&self) -> ErrorKind {
        match self.code {
            0 => ErrorKind::Success,
            ff::EAGAIN => ErrorKind::WouldBlock,
            _ => ErrorKind::Other,
        }
    }
}

impl Display for Error {
    fn fmt(&self, f: &mut Formatter<'_>) -> fmt::Result {
        const MAX_SIZE: usize = ff::AV_ERROR_MAX_STRING_SIZE as usize;
        let mut buffer = [0i8; MAX_SIZE];
        let s = unsafe {
            ff::av_strerror(self.code, buffer.as_mut_ptr(), buffer.len());
            std::ffi::CStr::from_ptr(buffer.as_mut_ptr())
        };

        write!(f, "{}", s.to_string_lossy())
    }
}

impl std::error::Error for Error {}
