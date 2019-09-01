#ifndef __GAMEINITEXIT
#define __GAMEINITEXIT

extern int gLevelID;
extern int gGameOn;
extern int gPlayerCarID;

void DisposeLevel();
void StartGame(int);
int LoadLevel();
void EndGame();

#endif