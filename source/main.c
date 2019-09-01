#include "initexit.h"
#include "gameframe.h"
#include "interface.h"
#include "gameinitexit.h"

void main()
{
	Init();
	while(!gExit) 
		if(gGameOn) GameFrame();
		else Eventloop();
	Exit();
}