## The core tasks of the controller

### 📡 Serial Communication Setup

- Uses **hardware `Serial`** for UART communication with the Modbus RTU master
- Uses **`SoftwareSerial`** or a secondary UART for the HC12 module

### 📦 Modbus RTU Data Encapsulation

- Reads Modbus RTU requests from UART
- Wraps requests in a custom protocol header for wireless transmission via HC12

### 🚀 HC12 Transmission

- Sends encapsulated Modbus RTU requests over HC12
- Receives wrapped responses wirelessly from the client

### 📥 Response Processing

- Validates incoming wrapped responses
- Unwraps the Modbus RTU data
- Forwards it back to the Modbus master via hardware `Serial`

### 🧪 Test Mode (Enabled via A0)

If **pin A0 is HIGH** during power-up or reset, the controller enters **testing mode**:

- It disables Modbus forwarding
- Instead, it periodically sends test messages over HC12 in the following format:

`Testing in progress: {n}`

- These messages are wrapped in the standard packet format below and sent every ~500ms
- `n` is an incrementing counter used to test communication reliability and packet loss

This mode is useful for verifying wireless reception, packet framing, and performance of the client-side system.

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

## 🔧 Notes

- Make sure the HC12 modules are on the same channel (e.g. `CH050`)
- The serial and HC12 baudrates may differ, but it is recommended to choose equal and low rates. However, it is important to match the timing of the packet frames.
