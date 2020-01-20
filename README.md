# The Hux Kernel

![languages](https://img.shields.io/github/languages/count/hgz12345ssdlh/hux-kernel?color=blue)
![top-lang](https://img.shields.io/github/languages/top/hgz12345ssdlh/hux-kernel?color=orange)
![code-size](https://img.shields.io/github/languages/code-size/hgz12345ssdlh/hux-kernel?color=lightgrey)

A handmade x86 32-bit toy operating system kernel built from scratch.

By Guanzhou Jose Hu @ 2020.


## Long-Term Goals

The Hux kernel aims at:

- A *minimal* workable core design
- Microkernel, highly *modularized*
- Use *up-to-date* concepts/techniques/components when possible

Theses are general and long-term goals which I will (hopefully) follow throughout the project.


## Development Doc

I will document the whole development process plus everything I reckon important during the development of Hux in [the wiki pages](https://github.com/hgz12345ssdlh/hux-kernel/wiki). If there are any typos/mistakes/errors, please raise an issue!


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

then, plug it onto any x86 IA32 computer, configure its BIOS to boot from USB, and start playing with Hux ;)


## References

See the "References" section [here](https://github.com/hgz12345ssdlh/hux-kernel/wiki/1.-Prerequisite-Readings).


## TODO List

- [x] The kernel skeleton
- [ ] Debugging stacks
- [ ] VGA text mode driver
- [ ] Descriptors
- [ ] Interrupts & Timer
- [ ] Memory management
- [ ] Multiprocessing
- [ ] Virtual file system
- [ ] Wiki pages & README
- [ ] Coding style spec
