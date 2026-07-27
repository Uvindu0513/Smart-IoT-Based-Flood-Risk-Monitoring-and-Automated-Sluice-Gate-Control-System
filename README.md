# 🌊 Smart IoT-Based Flood Risk Monitoring and Automated Sluice Gate Control System

## 📌 Overview

The **Smart IoT-Based Flood Risk Monitoring and Automated Sluice Gate Control System** is an ESP32-based IoT solution designed for real-time flood monitoring and intelligent water management in reservoirs, dams, lakes, and rivers.

The system continuously monitors water level changes and environmental conditions, evaluates flood risk, predicts possible overflow conditions, and automatically controls sluice gates to regulate water discharge and minimize flood risks.

---

## ✨ Features

- 🌊 **Real-time Water Level Monitoring**
  - Measures reservoir water level using distance-based sensing technology.

- 🚪 **Automatic Sluice Gate Control**
  - Adjusts gate position according to detected flood risk levels.

- 📈 **Water Level Trend Analysis**
  - Calculates the rate of water level increase for early flood prediction.

- ⏱️ **Overflow Time Prediction**
  - Estimates the remaining time before reaching critical water levels.

- ⏱️ **Rain Fall Intensity**
  - A Tipping Bucket Guage system is used with some calculations.

- 🌧️ **Rainfall Monitoring**
  - Detects rain start and stop times.
  - Measures rainfall intensity using a tipping bucket rain gauge.

- 🌡️ **Environmental Monitoring**
  - Temperature measurement
  - Humidity monitoring
  - Atmospheric pressure monitoring

- 📱 **IoT Remote Monitoring**
  - Real-time sensor data visualization through the Blynk IoT platform.

- 📩 **GSM Emergency Alerts**
  - Sends SMS notifications to registered users during critical flood conditions.

---

## ⚙️ Working Principle

The system collects real-time data from multiple sensors and processes the information using an **ESP32 microcontroller**.

The water level percentage and rate of increase are analysed to determine the current flood risk status. According to the risk level, the controller automatically operates the sluice gate to manage water flow.

The system also analyses rainfall conditions and environmental parameters to improve flood assessment accuracy.

All collected data, system status, and predictions are uploaded to the **Blynk IoT dashboard** for remote monitoring, while emergency warnings are transmitted through GSM communication.

---

## 🛠️ Technologies Used

### Hardware
- ESP32 Microcontroller
- Water Level Sensor
- Rainfall Sensor
- Tipping Bucket Rain Gauge
- Temperature & Humidity Sensor
- Atmospheric Pressure Sensor
- GSM Module
- Motorized Sluice Gate Mechanism

### Software & Platforms
- Arduino Framework
- Blynk IoT Platform
- Embedded C/C++
- IoT Communication
- Automated Control Systems

---

## 🌍 Applications

- Smart Reservoir Monitoring
- Flood Early Warning Systems
- Dam Water Management
- River Level Monitoring
- Automated Water Resource Control
- Disaster Prevention Systems

---

## 🚀 Future Improvements

- AI/ML-based flood prediction
- Weather API integration
- Solar-powered autonomous operation
- Cloud database and analytics dashboard
- LoRa/MQTT-based multi-station monitoring
- Remote manual gate control through mobile application

---

## 📷 Project Images
<img width="4160" height="3120" alt="IMG-20260724-WA0010" src="https://github.com/user-attachments/assets/6e465350-fe06-4b0d-8025-bc9919f362fc" />
<img width="3024" height="4032" alt="IMG-20260723-WA0036(1)" src="https://github.com/user-attachments/assets/f0b5fa1b-9fdf-4fb6-9ab8-338554215fc4" />
<img width="3024" height="4032" alt="IMG-20260724-WA0090" src="https://github.com/user-attachments/assets/ad39ee0d-f9c4-4b7d-92bc-ef5953778fbd" />
<img width="3024" height="4032" alt="IMG-20260724-WA0089" src="https://github.com/user-attachments/assets/c699e40c-54be-4320-bc28-d177633b5590" />
<img width="3024" height="4032" alt="IMG-20260724-WA0079" src="https://github.com/user-attachments/assets/2266867c-d43f-4827-a5f4-99d83c4b3fb4" />
<img width="3024" height="4032" alt="IMG-20260724-WA0074" src="https://github.com/user-attachments/assets/96d525e8-0019-43f3-a4aa-f9ad39c5ab70" />
<img width="3024" height="4032" alt="IMG-20260724-WA0068" src="https://github.com/user-attachments/assets/83302bdd-8e2f-43f9-bb92-dcd45ac5caeb" />
<img width="4160" height="3120" alt="IMG-20260724-WA0013" src="https://github.com/user-attachments/assets/a3d329ee-c896-4343-9e30-73b1fd727a8d" />


## 🔑 Keywords

`IoT` `ESP32` `Flood Monitoring` `Sluice Gate Control` `Embedded Systems` `GSM Alerts` `Blynk` `Water Level Prediction` `Smart Water Management`
