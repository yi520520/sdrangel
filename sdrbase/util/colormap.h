///////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2012 maintech GmbH, Otto-Hahn-Str. 15, 97204 Hoechberg, Germany //
// written by Christian Daniel                                                   //
// Copyright (C) 2014 John Greb <hexameron@spam.no>                              //
// Copyright (C) 2015-2017, 2019-2020, 2023 Edouard Griffiths, F4EXB <f4exb06@gmail.com> //
// Copyright (C) 2020 Kacper Michajłow <kasper93@gmail.com>                      //
// Copyright (C) 2022 Jon Beniston, M7RCE <jon@beniston.com>                     //
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

#ifndef INCLUDE_COLORMAP_H
#define INCLUDE_COLORMAP_H

#include "export.h"

#include <QHash>

// 256-entry floating point RGB color maps.
// "Angel" is SDRangel's waterfall colormap
// Some common maps from matplotlib
// Scientific colour maps from:
//  https://www.nature.com/articles/s41467-020-19160-7
//  https://zenodo.org/record/5501399#.YqhaAu7MLAQ
class SDRBASE_API ColorMap
{
public:
    static QStringList getColorMapNames();
    static const float *getColorMap(const QString &name);
    static constexpr int m_size = 256*3;

private:
    static QHash<QString, const float *> m_colorMaps;
    static const float m_angel[m_size];
    static const float m_jet[m_size];
    static const float m_turbo[m_size];
    static const float m_parula[m_size];
    static const float m_hot[m_size];
    static const float m_cool[m_size];
    static const float m_batlow[m_size];
    static const float m_hawaii[m_size];
    static const float m_acton[m_size];
    static const float m_imola[m_size];
    static const float m_tokyo[m_size];
    static const float m_lapaz[m_size];
    static const float m_buda[m_size];
    static const float m_devon[m_size];
    static const float m_lajolla[m_size];
    static const float m_bamako[m_size];
    static const float m_plasma[m_size];
    static const float m_rainbow[m_size];
    static const float m_prism[m_size];
    static const float m_viridis[m_size];
    static const float m_loggray[m_size];
    static const float m_shrimp[m_size];
    static const float m_cubehelix[m_size];
    static const float m_cubegamma[m_size];
    static const float m_cubehlx2[m_size];
    static const float m_icy[m_size];
    static const float m_mint[m_size];
};

#endif
