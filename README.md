# Embedded-I2C-Sensor-Emulator-Register-Level-Simulation-in-C

This project simulates an IMU sensor (accelerometer) similar to the **LSM6DSL** using C.  
It includes a virtual register map, an I2C protocol emulator, and a simple driver to read sensor data.  
The main goal is to understand how **firmware interacts with hardware at register-level**.

---

##  Why I Built This Project

In most embedded projects, drivers are written assuming hardware exists.  
I wanted to understand what happens **inside the hardware itself**.  

So I built a **sensor emulator** that behaves like a real IMU device, generating accelerometer data with motion, noise, and bias.

This helped me learn:
- I2C protocol mechanics at a low level  
- How sensors expose data through registers  
- How drivers and hardware interact in real embedded systems  

> This project simulates both the **sensor hardware and driver interaction**, providing a complete view of embedded system behavior.

---

##  System Architecture

The project is designed in a **layered approach**, similar to real firmware:

**Driver Layer**  
→ Handles sensor initialization and data reading  

**I2C Interface Layer**  
→ Simulates I2C protocol (START, ADDRESS, READ/WRITE, STOP)  

**Sensor Emulator Layer**  
→ Implements register map and generates synthetic sensor data  

This separation improves modularity and reflects production-quality embedded design.

---

##  Features

- Full I2C state machine simulation  
- 256-byte register map like a real sensor  
- WHO_AM_I register validation  
- Accelerometer simulation:  
  - Sinusoidal motion on X and Y axes  
  - Constant gravity-like Z-axis  
  - Random noise and sensor bias  
- Auto-increment burst reads (multi-byte XYZ data)  
- Simple driver to initialize and read sensor

---

## ⚙️ Register Map (Reference: LSM6DSL Datasheet)

| Register     | Address | Description           |
|--------------|--------|---------------------|
| WHO_AM_I     | 0x0F   | Device ID (0x6A)     |
| CTRL1_XL     | 0x10   | Accelerometer config |
| OUTX_L_A     | 0x28   | X-axis LSB           |
| OUTX_H_A     | 0x29   | X-axis MSB           |
| OUTY_L_A     | 0x2A   | Y-axis LSB           |
| OUTY_H_A     | 0x2B   | Y-axis MSB           |
| OUTZ_L_A     | 0x2C   | Z-axis LSB           |
| OUTZ_H_A     | 0x2D   | Z-axis MSB           |

---

##  Data Generation Logic

- X = 1000 × sin(angle)  
- Y = 1000 × cos(angle)  
- Z = 1000 (constant gravity-like value)  
- Random noise (±25) added  
- Bias added (100) to simulate sensor offset  

---

##  Key Learnings

- Understanding I2C communication at protocol level  
- Register-level hardware abstraction  
- Data formatting: LSB/MSB handling  
- Simulation of real-world sensor imperfections  
- Difference between driver logic and hardware behavior  

---

##  How to Run

```bash
gcc emulator.c -o emulator -lm
./emulator

## OUT PUT LOOKS

Starting Emulator...
[I2C] START
[I2C] ADDRESS OK
[I2C] REG = 0x10
[EMULATOR] Accel Enabled = 1
[I2C] STOP
[I2C] START
[I2C] ADDRESS OK
[I2C] STOP
X=175 | Y=1098 | Z=1104
-------------------------
[I2C] START
[I2C] ADDRESS OK
[I2C] STOP
X=314 | Y=1058 | Z=1113
-------------------------
[I2C] START
[I2C] ADDRESS OK
[I2C] STOP
X=380 | Y=1069 | Z=1108
-------------------------
[I2C] START
[I2C] ADDRESS OK
[I2C] STOP
X=505 | Y=1019 | Z=1103
-------------------------
[I2C] START
[I2C] ADDRESS OK
[I2C] STOP
X=594 | Y=956 | Z=1084
-------------------------
[I2C] START
[I2C] ADDRESS OK


