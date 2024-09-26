///////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2023 Jon Beniston, M7RCE <jon@beniston.com>                     //
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
#include <QDebug>

#include "util/simpleserializer.h"
#include "settings/serializable.h"
#include "freqscannersettings.h"

FreqScannerSettings::FreqScannerSettings() :
    m_channelMarker(nullptr),
    m_rollupState(nullptr)
{
    for (int i = 0; i < FREQSCANNER_COLUMNS; i++)
    {
        m_columnIndexes.append(i);
        m_columnSizes.append(-1);
    }
    resetToDefaults();
}

void FreqScannerSettings::resetToDefaults()
{
    m_inputFrequencyOffset = 0;
    m_channelBandwidth = 25000;
    m_channelFrequencyOffset = 25000;
    m_threshold = -60.0f;
    m_channel = "";
    m_frequencySettings = {};
    m_scanTime = 0.1f;
    m_retransmitTime = 2.0f;
    m_tuneTime = 100;
    m_priority = MAX_POWER;
    m_measurement = PEAK;
    m_mode = CONTINUOUS;

    for (int i = 0; i < FREQSCANNER_COLUMNS; i++)
    {
        m_columnIndexes[i] = i;
        m_columnSizes[i] = -1; // Autosize
    }

    m_rgbColor = QColor(0, 205, 200).rgb();
    m_title = "Frequency Scanner";
    m_streamIndex = 0;
    m_useReverseAPI = false;
    m_reverseAPIAddress = "127.0.0.1";
    m_reverseAPIPort = 8888;
    m_reverseAPIDeviceIndex = 0;
    m_reverseAPIChannelIndex = 0;
    m_workspaceIndex = 0;
    m_hidden = false;
}

QByteArray FreqScannerSettings::serialize() const
{
    SimpleSerializer s(1);

    s.writeS32(1, m_inputFrequencyOffset);
    s.writeS32(2, m_channelBandwidth);
    s.writeS32(3, m_channelFrequencyOffset);
    s.writeFloat(4, m_threshold);
    s.writeString(8, m_channel);
    s.writeFloat(9, m_scanTime);
    s.writeFloat(10, m_retransmitTime);
    s.writeS32(11, m_tuneTime);
    s.writeS32(12, (int)m_priority);
    s.writeS32(13, (int)m_measurement);
    s.writeS32(14, (int)m_mode);
    s.writeList(15, m_frequencySettings);

    s.writeList(20, m_columnIndexes);
    s.writeList(21, m_columnSizes);

    s.writeU32(40, m_rgbColor);
    s.writeString(41, m_title);
    if (m_channelMarker) {
        s.writeBlob(42, m_channelMarker->serialize());
    }
    s.writeS32(44, m_streamIndex);
    s.writeBool(45, m_useReverseAPI);
    s.writeString(46, m_reverseAPIAddress);
    s.writeU32(47, m_reverseAPIPort);
    s.writeU32(48, m_reverseAPIDeviceIndex);
    s.writeU32(49, m_reverseAPIChannelIndex);
    if (m_rollupState) {
        s.writeBlob(52, m_rollupState->serialize());
    }
    s.writeS32(53, m_workspaceIndex);
    s.writeBlob(54, m_geometryBytes);
    s.writeBool(55, m_hidden);

    return s.final();
}

bool FreqScannerSettings::deserialize(const QByteArray& data)
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
        d.readS32(2, &m_channelBandwidth, 25000);
        d.readS32(3, &m_channelFrequencyOffset, 25000);
        d.readFloat(4, &m_threshold, -60.0f);
        d.readString(8, &m_channel);
        d.readFloat(9, &m_scanTime, 0.1f);
        d.readFloat(10, &m_retransmitTime, 2.0f);
        d.readS32(11, &m_tuneTime, 100);
        d.readS32(12, (int*)&m_priority, (int)MAX_POWER);
        d.readS32(13, (int*)&m_measurement, (int)PEAK);
        d.readS32(14, (int*)&m_mode, (int)CONTINUOUS);
        d.readList(15, &m_frequencySettings);
        if (m_frequencySettings.size() == 0)
        {
            // Try reading old settings
            QList<bool> enabled;
            QList<qint64> frequencies;

            d.readList(6, &enabled);
            d.readList(7, &frequencies);
            if (frequencies.size() > 0)
            {
                for (int i = 0; i < frequencies.size(); i++)
                {
                    FrequencySettings frequencySettings;
                    frequencySettings.m_frequency = frequencies[i];
                    if (i < enabled.size()) {
                        frequencySettings.m_enabled = enabled[i];
                    } else {
                        frequencySettings.m_enabled = true;
                    }
                    m_frequencySettings.append(frequencySettings);
                }
            }
        }

        d.readList(20, &m_columnIndexes);
        d.readList(21, &m_columnSizes);

        d.readU32(40, &m_rgbColor, QColor(0, 205, 200).rgb());
        d.readString(41, &m_title, "Frequency Scanner");
        if (m_channelMarker)
        {
            d.readBlob(42, &bytetmp);
            m_channelMarker->deserialize(bytetmp);
        }
        d.readS32(44, &m_streamIndex, 0);
        d.readBool(45, &m_useReverseAPI, false);
        d.readString(46, &m_reverseAPIAddress, "127.0.0.1");
        d.readU32(47, &utmp, 0);
        if ((utmp > 1023) && (utmp < 65535)) {
            m_reverseAPIPort = utmp;
        } else {
            m_reverseAPIPort = 8888;
        }
        d.readU32(48, &utmp, 0);
        m_reverseAPIDeviceIndex = utmp > 99 ? 99 : utmp;
        d.readU32(49, &utmp, 0);
        m_reverseAPIChannelIndex = utmp > 99 ? 99 : utmp;
        if (m_rollupState)
        {
            d.readBlob(52, &bytetmp);
            m_rollupState->deserialize(bytetmp);
        }
        d.readS32(53, &m_workspaceIndex, 0);
        d.readBlob(54, &m_geometryBytes);
        d.readBool(55, &m_hidden, false);

        return true;
    }
    else
    {
        resetToDefaults();
        return false;
    }
}

void FreqScannerSettings::applySettings(const QStringList& settingsKeys, const FreqScannerSettings& settings)
{
    if (settingsKeys.contains("inputFrequencyOffset")) {
        m_inputFrequencyOffset = settings.m_inputFrequencyOffset;
    }
    if (settingsKeys.contains("channelBandwidth")) {
        m_channelBandwidth = settings.m_channelBandwidth;
    }
    if (settingsKeys.contains("channelFrequencyOffset")) {
        m_channelFrequencyOffset = settings.m_channelFrequencyOffset;
    }
    if (settingsKeys.contains("threshold")) {
        m_threshold = settings.m_threshold;
    }
    if (settingsKeys.contains("frequencySettings")) {
        m_frequencySettings = settings.m_frequencySettings;
    }
    if (settingsKeys.contains("channel")) {
        m_channel = settings.m_channel;
    }
    if (settingsKeys.contains("scanTime")) {
        m_scanTime = settings.m_scanTime;
    }
    if (settingsKeys.contains("retransmitTime")) {
        m_retransmitTime = settings.m_retransmitTime;
    }
    if (settingsKeys.contains("tuneTime")) {
        m_tuneTime = settings.m_tuneTime;
    }
    if (settingsKeys.contains("priority")) {
        m_priority = settings.m_priority;
    }
    if (settingsKeys.contains("measurement")) {
        m_measurement = settings.m_measurement;
    }
    if (settingsKeys.contains("mode")) {
        m_mode = settings.m_mode;
    }
    if (settingsKeys.contains("columnIndexes")) {
        m_columnIndexes = settings.m_columnIndexes;
    }
    if (settingsKeys.contains("columnSizes")) {
        m_columnSizes = settings.m_columnSizes;
    }
    if (settingsKeys.contains("rgbColor")) {
        m_rgbColor = settings.m_rgbColor;
    }
    if (settingsKeys.contains("title")) {
        m_title = settings.m_title;
    }
    if (settingsKeys.contains("streamIndex")) {
        m_streamIndex = settings.m_streamIndex;
    }
    if (settingsKeys.contains("useReverseAPI")) {
        m_useReverseAPI = settings.m_useReverseAPI;
    }
    if (settingsKeys.contains("reverseAPIAddress")) {
        m_reverseAPIAddress = settings.m_reverseAPIAddress;
    }
    if (settingsKeys.contains("reverseAPIPort")) {
        m_reverseAPIPort = settings.m_reverseAPIPort;
    }
    if (settingsKeys.contains("reverseAPIDeviceIndex")) {
        m_reverseAPIDeviceIndex = settings.m_reverseAPIDeviceIndex;
    }
    if (settingsKeys.contains("reverseAPIChannelIndex")) {
        m_reverseAPIChannelIndex = settings.m_reverseAPIChannelIndex;
    }
    if (settingsKeys.contains("workspaceIndex")) {
        m_workspaceIndex = settings.m_workspaceIndex;
    }
    if (settingsKeys.contains("hidden")) {
        m_hidden = settings.m_hidden;
    }
}

QString FreqScannerSettings::getDebugString(const QStringList& settingsKeys, bool force) const
{
    std::ostringstream ostr;

    if (settingsKeys.contains("inputFrequencyOffset") || force) {
        ostr << " m_inputFrequencyOffset: " << m_inputFrequencyOffset;
    }
    if (settingsKeys.contains("channelBandwidth") || force) {
        ostr << " m_channelBandwidth: " << m_channelBandwidth;
    }
    if (settingsKeys.contains("channelFrequencyOffset") || force) {
        ostr << " m_channelFrequencyOffset: " << m_channelFrequencyOffset;
    }
    if (settingsKeys.contains("threshold") || force) {
        ostr << " m_threshold: " << m_threshold;
    }
    if (settingsKeys.contains("frequencySettings") || force)
    {
        QStringList s;
        for (auto f : m_frequencySettings) {
            s.append(QString::number(f.m_frequency));
        }
        ostr << " m_frequencySettings: " << s.join(",").toStdString();
    }
    if (settingsKeys.contains("channel") || force) {
        ostr << " m_channel: " << m_channel.toStdString();
    }
    if (settingsKeys.contains("scanTime") || force) {
        ostr << " m_scanTime: " << m_scanTime;
    }
    if (settingsKeys.contains("retransmitTime") || force) {
        ostr << " m_retransmitTime: " << m_retransmitTime;
    }
    if (settingsKeys.contains("tuneTime") || force) {
        ostr << " m_tuneTime: " << m_tuneTime;
    }
    if (settingsKeys.contains("priority") || force) {
        ostr << " m_priority: " << m_priority;
    }
    if (settingsKeys.contains("measurement") || force) {
        ostr << " m_measurement: " << m_measurement;
    }
    if (settingsKeys.contains("mode") || force) {
        ostr << " m_mode: " << m_mode;
    }
    if (settingsKeys.contains("columnIndexes") || force) {
        // Don't display
    }
    if (settingsKeys.contains("columnSizes") || force) {
        // Don't display
    }
    if (settingsKeys.contains("rgbColor") || force) {
        ostr << " m_rgbColor: " << m_rgbColor;
    }
    if (settingsKeys.contains("title") || force) {
        ostr << " m_title: " << m_title.toStdString();
    }
    if (settingsKeys.contains("streamIndex") || force) {
        ostr << " m_streamIndex: " << m_streamIndex;
    }
    if (settingsKeys.contains("useReverseAPI") || force) {
        ostr << " m_useReverseAPI: " << m_useReverseAPI;
    }
    if (settingsKeys.contains("reverseAPIAddress") || force) {
        ostr << " m_reverseAPIAddress: " << m_reverseAPIAddress.toStdString();
    }
    if (settingsKeys.contains("reverseAPIPort") || force) {
        ostr << " m_reverseAPIPort: " << m_reverseAPIPort;
    }
    if (settingsKeys.contains("reverseAPIDeviceIndex") || force) {
        ostr << " m_reverseAPIDeviceIndex: " << m_reverseAPIDeviceIndex;
    }
    if (settingsKeys.contains("reverseAPIChannelIndex") || force) {
        ostr << " m_reverseAPIChannelIndex: " << m_reverseAPIChannelIndex;
    }
    if (settingsKeys.contains("workspaceIndex") || force) {
        ostr << " m_workspaceIndex: " << m_workspaceIndex;
    }
    if (settingsKeys.contains("hidden") || force) {
        ostr << " m_hidden: " << m_hidden;
    }

    return QString(ostr.str().c_str());
}


QByteArray FreqScannerSettings::FrequencySettings::serialize() const
{
    SimpleSerializer s(1);

    s.writeS64(1, m_frequency);
    s.writeBool(2, m_enabled);
    s.writeString(3, m_notes);
    s.writeString(4, m_threshold);
    s.writeString(5, m_channel);
    s.writeString(6, m_channelBandwidth);
    s.writeString(7, m_squelch);

    return s.final();
}

bool FreqScannerSettings::FrequencySettings::deserialize(const QByteArray& data)
{
    SimpleDeserializer d(data);

    if (!d.isValid()) {
        return false;
    }

    if (d.getVersion() == 1)
    {
        QByteArray blob;

        d.readS64(1, &m_frequency);
        d.readBool(2, &m_enabled);
        d.readString(3, &m_notes);
        d.readString(4, &m_threshold);
        d.readString(5, &m_channel);
        d.readString(6, &m_channelBandwidth);
        d.readString(7, &m_squelch);

        return true;
    }
    else
    {
        return false;
    }
}

QDataStream& operator<<(QDataStream& out, const FreqScannerSettings::FrequencySettings& settings)
{
    out << settings.serialize();
    return out;
}

QDataStream& operator>>(QDataStream& in, FreqScannerSettings::FrequencySettings& settings)
{
    QByteArray data;
    in >> data;
    settings.deserialize(data);
    return in;
}

QString FreqScannerSettings::getChannel(FreqScannerSettings::FrequencySettings *frequencySettings) const
{
    QString channel = m_channel;
    if (!frequencySettings->m_channel.isEmpty()) {
        channel = frequencySettings->m_channel;
    }
    return channel;
}

Real FreqScannerSettings::getThreshold(FreqScannerSettings::FrequencySettings *frequencySettings) const
{
    Real threshold = m_threshold;
    if (!frequencySettings->m_threshold.isEmpty())
    {
        bool ok;
        Real perFrequencyThreshold = frequencySettings->m_threshold.toFloat(&ok);
        if (ok) {
            threshold = perFrequencyThreshold;
        } else {
            qDebug() << "FreqScannerSettings::getThreshold: Failed to parse" << frequencySettings->m_threshold << "as a float";
        }
    }
    return threshold;
}

int FreqScannerSettings::getChannelBandwidth(FreqScannerSettings::FrequencySettings *frequencySettings) const
{
    int channelBandwidth = m_channelBandwidth;
    if (!frequencySettings->m_channelBandwidth.isEmpty())
    {
        bool ok;
        Real perFrequencyChannelBandwidth = frequencySettings->m_channelBandwidth.toInt(&ok);
        if (ok) {
            channelBandwidth = perFrequencyChannelBandwidth;
        } else {
            qDebug() << "FreqScannerSettings::getChannelBandwidth: Failed to parse" << frequencySettings->m_channelBandwidth << "as an int";
        }
    }
    return channelBandwidth;
}

FreqScannerSettings::FrequencySettings *FreqScannerSettings::getFrequencySettings(qint64 frequency)
{
    for (int i = 0; i < m_frequencySettings.size(); i++)
    {
        if (frequency == m_frequencySettings[i].m_frequency) {
            return &this->m_frequencySettings[i];
        }
    }
    return nullptr;
}

