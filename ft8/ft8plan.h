///////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2012 maintech GmbH, Otto-Hahn-Str. 15, 97204 Hoechberg, Germany //
// written by Christian Daniel                                                   //
// Copyright (C) 2015-2019, 2023 Edouard Griffiths, F4EXB <f4exb06@gmail.com>    //
//                                                                               //
// This is the code from ft8mon: https://github.com/rtmrtmrtmrtm/ft8mon          //
// reformatted and adapted to Qt and SDRangel context                            //
//                                                                               //
// This program is free software; you can redistribute it and/or modify          //
// it under the terms of the GNU General Public License as published by          //
// the Free Software Foundation as version 3 of the License, or                  //
// (at your option) any later version.                                           //
//                                                                               //
// This program is distributed in the hope that it will be useful,               //
// but WITHOUT ANY WARRANTY; without even the implied warranty of                //
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                  //
// GNU General Public License V3 for more details.                               //
//                                                                               //
// You should have received a copy of the GNU General Public License             //
// along with this program. If not, see <http://www.gnu.org/licenses/>.          //
///////////////////////////////////////////////////////////////////////////////////
#ifndef ft8plan_h
#define ft8plan_h

#include <fftw3.h>
#include <QMutex>

namespace FT8
{

class Plan
{
public:
    Plan(int n);
    ~Plan();

    int n_;
    int type_;

    //
    // real -> complex
    //
    fftwf_complex *c_; // (n_ / 2) + 1 of these
    float *r_;         // n_ of these
    fftwf_plan fwd_;   // forward plan
    fftwf_plan rev_;   // reverse plan

    //
    // complex -> complex
    //
    fftwf_complex *cc1_; // n
    fftwf_complex *cc2_; // n
    fftwf_plan cfwd_;    // forward plan
    fftwf_plan crev_;    // reverse plan
    // MEASURE=0, ESTIMATE=64, PATIENT=32
    static const int M_FFTW_TYPE = FFTW_ESTIMATE;
}; // Plan

}

#endif
