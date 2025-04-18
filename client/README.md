## The Core Tasks of the Client

### 📡 Setup Serial Communication

- Uses `SoftwareSerial` for UART communication with the HC12 wireless module  
- Uses hardware `Serial` for debug output

### 📥 Process Modbus RTU Requests

- Waits for incoming encapsulated Modbus RTU requests over HC12  
- Unwraps received packets and validates structure and checksum

### ⚙️ Handle Modbus Logic

- Supports:
  - Read Holding Registers (`0x03`)
  - Write Single Register (`0x06`)
- Maintains internal holding registers for:
  - Device ID
  - Shoot trigger
  - Success flag
- Triggers actions based on incoming Modbus commands

### 📤 Transmit Modbus RTU Responses

- Wraps Modbus RTU responses with a protocol header  
- Sends the encapsulated data back via HC12

---

## 📦 Data Encapsulation Format

All HC12-transmitted messages use a consistent wrapper protocol:

```
+--------------------+---------+----------------------+
| Field              | Size    | Description          |
+--------------------+---------+----------------------+
| Start byte (0xAA)  | 1 Byte  | Packet start byte    |
| Data size          | 2 Bytes | Data size            |
| Data               | X Bytes | Modbus RTU package   |
| Checksum           | 2 Bytes | Size + Data checksum |
| End byte (0x55)    | 1 Byte  | Packet end byte      |
+--------------------+---------+----------------------+
```

- The `Data` section contains a complete Modbus RTU request or response
- Destination is determined by the Modbus RTU address field inside the data

---

## 🔧 Notes

- Make sure the HC12 modules are on the same channel (e.g. `CH050`)  
- The client listens for shoot commands and activates GPIO pins accordingly
