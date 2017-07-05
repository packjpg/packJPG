# packJPG

[![GitHub license](http://img.shields.io/badge/license-LGPLv3-blue.svg?style=flat)](http://www.gnu.org/copyleft/lgpl.html)

packJPG performs lossless compression of JPEG files to a format labeled PJG, and can decompress a PJG file back to its original JPEG format. It typically reduces the file size of a JPEG file by 20%. Different versions of packJPG are incompatible, and the program produces an error message if you try to decompress PJG files with a different version than the one used for the compression.

* Version: 2.5k (older versions are available at http://www.elektronik.htw-aalen.de/packJPG/packjpg_m.htm)
* License: LGPLv3 (see [LGPL3.0](https://opensource.org/licenses/lgpl-3.0.html) for more information)
* Website: http://packjpg.encode.ru
* For questions and bug reports, contact packjpg (at) matthiasstirner.com
* Copyright 2006—2014 by HTW Aalen University and Matthias Stirner.

## Command Line Use


```
  Syntax: packjpg [options] [file(s)]
  Allowed options:
    -ver                       verify files after processing (converts the output back
                               to the input format, and compares)
    -v?                        level of verbosity; 0, 1, or 2 is allowed (default 0)
    -np                        no pause after processing files
    -o                         output overwrites an existing file if it exists
    -p                         proceed on warnings
    -d                         discard image metadata.
```

The program processes jpg and pjg files, but file type is determined by file content, not by file extension, and so a mislabeled file will not be incorrectly processed (or incorrectly not processed).

Wildcard file names are supported. Filenames for output files are generated automatically, with the convention being that for filename.jpg, filename.pjg is selected if it is available; otherwise, underscores are repeatedly appended until an unused filename is found (i.e. filename_.pjg, filename__.pjg, ...). If the overwrite option is used (-o), then filename.jpg's output will be written to filename.pjg. The same holds for a filename.pjg being converted to filename.jpg.

A file from stdin may be denoted by the special name "-". In this case, output is written to stdout.

Some usage examples:
```
 packJPG *.pjg
 packJPG lena.jpg
 packJPG kodim??.jpg
 packJPG - < sail.pjg > sail.jpg
 ```
 
The program has a low error tolerance for images; some JPEG files might not work with packJPG even if they work perfectly with other image processing software. By default, compression is cancelled on warnings. If warnings are skipped by using the -p option, most files with warnings can also be compressed, but the JPEG files reconstructed from PJG files might not be bitwise identical to the original JPEG files. However, there won't be any loss to image data or quality.

Image metadata can be discarded with the -d option, which can reduce a file's size. However, subsequently reconverted files will no longer contain the metadata information, and so not be bitwise identical with the original file. As with the -p option, this will not affect image data or quality.

There is no known case in which a file compressed by packJPG (without the -p option, see above) couldn't be reconstructed to exactly the state it was before. If you want an additional layer of safety you can also use the verify option -ver. In this mode, files are compressed, then decompressed and the decompressed file compared to the original file. If this test doesn't pass there will be an error message and the compressed file won't be written to the drive. 

The -ver option should not be used in conjunction with the -d and/or -p options, as the reconstructed files may not be bitwise identical with the original files, producing a false positive for errors.

Usage examples:

```
 packJPG -v1 -o baboon.pjg
 packJPG -ver lena.jpg
 packJPG -d tiffany.jpg
 packJPG -p *.jpg
 ```
 
### Developer Command Line Options
```
 -dev    needed to enable any of the switches bewlow
 
 -test   test algorithms, alert if error
 -split  split jpeg (to header & image data)
 -coll?  write coefficients (0=std,1=dhf,2=squ,3=unc)
 -fcol?  write filtered coefficients (see above)
 -zdst   write zero distribution lists to files	
 -info   write debug info to .nfo file
 -dist   write distribution data to file
 -pgm    convert and write to pgm files
 
 -s?     set global number of segments (1<=s<=49)
 -t?     set global noise threshold (0<=t<=10)
 -s?,?   set number of segments for component
 -t?,?   set noise threshold for component
```
 
The developer switch "-dev" is needed the enable any developer function. Other developer functions won't work without it. 

"-test" is a special case of the "-ver" option. Files are also verified, but if any error happens, all the output will be kept for the developer to have a look at it. 

"-split" splits a JPEG file to huffman coded image data and header data. This mode is useful if a closer look at the header of a given file is needed. 

"-coll?" and "-fcol?" write decompressed DCT coefficients to separate files (one for each color component). DCT coefficients are stored in two byte signed short integers. In the "-fcol?" option, DC coefficents are replaced by the corresponding DC prediction errors, while using the "-coll?" option will dump DC coeffcients without any prediction applied. The "?" has to be replaced by a number 0, 1, 2 or 3, which will determine the order the coefficients will be stored inside the files. Have a look at the included sample images or the source code to get an idea on which each number stands for. 

"-zdst" writes zero distribution lists to files (one for each color component). For each 8x8 image data block in a given color component, a zero distribution list contains the number of coefficients unequal zero. These numbers are stored as unsigned one byte chars. In packJPG, zero distribution lists are used as a replacement for JPEGs EOB symbol and for segmentation of data. 

"-info" dumps a ".nfo" text file with data about the JPEG file. Structure of header, type of coding and quantization tables for each component are included in this file. 

"-dump" dumps distribution for a given file to ".dist" files (again, one file per color component). For each band seperately this files contain the number of coefficients = 0; 1; 2; 3; 4; .... (absolute values). Have a look at the source code to get an idea about the structure of these files. 

"-pgm" mainly serves as a test function for the integrated DCT routines. Each color component is transformed back to it's image representation using the IDCT and dumped seperately as a ".PGM" (Portable GrayMap) file.

"-t?" and "-s?" control specific compression settings. By manipulating these parameters, you might achieve higher compression. By default, these values are set automatically. The default for segmentation ("-s?") is 10. The default for the noise threshold ("-t?") is dependant on the size of the input file. Higher sizes mean higher t - theres more data for the statistical model to learn in bigger files, so less has to be considered noise. 

### Drag and Drop (Windows)


If you try to drag and drop too many files, there might be a windowed error message about missing privileges. In that case you can try it again with less files or consider using the command prompt. The program works perfectly with thousands of input files on the command line interface. Since this issue also happens with drag and drop in other applications, it might not be a limitation of packJPG, but rather a limitation of Windows.

## Compiling packJPG


### Executable


#### Unix/Windows (not VS)


Makefiles are provided [for Microsoft Windows NT and Unix (Makefile)](source/Makefile) and [for Mac OS X (Makefile_osx)](source/Makefile_osx). Both have been tested and verified to work in GCC v3.4.5, but they should work with other compilers and newer/older versions of the GCC as well.

#### Visual Studio


Just create a new project and add the source files to it. 

### Static or Shared Library


To compile packJPG as a static or shared library, add the source files to a new static/shared library project. BUILD_LIB must be defined either at the beginning of [packjpg.cpp](source/packjpg.cpp) or from the compilers options. To compile a shared library, BUILD_DLL must also be defined has to be defined (don't define this for a static library!).

The source file [packjpgdll.h](source/packjpgdll.h) contains all public function declarations of the library and can be included in external projects which use the packJPG shared library. 

Developer functions are not available in library builds.

### Compiling with Developer Functions


If you want to include developer functions in the packJPG executable, 'DEV_BUILD' has to be defined. Developer functions are not available in library builds. They are not needed in any way for compression and decompression, and so can be omitted out to produce a smaller executable.

### Source File Descriptions

These files are necessary for compiling packJPG:

* 'Makefile' (universal makefile)
* 'aricoder.cpp' (arithmetic coder source file)
* 'aricoder.h' (arithmetic coder header file)
* 'bitops.cpp' (bitwise file I/O routines source file)
* 'bitops.h' (bitwise file I/O routines header file)
* 'dct8x8.h' (discrete cosine transform header file)
* 'packjpg.cpp' (packJPG main source file)
* 'packjpglib.h' (packJPG static library header file)
* 'pjpgtbl.h' (helper tables header file)

These files are included as well, but are not necessarily needed for compiling packJPG: 

* 'packjpg.spec' (RPM spec file, provided by Bryan Stillwell)
* 'Makefile_osx' (special OS X Makefile, provided by Ryan Flynn)
* 'packjpgdll.h' (packJPG DLL header file)
* 'file_icon.ico' (suggested .pjg icon in .ico format)
* 'app_icon.ico' (application icon in .ico format)
* 'icon.res' (application icon in Windows .res format)

Additionally, sample_images.zip contains some sample images to test packJPG with.

## Acknowledgements


packJPG is the result of countless hours of research and development. It is part of Matthias Stirner final year project for Hochschule Aalen.

Prof. Dr. Gerhard Seelmann from Hochschule Aalen supported the development of packJPG with his extensive knowledge in the field of data compression. Without his advice, packJPG would not be possible.

The packJPG logo and icon are designed by Michael Kaufmann.

## Special Permissions


All programs in this package are free software; you can redistribute them and/or modify them under the terms of the GNU Lesser General Public License as published by the Free Software Foundation; either version 3 of the License, or (at your option) any later version. 

The package is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more details at http://www.gnu.org/copyleft/lgpl.html. 

If the LGPL v3 license is incompatible with your software project you might contact us and ask for a special permission to use the packJPG library under different conditions. In any case, usage of the packJPG algorithm under the LGPL v3 or above is highly advised and special permissions will only be given where necessary on a case by case basis. This offer is aimed mainly at closed source freeware developers seeking to add PJG support to their software projects. 

## Version Naming Convention


The program version is composed of a major version number and a minor version string. For example, 2.4a, an older version of packJPG, has major version number 2.4 and minor version "a".

PJG files are compatible between minor versions. For example, 2.4a-compressed PJG files can be extracted by 2.4, 2.4b, 2.4c. PJG files are incompatible across major version numbers. For example, 2.4 can't extract 2.3 compressed .pjg files. 

The minor version is used to indicate smaller changes that don't break compatibility within a major version, such as bug fixes or speed improvements. The minor version can be empty for the first new main version in a series. For a highly specific change, the minor version might be a special name, such as "2.5fast". 

The major version number is set directly in the source code via the 'pjgversion' (main version number) and the 'subversion' (subversion string) constants. 

## History


v1.9a (04/20/2007) (non public)
 - first released version
 - only for testing purposes

v2.0  (05/28/2007) (public)
 - first public version of packJPG
 - minor improvements to overall compression
 - minor bugfixes

v2.2  (08/05/2007) (public)
 - around 40% faster compression & decompression
 - major improvements to overall compression (around 2% on average)
 - reading from stdin, writing to stdout
 - smaller executable
 - minor bugfixes
 - various minor improvements
 
v2.3  (09/18/2007) (public)
 - compatibility with JPEG progressive mode
 - compatibility with JPEG extended sequential mode
 - compatibility with the CMYK color space
 - compatibility with older CPUs
 - around 15% faster compression & decompression 
 - new switch: [-d] (discard meta-info)
 - various bugfixes

v2.3a (11/21/2007) (public)
 - crash issue with certain images fixed
 - compatibility with packJPG v2.3 maintained

v2.3b (12/20/2007) (public)
 - some minor errors in the packJPG library fixed
 - compatibility with packJPG v2.3 maintained
 
v2.4 (03/24/2010) (public)
 - major improvements to overall compression (1--2%)
 - around 10% faster compression & decompression
 - major improvements to JPG compatibility
 - size of executable reduced to ~33%
 - new switch: [-ver] (verify file after processing)
 - new switch: [-np] (no pause after processing)
 - new progress bar output mode
 - arithmetic coding routines rewritten from scratch
 - various smaller improvements to numerous to list here
 - new SFX (self extracting) archive format
 
v2.5 (11/11/2011) (public)
 - improvements to overall compression (~0.5%)
 - several minor bugfixes
 - major code cleanup
 - removed packJPX from the package
 - added packARC to the package
 - packJPG is now open source!
 
v2.5a (11/21/11) (public)
 - source code compatibility improvements (Gerhard Seelmann)
 - avoid some compiler warnings (Gerhard Seelmann)
 - source code clean up (Gerhard Seelmann)
 
v2.5b (01/27/12) (public)
 - further removal of redundant code
 - some fixes for the packJPG static library
 - compiler fix for Mac OS (thanks to Sergio Lopez)
 - improved compression ratio calculation
 - eliminated the need for temp files
 
v2.5c (04/13/12) (public)
 - various source code optimizations
 
v2.5d (07/03/12) (public)
 - fixed a rare bug with progressive JPEG
 
v2.5e (07/03/12) (public)
 - some minor source code optimization
 - changed packJPG licensing to LGPL
 - moved packARC to a separate package
 
v2.5f (02/24/13) (public)
 - fixed a minor bug in the JPG parser (thanks to Stephan Busch)
 
v2.5g (09/14/13) (public)
 - fixed a rare crash bug with manipulated JPEG files
 
v2.5h (12/07/13) (public)
 - added a warning for inefficient huffman coding (thanks to Moinak Ghosh)
 
v2.5i (12/26/13) (public)
 - fixed possible crash with malformed JPEG (thanks to Moinak Ghosh)
 
v2.5j (01/15/14) (public)
 - various source code optimizations (using cppcheck)

v2.5k (01/22/16) (public)
 - Updated contact info
 - fixed a minor bug
