#include <SoftwareSerial.h>

#define START_BYTE 0xAA
#define END_BYTE 0x55
#define MAX_DATA_SIZE 260

// HC12 module pins
const int hc12RxPin = 13;
const int hc12TxPin = 12;
const int hc12SetPin = 11;

const int hc12Channel = 50; // Channel number (1–100)

SoftwareSerial hc12(hc12TxPin, hc12RxPin); // RX, TX

uint8_t buffer[MAX_DATA_SIZE + 10];
bool receiving = false;
uint16_t recvIndex = 0;
uint32_t lastByteTime = 0;
uint32_t lastPacketTime = 0;
const uint8_t frameTimeout = 50; // ms

const uint16_t packetTimeout = 1000; // ms

#define HISTORY_SIZE 20
int16_t counterHistory[HISTORY_SIZE];
uint8_t historyIndex = 0;
bool historyFilled = false;

uint16_t timeoutCounter = 0;

// Unwrap received Modbus-like frame
bool unwrapModbusRTU(const uint8_t* packet, uint16_t packetSize, uint8_t* dataOut, uint16_t* dataSizeOut) {
  if (packetSize < 6 || packet[0] != START_BYTE || packet[packetSize - 1] != END_BYTE)
    return false;

  uint16_t size = (packet[1] << 8) | packet[2];
  if (size > MAX_DATA_SIZE || packetSize != size + 6)
    return false;

  uint16_t checksum = (size >> 8) + (size & 0xFF);
  for (uint16_t i = 0; i < size; i++) {
    dataOut[i] = packet[3 + i];
    checksum += dataOut[i];
  }

  uint16_t receivedChecksum = (packet[3 + size] << 8) | packet[4 + size];
  if (checksum != receivedChecksum)
    return false;

  *dataSizeOut = size;
  return true;
}

void setHC12Channel(uint8_t channel) {
  if (channel < 1 || channel > 100) return;

  char cmd[10];
  sprintf(cmd, "AT+C%03d", channel);
  Serial.print("Setting HC12 to channel: ");
  Serial.println(cmd);

  digitalWrite(hc12SetPin, LOW);
  delay(50);
  hc12.print(cmd);
  delay(100);

  while (hc12.available()) {
    Serial.print((char)hc12.read());
  }

  digitalWrite(hc12SetPin, HIGH);
  delay(50);
}

void trackPacketLoss(int16_t newCounter) {
  counterHistory[historyIndex++] = newCounter;
  if (historyIndex >= HISTORY_SIZE) {
    historyIndex = 0;
    historyFilled = true;
  }

  if (!historyFilled) return;

  int lostPackets = 0;
  for (uint8_t i = 1; i < HISTORY_SIZE; i++) {
    int16_t prev = counterHistory[(historyIndex + i - 1) % HISTORY_SIZE];
    int16_t curr = counterHistory[(historyIndex + i) % HISTORY_SIZE];
    if (curr > prev + 1) {
      lostPackets += (curr - prev - 1);
    }
  }

  float lossRate = (lostPackets * 100.0) / (HISTORY_SIZE - 1);
  Serial.print("Packet loss over last ");
  Serial.print(HISTORY_SIZE);
  Serial.print(" msgs: ~");
  Serial.print(lossRate, 1);
  Serial.println("%");
}

void setup() {
  pinMode(hc12SetPin, OUTPUT);
  digitalWrite(hc12SetPin, HIGH);

  Serial.begin(9600);
  hc12.begin(9600);

  setHC12Channel(hc12Channel);

  Serial.println("HC12 Test Client Ready.");
  lastPacketTime = millis();
}

void loop() {
  while (hc12.available()) {
    uint8_t byteIn = hc12.read();
    lastByteTime = millis();

    if (!receiving && byteIn == START_BYTE) {
      receiving = true;
      recvIndex = 0;
      buffer[recvIndex++] = byteIn;
    } else if (receiving) {
      if (recvIndex < sizeof(buffer)) {
        buffer[recvIndex++] = byteIn;
        if (byteIn == END_BYTE) break;
      } else {
        receiving = false;
      }
    }
  }

  if (receiving && (millis() - lastByteTime > frameTimeout)) {
    receiving = false;

    uint8_t dataOut[MAX_DATA_SIZE];
    uint16_t dataLen = 0;
    if (unwrapModbusRTU(buffer, recvIndex, dataOut, &dataLen)) {
      lastPacketTime = millis(); // update last valid packet time
      dataOut[dataLen] = '\0';
      Serial.print("Received: ");
      Serial.println((char*)dataOut);

      char* ptr = strchr((char*)dataOut, ':');
      if (ptr) {
        int counter = atoi(ptr + 1);
        trackPacketLoss(counter);
      }

    } else {
      Serial.println("Invalid packet received");
    }
  }

  // Check for timeout
  if (millis() - lastPacketTime > packetTimeout) {
    lastPacketTime = millis(); // reset so it doesn't flood
    timeoutCounter++;
    Serial.print("Timeout: No data for 1000ms (missed ");
    Serial.print(timeoutCounter);
    Serial.println(" packets)");
  }
}
