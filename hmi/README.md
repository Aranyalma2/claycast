## ClayCast HMI Device

The **HMI (Human-Machine Interface)** is the central touchscreen control unit of the **ClayCast** modular shooting range system. It serves as the **Modbus RTU master** and allows users to configure, start, monitor, and summarize clay shooting games.

### 📱 Overview

- **Device**: Kinco GL070E 7-inch touchscreen
- **Connection**: RS485 Modbus RTU to Controller
- **Role**: Game creation, fire control, diagnostics, and game summary

---

### 🧭 Features

- Full control over game parameters and device activation
- Real-time game monitoring interface
- Field test tools for individual machine verification
- End-of-game summary screen
- Sends Modbus RTU commands to Controller node

---

### 📺 Screens

#### 🎮 Game Screen
- Shows active machines
- Fire status, ammo level (left for game), and operational alerts
- Controls to:
  - Start, pause, or end game
  - Config screen jump

#### ⚙️ Config Screen
- Pre-game setup:
  - Select active machines (1–10)
  - Set max throws
  - Extra fire modes: Double, Triple throws
  - Set throw delay
- Stores settings and distribute ammo for the upcoming session

#### 📡 Field Test Screen
- Used for pre-game testing and hardware verification
- Send **test fire** commands to individual clients
- Displays response from each machine
- Helps ensure correct installation and operation

#### 🏁 End Screen
- Post-game overview:
  - Number of throws

---

### 🛠 Requirements

- **Hardware**: Kinco GL070E (or similiar architecture)
- **Software**: Kinco DTool (4.5.0.1) for programming
