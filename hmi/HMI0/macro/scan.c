#include "macrotypedef.h"
#include "math.h"

short state_arr[10] = {2,2,2,2,2,2,2,2,2,2};

int MacroEntry()
{
	WriteLocal("LW", 1600, 10, (void*)state_arr, 0);	
	return 0;
}
 