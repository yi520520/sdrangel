///////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2012 maintech GmbH, Otto-Hahn-Str. 15, 97204 Hoechberg, Germany //
// written by Christian Daniel                                                   //
// Copyright (C) 2015-2020, 2022 Edouard Griffiths, F4EXB <f4exb06@gmail.com>    //
// Copyright (C) 2020, 2022 Jon Beniston, M7RCE <jon@beniston.com>               //
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

#ifndef _AUDIOOUTPUT_AUDIOOUTPUTSETTINGS_H_
#define _AUDIOOUTPUT_AUDIOOUTPUTSETTINGS_H_

#include <QString>
#include "audio/audiodeviceinfo.h"

struct AudioOutputSettings {

    QString m_deviceName;       // Including realm, as from getFullDeviceName below
    float m_volume;
    enum IQMapping {
        LR,
        RL
    } m_iqMapping;

    bool     m_useReverseAPI;
    QString  m_reverseAPIAddress;
    uint16_t m_reverseAPIPort;
    uint16_t m_reverseAPIDeviceIndex;

    AudioOutputSettings();
    void resetToDefaults();
    QByteArray serialize() const;
    bool deserialize(const QByteArray& data);
    void applySettings(const QStringList& settingsKeys, const AudioOutputSettings& settings);
    QString getDebugString(const QStringList& settingsKeys, bool force=false) const;

    // Append realm to device names, because there may be multiple devices with the same name on Windows
    static QString getFullDeviceName(const AudioDeviceInfo &deviceInfo)
    {
        QString realm = deviceInfo.realm();
        if (realm != "" && realm != "default" && realm != "alsa")
            return deviceInfo.deviceName() + " " + realm;
        else
            return deviceInfo.deviceName();
    }
};

#endif /* _AUDIOOUTPUT_AUDIOOUTPUTSETTINGS_H_ */
