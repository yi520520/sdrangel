///////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2012 maintech GmbH, Otto-Hahn-Str. 15, 97204 Hoechberg, Germany //
// written by Christian Daniel                                                   //
// Copyright (C) 2015-2020, 2022 Edouard Griffiths, F4EXB <f4exb06@gmail.com>    //
// Copyright (C) 2021 Jon Beniston, M7RCE <jon@beniston.com>                     //
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

#ifndef INCLUDE_FEATURE_DEMODANALYZERSETTINGS_H_
#define INCLUDE_FEATURE_DEMODANALYZERSETTINGS_H_

#include <QByteArray>
#include <QString>

class Serializable;
class ChannelAPI;

struct DemodAnalyzerSettings
{
    int m_log2Decim;
    QString m_title;
    quint32 m_rgbColor;
    bool m_useReverseAPI;
    QString m_reverseAPIAddress;
    uint16_t m_reverseAPIPort;
    uint16_t m_reverseAPIFeatureSetIndex;
    uint16_t m_reverseAPIFeatureIndex;
    QString m_fileRecordName;
    bool m_recordToFile;
    int m_recordSilenceTime; //!< 100's ms
    Serializable *m_spectrumGUI;
    Serializable *m_scopeGUI;
    Serializable *m_rollupState;
    int m_workspaceIndex;
    QByteArray m_geometryBytes;

    DemodAnalyzerSettings();
    void resetToDefaults();
    QByteArray serialize() const;
    bool deserialize(const QByteArray& data);
    void setSpectrumGUI(Serializable *spectrumGUI) { m_spectrumGUI = spectrumGUI; }
    void setScopeGUI(Serializable *scopeGUI) { m_scopeGUI = scopeGUI; }
    void setRollupState(Serializable *rollupState) { m_rollupState = rollupState; }
    void applySettings(const QStringList& settingsKeys, const DemodAnalyzerSettings& settings);
    QString getDebugString(const QStringList& settingsKeys, bool force=false) const;

    static const QStringList m_channelURIs;
};

#endif // INCLUDE_FEATURE_DEMODANALYZERSETTINGS_H_
