///////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2012 maintech GmbH, Otto-Hahn-Str. 15, 97204 Hoechberg, Germany //
// written by Christian Daniel                                                   //
// Copyright (C) 2014 John Greb <hexameron@spam.no>                              //
// Copyright (C) 2015-2020, 2022 Edouard Griffiths, F4EXB <f4exb06@gmail.com>    //
// Copyright (C) 2021 FuzzyCheese <23639418+FuzzyCheese@users.noreply.github.com> //
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

#ifndef _HACKRF_HACKRFINPUTSETTINGS_H_
#define _HACKRF_HACKRFINPUTSETTINGS_H_

#include <QtGlobal>
#include <QString>

struct HackRFInputSettings {
	typedef enum {
		FC_POS_INFRA = 0,
		FC_POS_SUPRA,
		FC_POS_CENTER
	} fcPos_t;

	quint64 m_centerFrequency;
	qint32  m_LOppmTenths;
	quint32 m_bandwidth;
	quint32 m_lnaGain;
	quint32 m_vgaGain;
	quint32 m_log2Decim;
	fcPos_t m_fcPos;
	quint64 m_devSampleRate;
	bool m_biasT;
	bool m_lnaExt;
	bool m_dcBlock;
	bool m_iqCorrection;
	bool m_autoBBF;
    bool   m_transverterMode;
	qint64 m_transverterDeltaFrequency;
    bool m_iqOrder;
    bool     m_useReverseAPI;
    QString  m_reverseAPIAddress;
    uint16_t m_reverseAPIPort;
    uint16_t m_reverseAPIDeviceIndex;

	HackRFInputSettings();
	void resetToDefaults();
	QByteArray serialize() const;
	bool deserialize(const QByteArray& data);
    void applySettings(const QStringList& settingsKeys, const HackRFInputSettings& settings);
    QString getDebugString(const QStringList& settingsKeys, bool force=false) const;
};

#endif /* _HACKRF_HACKRFINPUTSETTINGS_H_ */
