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

#ifndef PLUGINS_CHANNELTX_MODRTTY_RTTYMODSETTINGS_H
#define PLUGINS_CHANNELTX_MODRTTY_RTTYMODSETTINGS_H

#include <QByteArray>
#include <stdint.h>
#include "dsp/dsptypes.h"
#include "util/baudot.h"

class Serializable;

struct RttyModSettings
{
    qint64 m_inputFrequencyOffset;
    float m_baud;
    int m_rfBandwidth;
    int m_frequencyShift;
    Real m_gain;
    bool m_channelMute;
    bool m_repeat;
    int m_repeatCount;
    int m_lpfTaps;
    bool m_rfNoise;
    bool m_writeToFile;
    QString m_text;     // Text to send
    bool m_pulseShaping;
    float m_beta;
    int m_symbolSpan;
    Baudot::CharacterSet m_characterSet;
    bool m_unshiftOnSpace;
    bool m_msbFirst;    // false = LSB first, true = MSB first
    bool m_spaceHigh;   // false = mark high frequency, true = space high frequency
    bool m_prefixCRLF;
    bool m_postfixCRLF;
    QStringList m_predefinedTexts;

    quint32 m_rgbColor;
    QString m_title;
    Serializable *m_channelMarker;
    int m_streamIndex;
    bool m_useReverseAPI;
    QString m_reverseAPIAddress;
    uint16_t m_reverseAPIPort;
    uint16_t m_reverseAPIDeviceIndex;
    uint16_t m_reverseAPIChannelIndex;
    bool m_udpEnabled;
    QString m_udpAddress;
    uint16_t m_udpPort;
    Serializable *m_rollupState;
    int m_workspaceIndex;
    QByteArray m_geometryBytes;
    bool m_hidden;

    RttyModSettings();
    void resetToDefaults();
    void setChannelMarker(Serializable *channelMarker) { m_channelMarker = channelMarker; }
    void setRollupState(Serializable *rollupState) { m_rollupState = rollupState; }
    QByteArray serialize() const;
    bool deserialize(const QByteArray& data);
    QString getMode() const;
};

#endif /* PLUGINS_CHANNELTX_MODRTTY_RTTYMODSETTINGS_H */
