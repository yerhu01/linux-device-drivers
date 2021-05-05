# Linux Device Driver Development

# Contents

1. [Introduction](#introduction)
1. [Setup](#setup)
1. [Hello Version Driver](#hello-version)
1. [Nunchuk Device Driver](#nunchuk-device)
1. [Serial Device Driver](#serial-device)

# Introduction
The purpose of this project is to illustrate the development of Linux device drivers. Specifically, it implements a basic kernel module, an I2C Nunchuk driver, and a UART memory-mapped device driver. The hardware platform used is the ARM-based **BeagleBone Black**.

# Setup
## Downloading Kernel Source Code
1. Clone the mainline Linux tree
    ```
    git clone git://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git
    ```
1. Access stable releases
    ```
    git remote add stable git://git.kernel.org/pub/scm/linux/kernel/git/stable/linux-stable.git
    git fetch stable
    ```
1. Choose a particular stable version
    ```
    git checkout -b 5.9.y stable/linux-5.9.y
    ```

## Board Setup
### Set up serial communication with the board
1. Connect board to PC with USB to Serial adapter
    - GND wire (blue) to pin 1
    - TX wire (red) to pin 4 (board RX)
    - RX wire (green) to pin 5 (board TX)
1. Communicate with board through serial port
    ```
    picocom -b 115200 /dev/ttyUSB0
    ```

### Set up networking to transfer files using TFTP
1. Network configuration on target
    * configure networking in U-Boot
    ```
    setenv ipaddr 192.168.0.100
    setenv serverip 192.168.0.1
    ```
    * configure ethernet over USB
    ```
    setenv ethact usb_ether
    setenv usbnet_devaddr f8:dc:7a:00:00:02
    setenv usbnet_hostaddr f8:dc:7a:00:00:01
    saveenv
    ```
1. Network configuration on PC host
    ```
    nmcli con add type ethernet ifname enxf8dc7a000001 ip4 192.168.0.1/24
    ```

## Kernel Configuration and Compiling
1. Get reference configuration for OMAP2
    ```
    make omap2plus_defconfig
    ```
1. Customize to support networking over USB
    * Set the following options: `CONFIG_USB_GADGET`, `CONFIG_USB_MUSB_HDRC`, `CONFIG_USB_MUSB_GADGET`, `CONFIG_USB_MUSB_DSPS`, `AM335X_PHY`
    ```
    make menuconfig
    ```
1. Set env to use definitions for arm platform and cross-compiler
    ```
    export CROSS_COMPILE="ccache arm-linux-gnueabi-"; export ARCH=arm
    ```
1. Compile
    ```
    make -j 8
    ```
1. Compiling only DTBs
    ```
    make dtbs
    ```

# Hello Version
This implements a simple standalone kernel module outside of main Linux sources that displays a message when loaded and unloaded.

This module accesses kernel internals and input parameters to display a greeting message, kernel version, and total elapsed time when loading or unloading the module.

1. Building the module
    ```
    make
    ```
1. Testing the module
    ```
    insmod ./hello/hello_version.ko whom=John
    rmmod hello_version
    ```

A patch was also created to add the new files to the mainline kernel.

# Nunchuk Device
This module implements a driver for an I2C device which offers the functionality of an I2C Nunchuk.

## Custom Device Tree
To let the Linux kernel handle the new device on the **BeagleBone Black**, the description of this device was added into the board device tree which can be found in the corresponding [patch file] (nunchuk/0001-Add-i2c1-and-nunchuk-nodes-in-dts.patch).

In particular, the second I2C bus (i2c1) is enabled and configured with the Nunchuk device declared as a child node at address 52 and communication frequency of 100KHz (coming from the Wii Nunchuk specification).

To access the bus data and clock signals, the pin muxing of the SoC was configured to MODE2 to obtain the functionality needed (`I2C1_SCL` and `I2C1_SDA` for pins A16 and B16 of AM335X SoC) according to the **BeagleBone Black System Reference Manual**. From the **AM335x Processor Manual**, the pin names of A16 and B16 were found to be `SPI0_CS0` and `SPI0_D1` with both supporting PULLUP/PULLDOWN modes.

A patch was also created to add the new files to the mainline kernel.

## I2C Driver
An `i2c_driver` structure is initialized and the nunchuk i2c driver is registered in it along with the probe and remove routines that are called when a Nunchuk is found. This driver is then registered using `module_i2c_driver`.

In the probe routine, before being able to read nunchuk registers, a handshake signal must be sent to initialize the nunchuk. This is done by first sending two bytes to the device (`0xf0` and `0x55`), delaying for 1ms, and then sending another two bytes (`0xfb` and `0x00`). Another specific behavior of the nunchuk requires an initial read to update the state of its internal registers.

The driver is also given an input interface so that device events are exposed to user space using the kernel based polling API for input devices using the input subsystem. As part of the device model, pointers are kept to manage between the physical (as handled by the physical I2C bus) and the logical (as handled by the input subsystem for running the polling routine).

The Wii Nunchuk outputs six bytes of data as follows:
|           |                                                                      |
|-----------|----------------------------------------------------------------------|
| Byte 0x00 | X-axis data of the joystick                                          |
| Byte 0x01 | Y-axis data of the joystick                                          |
| Byte 0x02 | X-axis data of the accellerometer (bit 2 to 9 for 10-bit resolution) |
| Byte 0x03 | Y-axis data of the accellerometer (bit 2 to 9 for 10-bit resolution) |
| Byte 0x04 | Z-axis data of the accellerometer (bit 2 to 9 for 10-bit resolution) |
| Byte 0x05 | bit 0 = Z button status - 0 = pressed and 1 = released               |
|	        | bit 1 as C button status - 0 = pressed and 1 = released              |
|           | bit 2 and 3 as lower 2 bits of X-axis data of the accellerometer     |
|           | bit 4 and 5 as lower 2 bits of Y-axis data of the accellerometer     |
|           | bit 6 and 7 as lower 2 bits of Z-axis data of the accellerometer     |

Thus, Z and C buttons states and joystick X and Y coordinates are retrieved from the registers in this driver. For the joystick to be usable by the user space application, classic buttons are also declared.

1. Building the module
    ```
    make
    ```
1. Testing the module
    ```
    insmod ./nunchuk.ko
    rmmod nunchuk
    ```
1. Testing the input interface
    ```
    evtest
    ```

# Serial Device
This module implements a character driver allowing to write data to additional CPU serial ports and to read data from them.

## Custom Device Tree
To let the Linux kernel handle the new device on the **BeagleBone Black**, the description of this device was added into the board device tree which can be found in the corresponding [patch file] (serial/0001-Add-uart-nodes-in-dts.patch).

A pin muxing section is created with declarations for UART2 and UART4. The pin muxing of the SoC was configured for MODE1 for pins B17 and A17 (`SPI0_SCLK` and `SPI0_D0`) of AM335X SoC for UART2 and MODE6 for pins T17 and U17 (`GPMC_WAIT0` and `GPMC_WPN`) of AM335X SoC for UART4 according to the **BeagleBone Black System Reference Manual** and **AM335X Processor Manual**.

A patch was also created to add the new files to the mainline kernel.

## Serial Driver
A `platform_driver` structure is initialized and the serial driver is registered in it along with the probe and remove routines that are called when the serial device is found (using `serial_dt_ids`). This driver is then registered using `module_platform_driver`.

To access device registers, the routines `reg_read` and `reg_write` are created to access offset registers from the base virtual address using `readl()` and `writel`. Note that all UART register offsets have standardized values and must be multiplied by 4 for OMAP SoCs.

In the probe routine, a `serial_dev` private structure is initialized with base virtual addresses for the device registers. To enable UART devices, power management is initialized, the line and baud rate is configured and a software reset is requested. This serial driver is also given a misc interface which has:  
- `serial_write()` write file operation stub which calls `serial_write_char` to write character from userspace data to `UART_TX` register
- `serial_read()` read file operation stub to put contents of circular buffer (from `UART_RX` register) to userspace and puts the process to sleep when no data is available
- `serial_ioctl()` maintain count of characters written through serial port and implements two `unlocked_ioctl` operations (`SERIAL_RESET_COUNTER` and `SERIAL_GET_COUNTER`)
- `file_operations` structure declaring these operations and sets `.owner` field to `THIS_MODULE` to tell kernel this module is in charge of this device and for module reference counting

The probe routine also enables interrupts for the UART device and registers an interrupt handler `serial_irq` which acknowledges the interrupts. `serial_irq` simply reads contents of the `UART_RX` register and stores them inside the circular buffer. It then wakes up all processes waiting on the wait queue to put contents in the buffer to user space.

Since this driver has two shared resources, the circular buffer that allows to transfer the received data in the interrupt handler to `serial_read()` and the device itself, a spinlock is used to prevent concurrent accesses to the shared buffer and the device. Note that two processes can write data at the same time to the serial port and is valid behavior.

Additionally, kernel debugging mechanisms are used in the `serial_write()` to show each character being written and the interrupt handler to show each character received. To see the debugging messages, recompile kernel with options: `CONFIG_DYNAMIC_DEBUG` and `CONFIG_DEBUG_INFO`. Also add `loglevel=8` to kernel command line in U-Boot to see messages directly in console.

1. Building the modules
    ```
    make
    ```
1. Starting another serial communication
   ```
    picocom -b 115200/dev/ttyUSB1
   ```
1. Testing write()
    * Run on target
    ```
    insmod ./serial/serial.ko
    lsmod
    echo sometext > /dev/serial-48024000
    ```
1. Testing read()
    * Run on target and type in picocom
    ```
    cat /dev/serial-48024000
    ```
1. Testing ioctl
    * `serial-get-counter.c` and `serial-reset-counter.c` programs are provided and take as argument the path to the device file corresponding to the UART device and can be compiled with:
    ```
    arm-linux-gnueabi-gcc -o serial-get-counter serial-get-counter.c
    arm-linux-gnueabi-gcc -o serial-reset-counter serial-reset-counter.c
    ```
    * Run on target
    ```
    ls /dev/serial*
    echo sometext > /dev/serial-48024000
    ./serial/serial-get-counter /dev/serial-48024000
    ./serial/serial-reset-counter /dev/serial-48024000
    ```
1. Testing module reference counting
    * Run on target
    ```
    cat > /dev/serial-48024000 &
    lsmod
    rmmod serial
    ps
    kill -9 <pid>
    lsmod
    rmmod serial
    ```
