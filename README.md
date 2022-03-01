# Open Reckless Drivin'

This project is an attempt to reimplement the classic Macintosh shareware game Reckless Drivin' as a cross-platform
game. The source code of the original game was released by the author Jonas Echterhoff in 2019, but relies heavily
on deprecated Macintosh system calls for most aspects of the game. Additionally, the game data (sprites, sounds,
level data, etc.) are all LZRW3 compressed inside the resource fork of the original game, which is an additional
barrier to reimplementation.

The original source may be found at https://github.com/jechter/RecklessDrivin.

## Current Status

**Update**: After a long break away from this project, I'm back and working on
this in the
[riiz](https://github.com/natecraddock/open-reckless-drivin/tree/riiz) branch,
where I'm rewriting in Zig! I discuss my rationale behind this in more detail on
[my blog post](https://nathancraddock.com/blog/moving-to-zig/).

**TL;DR**: The game is not playable. Binary data loading, decompression, decryption, and reading is working.
Preferences are read from a file. The source code has been restructured and uses CMake. Currently working on
using SDL2 for drawing sprites.

The original source contained a file called `Data` containing the resource fork. This has been converted into
a header file (`src/include/data`) to be embedded in the executable directly. This file contains various "Packs"
of data including Apple QuickDraw images, sounds, sprites, and fonts. Work has been done to reliably read,
decompress, decrypt, and interpret the data from this resource fork.

As an indicator of progress, PPic Packs 1000 through 1008 (the Apple QuickDraw images) have been read from the
resource fork. This verifies that the lzrw-3a decompression is working, and is a exciting marker of progress!
See the images on the [wiki](https://github.com/natecraddock/open-reckless-drivin/wiki/QuickDraw-Pictures-(PPic)).

The source code layout has been reorganized, with CMake as a build-system generator.

A small test with SDL was done to open a window with the loading screen, but has been disabled for now.

The preference file reading and writing is working well now. Because I'm developing on Linux, the paths are
Linux-specific, but I have structured the code to make it easy to update when I start testing on other platforms.
The format is INI for simplicity, and the expected file path is `~/.config/open-reckless-drivin/prefs.ini`. (or
`$XDG_CONFIG_HOME/open-reckless-drivin/prefs.ini` if set). The `open-reckless-drivin` directory must exist and will not
be created if it doesn't exist.

The **name** and **code** preferences are read for checking for a registered copy of the game, allowing for
decryption of levels 4 through 10.

Currently I am adding game loop code one function at a time. This is a slow going process. There are many
Macintosh system calls in each file that need to either be removed because they play no role in the game,
or the functions need to be replaced with a cross-platform alternative.

# Building

So far I have only built Open Reckless Drivin' on Arch Linux, but it should hopefully build elsewhere with no changes.
I prefer Ninja, but Makefiles also work fine. Suggested build instructions:

```text
mkdir build && cd build
cmake -GNinja ../
ninja
```

To run tests

```
ninja test
```

## Registration Keys

Jonas released the game for free after it was no longer viable to update it for modern systems. More information
can be found on the [game's website](http://jonasechterhoff.com/Reckless_Drivin.html). The free registration
information is:

Name: Free<br>
Code: B3FB09B1EB

Registration keys can also be generated in your name using a Python script.
See [the wiki](https://github.com/natecraddock/open-reckless-drivin/wiki/Decryption) for more info.
