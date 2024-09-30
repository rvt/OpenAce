
# Enclosures

> **_NOTE:_**:
> Currently only the 'full' (two tranceiver) version is included and I am working on a enclosure for the Lite version.

To open the `*.scad` file you need to use the latest version of [OpenSCAD](https://openscad.org). 
From OpenSCAD you can directly export an STL for 3D printing, or make your modifications if needed.
If you have any cool or usefull modifications do let me know, I would love to hear them!

## How to 3D print

From OpenSCAD from the menu use `Design -> Render`, this might take a few seconds depending on your CPU.
Then export the file as STL using `File -> Export -> Export as STL`, then in your favorite slicer import the STL and print.

I use PrucaSlicer (although I still have an old Ender 3), but the output files will look something like the below image.

![Pruca Slicer](../doc/img/prusa-slicer.png)).

## Generating a stl file from the KiCAD board to be used in OpenSCAD

From KiCAD menu export the board as step with the following settings: ![KiCAD STEP to STL](../doc/img/kicad-step-stl.png)). 
I use `openace.step`, but any other name should be file.
Then use [imagetostl.com](https://imagetostl.com/convert/file/step/to/stl#convert) to convert the STEP to a STL type file.
