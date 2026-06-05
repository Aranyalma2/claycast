#include "src/config.h"
#include "src/SetupLoader.h"
#include <Arduino.h>
#include <HC12Transporter.h>

// Encryption key (just for set, but no practical use)
static const uint8_t SHARED_KEY[16] = {
    0xDE, 0xAD, 0xBE, 0xEF, 0xCA, 0xFE, 0xBA, 0xBE,
    0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF};

HC12Driver radio;
RadioTransport transport;

uint32_t triggerTime = 0;

// --- onReceive callback (fires from background task) ---
// sendAsync() is safe here. send() is NOT safe here -- it will deadlock.
static void onPacket(uint8_t src, PacketType type,
                     const uint8_t* data, uint8_t len) {
    switch (type) {
        case PacketType::DATA:
        Serial.printf("DATA from 0x%02X (%d bytes)\n", src, len);
        if(len > 1) { return; }
            switch (data[0]) {
                case 0x01:
                    Serial.println(F("Trigger received"));
                    triggerTime = millis();
                    digitalWrite(RELAY_PIN, HIGH);
                    break;
                case 0x02:
                    Serial.println(F("Get Battery"));
                    uint16_t batteryVoltage = SetupLoader::getBatteryVoltage();
                    transport.sendAsync(src, PacketType::DATA, &batteryVoltage, 2);
                    break;
                default:
                    break;
            }
            break;

        case PacketType::PING:
            // Reply with a PONG carrying this node's radio address.
            Serial.printf("PING from 0x%02X -> PONG\n", src);
            {
                // Payload: [mylocal channel and address]
                uint8_t pong[2] = {SetupLoader::getChannel(), SetupLoader::getAddress()};
                transport.sendAsync(src, PacketType::PONG, pong, 2);
            }
            break;

        default:
            break;
    }
}

void setup() {
    Serial.begin(115200);
    SetupLoader::init();

    // --- HC-12 configuration ---

    HC12Config hcCfg = HC12_DEFAULT_CONFIG;
    hcCfg.channel = SetupLoader::getChannel();
    hcCfg.baud = 9600;

    if (!radio.begin(&Serial1, HC12_SET_PIN, HC12_RX_PIN, HC12_TX_PIN, hcCfg)) {
        Serial.println(F("HC-12 init FAILED"));
        delay(2000);
        ESP.restart();
    }
    Serial.println(F("HC-12 OK"));

    // --- Transport configuration ---
    TransportConfig tCfg = TRANSPORT_DEFAULT_CONFIG;
    tCfg.localAddr = SetupLoader::getAddress() + 0x10;
    tCfg.autoPowerEnabled = false;
    memcpy(tCfg.encryptionKey, SHARED_KEY, 16);

    if (!transport.begin(&radio, tCfg)) {
        Serial.println(F("Transport init FAILED"));
        delay(2000);
        ESP.restart();
    }

    transport.onReceive(onPacket);

    if (!transport.startTask()) {
        Serial.println(F("Transport task START FAILED"));
        delay(2000);
        ESP.restart();
    }

    Serial.printf("[SLAVE 0x%02X] Transport ready\n", SetupLoader::getAddress() + 0x10);
}

void loop() {
    if (triggerTime + TRIGGER_TIME < millis()) {
        digitalWrite(RELAY_PIN, LOW);
    }
}