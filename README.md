# The Hux Kernel

![languages](https://img.shields.io/github/languages/count/josehu07/hux-kernel?color=green)
![top-lang](https://img.shields.io/github/languages/top/josehu07/hux-kernel?color=orange)
![code-size](https://img.shields.io/github/languages/code-size/josehu07/hux-kernel?color=lightgrey)
![license](https://img.shields.io/github/license/josehu07/hux-kernel)

A handmade x86 32-bit toy operating system kernel built from scratch.

By Guanzhou Jose Hu @ 2020.


## Goals

The main goal of Hux is to be **Understandable**: structured in a way that is easy to understand (not mimicking existing UNIX-like systems). OS development seems scary at first glance for beginners mostly because it involves too many hairy technical details. I admit that, in real-world systems, we must face the complexity to ensure compatibility, performance, robustness, security, etc. Yet, a toy kernel project would help demonstrate the key concepts of an operating system, including layers of abstractions, virtualization, concurrency, and persistency.

I also choose *C* language with `i386-x32` architecture, since beginners tend to be more comfortable with this combination. More up-to-date system programming languages like *Rust* are great choices for OS dev (Philipp is making his Rust OS kernel [here](https://os.phil-opp.com/)), but I will start with easier settings for now to maintain better understandability.

Goals of the Hux kernel include:

1. **Understandable**: as stated above
2. **Minimal**: a minimal workable core design
3. **Modular**: though monolithic kernel, I will try to keep the code structure modularized
4. **Educational**: not targeting at practical use

These are general and long-term goals which I will (hopefully) follow throughout the project. I hope this can lead towards a full toy HuxOS which we can install on real devices and play with in the future 😁


## Development Doc

I document the whole development process of Hux, its skeleton, related theories, practice pitfalls, plus everything I reckon important in the [**WIKI pages 📝**](https://github.com/hgz12345ssdlh/hux-kernel/wiki). If there are any typos / mistakes / errors, please raise an issue!


## Playing with Hux

With QEMU (recommended), download the CDROM image `hux.iso` and do:

```bash
$ qemu-system-i386 -cdrom hux.iso
```

Or alternatively, you may prepare an empty USB stick then write the image into it to make it bootable by:

```bash
# Will erase all data on the device, be careful!
$ sudo dd if=hux.iso of=/dev/sd<x> && sync
```

then, plug it onto an x86 IA32 computer, configure its BIOS to boot from USB, and start playing with Hux 😆


## References

Main references:

- [The OSDev Wiki](https://wiki.osdev.org/) (IMPORTANT ✭)
- [Writing an OS in Rust](https://os.phil-opp.com/) by Philipp

Check the "References" section [here](https://github.com/hgz12345ssdlh/hux-kernel/wiki/1.-Prerequisite-Readings) for the full list.


## TODO List

- [x] The kernel skeleton
- [x] VGA text mode driver
- [x] Debugging stack
- [ ] Interrupts & Timer
- [x] Global Descriptors
- [ ] Memory management
- [ ] Multiprocessing
- [ ] Virtual file system
- [ ] Security concerns
- [ ] Wiki pages & README
- [ ] Coding style spec
- [ ] Experiment with Rust (call it Rux maybe)
- [ ] Self-designed smallest compiler
- [ ] Self-designed ISA and more
