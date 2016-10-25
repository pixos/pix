# PIX Internals

## Process tree
    + init
      + tty: tty driver
        + pash: shell
      + tcp: tcp server
      + telnet: telnet server application

## Kernel internals
* timer
* scheduler
* vfs?
  * ramfs?
  * devfs?

## Drivers
* tty
  * console: keyboard + video
  * serial (UART)

## Servers
* pm: process manager
* init
* tcp: TCP/IP

## Integrated Driver Servers for Fast Path Processor (Exclusive Processor)
* fe: Forwarding Engine
  * e1000
  * e1000e
  * ixgbe
  * i40e
