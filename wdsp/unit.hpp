/*  unit.hpp

This file is part of a program that implements a Software-Defined Radio.

Copyright (C) 2013 Warren Pratt, NR0V
Copyright (C) 2024 Edouard Griffiths, F4EXB Adapted to SDRangel

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

The author can be reached by email at

warren@wpratt.com

*/

#ifndef wdsp_unit_h
#define wdsp_unit_h

#include <QRecursiveMutex>

#include "export.h"

namespace WDSP {

class WDSP_API Unit
{
public:
    int in_rate;                // input samplerate
    int out_rate;               // output samplerate
    int in_size;                // input buffsize (complex samples) in a fexchange() operation
    int dsp_rate;               // sample rate for mainstream dsp processing
    int dsp_size;               // number complex samples processed per buffer in mainstream dsp processing
    int dsp_insize;             // size (complex samples) of the output of the r1 (input) buffer
    int dsp_outsize;            // size (complex samples) of the input of the r2 (output) buffer
    int out_size;               // output buffsize (complex samples) in a fexchange() operation
    QRecursiveMutex csDSP;      // used to block dsp while parameters are updated or buffers flushed
    QRecursiveMutex csEXCH;     // used to block fexchange() while parameters are updated or buffers flushed
    int state;                  // 0 for unit OFF; 1 for unit ON
};

} // namespace WDSP

#endif
