# The Hux Kernel

![languages](https://img.shields.io/github/languages/count/hgz12345ssdlh/hux-kernel?color=blue)
![top-lang](https://img.shields.io/github/languages/top/hgz12345ssdlh/hux-kernel?color=orange)
![code-size](https://img.shields.io/github/languages/code-size/hgz12345ssdlh/hux-kernel?color=lightgrey)

A handmade x86 32-bit toy operating system kernel built from scratch.

By Guanzhou Jose Hu @ 2020.


## Long-Term Goals

The main goal of Hux is to be **Understandable**: structured in a way that is easy to understand (not mimicking existing UNIX-like systems). OS development seems scary at first glance for beginners mostly because it involves too many hairy technical details. I admit that, in real-world systems, we must face the complexity to ensure compatibility, performance, robustness, security, etc. Yet, a toy kernel project would help demonstrate the key concepts of an operating system, including layers of abstractions, virtualization, concurrency, and persistency.

Other goals of the Hux kernel include:

1. **Minimal**: a minimal workable core design
2. **Modular**: microkernel, highly modularized
3. **Up-to-date**: adopt up-to-date design concepts/techniques/components when possible
4. **Experimental**: full of my own crazy ideas

These are general and long-term goals which I will (hopefully) follow throughout the project. I hope this can lead towards a full toy HuxOS running not over x86 but a self-designed simplified ISA in the future 😁


## Development Doc

I will document the whole development process of Hux plus everything I reckon important in the [**WIKI pages 📝**](https://github.com/hgz12345ssdlh/hux-kernel/wiki). If there are any typos/mistakes/errors, please raise an issue!


## Playing with Hux

With QEMU, download the CDROM image `hux.iso` and do:

```bash
$ qemy-system-i386 -cdrom hux.iso
```

Or alternatively, you may prepare an empty USB stick then write the image into it to make it bootable by:

```bash
# Will erase all data on the device, be careful not to destroy your hard disk!
$ sudo dd if=hux.iso of=/dev/sd<x> && sync
```

then, plug it onto any x86 IA32 computer, configure its BIOS to boot from USB, and start playing with Hux 😆


## References

Main references:

- [The OSDev Wiki](https://wiki.osdev.org/) (IMPORTANT ✭)
- [Writing an OS in Rust](https://os.phil-opp.com/)

Check the "References" section [here](https://github.com/hgz12345ssdlh/hux-kernel/wiki/1.-Prerequisite-Readings) for the full list.


## TODO List

- [x] The kernel skeleton
- [x] VGA text mode driver
- [ ] Debugging stack
- [ ] Global Descriptors
- [ ] Interrupts & Timer
- [ ] Memory management
- [ ] Multiprocessing
- [ ] Virtual file system
- [ ] Wiki pages & README
- [ ] Coding style spec
- [ ] Experiment with Rust (call it Rux maybe)
- [ ] Self-designed ISA and more
