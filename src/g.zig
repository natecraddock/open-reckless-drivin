//! The Reckless Drivin' source code uses many global variables. When I first started
//! translating the code to Zig, I decided to use as few globals as possible. But this
//! slowed down implementation speed.
//!
//! So I have decided to store all globals in this file. In the future things can be
//! made more organized as needed.

const Point = @import("point.zig").Point;

/// gFrameCount
pub var frame_count: u64 = 0;

/// A segment of track
///
/// tTrackSeg
pub const TrackSegment = struct {
    p1: Point,
    p2: Point,
    intensity: f32,
    time: u32,
};

/// The number of tracks
///
/// gTrackCount. Changed from i32 in the original code (it will never be negative)
pub var track_count: u32 = 0;

// TODO: maybe shared constants can go in c.zig?
const max_tracks = 4096;

/// All tracks
///
/// gTracks
pub var tracks: [max_tracks]TrackSegment = undefined;
