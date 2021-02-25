# Open Reckless Drivin'

This project is an attempt to reimplement the classic Macintosh shareware game Reckless Drivin' as a cross-platform
game. The source code of the original game was released by the author Jonas Echterhoff in 2019, but relies heavily
on deprecated Macintosh system calls for most aspects of the game. Additionally, the game data (sprites, sounds,
level data, etc.) are all LZRW3 compressed inside the resource fork of the original game, which is an additional
barrier to reimplementation.

---

## Current status

All data in the original `Data` resource fork has been separated into individual files, decrypted (when applicable)
and decompressed (lzrw3 decompression).

The next step is to interpret these bytes properly. I plan to start with the `PPic` types as these are the images
that show when loading the game. The data appears similar to a version 1 Macintosh QuickDraw image.

## Registration Keys

Jonas released the game for free after it was no longer viable to update it for modern systems. More information
can be found on the [game's website](http://jonasechterhoff.com/Reckless_Drivin.html). The free registration
information is:

Name: Free<br>
Code: B3FB09B1EB

Registration keys can also be generated in your name using a Python script. See [the wiki](https://github.com/natecraddock/open-reckless-drivin/wiki/Decryption) for more info.

---

This is the original source code for the mac shareware game "Reckless Drivin'", originally released in 2000.

The source code is in C, project.mcp is a CodeWarrior project file which was used to build it. To be able to upload this to git, Line endings have been converted to Unix style, and the resource forks of the rsrc files have been moved to the data fork. You may need to revert these changes before being able to build it on a classic Macintosh.
