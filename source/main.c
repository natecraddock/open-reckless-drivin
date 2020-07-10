#include "gameframe.h"
#include "gameinitexit.h"
#include "initexit.h"
#include "interface.h"

void main() {
  Init();
  while (!gExit)
    if (gGameOn)
      GameFrame();
    else
      Eventloop();
  Exit();
}
