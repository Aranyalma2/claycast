#include "macrotypedef.h"
#include "math.h"

int MacroEntry()
{
	short nextFires[10] = {0};
	WriteLocal("LW", 200, 10, (void *)nextFires, 0);
	return 0;
}

 