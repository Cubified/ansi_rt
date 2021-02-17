## ansi_rt

A simple terminal-based raytracer, capable of reading scene data from a text file and drawing it directly to the screen using ANSI escape codes.

This is not the most fully-featured or correct raytracer out there, but it has been an excellent learning experience for me, and is a nice tech demo of what a terminal with truecolor support can do.  I have plans to expand upon it (because many features are either partially-functionally or not yet implemented), but it is functional in its current state.

### Screenshot

![Demo](https://github.com/Cubified/ansi_rt/blob/main/demo.png)

### Features

`ansi_rt` currently includes:

- Ray traced graphics with truecolor (16 million color) support
- Sphere, cube, and plane objects
- Point lights
- Reflections

### Usage

Compile and run `ansi_rt` using:

     $ make
     $ ./ansi_rt [scene file]

Where `[scene file]` is a file which follows this format:

     shape_type pos_x pos_y pos_z rot_x rot_y rot_z scale_x scale_y scale_z is_reflective material_r material_g material_b

Refer to [the example scene file](https://github.com/Cubified/ansi_rt/blob/main/scene) for an example.

This format is not good by any measure, but it accomplishes what it needs to for this situation.
