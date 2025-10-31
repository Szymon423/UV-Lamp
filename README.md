<div align="center">

# 💅 UV Lamp Project

*A custom-made UV lamp for nail polish curing*

</div>

---

## 📖 About

In this project, I wanted to address a common issue in the beauty industry: **misleading UV lamp specifications**. 

Most commercial UV lamps are advertised as "60W UV lamps" while being supplied with only 15W power adapters. I wanted to create something honest and effective, so I designed this lamp from scratch.

### ✨ Key Features

- **True 45W electrical power** - honest specifications
- **Adjustable power settings** - customize intensity from 0-100%
- **Configurable timer** - set precise curing times up to 3 minutes
- **Intuitive touchscreen interface** - easy configuration with rotary encoder
- **Settings persistence** - preferences saved in flash memory
- **Professional UV wavelength** - 395-405nm LEDs for optimal curing

<div align="center">

### 🎨 Main Design

![Main Render](/Hardware/3D-Model/Renders/01.png)
![Main Render](/Hardware/3D-Model/Renders/02.png)

### 🖼️ Detailed Views

<table>
  <tr>
    <td width="50%">
      <img src="/Hardware/3D-Model/Renders/03.png" alt="Side View" width="100%"/>
      <p align="center"><em>Side perspective</em></p>
    </td>
    <td width="50%">
      <img src="/Hardware/3D-Model/Renders/04.png" alt="Top View" width="100%"/>
      <p align="center"><em>Top view</em></p>
    </td>
  </tr>
  <tr>
    <td width="50%">
      <img src="/Hardware/3D-Model/Renders/05.png" alt="Internal View" width="100%"/>
      <p align="center"><em>Internal components</em></p>
    </td>
    <td width="50%">
      <img src="/Hardware/3D-Model/Renders/06.png" alt="PCB Layout" width="100%"/>
      <p align="center"><em>PCB arrangement</em></p>
    </td>
  </tr>
  <tr>
    <td width="50%">
      <img src="/Hardware/3D-Model/Renders/07.png" alt="Assembly Detail" width="100%"/>
      <p align="center"><em>Assembly detail</em></p>
    </td>
    <td width="50%">
      <img src="/Hardware/3D-Model/Renders/08.png" alt="Component Layout" width="100%"/>
      <p align="center"><em>Component layout</em></p>
    </td>
  </tr>
  <tr>
    <td colspan="2">
      <img src="/Hardware/3D-Model/Renders/09.png" alt="Full Assembly" width="100%"/>
      <p align="center"><em>Complete assembly</em></p>
    </td>
  </tr>
</table>

</div>

---

## ⚡ Hardware Design

The hardware was designed in **KiCad** around the **Raspberry Pi RP2040** microcontroller. The lamp is powered through a C8 socket connector from 230V AC mains.

### 🔧 PCB Architecture

The system consists of three types of PCBs:

1. **Power & Driver Board**
   - Converts 230V AC to 48V DC for LED drivers
   - Provides 3.3V DC for microcontroller and peripherals
   - LED current drivers with PWM control capability

2. **Control Board**
   - RP2040 microcontroller
   - 1.69" LCD display with LVGL interface
   - Rotary encoder for navigation
   - Two control buttons
   - Flash memory for settings storage

3. **LED Arrays**
   - Multiple LED strings with 395-405nm wavelength
   - Optimized thermal management
   - Uniform light distribution

### 📊 Technical Specifications

- **Input voltage:** 230V AC (50/60Hz)
- **LED power:** 48V DC, up to 45W total
- **UV wavelength:** 395-405nm
- **Control:** PWM dimming, 0-100%
- **Timer range:** 0-180 seconds
- **Display:** 1.69" color LCD (280x240)

### 📸 Additional Renders

<div align="center">

<table>
  <tr>
    <td width="50%"><img src="/Hardware/3D-Model/Renders/03.png" alt="Render 02" width="100%"/></td>
    <td width="50%"><img src="/Hardware/3D-Model/Renders/04.png" alt="Render 03" width="100%"/></td>
  </tr>
  <tr>
    <td width="50%"><img src="/Hardware/3D-Model/Renders/05.png" alt="Render 04" width="100%"/></td>
    <td width="50%"><img src="/Hardware/3D-Model/Renders/06.png" alt="Render 05" width="100%"/></td>
  </tr>
  <tr>
    <td width="50%"><img src="/Hardware/3D-Model/Renders/07.png" alt="Render 04" width="100%"/></td>
    <td width="50%"><img src="/Hardware/3D-Model/Renders/08.png" alt="Render 05" width="100%"/></td>
  <tr>
  <tr>
    <td width="100%"><img src="/Hardware/3D-Model/Renders/09.png" alt="Render 05" width="100%"/></td>
  <tr>
</table>

</div>

---

## 💻 Software

The firmware is written in C for the RP2040 and utilizes the following technologies:

### 🛠️ Core Technologies

- **LVGL (Light and Versatile Graphics Library)** - for the graphical interface
- **SquareLine Studio** - for UI design
- **Pico SDK** - official Raspberry Pi development kit
- **Hardware PWM** - for precise LED dimming control

### 🖥️ User Interface

<div align="center">

![User Interface](/Hardware/3D-Model/Renders/10.png)

</div>

The interface features three main screens:

1. **Main Screen** - displays current timer and power settings with arc progress indicator
2. **Power Configuration** - adjust UV intensity from 0-100%
3. **Timer Configuration** - set curing time from 0 to 180 seconds

### 🎮 Controls

- **Rotary Encoder** - navigate and adjust values
- **Button 1** - short press: change screen | long press: enter/exit edit mode
- **Button 2** - short press: start/pause timer | long press: reset timer

### 🚀 Features

- Smooth animations and transitions
- Real-time countdown with arc visualization
- Automatic settings save to flash memory
- Debounced button inputs with long-press detection
- 30 FPS refresh rate for fluid UI

---

## 📜 License

This project is open source. Feel free to use, modify, and distribute according to the license terms.

---

## 🙏 Acknowledgments

This project was created because my girlfriend complained about her unreliable UV lamp. It combines my passion for electronics and design with practical problem-solving.

---

<div align="center">

*Made with dedication to quality and honesty*

</div>