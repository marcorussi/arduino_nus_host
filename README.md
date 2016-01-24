# arduino_nus_host
An Arduino host example for my multilink NUS central role on nrf51. The following commands simplify NUS central role management from an enternal Arduino board. The project is intended to be composed by an Arduino board connected to an nrf51 module, flashed with my **multilink NUS central** software (https://github.com/marcorussi/nrf51_multi_nus_central), through UART interface. 

**Arduino UART TX and RX pins are respectively 9 and 8.** See README file of multilink NUS central for nrf51 UART pins.

There are two main modes: 
- configuration mode: where received data are parsed as a command and executed if valid;
- data mode: where data are sent directly through the last selected connection.
At power up the device is in configuration mode.

Find serial commands here below:
- "AT": query the module if it is running properly;
- "SCAN": start a scan of devices and after stops it after a few seconds. Number of found devices is displayed on terminal. Then, it requires info (address and name) of each found devices and displays them on terminal;
- "CONN=*i*": request a connection of device at index *i*. This index must be between 0 and *n*-1 devices where *n* is the number of found devices shown previously after a scan request. Upon successfull connection, any further received data wll be sent to the connected device (data mode);
- "SWITCH=*i*": request to switch next data to device at index *i*. This implies that a previous successfull connection is established with that device. Then, any further received data wll be sent to the selected device (data mode);
- "DROP=*i*": drop an established connection with device at index *i*. This implies that a previous successfull connection is established with that device. Upon disconnection, device is in configuration mode;
- "AUTO": enter into data mode with the last valid device index. This implies that an escape character has been previously sent during the data mode of an established connection.

When the device is in data mode, it is possible to escape and so enter in command mode by sending "***" string. For entering back in data mode send the "AUTO" command as described above.

This software is still in progress and could drastically change in future.
