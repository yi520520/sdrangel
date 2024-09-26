///////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2012 maintech GmbH, Otto-Hahn-Str. 15, 97204 Hoechberg, Germany //
// written by Christian Daniel                                                   //
// Copyright (C) 2015-2020, 2022-2023 Edouard Griffiths, F4EXB <f4exb06@gmail.com> //
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

#ifndef _AUDIOINPUT_AUDIOINPUTSETTINGS_H_
#define _AUDIOINPUT_AUDIOINPUTSETTINGS_H_

#include <QString>
#include "audio/audiodeviceinfo.h"

struct AudioInputSettings {

	typedef enum {
		FC_POS_INFRA = 0,
		FC_POS_SUPRA,
		FC_POS_CENTER
	} fcPos_t;

    QString m_deviceName;       // Including realm, as from getFullDeviceName below
    int m_sampleRate;
    float m_volume;
    quint32 m_log2Decim;
    enum IQMapping {
        L,
        R,
        LR,
        RL
    } m_iqMapping;
	bool m_dcBlock;
	bool m_iqImbalance;
    fcPos_t m_fcPos;

    bool     m_useReverseAPI;
    QString  m_reverseAPIAddress;
    uint16_t m_reverseAPIPort;
    uint16_t m_reverseAPIDeviceIndex;

    AudioInputSettings();
    void resetToDefaults();
    QByteArray serialize() const;
    bool deserialize(const QByteArray& data);

    // Append realm to device names, because there may be multiple devices with the same name on Windows
    static QString getFullDeviceName(const AudioDeviceInfo &deviceInfo)
    {
        QString realm = deviceInfo.realm();
        if (realm != "" && realm != "default" && realm != "alsa")
            return deviceInfo.deviceName() + " " + realm;
        else
            return deviceInfo.deviceName();
    }
    void applySettings(const QStringList& settingsKeys, const AudioInputSettings& settings);
    QString getDebugString(const QStringList& settingsKeys, bool force=false) const;
};

#endif /* _AUDIOINPUT_AUDIOINPUTSETTINGS_H_ */
