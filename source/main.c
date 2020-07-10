#include "gameframe.h"
#include "gameinitexit.h"
#include "initexit.h"
#include "interface.h"

int main(void) {
  Init();
  while (!gExit) {
    if (gGameOn) {
      GameFrame();
    }
    else {
      Eventloop();
    }
  }
  Exit();

  return 0;
}
