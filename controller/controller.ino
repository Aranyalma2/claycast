#include <SoftwareSerial.h>

// Define pins for HC-12
#define HC12_RX 13 // Arduino RX
#define HC12_TX 12 // Arduino TX
#define HC12_SET 11 // HC-12 SET pin

#define RADIO_CHANNEL "AT+C001" // HC-12 channel

// Define UART for HC12
#define RS485_DE 2 // RS485 DE pin
SoftwareSerial HC12(HC12_RX, HC12_TX);

// Define buffer sizes
#define BUFFER_SIZE 260 // Start (1) + Length (1) + Modbus RTU (255) + End (1)
uint8_t modbusRequest[BUFFER_SIZE];
uint8_t modbusResponse[BUFFER_SIZE];

// Serial baud rates
#define MODBUS_BAUD 9600
#define HC12_BAUD 9600

// Encapsulation start and end markers
#define PACKET_START 0xAA
#define PACKET_END 0x55

void setup() {
  // Initialize UART Serial for Modbus RTU
  pinMode(RS485_DE, OUTPUT);
  digitalWrite(RS485_DE, LOW); // Set RS485 to receive mode
  Serial.begin(MODBUS_BAUD);

  // Initialize SoftwareSerial for HC12
  HC12.begin(HC12_BAUD);
  configureHC12(); // Configure HC-12 for reliable communication
}

void loop() {
  // Check if data is available from Modbus RTU Master
  if (Serial.available()) {
    // Read Modbus RTU data
    HC12.write("aaa");

    short requestLength = readModbusRequest(modbusRequest, BUFFER_SIZE);


    // Encapsulate and send over HC12
    sendOverHC12(modbusRequest, requestLength);
  }

  // Check if data is received from HC12
  if (HC12.available()) {
    // Read response from HC12
    short responseLength = receiveFromHC12(modbusResponse, BUFFER_SIZE);

    // Forward response to Modbus Master
    if (responseLength > 0) {
      digitalWrite(RS485_DE, HIGH); // Set RS485 to transmit mode
      delay(1); // Allow time for DE to take effect
      Serial.write(modbusResponse, responseLength);
      digitalWrite(RS485_DE, LOW); // Set RS485 back to receive mode
    }
  }
}

// Function to read Modbus RTU request
short readModbusRequest(uint8_t *buffer, short bufferSize) {
  short index = 0;
  while (Serial.available() && index < bufferSize) {
    buffer[index++] = Serial.read();
    delay(2); // Small delay for UART data
  }
  return index;
}

// Function to encapsulate and send data over HC12
void sendOverHC12(uint8_t *data, short length) {
  HC12.write(PACKET_START);       // Start byte for encapsulation
  HC12.write(length);     // Length of data
  HC12.write(data, length); // Actual Modbus RTU data
  HC12.write(PACKET_END);       // End byte for encapsulation
}

// Function to receive and process data from HC12
short receiveFromHC12(uint8_t *buffer, short bufferSize) {
  short index = 0;
  bool startDetected = false;

  while (HC12.available() && index < bufferSize) {
    uint8_t byte = HC12.read();

    // Detect start byte
    if (byte == 0xAA && !startDetected) {
      startDetected = true;
      index = 0; // Reset buffer index
    } else if (byte == 0x55 && startDetected) {
      // End byte detected
      break;
    } else if (startDetected) {
      buffer[index++] = byte;
    }
  }
  return index;
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
  HC12.println(RADIO_CHANNEL); // Set channel
  delay(100);

  digitalWrite(HC12_SET, HIGH); // Exit AT Command mode
  delay(100);
}