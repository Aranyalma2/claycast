/*******************************
 * Title: Kinco Shooting range machine selection Macro
 * Created: 2025-02-01
 * Version: 1.0
 * Author: Nemeth Balint
 *
 * Description:
 * This macro is used to randomly select the next machine(s) to fire. The probability of
 * each machine being selected is determined by the number of ammo it has. The more ammo
 * a machine has, the higher the probability it will be selected.
 *
 * Input:
 *  - LW0-LW9: the number of enabled machines (1 means a machine is enabled, 0 means disabled).
 *  - LW10-LW19: the decreased ammo value for the selected machine(s) (overall).
 *  - LW100-LW109: the number of ammo for each machine (specific to the game).
 *  - LW110: the maximum shootable ammo across all machines.
 *  - LW111: the number of double fires allowed.
 *  - LW112: the number of triple fires allowed.
 *  - LW113: the delay between fires.
 *
 *  * Background Registers (IN/OUT):
 *   - LW1000      : Remaining delay; indicates how many cycles remain until another fire can be initiated.
 *   - LW114        : Fired ammo; 
*
 * Output:
 *  - LW10-LW19: updated overall ammo values for the selected machine(s).
 *  - LW100-LW109: updated game-specific ammo values for the selected machine(s).
 *  - LW110: updated maximum shootable ammo after firing.
 *  - LW111: updated double fire count.
 *  - LW112: updated triple fire count.
 *  - LW200-LW209: indicates which machines were selected to fire in this cycle.
 */

#include "macrotypedef.h"
#include "math.h"
#include <stdlib.h>
#include <time.h>

// Global variables representing machine states, ammo counts, and firing rules.
short usableMachines[10] = {0}; // Stores flags indicating which machines are enabled (1) or not (0).
short nextFires[10] = {0};			// Tracks which machines are chosen to fire in each cycle (1 = chosen, 0 = not chosen).
short ammo_machine[10] = {0};		// Tracks the overall ammo available for each machine.
short ammo_game[10] = {0};			// Tracks the game-specific ammo for each machine.
short maxShootableAmmo;					// The maximum total ammo that can be fired before the game ends.
short doubleFire;								// The number of double (two-in-one) fires allowed.
short tripleFire;								// The number of triple (three-in-one) fires allowed.
short delay = 0;								// The delay (in cycles or ticks) that must occur between firing actions.
short remainingDelay = 0;				// How many cycles/ticks remain before another firing action can occur.
short fired = 0;
const endWindowID = 12;

/**
 * Function: reset_selected_machines
 * ---------------------------------
 * Sets all entries in nextFires to 0, indicating that no machines have been chosen
 * for firing in this cycle. This is called at the start of each macro cycle to ensure
 * a fresh selection.
 */
void reset_selected_machines()
{
	short i;
	for (i = 0; i < 10; i++)
	{
		nextFires[i] = 0;
	}
}

/**
 * Function: delayer
 * -----------------
 * Manages a delay mechanism between consecutive firings.
 *
 * Returns:
 *   - true if the delay should still prevent firing (i.e., we are within a delay period),
 *   - false if the firing can proceed (i.e., the delay period has elapsed).
 *
 * Explanation:
 *  - If delay is less than or equal to 0, we treat it as an error scenario and return true
 *    to indicate we cannot proceed with firing.
 *  - If remainingDelay is 0, it means no delay is pending, so we set remainingDelay to the
 *    configured delay and allow a new firing action.
 *  - Otherwise, we decrement remainingDelay, indicating that one cycle of delay has passed,
 *    and return true to prevent firing if any delay remains.
 */
bool delayer()
{
	if (delay <= 0)
	{
		// Negative or zero delay is treated as an error or invalid scenario.
		return true;
	}

	if (remainingDelay == 0)
	{
		// No existing delay, so we reset the remainder delay to the 'delay' value
		// and allow firing to proceed by returning false.
		remainingDelay = delay;
		return false;
	}
	else
	{
		// Delay is ongoing; reduce the remaining delay by one and prohibit firing this cycle.
		remainingDelay--;
		return true;
	}
}

/**
 * Function: checkEndGame
 * ----------------------
 * Evaluates whether the current game has ended by checking if maxShootableAmmo has
 * dropped to 0 or below. If it has, all firing modes are set to zero, and the
 * ammo_game array is cleared.
 */
bool checkEndGame()
{
	if (maxShootableAmmo <= 0)
	{
		// No more ammo can be fired, so disable double and triple fire modes.
		doubleFire = 0;
		tripleFire = 0;

		// Clear the game-specific ammo for all machines.
		short i;
		for (i = 0; i < 10; i++)
		{
			ammo_game[i] = 0;
		}
		return true;
	}
	return false;
}

/**
 * Function: select_mode
 * ---------------------
 * Randomly selects whether the next fire should be single, double, or triple. This is determined
 * by comparing a random value with weighted probabilities (based on the remaining number of
 * single, double, or triple fires allowed).
 *
 * Returns:
 *   - 1 for a single fire,
 *   - 2 for a double fire,
 *   - 3 for a triple fire,
 *   - 0 in case no firing mode is available (e.g., zero or negative ammo).
 *
 * Explanation:
 *  - We calculate simpleFire as maxShootableAmmo minus the ammo needed for all potential double and triple fires.
 *  - We create weights based on how many single, double, and triple fires remain feasible.
 *  - We then do a weighted random to decide which mode is chosen.
 *  - If the program cannot find a suitable mode after 100 attempts, it defaults to a single fire mode (returns 1).
 */
short select_mode()
{
	// Calculate how many single-fires can still occur if we commit to using all double/triple fires.
	short simpleFire = maxShootableAmmo - doubleFire * 2 - tripleFire * 3;

	// Track the current amount of ammo that can still be fired in total.
	short remainingAmmo = maxShootableAmmo;

	// These arrays help us quickly handle weighting logic for the random selection.
	short availableModes[3] = {1, 2, 3};
	short weights[3] = {simpleFire, doubleFire * 2, tripleFire * 3};

	// If there's not enough ammo to proceed, return 0.
	if (remainingAmmo <= 0)
		return 0;

	short unable_to_resolve = 0;

	while (1)
	{
		// Weighted random - pick a number within the sum of all weights.
		short choice = rand() % (weights[0] + weights[1] + weights[2]);

		// If the random "choice" falls in the range for single fires
		if (choice < weights[0] && remainingAmmo >= 1)
		{
			return 1; // Single fire
		}
		// If it falls in the range for double fires
		else if (choice < weights[0] + weights[1] && remainingAmmo >= 2 && doubleFire > 0)
		{
			return 2; // Double fire
		}
		// Otherwise, it must be in the triple-fire range
		else if (choice < weights[0] + weights[1] + weights[2] && remainingAmmo >= 3 && tripleFire > 0)
		{
			return 3; // Triple fire
		}
		else
		{
			// If we cannot determine the mode, increment the fail counter.
			unable_to_resolve++;
			if (unable_to_resolve > 100)
			{
				// If this keeps failing, default to a single fire.
				return 1;
			}
		}
	}
}

/**
 * Function: weighted_random_selection
 * -----------------------------------
 * Chooses one machine to fire based on its weighted probability. Machines with higher game ammo
 * have a higher chance of being selected.
 *
 * Returns:
 *   - The index of the chosen machine (0-9), or
 *   - -1 if no machines have any ammo available for firing.
 *
 * Explanation:
 *  - We first sum the ammo_game values for all machines that are enabled.
 *  - A random value is chosen within 0 and total_weight - 1.
 *  - We then walk through enabled machines summing up their ammo_game counts until
 *    we exceed the random value, indicating that machine was chosen.
 */
short weighted_random_selection()
{
	int total_weight = 0;			 // Sum of all enabled machines' ammo counts
	int cumulative_weight = 0; // Running tally used to compare against random_value
	int random_value;
	short i;

	// Calculate the total weight for all enabled machines.
	for (i = 0; i < 10; i++)
	{
		if (usableMachines[i] && ammo_game[i] > 0)
		{
			total_weight += ammo_game[i];
		}
	}

	// If no machine has ammo, return -1.
	if (total_weight == 0)
		return -1;

	// Choose a random value in the inclusive range [0, total_weight-1].
	random_value = rand() % total_weight;

	for (i = 0; i < 10; i++)
	{
		if (usableMachines[i] && ammo_game[i] > 0)
		{
			// Accumulate weights until we surpass the random_value.
			cumulative_weight += ammo_game[i];
			if (random_value < cumulative_weight)
			{
				return i; // This machine is selected.
			}
		}
	}
	// In a normal scenario, we won't reach here, but return -1 if no selection is made.
	return -1;
}

/**
 * Function: process_fire
 * ----------------------
 * Executes the actual firing on one or more machines. The number of machines to fire
 * is indicated by 'count'. This can be 1 (single fire), 2 (double), or 3 (triple).
 *
 * Parameters:
 *   - count: How many machines should fire this cycle (1, 2, or 3).
 *
 * Explanation:
 *  - We call weighted_random_selection() 'count' times to pick distinct machines.
 *  - Each selected machine's ammo is decremented (both overall and game-specific).
 *  - The machine is flagged in nextFires and then disabled for subsequent picks
 *    within the same cycle to avoid picking it multiple times.
 *  - Depending on 'count', we decrement the appropriate doubleFire or tripleFire counters.
 *  - If the selection fails, we simply do not pick a machine (indicated by -1).
 */
void process_fire(int count)
{
	short i, selected;
	for (i = 0; i < count; i++)
	{
		selected = weighted_random_selection();
		if (selected != -1)
		{
			// Reduce ammo for overall and game-specific tracking
			ammo_machine[selected]--;
			ammo_game[selected]--;

			// Reduce total available shoots
			maxShootableAmmo--;

			// Mark the chosen machine for firing
			nextFires[selected] = 1;

			// Disable the machine so it won't be chosen again in this cycle
			usableMachines[selected] = 0;

			fired++;
		}
		else
		{
			// If we fail to pick a machine, we can log or handle an error scenario
			// but for simplicity, we do nothing.
		}
	}

	// Adjust the count of double/triple fires allowed if used in this cycle.
	switch (count)
	{
	case 1:
		// Single fire, no double or triple counters to decrement
		break;
	case 2:
		// Double fire used
		doubleFire--;
		break;
	case 3:
		// Triple fire used
		tripleFire--;
		break;
	}
}

/**
 * Function: MacroEntry
 * --------------------
 * This is the main macro entry point. It performs the following steps:
 *   1. Initializes the random number generator.
 *   2. Reads the necessary inputs (machine states, ammo values, firing rules, etc.).
 *   3. Resets the nextFires state to ensure a fresh cycle.
 *   4. Checks and handles delay logic.
 *   5. If firing is allowed (no delay), determines how many machines will fire and processes it.
 *   6. Checks whether the game has ended or if more firings can proceed.
 *   7. Writes back all updated outputs (changed ammo values, nextFires state, etc.).
 *
 * Returns:
 *   - 0 to indicate successful execution.
 */
int MacroEntry()
{
	// Seed the random number generator once per macro call with the current time.
	srand(time(NULL));

	// Read in the inputs for configuring the firing logic and ammo tracking.
	ReadLocal("LW", 0, 10, (void *)usableMachines, 0);
	ReadLocal("LW", 10, 10, (void *)ammo_machine, 0);
	ReadLocal("LW", 100, 10, (void *)ammo_game, 0);
	ReadLocal("LW", 110, 1, (void *)&maxShootableAmmo, 0);
	ReadLocal("LW", 111, 1, (void *)&doubleFire, 0);
	ReadLocal("LW", 112, 1, (void *)&tripleFire, 0);
	ReadLocal("LW", 113, 1, (void *)&delay, 0);
	ReadLocal("LW", 114, 1, (void *)&fired, 0);
	ReadLocal("LW", 200, 10, (void *)nextFires, 0);
	ReadLocal("LW", 1000, 1, (void *)&remainingDelay, 0);

	// Ensure no machines are marked as "firing" at the start of this macro cycle.
	if (remainingDelay < delay / 2) {
		reset_selected_machines();
	}

	// If delayer() returns false, no delay is preventing firing, so we can proceed.
	if (!delayer())
	{
		// If we've used up all ammo, end the game by resetting relevant counters, etc.
		if (checkEndGame()) {
			short zero = 0;	
			WriteLocal("LW", 400, 1, (void *)&zero, 0);
			short i;
			for (i = 0; i < 10; i++)
			{
				nextFires[i] = 0;
			}
			WriteLocal("LW", 500, 1, (void *)&endWindowID, 0);
		}
		else{
			// Decide how many machines should fire in this cycle (1, 2, or 3).
			process_fire(select_mode());
		}
	}

	// Write back the updated states for both the overall ammo and the game-specific ammo.
	WriteLocal("LW", 10, 10, (void *)ammo_machine, 0);
	WriteLocal("LW", 100, 10, (void *)ammo_game, 0);
	WriteLocal("LW", 110, 1, (void *)&maxShootableAmmo, 0);
	WriteLocal("LW", 111, 1, (void *)&doubleFire, 0);
	WriteLocal("LW", 112, 1, (void *)&tripleFire, 0);
	WriteLocal("LW", 114, 1, (void *)&fired, 0);

	// Write which machines were chosen (nextFires array) to LW200-LW209,
	// and also update the remaining delay.
	WriteLocal("LW", 200, 10, (void *)nextFires, 0);
	WriteLocal("LW", 1000, 1, (void *)&remainingDelay, 0);

	// Return 0 indicating successful execution of the macro.
	return 0;
}
 