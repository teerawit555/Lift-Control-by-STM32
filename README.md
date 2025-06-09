# STM32 Elevator Control System (Simplified Assignment Version)

> A basic automatic elevator control system simulation using **STM32F411CEU6** and **Raspberry Pi Zero 2 W** <br>
> ENE 330 EMBEDDED SYSTEM DEVELOPMENT (FINAL PROJECT)

---

## Members
65070502414 Nutsata Dhanyalakvanich
#### 65070502423 Teerawit Pongkunawut (Created)
65070502433 Pongpira Wanachompoo

## 📌 Project Description

This system is designed to simulate the control of 3 elevators in a building. It **does not implement a full queueing system**, but instead uses a simple scoring logic to assign the most suitable elevator based on its current position and workload.

The system is ideal for demonstrating UART communication, command parsing, and basic GPIO-based LED control in embedded development.

### Actual behavior based on current code:
- Chooses the nearest elevator to the `from` floor using a simple `distance + workload` score
- Accepts commands in the form `UP,from,to` or `DOWN,from,to` via USB CDC
- Elevator moves to `from` floor, then continues to `to`
- No task queue or multiple-user support
- Displays elevator position via LEDs on GPIO pins

---

## 🧠 Features

- Controls 3 elevators in real-time using STM32
- Communicates with Simulation Board (SB) via UART
- Receives commands over USB CDC, e.g.:
  - `UP,2,5\r\n` → Elevator goes to receive user on floor 3, then go to floor 6 (destination)
  - `DOWN,5,2\r\n` → Elevator goes down from floor 6, then go to floor 3 (destination) 
  - `GETCurrFloor,1\r\n` → Queries current floor of elevator 1
  - `GETuserFloor,1\r\n` → Queries current user floor
  - `GETuserReq,1\r\n` → Queries destination that user request
- GPIO LEDs indicate elevator position (on ports PA, PB)
- Uses a 100ms timer interrupt to simulate elevator movement logic

---

## 📄 Documentation

- [Download Simulation Manual (PDF)](./Docs/elevator_SB_Manual_Documentation.pdf)

---

## 🔧 Comparison with Full Requirement

| Feature | This System | Expected by Instructor |
|---------|-------------|------------------------|
| Multi-request Queue | ❌ Not supported | ✅ Required (per-car or global queue) |
| Pickup and drop flow | ✔️ from → to | ✅ Should support multiple sequential requests |
| Multi-user request handling | ❌ Not supported | ✅ Required |
| Priority-based queueing | ❌ Simple distance/workload only | ✅ Should evaluate based on time or fairness |
| Communication | ✔️ USB CDC + UART | ✔️ Correct |

---

## 💻 Key Files

- `main.c`: Core elevator logic, timer interrupt, GPIO control
- `usbd_cdc_if.c`: Handles USB CDC commands and parsing logic
- `elevator_SB_Manual.pdf`: Command protocol for Simulation Board

---



