# VGAmod
General purpose RF IQ modulator using VGA graphics DAC

Copyright (C) 2011 Elkhazin
Copyright (C) 2011-2015 Rashid Mustapha [rashid.mustapha@gmail.com]

http://github.com/dabfree
http://opendigitalradio.org/

VGAMod is free software; you can redistribute it and/or
modify it under the terms of the General Public License V3 as 
published by the Free Software Foundation; either version 3 of the
License, or (at your option) any later version.

VGAMod is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the General Public License 3 for
more details.

You should have received a copy of the General Public License V3 along 
with VGAmod; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

*********************
INTRODUCTION
*********************

VGAmod is a proof of concept Software Defined Radio IQ modulator signal generator
inspired by the amazing work of Fabrice Bellard (www.bellard.org). Certain 
functionality was cribbed from the author of VGAsig, an FM transmitter by Bartek 
Kania.

It can be used to generate a modulated RF signal from the VGA DAC from either an 
a file of IQ samples (32-bit complex floats) or stdin.
See http://opendigitalradio.org

*********************
INSTALLATION
*********************

Dependencies: the SDL library must be installed.

gcc -O3 VGAmod.c  -lm -lSDL -o VGAmod
 
gcc -O3 testmod.c  -lm -lSDL -o testmod


*********************
USAGE
*********************

./VGAmod -w [width] -W [wtotal] -h [height] -H [htotal


-r [rate]
 
-c [CarrierFreq]
 
-s [SampleFreq] 

-i [InputFile]

-C [CarrierSignalShift] 

-I [InputSignalShift]

-O [OutputShiftRight] 

-M [OutputMask] 

-L [OutputShiftLeft]


*********************
TO DO
*********************

1.
    Fix testmod.

2.
    Fix slight frequency drift is reported between the input/carrier and vsync clocks.
This show up as a slight 
drift left or right in the carrier/input signals. There
is no obvious reason for this. You can 
try playing with the vsyncOffsetSignal/
vsyncOffsetCarrier parameters. You could also try a mutex 
around the buffer 
red/write pointers, but the volatile declaration should do the trick.

3.
    Add ZeroMQ input

4.
    Interpolation and filtering?

5.
    Create as GNURadio sink?


*********************
PEOPLE
*********************

Elkhazin
Rash <rashid.mustapha@gmail.com>


*********************
REFERENCES
*********************
(Inspired by:)

http://www.bellard.org/dvbt/

(Features/ideas poached from)

http://bk.gnarf.org/creativity/vgasig/

(For)

http://opendigitalradio.org
