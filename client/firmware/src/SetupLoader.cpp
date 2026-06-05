#include "SetupLoader.h"

#include "config.h"

void SetupLoader::init() {
    pinMode(CHANNEL_SET, INPUT);
    pinMode(NODE_ADDRESS, INPUT);
    pinMode(BATTERY_MEASUREMENT_PIN, INPUT);

    analogReadResolution(12);

    uint32_t sum_ch = 0;
    uint32_t sum_addr = 0;

    for (uint8_t i = 0; i < 16; i++) {
        sum_ch += analogRead(CHANNEL_SET);
        delay(1);
    }

    for (uint8_t i = 0; i < 16; i++) {
        sum_addr += analogRead(NODE_ADDRESS);
        delay(1);
    }

    radioChannel = dipConversationTable(sum_ch / 16, 5);
    nodeAddress = dipConversationTable(sum_addr / 16, 4);

    batteryVoltageSum = 0;

    for (uint8_t i = 0; i < 16; i++) {
        batteryVoltageSum += analogRead(BATTERY_MEASUREMENT_PIN);
        delay(1);
    }
}

uint8_t SetupLoader::getChannel() {
    return radioChannel;
}

uint8_t SetupLoader::getAddress() {
    return nodeAddress;
}

uint16_t SetupLoader::getBatteryVoltage() {
    batteryVoltageSum *= 0.9375f;
    batteryVoltageSum += analogRead(BATTERY_MEASUREMENT_PIN);

    return batteryVoltageSum / 16;
}

/*
 * Converts the analog value of a dip switch to its binary value.
 * The pull down is 1K resistor.
 * Each dip has 2^n kohm and add them in parallel. (Binary-weighted resistor)
 * DIP1 (MSB) = 1k, DIP2 = 2k, DIP3 = 4k, DIP4 = 8k, DIP5 = 16k, DIP6 = 32k (LSB).
 * @param measurement: the analog value of the pin. (ref_max is 4095)
 * @param dipSize: the number of dip switches.
 * @return the value of the dip switches.
 */
uint8_t SetupLoader::dipConversationTable(uint16_t measurement, uint8_t dipSize) {
    // Hardware limitation, it also handles the case when it's not connected
    if (dipSize < 1 || dipSize > 6) return 0;

    uint8_t bestMatch = 0;
    uint16_t minDifference = 0xFFFF;  // Start with the maximum possible difference

    // Total possible digital values (e.g., dipSize 6 -> 64 combinations)
    uint16_t maxCombinations = 1 << dipSize;

    // Iterate through every possible digital value to find the closest ADC match
    for (uint16_t v = 0; v < maxCombinations; ++v) {
        float S = 0.0f;  // S is the sum of parallel conductances

        for (uint8_t i = 0; i < dipSize; ++i) {
            // Check if the i-th bit is set (i=0 is LSB, i=dipSize-1 is MSB)
            if (v & (1 << i)) {
                // MSB is always 1k. LSB scales up based on dipSize.
                // E.g., for dipSize=4: MSB (bit 3) = 1k, LSB (bit 0) = 8k.
                S += 1.0f / (float)(1 << (dipSize - 1 - i));
            }
        }

        // Calculate theoretical ADC value using a 1k pull-down
        float theoreticalAdcFloat = 4095.0f * S / (S + 1.0f);
        uint16_t theoreticalAdc = (uint16_t)(theoreticalAdcFloat + 0.5f);  // Round

        // Absolute difference between actual reading and theoretical reading
        uint16_t diff = (measurement > theoreticalAdc)
                            ? (measurement - theoreticalAdc)
                            : (theoreticalAdc - measurement);

        // Save the closest match
        if (diff < minDifference) {
            minDifference = diff;
            bestMatch = v;
        }
    }

    return bestMatch;
}