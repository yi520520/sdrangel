///////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2012 maintech GmbH, Otto-Hahn-Str. 15, 97204 Hoechberg, Germany //
// written by Christian Daniel                                                   //
// Copyright (C) 2015-2019, 2022 Edouard Griffiths, F4EXB <f4exb06@gmail.com>    //
// Copyright (C) 2021, 2023 Jon Beniston, M7RCE <jon@beniston.com>               //
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

#ifndef PLUGINS_CHANNELRX_DEMODAM_AMDEMODSETTINGS_H_
#define PLUGINS_CHANNELRX_DEMODAM_AMDEMODSETTINGS_H_

#include "dsp/dsptypes.h"
#include <QByteArray>

class Serializable;

struct AMDemodSettings
{
    enum SyncAMOperation
    {
        SyncAMDSB,
        SyncAMUSB,
        SyncAMLSB
    };
    enum FrequencyMode {
        Offset,
        MediumWave,
        Airband25k,
        Airband8K
    };

    qint32 m_inputFrequencyOffset;
    Real m_rfBandwidth;
    Real m_squelch;
    Real m_volume;
    bool m_audioMute;
    bool m_bandpassEnable;
    Real m_afBandwidth;  //!< High frequency for bandpass
    quint32 m_rgbColor;
    QString m_title;
    Serializable *m_channelMarker;
    QString m_audioDeviceName;
    bool m_pll;
    SyncAMOperation m_syncAMOperation;
    FrequencyMode m_frequencyMode;
    qint64 m_frequency;
    bool m_snap;
    int m_streamIndex; //!< MIMO channel. Not relevant when connected to SI (single Rx).
    bool m_useReverseAPI;
    QString m_reverseAPIAddress;
    uint16_t m_reverseAPIPort;
    uint16_t m_reverseAPIDeviceIndex;
    uint16_t m_reverseAPIChannelIndex;
    Serializable *m_rollupState;
    int m_workspaceIndex;
    QByteArray m_geometryBytes;
    bool m_hidden;

    AMDemodSettings();
    void resetToDefaults();
    void setChannelMarker(Serializable *channelMarker) { m_channelMarker = channelMarker; }
    void setRollupState(Serializable *rollupState) { m_rollupState = rollupState; }
    QByteArray serialize() const;
    bool deserialize(const QByteArray& data);
};



#endif /* PLUGINS_CHANNELRX_DEMODAM_AMDEMODSETTINGS_H_ */
