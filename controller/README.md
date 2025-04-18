## The core tasks of the controller

### Setup Serial Communication:

* Use hardware Serial for UART communication with the Modbus RTU master.
* Use SoftwareSerial or another UART for HC12 communication.

### Encapsulate Modbus RTU Data:

* Read data from the Serial UART (Modbus RTU data).
* Encapsulate the Modbus data with an additional header or wrapper for wireless transmission.

### Transmit Data via HC12:

* Send the encapsulated data over the air.

### Receive and Process Data over HC12:

* Read incoming data from the HC12.
* Validate and process it (remove the wireless wrapper).
* Forward the unwrapped response to the Modbus master via Serial UART.

## Data encapsulation

All messages follow the same header format:
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
Destination determined by the content address field.