# Open Reckless Drivin'

This project is an attempt to reimplement the classic Macintosh shareware game
Reckless Drivin' as a cross-platform game. The source code of the original game
was released by the author Jonas Echterhoff in 2019, but relies heavily on
deprecated Macintosh system calls for most aspects of the game. Additionally,
the game data (sprites, sounds, level data, etc.) are all LZRW3 compressed
inside the resource fork of the original game, which is an additional barrier to
reimplementation.

The original source may be found at https://github.com/jechter/RecklessDrivin.

## Current Status

**Update:** I am finished porting all my work in C to Zig. Overall I am very
pleased with the rewrite, and I appreciate all of the little improvements to the
code that were lacking in C, like knowing the length of a slice without
resorting to offsets and weird fat pointer tricks I was using previously.

At this stage, the game is not playable. But at startup all of the data is
decompressed and loaded into memory, and then promptly freed before exiting. The
next step is to start the game loop! I have chosen to ignore the main menu for
now so we can get a playable game as soon as possible.

## Details

The original source contained a file called `Data` containing the resource fork.
This has been converted into a header file (`src/include/data`) to be embedded
in the executable directly. This file contains various "Packs" of data including
Apple QuickDraw images, sounds, sprites, and fonts. Work has been done to
reliably read, decompress, decrypt, and interpret the data from this resource
fork.

As an indicator of progress, PPic Packs 1000 through 1008 (the Apple QuickDraw
images) have been read from the resource fork. This verifies that the lzrw-3a
decompression is working, and is a exciting marker of progress! See the images
on the
[wiki](https://github.com/natecraddock/open-reckless-drivin/wiki/QuickDraw-Pictures-(PPic)).

# Building

So far I have only built Open Reckless Drivin' on Arch Linux, but I see no
reason why it wouldn't work elsewhere. Using the latest stable version of Zig:

```text
zig build
```

The binary will be placed in `zig-out/bin/reckless-drivin`. Run with the
`register [name]` option to generate a new activation code.

To run tests

```
zig build test
```

## Registration Keys

Jonas released the game for free after it was no longer viable to update it for
modern systems. More information can be found on the [game's
website](http://jonasechterhoff.com/Reckless_Drivin.html). The free registration
information is:

Name: Free<br>
Code: B3FB09B1EB
