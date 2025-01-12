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

Shield: [![CC BY-NC-SA 4.0][cc-by-nc-sa-shield]][cc-by-nc-sa]

This work is licensed under a
[Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International License][cc-by-nc-sa].

[![CC BY-NC-SA 4.0][cc-by-nc-sa-image]][cc-by-nc-sa]

[cc-by-nc-sa]: http://creativecommons.org/licenses/by-nc-sa/4.0/
[cc-by-nc-sa-image]: https://licensebuttons.net/l/by-nc-sa/4.0/88x31.png
[cc-by-nc-sa-shield]: https://img.shields.io/badge/License-CC%20BY--NC--SA%204.0-lightgrey.svg