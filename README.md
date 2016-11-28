# pix: Packet-based Information Chaining Service

## About
We are developing an operating system for networking, called pix, as a fast
and extensible packet forwarding engine for various network functions.
The initial version (v0.0.1) only supports ethernet switching for simplicity
of the proof-of-concept operating system.  Other functions such as IPv4/IPv6
routing, VPN, and Layer 2-4 filtering (firewall) will be supported soon.

## Images
| Version  | Release Date  | Image      |
| :------- | :------------ | :--------- |
| 0.0.1rc1 | Nov. 28, 2016 | [pix-v0.0.1rc1.img](https://pix.jar.jp/images/pix-v0.0.1rc1.img "pix-v0.0.1rc1.img") |
| 0.0.1b   | Nov. 6, 2016  | [pix-v0.0.1b.img](https://pix.jar.jp/images/pix-v0.0.1b.img "pix-v0.0.1b.img") |
| 0.0.1a   | Oct. 31, 2016 | [pix-v0.0.1a.img](https://pix.jar.jp/images/pix-v0.0.1a.img "pix-v0.0.1a.img") |

## Supported Hardware

### CPUs
Intel's SandyBridge or later CPUs

### Network Interface Cards
#### 1 GbE NICs
* Intel Pro 1000/MT Desktop (82540EM)
* Intel Pro 1000/MT Server (82545EM)
* Intel 82541PI
* Intel 82543GC
* Intel 82573L
* Intel 82567LM
* Intel 82577LM
* Intel 82579LM

#### 10 GbE NICs
* Intel X520-DA2
* Intel X520-QDA1

## How to Install
pix does not have an installer program.  You can install pix onto your disk
(e.g., SSD, USB flash) by the disk image copy command.
In BSD/Linux operation systems, you can use ```dd``` command.

```
# dd if=pix-v0.0.1a.img of=/path/to/target/disk bs=4096
```

## Developer
Hirochika Asai

## Logo
![Alt text](pix.png?raw=true "pix")
