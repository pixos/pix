# PIX Internals

## Process tree
    + init
      + tty: tty driver
        + pash: shell
      + tcp: tcp server
      + telnet: telnet server application
      + fe: forwarding engine integrated driver server

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

## Memory allocator for forwarding engine
* Requirements
  * Physically contiguous memory space for packets, descriptors, and
    miscellaneous MMIO-based region
  * Aligned to an arbitrary boundary (e.g., 64-byte cache line, 128-bytes)
  * Fast physical address resolution from virtual address for packets
  * Cache pollution minimization (avoidance)
* Design
  * Two interfaces to allocate memory
    * pix_malloc(): to allocate memory for descriptors and misc.
    * pix_malloc_
