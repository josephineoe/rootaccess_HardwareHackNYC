# Project Context

## Target Hardware
- **Primary MCU/Processor**: STM32G474RE
- **Secondary Processors**: MPU-6050 (IMU — accel/gyro/temp via I2C)
- **Dev Board / Custom PCB**: NUCLEO-G474RE
- **Board Revision**:

## Clock Configuration
- **HSE/HSI**: HSE 24 MHz (on-board)
- **PLL Config**: PLLM=DIV6, PLLN=85, PLLR=DIV2 → 170 MHz
- **System Clock Frequency**: 170 MHz

## Pin Mapping / GPIO Assignments
- PA2: USART2_TX (ST-LINK VCP)
- PA3: USART2_RX (ST-LINK VCP)
- PA5: LD2 user LED (output)
- PB4: TIM3_CH1 PWM — SG90 servo signal (Arduino D5 / CN9 pin 6)
- PB8: I2C1_SCL — MPU-6050 (Arduino D15 / CN5 pin 10)
- PB9: I2C1_SDA — MPU-6050 (Arduino D14 / CN5 pin 9)

## Memory Layout
- **Flash Size**:
- **RAM Size**:
- **Linker Script Regions**:

## Peripherals & Interfaces
-

## External Devices & Sensors
-

## Communication Protocols
- **Protocol (e.g. CAN, UART, SPI, I2C)**: I2C1 (MPU-6050), USART2 (ST-LINK VCP debug output)
- **Baud Rate / Mode**: I2C 100 kHz Standard Mode; UART 115200 8N1
- **I2C Pins**: SCL → PB8 (Arduino D15 / CN5 pin 10), SDA → PB9 (Arduino D14 / CN5 pin 9)

## Software Stack
- **Build System**: 
- **Compile Command**:
- **HAL / Framework**:
- **RTOS**:
- **Toolchain**:
- **Compiler Flags**:
- **External Libraries**:

## Development Environment
- **Host OS**: Linux
- **IDE**:
- **Debug Interface (SWD/JTAG)**:
- **Debug Probe (ST-Link/J-Link/OpenOCD)**:

## Debug Configuration
<!-- Auto-learned during debug sessions. Edit these directly to change debug settings. -->
- **GDB Server Command**:
- **GDB Executable**:
- **Firmware ELF Path**:
- **GDB Server Port**:

## Power Management
- **Sleep Modes**:
- **Power Domains**:
- **Battery / Power Source**:

## Boot Configuration
- **Bootloader**:
- **OTA Mechanism**:

## Interrupt Configuration
- **NVIC Priority Groups**:
- **Key ISRs**:

## Compliance & Standards
-

## SDK / Library Paths
<!-- Paths to vendor SDKs or libraries the agent cannot download. -->
<!-- Example: /opt/zephyr-sdk-0.16.8, /home/user/STM32CubeF4, ~/ncs/v2.7.0 -->
-

## Supporting Devices
-

## Project Notes

