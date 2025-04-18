#include <SoftwareSerial.h>

#define DEBUG 1  // Set to 1 to enable debug messages, 0 to disable

// HC12 module pins
const int hc12RxPin = 12;
const int hc12TxPin = 13;
const int hc12SetPin = 11;

const int hc12Channel = 50;  // Channel number (1–100)

// Create SoftwareSerial for HC12
SoftwareSerial hc12(hc12RxPin, hc12TxPin); // RX, TX

#define START_BYTE 0xAA
#define END_BYTE   0x55
#define MAX_DATA_SIZE 260

#define RS485_DE 2 // RS485 DE pin

uint8_t modbusBuffer[MAX_DATA_SIZE + 10];  // buffer for serial to HC12
uint8_t hc12Buffer[MAX_DATA_SIZE + 10];    // buffer for HC12 to serial

uint16_t wrapModbusRTU(const uint8_t* data, uint16_t dataSize, uint8_t* outBuffer) {
    if (dataSize > MAX_DATA_SIZE) return 0;

    uint16_t checksum = (dataSize >> 8) + (dataSize & 0xFF);
    for (uint16_t i = 0; i < dataSize; i++) checksum += data[i];

    uint16_t index = 0;
    outBuffer[index++] = START_BYTE;
    outBuffer[index++] = (dataSize >> 8) & 0xFF;
    outBuffer[index++] = dataSize & 0xFF;
    for (uint16_t i = 0; i < dataSize; i++) outBuffer[index++] = data[i];
    outBuffer[index++] = (checksum >> 8) & 0xFF;
    outBuffer[index++] = checksum & 0xFF;
    outBuffer[index++] = END_BYTE;

    return index;
}

bool unwrapModbusRTU(const uint8_t* packet, uint16_t packetSize, uint8_t* dataOut, uint16_t* dataSizeOut) {
    if (packetSize < 6 || packet[0] != START_BYTE || packet[packetSize - 1] != END_BYTE) return false;

    uint16_t size = (packet[1] << 8) | packet[2];
    if (size > MAX_DATA_SIZE || packetSize != size + 6) return false;

    uint16_t checksum = (size >> 8) + (size & 0xFF);
    for (uint16_t i = 0; i < size; i++) {
        dataOut[i] = packet[3 + i];
        checksum += dataOut[i];
    }

    uint16_t receivedChecksum = (packet[3 + size] << 8) | packet[4 + size];
    if (checksum != receivedChecksum) return false;

    *dataSizeOut = size;
    return true;
}

void setHC12Channel(uint8_t channel) {
    if (channel < 1 || channel > 100) return;  // Out of range

    char cmd[10];
    sprintf(cmd, "AT+C%03d", channel);  // Format: AT+C005, AT+C100, etc.
    Serial.print("Setting HC12 to channel: ");
    Serial.println(cmd);

    digitalWrite(hc12SetPin, LOW);  // Enter AT command mode
    delay(50);

    hc12.print(cmd);
    delay(100); // Give HC12 time to process

    // Optional: Wait for OK response
    while (hc12.available()) {
        char c = hc12.read();
        // Optionally print to Serial for debug
        if (DEBUG) {
            Serial.print(c);
        }
    }

    digitalWrite(hc12SetPin, HIGH); // Back to transparent mode
    delay(50);
}


void setup() {
    pinMode(RS485_DE, OUTPUT); // RS485 DE pin
    digitalWrite(RS485_DE, LOW); // Set to receive mode
    pinMode(hc12SetPin, OUTPUT);
    digitalWrite(hc12SetPin, HIGH); // Default mode

    Serial.begin(9600);     // Modbus RTU side
    hc12.begin(9600);       // HC12 communication

    setHC12Channel(hc12Channel); // Set channel before any data is sent
}

void loop() {
    // Relay Modbus data from Serial to HC12
    if (Serial.available()) {
        delay(10); // Wait to receive the full Modbus frame

        uint16_t len = 0;
        while (Serial.available() && len < MAX_DATA_SIZE) {
            modbusBuffer[len++] = Serial.read();
        }

        uint16_t wrappedLen = wrapModbusRTU(modbusBuffer, len, hc12Buffer);
        if (wrappedLen > 0) {
            hc12.write(hc12Buffer, wrappedLen);
        }
    }

    // Relay response from HC12 to Serial
    static uint8_t rxIndex = 0;
    while (hc12.available()) {
        uint8_t b = hc12.read();
        if (rxIndex < sizeof(hc12Buffer)) {
            hc12Buffer[rxIndex++] = b;
        }

        // Check if we might have a full packet
        if (rxIndex >= 6 && hc12Buffer[0] == START_BYTE && hc12Buffer[rxIndex - 1] == END_BYTE) {
            uint8_t unwrapped[MAX_DATA_SIZE];
            uint16_t unwrappedLen = 0;

            if (unwrapModbusRTU(hc12Buffer, rxIndex, unwrapped, &unwrappedLen)) {
                digitalWrite(RS485_DE, HIGH); // Set to transmit mode
                Serial.write(unwrapped, unwrappedLen);
                digitalWrite(RS485_DE, LOW); // Set back to receive mode
            }
            else {
                if (DEBUG) {
                    Serial.print("Invalid packet: ");
                    for (uint16_t i = 0; i < rxIndex; i++) {
                        Serial.print(hc12Buffer[i], HEX);
                        Serial.print(" ");
                    }
                    Serial.println();
                }
            }
            rxIndex = 0; // Reset for next packet
        }
    }
}
