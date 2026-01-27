//! ATOMiK Rust SDK
//!
//! Delta-state computing primitives based on XOR algebra.

pub mod video {
    pub mod streaming;
}

pub use video::streaming::H264Delta;
