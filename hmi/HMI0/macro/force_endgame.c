#include "macrotypedef.h"
#include "math.h"

int MacroEntry()
{
	const short zero[10] = {0};
	WriteLocal("LW", 110, 3, (void*)zero, 0);
	WriteLocal("LW", 100, 10, (void*)zero, 0);
	const short endWindowID = 12;
	WriteLocal("LW", 500, 1, (void *)&endWindowID, 0);

	return 0;
}

 