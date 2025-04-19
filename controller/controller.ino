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
    if (DEBUG) {
      digitalWrite(RS485_DE, HIGH); // Set to transmit mode
      Serial.print("Setting HC12 to channel: ");
      Serial.println(cmd);
      digitalWrite(RS485_DE, LOW); // Set to receive mode
    }

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

    // Debug output
    if (DEBUG) {
      digitalWrite(RS485_DE, HIGH); // Set to transmit mode
      Serial.println("ClayCast Modbus RTU Controller");
      Serial.println("HC-12 and Modbus RTU setup complete.");
      digitalWrite(RS485_DE, LOW); // Set back to receive mode
    }

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
        if(DEBUG) {
            digitalWrite(RS485_DE, HIGH); // Set to transmit mode
            Serial.print("Wrapped length: ");
            Serial.println(wrappedLen);
            Serial.print("Wrapped data: ");
            for (uint16_t i = 0; i < wrappedLen; i++) {
                if ((uint8_t)hc12Buffer[i] < 0x10) Serial.print('0');  // Leading zero for single-digit hex
                Serial.print((uint8_t)hc12Buffer[i], HEX);
                Serial.print(' ');  // Optional: space between hex values
            }
            Serial.println(")");
            digitalWrite(RS485_DE, LOW); // Set back to receive mode
        }
        if (wrappedLen > 0) {
            hc12.write(hc12Buffer, wrappedLen);
        }
    }

    // Relay response from HC12 to Serial
    if (hc12.available()) {
        delay(10);
        uint16_t len = 0;
        while (hc12.available() && len < MAX_DATA_SIZE) {
            hc12Buffer[len++] = hc12.read();
        }
        uint16_t unwrappedLen = 0;
        uint8_t unwrapped[MAX_DATA_SIZE];
        if (unwrapModbusRTU(hc12Buffer, len, unwrapped, &unwrappedLen)) {
            digitalWrite(RS485_DE, HIGH); // Set to transmit mode
            delay(10); // Allow time for RS485 to switch
            Serial.write(unwrapped, unwrappedLen);
            digitalWrite(RS485_DE, LOW); // Set back to receive mode
        } else {
            if(DEBUG) {
                digitalWrite(RS485_DE, HIGH); // Set to transmit mode
                Serial.println("Invalid packet");
                digitalWrite(RS485_DE, LOW); // Set back to receive mode
            }
        }
    }
}
