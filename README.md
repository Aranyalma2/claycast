<h1 align="center">ClayCast</h1>
<p align="center">
<img src="./assets/banner.png">
</p>
<p align="center">
<img alt="Github top language" src="https://img.shields.io/github/languages/top/Aranyalma2/claycast?color=8f3d3d">
<img alt="Repository size" src="https://img.shields.io/github/repo-size/Aranyalma2/claycast?color=532BEAF">
<img alt="License" src="https://img.shields.io/github/license/Aranyalma2/claycast?color=56BEB8">
</p>

**ClayCast** is a wireless Modbus RTU communication bridge designed for clay pigeon shooting ranges. It allows remote control of clay thrower machines via Arduino-based controllers and clients using HC12 radio modules.

## 🎯 Features

- Two-way wireless communication using HC12 (433MHz)
- Full Modbus RTU packet encapsulation (avoid packet fragmantation)
- Up to **255 clients** (targets) per controller (prefer way lower in practise)
- Lightweight, low-latency, serial-based protocol
- Built using **Atmega328p**, **Arduino**, and **HC12**

## 🧩 System Architecture

ClayCast operates with two primary components:

### 🔸 Controller
- Receives Modbus RTU packets via UART
- Encapsulates and sends packets wirelessly to clients
- Receives wireless responses from clients
- Decapsulates packets and forwards responses via UART

### 🔹 Client
- Identified by unique Modbus slave address
- Implements a minimal Modbus slave (e.g. trigger + status registers)
- Receives and responds to Modbus requests
- Never initiates communication (response-only device)

## 📡 Communication Flow

1. Controller receives a Modbus RTU request from a master device via UART.
2. The packet is encapsulated and transmitted over HC12.
3. The appropriate client (based on Modbus address) receives and processes the request.
4. The client sends the response back to the controller.
5. The controller decapsulates and forwards the response to the original Modbus master.

## 🛠️ Hardware Requirements

- **Controller**:
  - Arduino (Atmega328p)
  - HC12 radio module
  - UART connection to Modbus master over RS485

- **Client**:
  - Arduino (Atmega328p)
  - HC12 radio module
  - Clay thrower trigger interface (Hardware, realy, microswitch, etc.)

## 📦 Modbus Support

Clients implement:
- **Trigger register** – Fire the clay thrower
- **Status registers** – Read current state or diagnostics

## 🚧 Status

🛠️ Currently under development – initial hardware testing and firmware structure in progress.

---

## 📜 License

This project is under MIT License. For more details, see the [LICENSE](LICENSE.md) file.

---

## 🤝 Contributions

Made with :heart: by <a href="https://github.com/Aranyalma2" target="_blank">Aranyalma2</a>

This is a self maintained project, but feel free to use and modify it on your own. I listen to mistakes and advice.