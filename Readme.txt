Getting Started

  1. Install WinAmp
    
  2. Copy required DLLs to the same directory as fhgaacenc.exe
  (Included in this archive)
    - libsndfile-1.dll
  (Optional: copy from WinAmp directory if you want to uninstall WinAmp)
    - enc_fhgaac.dll
    - libmp4v2.dll
    - nsutil.dll

  3. Install VC2008 runtime if you don't have yet
    http://www.microsoft.com/downloads/details.aspx?familyid=A5C84275-3B97-4AB7-A40D-3802B2AF5FC2

  4. Enjoy!


Command Line Options

fhgaacenc.exe [options] infile [outfile]

  --cbr <bitrate> : encode in CBR mode, bitrate=8..576
  --vbr <preset>  : encode in VBR mode, preset=1..6 [default]
  --profile <auto|lc|he|hev2> : choose AAC profile (only for CBR mode)
    auto : automatically choose the optimum profile according to the bitrate [default]
    lc   : force use LC-AAC profile
    he   : force use HE-AAC (AAC+SBR) profile
    hev2 : force use HE-AAC v2 (AAC+SBR+PS) profile
  --adts : use ADTS container instead of MPEG-4
  --ignorelength : ignore the size of data chunk when encoding from pipe
  --quiet        : don't print the progress


Credits

fhgaacenc is distributed with libsndfile version 1.0.24, which is released under GNU LGPL.
You can get the source code of libsndfile in http://www.mega-nerd.com/libsndfile/ .
See http://www.gnu.org/licenses/lgpl.txt for GNU LGPL.

    Copyright (C) 1999-2011 Erik de Castro Lopo <erikd@mega-nerd.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, either version 2.1 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

