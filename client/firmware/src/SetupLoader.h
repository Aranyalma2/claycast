#ifndef __SETUPLOADER_H__
#define __SETUPLOADER_H__

#include <stdint.h>

// Static class to measure analog pins and set channel and node address

class SetupLoader {
   private:
    uint8_t radioChannel;
    uint8_t nodeAddress;
    uint32_t batteryVoltageSum;
    uint8_t dipConversationTable(uint16_t measurement, uint8_t dipSize);

   public:
    static void init();
    static uint8_t getChannel();
    static uint8_t getAddress();
    static uint16_t getBatteryVoltage();
};
#endif  // __SETUPLOADER_H__