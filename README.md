# ⚙️ Embedded Systems & IoT Portfolio: Industrial Control and Sensing

## 💡 Overview
This repository serves as a portfolio summary of projects developed using microcontrollers (Arduino, ESP32) for industrial automation, process control, and data acquisition. These examples demonstrate expertise in hardware-software integration, PID loop control, and sensor/actuator management.

### **Core Expertise**
* **PID Control:** Implementation and tuning for precise thermal, motion, or level control.
* **Industrial Sensing:** Integration of specialized sensors (NTC, load cells, optical).
* **Actuator Control:** Driving SSRs, relays, stepper motors, and other industrial outputs.
* **Microcontroller Platforms:** Arduino and ESP32 programming in C/C++.

---

## 🔬 Project 1: **PID Temperature Control Test Bench (NTC + SSR)**

This is a modular test code developed during the R&D phase of a commercial chocolate tempering machine to validate PID loop stability and SSR control logic. This code is a direct example of applied process control.

| Feature | Implementation Detail |
| :--- | :--- |
| **Control Algorithm** | **PID Control** (Proportional-Integral-Derivative) using the PID_v1 library. |
| **Setpoint Management** | [cite_start]Dynamic adjustment of target temperature via **Up/Down physical buttons**[cite: 8, 9, 10, 11]. |
| **Sensing** | [cite_start]**NTC Thermistor** connected to ADC [cite: 1, 12, 13][cite_start], with **Exponential Smoothing** for stable input[cite: 2, 14]. |
| **Actuator** | [cite_start]**SSR (Solid State Relay)** control for heating elements[cite: 1, 15]. |
| **SSR Logic** | [cite_start]Custom logic for full ON/OFF control when far from setpoint, transitioning to **Time-Proportional Control** (PWM simulation) using a **3000ms cycle time** when near the setpoint[cite: 5, 15, 19, 21]. |
| **Interface** | [cite_start]TM1637 **7-Segment Display** for real-time temperature and setpoint display[cite: 1, 24, 25]. |

> [cite_start]➡️ **Access the Code:** The full, documented test code is available in this repository as `NTC_SSR_Display_Buton_Test_parametrik.ino`[cite: 1].

---

## 🛠️ Other Embedded Systems Projects (Summaries)

* **[Proje Adı 2]:** Brief description of the project, focusing on the problem solved (e.g., *Multi-channel data logging system for pressure sensors using ESP32 and MQTT for cloud transmission*).
* **[Proje Adı 3]:** Brief description (e.g., *Stepper motor control system implemented in C for a precision dosing machine with limit switch integration*).
* **[Proje Adı 4]:** Brief description (e.g., *Remote power management unit using Arduino and custom relay shields controlled via a simple serial interface*).

---

### ⚠️ Note on Source Code

Source code for the commercial projects summarized here, including the final Chocolate Tempering Machine firmware, is **not available** due to proprietary reasons and client agreements. The provided test bench code is for demonstration and educational purposes only.

https://www.linkedin.com/in/misolmaz/
