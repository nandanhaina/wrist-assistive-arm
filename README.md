# Wrist-Mounted Assistive Robotic Arm with Motion Imitation Capability

A wearable, cost-effective assistive robotic hand designed to help people with limb impairments restore basic finger movements. This project leverages an embedded ESP32 control system to offer a dual-mode operation:
1. **Motion Imitation Mode:** Uses flex sensors on a healthy hand to wirelessly replicate finger movements on the robotic hand for demonstration and rehabilitation.
2. **Assistive Mode (EMG):** Uses muscle activity signals (Electromyography) to allow intuitive finger control based on human intent without requiring physical finger movement.

## ✨ Features
* **Dual-Mode Control:** Switch between flex sensor motion mimicry and EMG muscle control.
* **Wireless Operation:** Uses the low-latency ESP-NOW protocol for fast, wireless communication between the sensor transmitter and the robotic hand receiver.
* **Affordable & Accessible:** Built with readily available components like the ESP32, standard flex sensors, and RC-A056 EMG modules.
* **Smart Filtering:** Advanced signal processing, exponential moving averages (EMA), and dynamic baseline calibration to filter out noise and ensure smooth finger actuation.

---

## 🛠️ Hardware Requirements
* **Microcontrollers:** 2x ESP32 Development Boards
* **Actuators:** 5x Servo Motors (for individual finger actuation)
* **Sensors:**
  * 5x Flex Sensors (for Motion Imitation Mode)
  * 1x RC-A056 EMG Sensor Module with electrodes (for Assistive Mode)
* **Power Supply:** External 5V Adapter (for servos) and ±9V battery setup (for EMG)
* **Passive Components:** Resistors for flex sensor voltage dividers (100kΩ, 33kΩ, 18kΩ)

---

## 🔌 Wiring & Circuit Diagrams

### 1. Flex Sensor Transmitter Circuit
Each of the 5 flex sensors is set up in a voltage divider configuration connected to the ESP32 ADC pins.
* **Power:** Flex sensors connected to ESP32 `3.3V`.
* **Voltage Divider Resistors (to GND):**
  * Thumb, Pinky, Ring: `100 kΩ`
  * Middle Finger: `33 kΩ`
  * Index Finger: `18 kΩ`
* **Signal Pins:** Connected to ESP32 GPIO pins `34, 35, 36, 39, 32`.

*(Tip: Add a photo of your flex sensor circuit diagram here!)*

### 2. Servo Motor Receiver Circuit
Because 5 servo motors draw more current than the ESP32 can provide, an external 5V power supply is required.
* **Power (Red):** Connect to External 5V Power Supply.
* **Ground (Brown):** Connect to External Power GND **AND** ESP32 GND (Common Ground).
* **Signal (Orange):** Connect to ESP32 GPIO Pins `13, 12, 14, 27, 26`.

*(Tip: Add a photo of your servo circuit diagram here!)*

### 3. EMG Sensor Circuit
The RC-A056 EMG sensor requires a dual power supply (±9V) and outputs an analog signal.
* **+Vs:** Positive terminal of 9V battery
* **-Vs:** Negative terminal of 9V dual sensor setup
* **GND:** Common ground of battery and ESP32
* **SIG:** Connect to ESP32 GPIO `34`
* **Electrode Placement:**
  * Active (Red): Muscle belly
  * Secondary (Green): 3-4 cm along the same muscle line
  * Reference (Yellow): Bony region (e.g., wrist or elbow)

---

## 📂 Repository Structure

The code is divided into 5 Arduino sketches catering to different testing and operational phases:

* 📁 `Single_flex_sensor/`: Initial single-finger testing code with dynamic baseline logic.
* 📁 `Hand_mimic_2angles/`: 5-Finger ESP-NOW Transmitter (2 Angles: 0° / 180°).
* 📁 `Updated_hand_mimic/`: Advanced 5-Finger ESP-NOW Transmitter using Object-Oriented logic for 3 nuanced angles (0° / 90° / 180°).
* 📁 `emg_sensor_2_angles/`: EMG ESP-NOW Transmitter utilizing an envelope follower algorithm for FIST (180°) and RELAXED (0°) gesture detection.
* 📁 `servo_motor_receiver/`: The ESP-NOW Receiver code that drives the 5 servo motors.

---

## 🚀 Setup & Usage Instructions

### 1. Software Setup
1. Install the [Arduino IDE](https://www.arduino.cc/en/software).
2. Install the ESP32 Board package via the Boards Manager.
3. Install the **`ESP32Servo`** library by Kevin Harrington via the Library Manager.

### 2. Pairing the ESP32s (MAC Address)
For the ESP-NOW wireless communication to work, the transmitters must know the MAC address of the receiver.
1. Flash the `servo_motor_receiver.ino` code to your Receiver ESP32.
2. Open the Serial Monitor (115200 baud). It will print its MAC address (e.g., `F4:2D:C9:71:CF:94`).
3. Open any of the Transmitter codes and update the `receiverMacAddress` array with this MAC address.

### 3. Calibration
* **Flex Sensor Mode:** Upon booting the transmitter, the serial monitor will prompt you to hold your fingers completely straight for 3 seconds, followed by completely bent for 3 seconds. The ESP32 uses this to automatically calculate custom bending thresholds for your hand.
* **EMG Mode:** Keep your arm completely relaxed for 5 seconds when booting. The system will calibrate the resting muscle baseline to differentiate intentional spikes (contractions) from noise.

---
*This project was developed by Atulya Jauhari, Devesh Bharati, Nandan Soni, Soham Rajesh Wani, and Vivek Kumar Singh at Gati Shakti Vishwavidyalaya.*
