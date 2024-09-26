///////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2012 maintech GmbH, Otto-Hahn-Str. 15, 97204 Hoechberg, Germany //
// written by Christian Daniel                                                   //
// Copyright (C) 2015-2019, 2021-2022 Edouard Griffiths, F4EXB <f4exb06@gmail.com> //
// Copyright (C) 2021-2023 Jon Beniston, M7RCE <jon@beniston.com>                //
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

#ifndef INCLUDE_RADIOSONDEDEMODSETTINGS_H
#define INCLUDE_RADIOSONDEDEMODSETTINGS_H

#include <QByteArray>
#include <QString>

#include "dsp/dsptypes.h"

class Serializable;

// Number of columns in the tables
#define RADIOSONDEDEMOD_FRAME_COLUMNS 28

struct RadiosondeDemodSettings
{
    qint32 m_baud;
    qint32 m_inputFrequencyOffset;
    Real m_rfBandwidth;
    Real m_fmDeviation;
    Real m_correlationThreshold;
    QString m_filterSerial;
    bool m_udpEnabled;
    QString m_udpAddress;
    uint16_t m_udpPort;
    int m_scopeCh1;
    int m_scopeCh2;

    QString m_logFilename;
    bool m_logEnabled;
    bool m_useFileTime;

    quint32 m_rgbColor;
    QString m_title;
    Serializable *m_channelMarker;
    int m_streamIndex; //!< MIMO channel. Not relevant when connected to SI (single Rx).
    bool m_useReverseAPI;
    QString m_reverseAPIAddress;
    uint16_t m_reverseAPIPort;
    uint16_t m_reverseAPIDeviceIndex;
    uint16_t m_reverseAPIChannelIndex;
    Serializable *m_scopeGUI;
    Serializable *m_rollupState;
    int m_workspaceIndex;
    QByteArray m_geometryBytes;
    bool m_hidden;

    int m_frameColumnIndexes[RADIOSONDEDEMOD_FRAME_COLUMNS];//!< How the columns are ordered in the table
    int m_frameColumnSizes[RADIOSONDEDEMOD_FRAME_COLUMNS];  //!< Size of the columns in the table

    static const int RADIOSONDEDEMOD_CHANNEL_SAMPLE_RATE = 57600; //!< 12x 4800 baud rate (use even multiple so Gaussian filter has odd number of taps)

    RadiosondeDemodSettings();
    void resetToDefaults();
    void setChannelMarker(Serializable *channelMarker) { m_channelMarker = channelMarker; }
    void setRollupState(Serializable *rollupState) { m_rollupState = rollupState; }
    void setScopeGUI(Serializable *scopeGUI) { m_scopeGUI = scopeGUI; }
    QByteArray serialize() const;
    bool deserialize(const QByteArray& data);
};

#endif /* INCLUDE_RADIOSONDEDEMODSETTINGS_H */
