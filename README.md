# Reckless Drivin'

This is an attempt to port Reckless Drivin' from Macintosh to openGL on Linux.

The game is now available for free http://jonasechterhoff.com/Reckless_Drivin.html

---

# Current status

Working on loading the packed data from `Data` (the resource fork). I have successfully written a function to fetch the correct resource, and decompress it with lzrw decompression.

The next step is to interpret the bytes correctly!

---

Name: Free<br>
Code: B3FB09B1EB

---

This is the original source code for the mac shareware game "Reckless Drivin'", originally released in 2000.

The source code is in C, project.mcp is a CodeWarrior project file which was used to build it. To be able to upload this to git, Line endings have been converted to Unix style, and the resource forks of the rsrc files have been moved to the data fork. You may need to revert these changes before being able to build it on a classic Macintosh.
