///////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2012 maintech GmbH, Otto-Hahn-Str. 15, 97204 Hoechberg, Germany //
// written by Christian Daniel                                                   //
// Copyright (C) 2015-2022 Edouard Griffiths, F4EXB <f4exb06@gmail.com>          //
// Copyright (C) 2020-2021, 2023 Jon Beniston, M7RCE <jon@beniston.com>          //
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

#include <QColor>

#include "util/simpleserializer.h"
#include "settings/serializable.h"
#include "navtexdemodsettings.h"

NavtexDemodSettings::NavtexDemodSettings() :
    m_channelMarker(nullptr),
    m_scopeGUI(nullptr),
    m_rollupState(nullptr)
{
    resetToDefaults();
}

void NavtexDemodSettings::resetToDefaults()
{
    m_inputFrequencyOffset = 0;
    m_rfBandwidth = 450.0f; // OBW for 2FSK = 2 * deviation + data rate. Then add a bit for carrier frequency offset
    m_navArea = 1;
    m_filterStation = "All";
    m_filterType = "All";
    m_udpEnabled = false;
    m_udpAddress = "127.0.0.1";
    m_udpPort = 9999;
    m_logFilename = "navtex_log.csv";
    m_logEnabled = false;
    m_scopeCh1 = 0;
    m_scopeCh2 = 1;

    m_rgbColor = QColor(100, 25, 207).rgb();
    m_title = "Navtex Demodulator";
    m_streamIndex = 0;
    m_useReverseAPI = false;
    m_reverseAPIAddress = "127.0.0.1";
    m_reverseAPIPort = 8888;
    m_reverseAPIDeviceIndex = 0;
    m_reverseAPIChannelIndex = 0;
    m_workspaceIndex = 0;
    m_hidden = false;

    for (int i = 0; i < NAVTEXDEMOD_COLUMNS; i++)
    {
        m_columnIndexes[i] = i;
        m_columnSizes[i] = -1; // Autosize
    }
}

QByteArray NavtexDemodSettings::serialize() const
{
    SimpleSerializer s(1);
    s.writeS32(1, m_inputFrequencyOffset);
    s.writeS32(2, m_streamIndex);
    s.writeS32(3, m_navArea);
    s.writeString(4, m_filterStation);
    s.writeString(5, m_filterType);

    if (m_channelMarker) {
        s.writeBlob(6, m_channelMarker->serialize());
    }
    s.writeFloat(7, m_rfBandwidth);

    s.writeBool(9, m_udpEnabled);
    s.writeString(10, m_udpAddress);
    s.writeU32(11, m_udpPort);
    s.writeString(12, m_logFilename);
    s.writeBool(13, m_logEnabled);
    s.writeS32(14, m_scopeCh1);
    s.writeS32(15, m_scopeCh2);

    s.writeU32(20, m_rgbColor);
    s.writeString(21, m_title);
    s.writeBool(22, m_useReverseAPI);
    s.writeString(23, m_reverseAPIAddress);
    s.writeU32(24, m_reverseAPIPort);
    s.writeU32(25, m_reverseAPIDeviceIndex);
    s.writeU32(26, m_reverseAPIChannelIndex);

    if (m_rollupState) {
        s.writeBlob(27, m_rollupState->serialize());
    }

    s.writeS32(28, m_workspaceIndex);
    s.writeBlob(29, m_geometryBytes);
    s.writeBool(30, m_hidden);
    s.writeBlob(31, m_scopeGUI->serialize());

    for (int i = 0; i < NAVTEXDEMOD_COLUMNS; i++) {
        s.writeS32(100 + i, m_columnIndexes[i]);
    }

    for (int i = 0; i < NAVTEXDEMOD_COLUMNS; i++) {
        s.writeS32(200 + i, m_columnSizes[i]);
    }

    return s.final();
}

bool NavtexDemodSettings::deserialize(const QByteArray& data)
{
    SimpleDeserializer d(data);

    if(!d.isValid())
    {
        resetToDefaults();
        return false;
    }

    if(d.getVersion() == 1)
    {
        QByteArray bytetmp;
        uint32_t utmp;
        QString strtmp;

        d.readS32(1, &m_inputFrequencyOffset, 0);
        d.readS32(2, &m_streamIndex, 0);
        d.readS32(3, &m_navArea, 1);
        d.readString(4, &m_filterStation, "All");
        d.readString(5, &m_filterType, "All");

        if (m_channelMarker)
        {
            d.readBlob(6, &bytetmp);
            m_channelMarker->deserialize(bytetmp);
        }
        d.readFloat(7, &m_rfBandwidth, 450.0f);

        d.readBool(9, &m_udpEnabled);
        d.readString(10, &m_udpAddress);
        d.readU32(11, &utmp);

        if ((utmp > 1023) && (utmp < 65535)) {
            m_udpPort = utmp;
        } else {
            m_udpPort = 9999;
        }
        d.readString(12, &m_logFilename, "navtex_log.csv");
        d.readBool(13, &m_logEnabled, false);
        d.readS32(14, &m_scopeCh1, 0);
        d.readS32(15, &m_scopeCh2, 0);

        d.readU32(20, &m_rgbColor, QColor(100, 25, 207).rgb());
        d.readString(21, &m_title, "Navtex Demodulator");
        d.readBool(22, &m_useReverseAPI, false);
        d.readString(23, &m_reverseAPIAddress, "127.0.0.1");
        d.readU32(24, &utmp, 0);

        if ((utmp > 1023) && (utmp < 65535)) {
            m_reverseAPIPort = utmp;
        } else {
            m_reverseAPIPort = 8888;
        }

        d.readU32(25, &utmp, 0);
        m_reverseAPIDeviceIndex = utmp > 99 ? 99 : utmp;
        d.readU32(26, &utmp, 0);
        m_reverseAPIChannelIndex = utmp > 99 ? 99 : utmp;

        if (m_rollupState)
        {
            d.readBlob(27, &bytetmp);
            m_rollupState->deserialize(bytetmp);
        }

        d.readS32(28, &m_workspaceIndex, 0);
        d.readBlob(29, &m_geometryBytes);
        d.readBool(30, &m_hidden, false);

        if (m_scopeGUI)
        {
            d.readBlob(31, &bytetmp);
            m_scopeGUI->deserialize(bytetmp);
        }

        for (int i = 0; i < NAVTEXDEMOD_COLUMNS; i++) {
            d.readS32(100 + i, &m_columnIndexes[i], i);
        }

        for (int i = 0; i < NAVTEXDEMOD_COLUMNS; i++) {
            d.readS32(200 + i, &m_columnSizes[i], -1);
        }

        return true;
    }
    else
    {
        resetToDefaults();
        return false;
    }
}

