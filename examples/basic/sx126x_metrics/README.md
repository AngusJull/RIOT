examples/basic/sx126x_metrics
================

This example is designed for collecting and displaying different statistics, or configuring hardware for testing

The example is designed to launch into a shell, as well as display information of the esp32-heltec-lora32-v3 OLED display
included.

The statistics that are included so far are battery (in millivolts), statistics about tx/rx bytes and packets on the sx126x connected as an IEEE 802.15.4 device,
and a small amount of information on recently sent/received IPV6 packets

Usage
=====

Build, flash and start the application:

```
make
make flash
make term
```

Later, the ability to dump the stored information as JSON output will be added
