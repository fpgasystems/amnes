Memcopy OpenCL Design Example README file
=================================================
=================================================

This readme file for the Memory Bandwidth OpenCL Design Example contains information
about the design example package. For more examples, please visit the page:
http://www.altera.com/support/examples/opencl/opencl.html

This file contains the following information:

- Example Description
- Software and Board Requirements
- Usage Instructions
- Package Contents and Directory Structure
- Release History
- Disclaimer & Licenses
- Contacting Altera(R)


Example Description
===================

This example runs a basic OpenCL kernel that tests the bandwidth of a memory to memory copy, as well as read and write only.


Software and Board Requirements
===============================

This design example is for use with the following versions of the 
Altera Complete Design Suite and Quartus(R) II software and
Altera SDK for OpenCL software:
    - 16.0.2

For host program compilation, the requirements are:
    - Linux: GNU Make and gcc
    - Windows: Microsoft Visual Studio 2010

The supported operating systems for this release:
    - All operating systems supported by the Altera SDK for OpenCL


Usage Instructions
==================

Linux:
  1. make
  2. ./bin/correlatio_matrix // <optional number of 64 byte lines to transfer>

The FPGA should be loaded with an OpenCL image before running.
The output will include a wall-clock time of the OpenCL execution time
and the kernel time as reported by the OpenCL event profiling API, as well as the calculated bandwidth. The host
program includes verification against the host CPU.


Package Contents and Directory Structure
========================================

/correlation
  /device
     Contains OpenCL kernel source files (.cl)
  /host
    /src
      Contains host source (.cpp) files
  /bin
    Contains host binary and OpenCL binaries (.aocx)


Generating Kernel
=================

To compile the kernel, run:

  aoc device/correlation.cl -o bin/correlation.aocx -board=<value>
  
  value = pac_a10 in my case (Arria 10GX)

where <board> matches the board you have in your system. If you are unsure
of the board name, use the following command to list available boards:

  aoc --list-boards

This compilation command can also be used to target the emulator by adding 
the -march=emulator flag.

If the board already has a AOCX file (see AOCX selection section above),
be sure to either replace or relocate that AOCX file.


Release History
===============

SDK Version   Example Version   Comments
-------------------------------------------------------------------------------
16.0          1.0               First release of example
16.0.2          1.1               Cleaning up SVM APIs and enabling reprogram-on-the-fly

Disclaimer & Licenses
=====================

Copyright (C) 2013-2015 Altera Corporation, San Jose, California, USA. All rights reserved. 
Permission is hereby granted, free of charge, to any person obtaining a copy of this 
software and associated documentation files (the "Software"), to deal in the Software 
without restriction, including without limitation the rights to use, copy, modify, merge, 
publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to 
whom the Software is furnished to do so, subject to the following conditions: 
The above copyright notice and this permission notice shall be included in all copies or 
substantial portions of the Software. 
 
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, 
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES 
OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND 
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT 
HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, 
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR 
OTHER DEALINGS IN THE SOFTWARE. 
 
This agreement shall be governed in all respects by the laws of the State of California and 
by the laws of the United States of America. 


Contacting Altera
=================

Although we have made every effort to ensure that this design example works
correctly, there might be problems that we have not encountered. If you have
a question or problem that is not answered by the information provided in 
this readme file or the example's documentation, please contact Altera(R) 
support.

http://www.altera.com/mysupport/

