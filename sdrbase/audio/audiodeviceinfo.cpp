///////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2012 maintech GmbH, Otto-Hahn-Str. 15, 97204 Hoechberg, Germany //
// written by Christian Daniel                                                   //
// Copyright (C) 2017-2018 Edouard Griffiths, F4EXB <f4exb06@gmail.com>          //
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

#include "audiodeviceinfo.h"

bool inputDevicesEnumerated = false, outputDevicesEnumerated = false;
QList<AudioDeviceInfo> inputDevices, outputDevices;
AudioDeviceInfo defaultInputDevice_, defaultOutputDevice_;

QString AudioDeviceInfo::deviceName() const
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    return m_deviceInfo.description();
#else
    return m_deviceInfo.deviceName();
#endif
}

bool AudioDeviceInfo::isFormatSupported(const QAudioFormat &settings) const
{
    return m_deviceInfo.isFormatSupported(settings);
}

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
QList<int> AudioDeviceInfo::supportedSampleRates() const
{
    // QAudioDevice is a bit more flexible than QAudioDeviceInfo, in that it supports
    // min and max rate, rather than a specific list
    // For now, we just list some common rates.
    QList<int> sampleRates = {8000, 11025, 22050, 44100, 48000, 96000, 192000};
    QList<int> supportedRates;
    for (auto sampleRate : sampleRates)
    {
        if ((sampleRate <= m_deviceInfo.maximumSampleRate()) && (sampleRate >= m_deviceInfo.minimumSampleRate())) {
            supportedRates.append(sampleRate);
        }
    }
    return supportedRates;
}
#else
QList<int> AudioDeviceInfo::supportedSampleRates() const
{
    return m_deviceInfo.supportedSampleRates();
}
#endif

QString AudioDeviceInfo::realm() const
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    return ""; // Don't appear to have realms in Qt6
#else
    return m_deviceInfo.realm();
#endif
}

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
const QList<AudioDeviceInfo> &AudioDeviceInfo::availableInputDevices()
{
    if (!inputDevicesEnumerated) {
        QList<QAudioDevice> devInfos = QMediaDevices::audioInputs();
        for (auto devInfo : devInfos) {
            inputDevices.append(AudioDeviceInfo(devInfo));
        }
        inputDevicesEnumerated = true;
    }

    return inputDevices;
}

const QList<AudioDeviceInfo> &AudioDeviceInfo::availableOutputDevices()
{
    if (!outputDevicesEnumerated) {
        QList<QAudioDevice> devInfos = QMediaDevices::audioOutputs();
        for (auto devInfo : devInfos) {
            outputDevices.append(AudioDeviceInfo(devInfo));
        }
        outputDevicesEnumerated = true;
    }

    return outputDevices;
}
#else
const QList<AudioDeviceInfo> &AudioDeviceInfo::availableInputDevices()
{
    if (!inputDevicesEnumerated) {
        QList<QAudioDeviceInfo> devInfos = QAudioDeviceInfo::availableDevices(QAudio::AudioInput);
        for (auto devInfo : devInfos) {
            inputDevices.append(AudioDeviceInfo(devInfo));
        }
        inputDevicesEnumerated = true;
    }

    return inputDevices;
}

const QList<AudioDeviceInfo> &AudioDeviceInfo::availableOutputDevices()
{
    if (!outputDevicesEnumerated) {
        QList<QAudioDeviceInfo> devInfos = QAudioDeviceInfo::availableDevices(QAudio::AudioOutput);
        for (auto devInfo : devInfos) {
            outputDevices.append(AudioDeviceInfo(devInfo));
        }
        outputDevicesEnumerated = true;
    }

    return outputDevices;
}
#endif

const AudioDeviceInfo &AudioDeviceInfo::defaultOutputDevice()
{
    if (defaultOutputDevice_.m_deviceInfo.isNull()) 
    {
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        defaultOutputDevice_ = AudioDeviceInfo(QMediaDevices::defaultAudioOutput());
#else
        defaultOutputDevice_ = AudioDeviceInfo(QAudioDeviceInfo::defaultOutputDevice());
#endif
    }
    return defaultOutputDevice_;
}

const AudioDeviceInfo &AudioDeviceInfo::defaultInputDevice()
{
    if (defaultInputDevice_.m_deviceInfo.isNull()) 
    {
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        defaultInputDevice_ = AudioDeviceInfo(QMediaDevices::defaultAudioInput());
#else
        defaultInputDevice_ = AudioDeviceInfo(QAudioDeviceInfo::defaultInputDevice());
#endif
    }
    return defaultInputDevice_;
}
