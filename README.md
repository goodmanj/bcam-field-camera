# BCam - Magnetic Field Camera

## About
BCam lets you see see invisible magnetic fields!  It uses an array of magnetic field sensors to map out the strength and direction of magnetic fields passing through it, displaying the fields on an LCD screen in realtime.  It can sense strong permanent magnets and electromagnets, as well as the weak fields from single current-carrying wires, and the Earth's magnetic field.

BCam is based on the [RP2040 Raspberry Pi Pico](https://www.raspberrypi.com/documentation/microcontrollers/pico-series.html) microcontroller, the [TMAG5273](https://www.ti.com/product/TMAG5273) Hall-effect magnetic field sensor, and the [ILI9488](http://www.lcdwiki.com/3.5inch_SPI_Module_ILI9488_SKU:MSP3520) touch LCD screen.  

BCam was originally designed as a teaching tool for physics classrooms.  The [documentation](doc/) includes demos, lab handouts, and other teaching material.

##Build your own!

This repo contains everything you need to build a BCam:
* [pcb/](pcb/): Printed circuit board files
* [src/](src/): Arduino source code
* [cad/](cad/): 3-d design files
* [doc/](doc/): Documentation
* [bom.md](bom.md): Bill of materials
BCam is designed to be built by hand.  All surface-mount parts are hand-solderable (some skill required).  The case is 3-d printed.  You can use a desktop CNC machine to mill your own printed circuit board, or have it made by an online board manufacturer such as [JLCPCB](https://jlcpcb.com/), [PCBWay](https://www.pcbway.com/), or [OSHPark](https://oshpark.com/).

BCam was originally developed as a project for [Fab Academy](https://fabacademy.org/), a hands-on global class in rapid prototyping and digital fabrication.  The original project documentation is [archived at Fab Academy's site](https://fabacademy.org/2023/labs/wheaton/students/jason-goodman/final-project/index.html), but is superseded by this repo.

## License

Copyright (c) 2023-2025 Jason Goodman <jcgoodman@gmail.com>

Creative Commons License CC BY-NC-SA
https://creativecommons.org/licenses/by-nc-sa/4.0/

  You are free to:
  Share — copy and redistribute the material in any medium or format
  Adapt — remix, transform, and build upon the material

  Under the following terms:
  - Attribution — You must give appropriate credit , provide a link to the license, and indicate if changes were made
  You may do so in any reasonable manner, but not in any way that suggests the licensor endorses you or your use.
  - NonCommercial — You may not use the material for commercial purposes .
  - ShareAlike — If you remix, transform, or build upon the material, you must distribute your contributions under the same license as the original.
  - No additional restrictions — You may not apply legal terms or technological measures that legally restrict others from doing anything the license permits.