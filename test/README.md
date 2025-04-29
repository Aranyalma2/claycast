# HC-12 Test Client

This receives and prints test messages sent wirelessly via the HC-12 433MHz module. It's designed to work with a Modbus RTU controller in **test mode**, which periodically sends messages like `Testing in progress: {n}`.

## 📡 Overview

The client listens for specially framed data using the following packet structure:

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

It validates and unwraps each packet, and prints the message content to the serial monitor.

## 📈 Packet Loss Tracking

The client now tracks the message counters embedded in the test messages (e.g. `Testing in progress: 124`) to detect dropped packets.

- It maintains a history of the **last 20 counter values**.
- It calculates the **average packet loss percentage** by comparing gaps between successive counters.
- The estimated packet loss rate is printed in the serial monitor.

Example output:

`Received: Testing in progress: 5 Packet loss over last 20 msgs: ~25.0%`

### ⏱ Timeout Detection

- If **no message** is received for **more than 1000ms**, it's considered a **timeout**.
- The client prints a message and increments a **"timeout counter"**, helping to diagnose communication blackouts.

Example:

`Timeout: No data received for 1000ms (missed packet #3)`

## 📟 Serial Monitor Output

Open the Serial Monitor at **9600 baud** to view:
- Incoming test messages
- Packet loss statistics
- Timeout alerts

## 🔧 Notes

- Make sure the HC12 modules are on the same channel (e.g. `CH050`)
- On the transmitter/controller side, test mode is triggered by holding **A0 HIGH during startup**.