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

uint8_t hc12Buffer[MAX_DATA_SIZE + 10];    // buffer for HC12 to serial

// Define pins for shoot and success signals
#define SHOOT_PIN 5
#define SUCCESS_PIN 6

// Modbus constants
#define MODBUS_ADDRESS 1 // Slave address
#define MODBUS_FUNCTION_READ_HOLDING_REGISTERS 0x03
#define MODBUS_FUNCTION_WRITE_SINGLE_REGISTER  0x06
#define MODBUS_EXCEPTION_ILLEGAL_FUNCTION      0x01
#define MODBUS_EXCEPTION_ILLEGAL_DATA_ADDRESS  0x02
#define MODBUS_EXCEPTION_ILLEGAL_DATA_VALUE    0x03

#define RS485_DE 2 // RS485 DE pin

// Holding registers array (3 registers)
enum RegisterIndex {
  DEVICE_ID = 0,
  SHOOT = 1,
  SUCCESS = 2
};
uint16_t holdingRegisters[3] = {MODBUS_ADDRESS, 0, 0}; // Initialize registers

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

  hc12.begin(9600);       // HC12 communication
  setHC12Channel(hc12Channel); // Set channel before any data is sent

  // Debug output
  if (DEBUG) {
    Serial.begin(9600); // Modbus RTU side
    Serial.println("HC-12 and Modbus RTU setup complete.");
  }
  
}

void loop() {
  static uint8_t recvBuffer[MAX_DATA_SIZE + 10];
  static uint16_t recvIndex = 0;
  static bool receiving = false;
  static unsigned long lastByteTime = 0;

  // 1. Receive and buffer data from HC12
  while (hc12.available()) {
    uint8_t byteIn = hc12.read();
    lastByteTime = millis();

    if (!receiving && byteIn == START_BYTE) {
      receiving = true;
      recvIndex = 0;
      recvBuffer[recvIndex++] = byteIn;
    } else if (receiving) {
      if (recvIndex < sizeof(recvBuffer)) {
        recvBuffer[recvIndex++] = byteIn;
        if (byteIn == END_BYTE) break; // Possible end of packet
      } else {
        receiving = false; // Overflow
      }
    }
  }

  // 2. Timeout: process if no new byte for 10ms
  if (receiving && (millis() - lastByteTime > 10)) {
    receiving = false;

    uint8_t modbusData[MAX_DATA_SIZE];
    uint16_t modbusSize = 0;

    if (unwrapModbusRTU(recvBuffer, recvIndex, modbusData, &modbusSize)) {
#if DEBUG
      Serial.print("Received valid Modbus packet (size ");
      Serial.print(modbusSize);
      Serial.println(")");
#endif

      // 3. Process Modbus request
      uint8_t response[MAX_DATA_SIZE];
      int responseSize = processModbusRequest(modbusData, modbusSize, response);

      // 4. Wrap and send the response if valid
      if (responseSize > 0) {
        uint16_t wrappedSize = wrapModbusRTU(response, responseSize, hc12Buffer);
        hc12.write(hc12Buffer, wrappedSize);

#if DEBUG
        Serial.print("Sent response (size ");
        Serial.print(wrappedSize);
        Serial.println(")");
#endif
      }

    } else {
#if DEBUG
      Serial.println("Invalid packet received.");
#endif
    }

    recvIndex = 0; // Ready for next
  }

  // 5. Modbus-side logic (as before)
  if (holdingRegisters[SHOOT] == 1) {
    holdingRegisters[SUCCESS] = 0;
    triggerShoot();
    holdingRegisters[SHOOT] = 0;
  }

  if (holdingRegisters[SHOOT] == 0) {
    holdingRegisters[SUCCESS] = digitalRead(SUCCESS_PIN);
  }
}


// Process a Modbus RTU request and generate a response
int processModbusRequest(uint8_t *request, int requestLength, uint8_t *response) {
  // Validate CRC
  if (!validateCRC(request, requestLength)) {
    return 0; // Invalid CRC, ignore request
  }

  // Extract address, function code, and data
  uint8_t address = request[0];
  uint8_t functionCode = request[1];

  // Check if the request is for this slave
  if (address != MODBUS_ADDRESS) {
    return 0; // Ignore requests not addressed to this slave
  }

  switch (functionCode) {
    case MODBUS_FUNCTION_READ_HOLDING_REGISTERS:
      return handleReadHoldingRegisters(request, requestLength, response);
    case MODBUS_FUNCTION_WRITE_SINGLE_REGISTER:
      return handleWriteSingleRegister(request, requestLength, response);
    default:
      return generateExceptionResponse(request, response, MODBUS_EXCEPTION_ILLEGAL_FUNCTION);
  }
}

// Handle Read Holding Registers (Function Code 0x03)
int handleReadHoldingRegisters(uint8_t *request, int requestLength, uint8_t *response) {
  uint16_t startAddress = (request[2] << 8) | request[3];
  uint16_t quantity = (request[4] << 8) | request[5];

  if (startAddress + quantity > sizeof(holdingRegisters) / sizeof(holdingRegisters[0])) {
    return generateExceptionResponse(request, response, MODBUS_EXCEPTION_ILLEGAL_DATA_ADDRESS);
  }

  response[0] = MODBUS_ADDRESS;
  response[1] = MODBUS_FUNCTION_READ_HOLDING_REGISTERS;
  response[2] = quantity * 2;

  int index = 3;
  for (int i = 0; i < quantity; i++) {
    response[index++] = holdingRegisters[startAddress + i] >> 8;
    response[index++] = holdingRegisters[startAddress + i] & 0xFF;
  }

  appendCRC(response, index);
  return index + 2;
}

// Handle Write Single Register (Function Code 0x06)
int handleWriteSingleRegister(uint8_t *request, int requestLength, uint8_t *response) {
  uint16_t address = (request[2] << 8) | request[3];
  uint16_t value = (request[4] << 8) | request[5];

  if (address >= sizeof(holdingRegisters) / sizeof(holdingRegisters[0])) {
    return generateExceptionResponse(request, response, MODBUS_EXCEPTION_ILLEGAL_DATA_ADDRESS);
  }

  if (address == DEVICE_ID || address == SUCCESS) {
    return generateExceptionResponse(request, response, MODBUS_EXCEPTION_ILLEGAL_DATA_VALUE);
  }

  holdingRegisters[address] = value;

  memcpy(response, request, requestLength);
  appendCRC(response, requestLength);
  return requestLength + 2;
}

// Generate an exception response
int generateExceptionResponse(uint8_t *request, uint8_t *response, uint8_t exceptionCode) {
  response[0] = MODBUS_ADDRESS;
  response[1] = request[1] | 0x80; // Add error flag
  response[2] = exceptionCode;
  appendCRC(response, 3);
  return 5;
}

// Validate CRC
bool validateCRC(uint8_t *buffer, int length) {
  uint16_t calculatedCRC = calculateCRC(buffer, length - 2);
  uint16_t receivedCRC = (buffer[length - 1] << 8) | buffer[length - 2];
  return calculatedCRC == receivedCRC;
}

// Append CRC to a Modbus packet
void appendCRC(uint8_t *buffer, int length) {
  uint16_t crc = calculateCRC(buffer, length);
  buffer[length] = crc & 0xFF;
  buffer[length + 1] = crc >> 8;
}

// Calculate CRC-16
uint16_t calculateCRC(uint8_t *buffer, int length) {
  uint16_t crc = 0xFFFF;
  for (int i = 0; i < length; i++) {
    crc ^= buffer[i];
    for (int j = 0; j < 8; j++) {
      if (crc & 1) {
        crc = (crc >> 1) ^ 0xA001;
      } else {
        crc >>= 1;
      }
    }
  }
  return crc;
}

// Trigger the shoot mechanism by setting the SHOOT_PIN
void triggerShoot() {
  digitalWrite(SHOOT_PIN, HIGH);
  delay(100); // Simulate short trigger pulse
  digitalWrite(SHOOT_PIN, LOW);
}