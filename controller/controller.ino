#include <SoftwareSerial.h>

#define DEBUG 0 // Set to 1 to enable debug messages, 0 to disable

// HC12 module pins
const int hc12RxPin = 13;
const int hc12TxPin = 12;
const int hc12SetPin = 11;

const int hc12Channel = 50; // Channel number (1–100)

// Create SoftwareSerial for HC12
SoftwareSerial hc12(hc12TxPin, hc12RxPin); // RX, TX

#define START_BYTE 0xAA
#define END_BYTE 0x55
#define MAX_DATA_SIZE 260

#define BAUD_RATE 9600
#define RS485_DE 2 // RS485 DE pin

uint8_t hc12_buffer[MAX_DATA_SIZE + 10]; // buffer for HC12 to serial
bool hc12_receiving = false;
uint16_t hc12_recvIndex = 0;
uint32_t hc12_lastByteTime = 0;
uint8_t hc12_frameTime = 40; // Frame time in ms

uint8_t serial_buffer[MAX_DATA_SIZE]; // buffer for serial to HC12
bool serial_receiving = false;
uint16_t serial_recvIndex = 0;
uint32_t serial_lastByteTime = 0;
uint8_t serial_frameTime = 4; // Frame time in ms (Modbus RTU standard)

bool testMode = false;
uint32_t lastTestSendTime = 0;
uint16_t testCounter = 0;

uint8_t packetBuffer[MAX_DATA_SIZE + 10]; // Buffer for transfer data packet

uint16_t wrapModbusRTU(const uint8_t * data, uint16_t dataSize, uint8_t * outBuffer) {
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

bool unwrapModbusRTU(const uint8_t * packet, uint16_t packetSize, uint8_t * dataOut, uint16_t * dataSizeOut) {
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
  if (channel < 1 || channel > 100) return; // Out of range

  char cmd[10];
  sprintf(cmd, "AT+C%03d", channel); // Format: AT+C005, AT+C100, etc.
  if (DEBUG) {
    digitalWrite(RS485_DE, HIGH); // Set to transmit mode
    Serial.print("Setting HC12 to channel: ");
    Serial.println(cmd);
    digitalWrite(RS485_DE, LOW); // Set to receive mode
  }

  digitalWrite(hc12SetPin, LOW); // Enter AT command mode
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

  pinMode(A0, INPUT); // Enable A0 as input to check test trigger
  pinMode(A1, OUTPUT); // Enable A0 as input to check test trigger
  digitalWrite(A1, HIGH); // Set A1 to HIGH to pull A0 HIGH
  testMode = (digitalRead(A0) == HIGH); // Activate test mode if A0 is HIGH at startup

  Serial.begin(BAUD_RATE); // Modbus RTU side
  hc12.begin(BAUD_RATE); // HC12 communication

  // Debug output
  if (DEBUG || testMode) {
    digitalWrite(RS485_DE, HIGH); // Set to transmit mode
    Serial.println("ClayCast Modbus RTU Controller");
    Serial.println("HC-12 and Modbus RTU setup complete.");
    digitalWrite(RS485_DE, LOW); // Set back to receive mode
    delay(5);
  }

  if (testMode) {
    digitalWrite(RS485_DE, HIGH);
    Serial.println("Test mode activated. Sending test data every 500ms.");
    digitalWrite(RS485_DE, LOW);
  }

  setHC12Channel(hc12Channel); // Set channel before any data is sent
}

void loop() {

  if (testMode) {
    uint32_t now = millis();
    if (now - lastTestSendTime >= 500) {
      lastTestSendTime = now;

      // Create test message
      char message[50];
      sprintf(message, "Testing in progress: %u", testCounter++);

      // Wrap and send
      uint16_t wrappedLen = wrapModbusRTU((uint8_t *)message, strlen(message), packetBuffer);
      if (wrappedLen > 0) {
        hc12.write(packetBuffer, wrappedLen);
        hc12.flush();
      }
    }
    return; // Skip normal loop logic in test mode
  }

  // Receive and buffer data from Serial
  while (Serial.available()) {
    uint8_t byteIn = Serial.read();
    serial_lastByteTime = millis();

    if (!serial_receiving) {
      serial_receiving = true;
      serial_recvIndex = 0;
      serial_buffer[serial_recvIndex++] = byteIn;
    } else if (serial_receiving) {
      if (serial_recvIndex < sizeof(serial_buffer)) {
        serial_buffer[serial_recvIndex++] = byteIn;
      } else {
        serial_receiving = false; // Overflow
      }
    }
  }

  // Receive and buffer data from HC12
  while (hc12.available()) {
    uint8_t byteIn = hc12.read();
    hc12_lastByteTime = millis();

    if (!hc12_receiving && byteIn == START_BYTE) {
      hc12_receiving = true;
      hc12_recvIndex = 0;
      hc12_buffer[hc12_recvIndex++] = byteIn;
    } else if (hc12_receiving) {
      if (hc12_recvIndex < sizeof(hc12_buffer)) {
        hc12_buffer[hc12_recvIndex++] = byteIn;
        if (byteIn == END_BYTE) break; // Possible end of packet
      } else {
        hc12_receiving = false; // Overflow
      }
    }
  }

  // Timeout: process if no new byte on Serial
  if (serial_receiving && (millis() - serial_lastByteTime > serial_frameTime)) {
    serial_receiving = false;
    if (serial_recvIndex < 6) return; // Minimum packet size

    uint16_t wrappedLen = wrapModbusRTU(serial_buffer, serial_recvIndex, packetBuffer);
    if (wrappedLen > 0) {
      hc12.write(packetBuffer, wrappedLen);
      hc12.flush(); // Ensure all data is sent
    }
  }

  // Timeout: process if no new byte on HC12
  if (hc12_receiving && (millis() - hc12_lastByteTime > hc12_frameTime)) {
    hc12_receiving = false;

    uint16_t unwrappedLen = 0;
    if (unwrapModbusRTU(hc12_buffer, hc12_recvIndex, packetBuffer, & unwrappedLen)) {
      digitalWrite(RS485_DE, HIGH); // Set to transmit mode
      delay(5); // Allow time for RS485 to switch
      Serial.write(packetBuffer, unwrappedLen);
      Serial.flush();
      delay(5); // Allow time for RS485 to switch back
      digitalWrite(RS485_DE, LOW); // Set back to receive mode
    }
  }
}