# Open Reckless Drivin'

This project is an attempt to reimplement the classic Macintosh shareware game Reckless Drivin' as a cross-platform
game. The source code of the original game was released by the author Jonas Echterhoff in 2019, but relies heavily
on deprecated Macintosh system calls for most aspects of the game. Additionally, the game data (sprites, sounds,
level data, etc.) are all LZRW3 compressed inside the resource fork of the original game, which is an additional
barrier to reimplementation.

The original source may be found at https://github.com/jechter/RecklessDrivin.
## Current Status

All data in the original `Data` resource fork has been separated into individual files, decrypted (when applicable)
and decompressed (lzrw3 decompression).

The PPic images 1000 through 1008 have been interpreted and read from memory. This verifies
that the decompression works! Image 1009 is in a more complex format so I might take a break
from PPic and QuickDraw for a while. See the images on the wiki: https://github.com/natecraddock/open-reckless-drivin/wiki/QuickDraw-Pictures-(PPic)

Next I will take some time to organize the source code. I will likely use CMake.

## Registration Keys

Jonas released the game for free after it was no longer viable to update it for modern systems. More information
can be found on the [game's website](http://jonasechterhoff.com/Reckless_Drivin.html). The free registration
information is:

Name: Free<br>
Code: B3FB09B1EB

Registration keys can also be generated in your name using a Python script. See [the wiki](https://github.com/natecraddock/open-reckless-drivin/wiki/Decryption) for more info.
