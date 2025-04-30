<p align="center">
<img src="./assets/banner.png">
</p>
<p align="center">
<img alt="Github top language" src="https://img.shields.io/github/languages/top/Aranyalma2/claycast?color=8f3d3d">
<img alt="Repository size" src="https://img.shields.io/github/repo-size/Aranyalma2/claycast?color=532BEAF">
<img alt="License" src="https://img.shields.io/github/license/Aranyalma2/claycast?color=56BEB8">
</p>

**ClayCast** is a modular wireless control system for clay pigeon shooting ranges.  
It enables field centralized game management via a touchscreen **HMI device**, which controls up to 10 clay thrower machines through a wireless **Modbus RTU** network.  
Communication is handled by **controller node** that act as wireless relays to **client devices** attached to each clay thrower.

## 🎯 Features

- **Modular, multi-device architecture**: HMI (master), Controller (relay), and Client (thrower control)
- **Game management interface** via 7-inch touchscreen HMI
  - Enable/Disable machines (up to 10)
  - Pre-configure next game (max throw, double, triple)
  - Monitor clay disk levels with low-ammo alarms
  - Game screen
  - Summary screen
- **Wireless Modbus RTU bridging** over HC12 (~433MHz)
  - Up to **255 clients** per controller (practically limited to fewer)
  - Reliable full-frame encapsulation (no packet fragmentation)
- **Low-latency protocol** using UART and lightweight firmware
- Built with **Arduino framework**, **Atmega328p**, **ESP32**, and **HC12 modules**
- **Open and customizable** — easy to extend or adapt for other range configurations

## 🧩 Core Components

**ClayCast** consists of three main components:

### 💠 HMI Device
- Acts as the **Modbus RTU Master**
- Human-Machine Interface with touchscreen
- Controls up to **10 clay thrower machines**
- Manages and runs the **preconfigured game**
- Monitors **ammo levels** and triggers alarms if low
- Central command and monitoring unit for the entire system  
[More about here](hmi/README.md)

### 🔸 Controller
- Acts as a wireless Modbus RTU bridge
- Receives Modbus requests from a master device via UART (RS485)
- Encapsulates and broadcasts packets wirelessly to clients
- Receives wireless responses, decapsulates, and forwards them to the master  
[More about here](controller/README.md)

### 🔹 Client
- Identified by a unique Modbus slave address
- Controls an individual clay thrower machine
- Listens for Modbus requests, executes commands (e.g., fire), and returns status
- Response-only device; never initiates communication  
[More about here](client/README.md)

## 📡 Communication Flow

1. The HMI device sends a Modbus RTU request to the Controller via UART.
2. The Controller encapsulates the packet and transmits it via HC12.
3. The designated Client receives, processes, and responds to the request.
4. The Controller receives the response, decapsulates it, and returns it to the HMI.

## 🛠️ Hardware Requirements

- **HMI Device**:
  - Kinco HMI [GL070E](https://en.kinco.cn/productdetail/gl070e90.html)

- **Controller**:
  - Arduino (Atmega328p)
  - HC12 radio module
  - RS485 interface to HMI (Modbus master)

- **Client**:
  - Arduino (Atmega328p)
  - HC12 radio module
  - Clay thrower trigger mechanism (relay, microswitch, etc.)

## 📦 Modbus Support

Clients implement:
- **Trigger register** – Fire the clay thrower
- **Status registers** – Report state, diagnostics, or ammo level

## 🚧 Status & Development Milestones

🛠️ Currently Under Development — Working Prototype Deployed in Test Field

- ✅ **Prototype Hardware Evaluation**  
  Initial testing and validation of temporary hardware components.

- ✅ **Firmware Architecture Design**  
  Core structure and logic for both controller and client firmware defined and implemented.

- ✅ **Interim Hardware Integration**  
  Temporary hardware assembled and tested for early firmware and communication debugging.

- ✅ **Wireless Communication Tester**  
  Radio signal testing module completed to assess transmission range and reliability.

- 🔧 **Final Hardware Assembly** *(In Progress)*  
  Development and deployment of production-ready, dedicated hardware.
  
- 🚀 **Human-Machine Interface (HMI)** *(In Testing)*  
  - Game customizability, fire control, data logging  
  - Ammo monitoring with alarms  
  - Controls up to 10 clay throwers

## 🔀 Alternative use

Create a Modbus RTU bridge over the air with 2 or more **Controllers**. However, radio medium usage is not controlled, so it is easy to cause interference. Incidentally it does not include any error detection and/or correction, leaving everything to the application layer protocol.
Not ready for any production usage.

---

## 📜 License

This project is licensed under the MIT License. See the [LICENSE](LICENSE.md) file for details.

---

## 🤝 Contributions

Made with ❤️ by <a href="https://github.com/Aranyalma2" target="_blank">Aranyalma2</a>

This is a self-maintained project. Feel free to use, fork, or modify it. Feedback and suggestions are always welcome!
