# 🚨 Smart Gas Leak Detection & Auto-Ventilation System

## Overview
LPG gas leakage is a major safety hazard in homes and commercial kitchens. This project is a complete automated safety system built around an Arduino UNO that continuously monitors air quality using dual MQ-2 gas sensors. 

When a leak is detected, the system does not just sound an alarm—it actively mitigates the danger by switching on an exhaust fan, rotating a servo motor to mechanically close the gas valve, and sending an emergency SMS to the user. To ensure reliability, the system implements a calibration phase, sensor value averaging, and a confirmation counter to eliminate false positives.

## 🎥 Video Demonstration
**Watch the project in action here:** [Project Presentation & Demo](https://youtu.be/1BztYzng-nU)

## ✨ Key Features
* **Dual Sensor Redundancy:** Uses two MQ-2 gas sensors for a wider coverage area and more reliable detection.
* **Automated Mitigation:** Immediately triggers a relay-controlled DC exhaust fan to ventilate the area and uses a servo motor to shut off the gas supply line.
* **Remote SMS Alerts:** Integrates a SIM800L GSM module to send real-time text message alerts to a registered phone number.
* **Audio-Visual Alarms:** Features an active buzzer and Red/Green LED status indicators.
* **Software Filtering:** Implements baseline clean-air calibration, 10-sample averaging, and a leak confirmation counter to prevent false alarms from temporary spikes.
* **Latching Alarm:** Once triggered, the safety mechanisms (valve closed, fan on, red LED) remain active until the system is manually reset by the user.

## 🛠️ Hardware Components
* **Microcontroller:** Arduino UNO R3
* **Sensors:** 2x MQ-2 Combustible Gas Sensors
* **Communication:** SIM800L GSM Module (2G)
* **Actuators:** SG90 Servo Motor, 5V DC Fan, 1-Channel Relay Module (Active-High/Low configurable)
* **Alerts:** Active Buzzer, Red & Green LEDs
* **Power Regulation:** LM2596 Buck Converter, 1000 µF Capacitor (for SIM800L stability), Voltage Divider (10 kΩ + 15 kΩ for GSM RX pin)
* **Power Supply:** 2x 5V Power Banks (Isolated power rails with a common ground to prevent voltage drops during actuator switching)

## ⚡ Circuit & Power Architecture
To ensure system stability, the power architecture is split into two rails with a **common ground**:
1. **Power Bank 1 (Logic & Sensors):** Powers the Arduino, MQ-2 sensors, LEDs, and Buzzer.
2. **Power Bank 2 (Heavy Loads & Comm):** Powers the Relay (Fan), Servo Motor, and the LM2596 Buck Converter. The LM2596 steps the 5V down to ~4.0V to safely power the SIM800L, stabilized by a 1000 µF capacitor to prevent network-registration resets.

## 🚀 How to Run / Setup

1. **Clone the repository:**
   `git clone https://github.com/Mayen-Shahriar-Kabir/Smart-Gas-Leak-Detector.git`

2. **Hardware Assembly:** 
   Wire the components according to the pin definitions in the source code. Ensure the SIM800L is powered correctly (~4.0V) and that the TX/RX lines use a logic level converter or voltage divider.

3. **Configure the Code:** 
   * Open `gasLeakageDetection.ino` in the Arduino IDE.
   * Update the `PHONE_NUMBER` variable with your emergency contact number (e.g., `+8801XXXXXXXXX`).
   * Adjust `VALVE_OPEN_ANGLE` and `VALVE_CLOSED_ANGLE` to match your physical servo mount.

4. **Upload:** 
   Connect your Arduino UNO to your computer and upload the code.

5. **Calibration Phase:** 
   Upon startup, the system will blink the Green LED and take ~5 seconds to warm up, followed by a calibration sequence in clean air. Do not introduce gas during this phase.
