# Open Reckless Drivin'

This project is an attempt to reimplement the classic Macintosh shareware game Reckless Drivin' as a cross-platform
game. The source code of the original game was released by the author Jonas Echterhoff in 2019, but relies heavily
on deprecated Macintosh system calls for most aspects of the game. Additionally, the game data (sprites, sounds,
level data, etc.) are all LZRW3 compressed inside the resource fork of the original game, which is an additional
barrier to reimplementation.

The original source may be found at https://github.com/jechter/RecklessDrivin.

## Current Status

The original source contained a file called `Data` containing the resource fork. This has been converted into
a header file (`src/include/data`) to be embedded in the executable directly. This file contains various "Packs"
of data including Apple QuickDraw images, sounds, sprites, and fonts. Work has been done to reliably read,
decompress, decrypt, and interpret the data from this resource fork.

As an indicator of progress, PPic Packs 1000 through 1008 (the Apple QuickDraw images) have been read from the
resource fork. This verifies that the lzrw-3a decompression is working, and is a exciting marker of progress!
See the images on the wiki: https://github.com/natecraddock/open-reckless-drivin/wiki/QuickDraw-Pictures-(PPic)

The source code layout has been reorganized, with CMake as a build-system generator.

A small test with SDL was done to open a window with the loading screen.

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

Registration keys can also be generated in your name using a Python script. See [the wiki](https://github.com/natecraddock/open-reckless-drivin/wiki/Decryption) for more info.
