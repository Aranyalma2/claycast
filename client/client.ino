#include <SoftwareSerial.h>

// Define Modbus Slave Address (compile-time)
#define MODBUS_ADDRESS 1

// Define pins for HC-12
#define HC12_RX 11 // Arduino RX
#define HC12_TX 12 // Arduino TX
#define HC12_SET 13 // HC-12 SET pin

// Define pins for shoot and success signals
#define SHOOT_PIN 5
#define SUCCESS_PIN 6

// Modbus constants
#define BUFFER_SIZE 260 // Start (1) + Length (1) + Modbus RTU (255) + End (1)
#define MODBUS_FUNCTION_READ_HOLDING_REGISTERS 0x03
#define MODBUS_FUNCTION_WRITE_SINGLE_REGISTER  0x06
#define MODBUS_EXCEPTION_ILLEGAL_FUNCTION      0x01
#define MODBUS_EXCEPTION_ILLEGAL_DATA_ADDRESS  0x02
#define MODBUS_EXCEPTION_ILLEGAL_DATA_VALUE    0x03

// Holding registers array (3 registers)
enum RegisterIndex {
  DEVICE_ID = 0,
  SHOOT = 1,
  SUCCESS = 2
};
uint16_t holdingRegisters[3] = {MODBUS_ADDRESS, 0, 0}; // Initialize registers

// HC-12 communication
SoftwareSerial HC12(HC12_RX, HC12_TX);

// Encapsulation start and end markers
#define PACKET_START 0xAA
#define PACKET_END 0x55

void setup() {
  // Setup pins
  pinMode(SHOOT_PIN, OUTPUT);
  pinMode(SUCCESS_PIN, INPUT);
  pinMode(HC12_SET, OUTPUT);

  // Initialize HC-12
  HC12.begin(9600); // Default HC-12 baud rate
  configureHC12(); // Configure HC-12 for reliable communication

  // Debug output
  Serial.begin(9600); // Debugging on Serial Monitor
  Serial.println("Custom Modbus Client Initialized...");
}

void loop() {
  // Poll for incoming encapsulated data from HC-12
  if (HC12.available()) {
    uint8_t request[BUFFER_SIZE];
    int requestLength = receiveEncapsulatedPacket(request, BUFFER_SIZE);

    // If a valid packet is received, process it
    if (requestLength > 0) {
      uint8_t response[BUFFER_SIZE];
      int responseLength = processModbusRequest(request, requestLength, response);

      // If a valid response is generated, send it back encapsulated
      if (responseLength > 0) {
        sendEncapsulatedPacket(response, responseLength);
      }
    }
  }

  // Handle shoot signal if SHOOT register is set
  if (holdingRegisters[SHOOT] == 1) {
    triggerShoot();
    holdingRegisters[SHOOT] = 0; // Reset after triggering
  }

  // Update SUCCESS register from the SUCCESS_PIN state
  holdingRegisters[SUCCESS] = digitalRead(SUCCESS_PIN);
}

// Receive an encapsulated packet, decapsulate, and return the payload length
int receiveEncapsulatedPacket(uint8_t *buffer, int maxLength) {
  bool startDetected = false;
  int index = 0;

  while (HC12.available()) {
    uint8_t byte = HC12.read();

    // Detect start marker
    if (byte == PACKET_START && !startDetected) {
      startDetected = true;
      index = 0; // Reset buffer index
    } else if (byte == PACKET_END && startDetected) {
      // End marker detected, return the length of the payload
      return index;
    } else if (startDetected && index < maxLength) {
      // Store payload bytes
      buffer[index++] = byte;
    }
  }

  return 0; // No complete packet received
}

// Send a payload encapsulated with start and end markers
void sendEncapsulatedPacket(uint8_t *data, int length) {
  HC12.write(PACKET_START); // Start marker
  HC12.write(data, length); // Payload
  HC12.write(PACKET_END); // End marker
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

// Configure HC-12 for reliable communication
void configureHC12() {
  digitalWrite(HC12_SET, LOW); // Enter AT Command mode
  delay(100);

  HC12.println("AT+P8"); // Set maximum power (100mW)
  delay(100);
  HC12.println("AT+B9600"); // Set baud rate to 9600
  delay(100);
  HC12.println("AT+FU3"); // Set FU3 mode for long-distance communication
  delay(100);

  digitalWrite(HC12_SET, HIGH); // Exit AT Command mode
  delay(100);
}