/*******************************
 * Title: Kinco Shooting Range Game Setup Macro
 * Created: 2025-02-02
 * Version: 1.0
 * Author: Nemeth Balint
 *
 * Description:
 * This macro is used to pre-calculate ammo allocations for the next game, using the
 * information about how many machines are enabled, how much ammo they hold, and
 * how many double or triple fires are required.
 *
 * Input Registers:
 *   - LW0-LW9     : Flags indicating if each machine is enabled (1) or disabled (0).
 *   - LW10-LW19   : The decreased ammo values for selected machine(s) (overall).
 *   - LW110       : The max fireable ammo that can still be distributed (saved).
 *   - LW111       : The number of double fires (two fires in one cycle) (saved).
 *   - LW112       : The number of triple fires (three fires in one cycle) (saved).
 *   - LW113       : Delay between fires (saved).
 *   - LW110       : The max fireable ammo that can still be distributed (input).
 *   - LW111       : The number of double fires (two fires in one cycle) (input).
 *   - LW112       : The number of triple fires (three fires in one cycle) (input).
 *   - LW113       : Delay between fires (input).
 *
 * Background Registers (IN/OUT):
 *   - LW1000      : Remaining delay; indicates how many cycles remain until another fire can be initiated.
 *
 * Output Registers:
 *   - LW100-LW109 : The amount of ammo assigned to each machine for the upcoming game.
 *   - LW114       : Clear fired ammo register.
 *   - LW300       : Error code:
 *                      0 -> No error
 *                      1 -> All configured double/triple fires are not possible with the current ammo distribution
 *                      2 -> Ammo was not evenly distributed among machines
 *                      3 -> Not enough ammo or machines for the requested distribution
 *******************************/

#include "macrotypedef.h"
#include "math.h"

/*
 * Global Variables:
 * - usableMachines[]: Tracks which machines are enabled.
 * - ammo_machine[]  : Tracks each machine's currently available ammo (overall).
 * - ammo_game[]     : Tracks how much ammo each machine is allocated for the upcoming game.
 * - maxShootableAmmo: The maximum amount of ammo that can be fired across all machines.
 * - doubleFire      : The number of double-fire events to accommodate.
 * - tripleFire      : The number of triple-fire events to accommodate.
 * - delay           : The delay period between fires (e.g., cycles or ticks).
 * - errorFlag       : Tracks warnings or errors that arise during setup (0 means no error).
 */

short usableMachines[10] = {0}; // 1 = enabled, 0 = disabled for each of the 10 machines
short ammo_machine[10] = {0};		// Current ammo "capacity" for each machine (overall)
short ammo_game[10] = {0};			// Calculated ammo for the next game (what we'll distribute)
short maxShootableAmmo;					// How much total ammo can be fired across machines
short doubleFire;								// Number of double fires required
short tripleFire;								// Number of triple fires required
short delay = 0;								// Delay in ticks/cycles between fires
short errorFlag = 0;						// Global error flag (0 indicates no errors, >0 indicates a warning/error)
const short zero = 0;

/**
 * Function: reset_selected_machines
 * ---------------------------------
 * Resets the ammo_game array to zero for every machine. This ensures
 * we start with a clean slate before distributing ammo in distribute_ammo().
 */
void reset_selected_machines()
{
	short i;
	for (i = 0; i < 10; i++)
	{
		ammo_game[i] = 0;
	}
}

/**
 * Function: can_fire_x_times
 * --------------------------
 * Simulates whether we can actually perform the configured number of double and triple fires
 * using the allocated ammo in ammo_game[]. It does not change the real ammo_game[] array;
 * instead, it copies the data and decrements from the copy.
 *
 * Parameters:
 *   - total_ammo     : The total ammo we’re trying to validate (might be the same as maxShootableAmmo).
 *   - double_fires   : How many double fires we need to check feasibility for.
 *   - triple_fires   : How many triple fires we need to check feasibility for.
 *
 * Returns:
 *   - 1 if the distribution supports the necessary double/triple fires,
 *   - 0 otherwise.
 *
 * Algorithm:
 *   1. Copy ammo_game[] into a temporary array (ammo[]).
 *   2. Deduct three bullets per triple fire from the array, ensuring each bullet
 *      comes from a machine that has at least one bullet left.
 *   3. Deduct two bullets per double fire from the array in the same manner.
 *   4. If at any time there isn’t enough ammo to complete a triple or double fire, return 0.
 *   5. If all fires can be allocated correctly, verify that our leftover ammo aligns with
 *      what we expect (remaining_ammo + used bullets == maxShootableAmmo).
 */
int can_fire_x_times(short total_ammo, short double_fires, short triple_fires)
{
	short i, count;
	short remaining_ammo = maxShootableAmmo;
	short used_double = 0, used_triple = 0;

	// Copy the allocated ammo for each machine into a temporary array for simulation
	short ammo[10] = {0};
	for (i = 0; i < 10; i++)
	{
		ammo[i] = ammo_game[i];
	}

	// First, handle triple fires
	while (used_triple < tripleFire)
	{
		count = 0;
		// Attempt to find 3 bullets total, each from possibly different machines
		for (i = 0; i < 10 && count < 3; i++)
		{
			if (ammo[i] > 0)
			{
				ammo[i]--;
				count++;
			}
		}

		// If we don't find 3 bullets total, it means we cannot fulfill a triple fire
		if (count < 3)
			return 0;

		used_triple++;
		remaining_ammo -= 3; // Decrement the total ammo used by 3
	}

	// Next, handle double fires
	while (used_double < doubleFire)
	{
		count = 0;
		// Attempt to find 2 bullets total from the machines
		for (i = 0; i < 10 && count < 2; i++)
		{
			if (ammo[i] > 0)
			{
				ammo[i]--;
				count++;
			}
		}

		// If we don't find 2 bullets total, we cannot fulfill a double fire
		if (count < 2)
			return 0;

		used_double++;
		remaining_ammo -= 2; // Decrement the total ammo used by 2
	}

	// Finally, check the math to ensure we haven't used more or fewer bullets than expected
	// (remaining_ammo + total used for double + triple should equal maxShootableAmmo)
	return (remaining_ammo + used_double * 2 + used_triple * 3) == maxShootableAmmo;
}

/**
 * Function: distribute_ammo
 * -------------------------
 * Distributes the maxShootableAmmo evenly among all enabled machines. If a machine has
 * less capacity than the evenly calculated amount, we adjust accordingly and mark errorFlag
 * to indicate an uneven distribution. We also handle leftover ammo re-distribution if there
 * is any remainder after the initial pass.
 *
 * Steps:
 *   1. Count how many machines are enabled and have ammo capacity (ammo_machine[i] > 0).
 *   2. If no machines are enabled or we have zero maxShootableAmmo, set errorFlag to 3.
 *   3. Calculate ammoPerMachine = maxShootableAmmo / number_of_enabled_machines.
 *   4. The remainder (maxShootableAmmo % totalMachines) is leftover ammo that will be
 *      distributed one unit at a time.
 *   5. If some machines cannot handle the even split, we mark errorFlag = 2.
 *   6. Once the main distribution is done, we distribute leftover ammo to machines
 *      that can still handle more allocations.
 *   7. If we run out of machines that can accept leftover ammo but still have leftover,
 *      we set errorFlag = 3.
 */
void distribute_ammo()
{
	short totalMachines = 0;
	short i, ammoPerMachine, remainingAmmo;

	// Count how many machines can receive ammo
	for (i = 0; i < 10; i++)
	{
		if (usableMachines[i] && ammo_machine[i] > 0)
		{
			totalMachines++;
		}
	}

	// If there are no machines or if there is no ammo to distribute, set errorFlag = 3
	if (totalMachines == 0 || maxShootableAmmo == 0)
	{
		errorFlag = 3;
		return;
	}

	// Calculate the base distribution and leftover remainder
	ammoPerMachine = maxShootableAmmo / totalMachines;
	remainingAmmo = maxShootableAmmo % totalMachines;

	// Primary distribution across enabled machines
	for (i = 0; i < 10; i++)
	{
		if (usableMachines[i] && ammo_machine[i] > 0)
		{
			// If the machine's overall capacity is >= even allotment, use the even allotment
			if (ammo_machine[i] >= ammoPerMachine)
			{
				ammo_game[i] = ammoPerMachine;
			}
			else
			{
				// If the machine has less capacity than the calculated even share,
				// assign its maximum and redistribute the leftover to other machines
				ammo_game[i] = ammo_machine[i];
				remainingAmmo += (ammoPerMachine - ammo_machine[i]);
				// Mark a warning that distribution wasn't perfectly even
				errorFlag = 2;
			}
		}
	}

	// Distribute leftover ammo one unit at a time
	while (remainingAmmo > 0)
	{
		short unableToDistributeMore = 1;

		// Try assigning leftover ammo to machines that still have capacity
		for (i = 0; i < 10 && remainingAmmo > 0; i++)
		{
			if (usableMachines[i] && ammo_machine[i] > ammo_game[i])
			{
				ammo_game[i]++;
				remainingAmmo--;
				unableToDistributeMore = 0;
			}
		}

		// If we couldn't distribute any leftover in this pass, it's an error
		if (unableToDistributeMore)
		{
			errorFlag = 3;
			//Set the distributed as the maxShootableAmmo
			short distributed = 0;
			short i;
			for (i = 0; i < 10; i++){
				distributed += ammo_game[i];
			}
			maxShootableAmmo = distributed;
			break;
		}
	}
}

/**
 * Function: MacroEntry
 * --------------------
 * This is the main routine that executes once each cycle when the macro is called. It:
 *   1. Reads input registers to determine machine states, ammo, and timing constraints.
 *   2. Resets the allocated ammo array for a clean slate.
 *   3. Distributes ammo among machines.
 *   4. Validates if the distribution allows for the configured double/triple fires.
 *   5. Writes updated data and any errors or warnings back to the registers.
 *
 * Returns:
 *   - 0 to indicate successful completion of the macro.
 */
int MacroEntry()
{
	// Step 1: Read in the inputs (machine states, ammo data, config)
	ReadLocal("LW", 0, 10, (void *)usableMachines, 0);
	ReadLocal("LW", 10, 10, (void *)ammo_machine, 0);
	ReadLocal("LW", 100, 10, (void *)ammo_game, 0);
	ReadLocal("LW", 1100, 1, (void *)&maxShootableAmmo, 0);
	ReadLocal("LW", 1101, 1, (void *)&doubleFire, 0);
	ReadLocal("LW", 1102, 1, (void *)&tripleFire, 0);
	ReadLocal("LW", 1103, 1, (void *)&delay, 0);

	// Reset any prior error state and clear the allocated ammo distribution
	errorFlag = 0;
	reset_selected_machines();

	// Step 2: Distribute the ammo among the machines
	distribute_ammo();

	// Step 3: If no critical error occurred yet, check if the required fires can actually be performed
	if (errorFlag == 0)
	{
		// If can_fire_x_times returns 0, that means the distribution is insufficient for the configured fires
		errorFlag = !can_fire_x_times(maxShootableAmmo, doubleFire, tripleFire);
	}

	if (delay < 4)
	{
		delay = 4; // Ensure the delay is at least 2 ticks/cycles
	}

	// Step 4: Write back the updated states and any error codes
	WriteLocal("LW", 10, 10, (void *)ammo_machine, 0);
	WriteLocal("LW", 100, 10, (void *)ammo_game, 0);
	WriteLocal("LW", 110, 1, (void *)&maxShootableAmmo, 0);
	WriteLocal("LW", 111, 1, (void *)&doubleFire, 0);
	WriteLocal("LW", 112, 1, (void *)&tripleFire, 0);
	WriteLocal("LW", 113, 1, (void *)&delay, 0);
	WriteLocal("LW", 114, 1, (void *)&zero, 0);
	WriteLocal("LW", 300, 1, (void *)&errorFlag, 0);
	WriteLocal("LW", 1000, 1, (void *)&delay, 0);
	// Return 0 indicating the macro ran fully without exceptions
	return 0;
}
 