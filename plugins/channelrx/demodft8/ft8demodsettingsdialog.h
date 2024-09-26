///////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2012 maintech GmbH, Otto-Hahn-Str. 15, 97204 Hoechberg, Germany //
// written by Christian Daniel                                                   //
// Copyright (C) 2015-2019, 2023 Edouard Griffiths, F4EXB <f4exb06@gmail.com>    //
// Copyright (C) 2021 Jon Beniston, M7RCE <jon@beniston.com>                     //
// Copyright (C) 2023 Mohamed <mohamedadlyi@github.com>                          //
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

#ifndef PLUGINS_CHANNELRX_DEMODFT8_FT8DEMODSETTINGSDIALOG_H_
#define PLUGINS_CHANNELRX_DEMODFT8_FT8DEMODSETTINGSDIALOG_H_

#include "ui_ft8demodsettingsdialog.h"
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
#include <QStringList>
#else
class QStringList;
#endif
class FT8DemodSettings;
class QTableWidgetItem;

class FT8DemodSettingsDialog : public QDialog {
    Q_OBJECT
public:
    FT8DemodSettingsDialog(FT8DemodSettings& settings, QStringList& settingsKeys, QWidget* parent = nullptr);
    ~FT8DemodSettingsDialog();

private:
    Ui::FT8DemodSettingsDialog* ui;
    FT8DemodSettings& m_settings;
    QStringList& m_settingsKeys;

    enum BandCol {
        BAND_NAME,
        BAND_BASE_FREQUENCY,
        BAND_OFFSET_FREQUENCY,
    };

    void resizeBandsTable();
    void populateBandsTable();
    QList<QTableWidgetItem*> takeRow(int row);
    void setRow(int row, const QList<QTableWidgetItem*>& rowItems);

private slots:
    void accept();
    void reject();
    void on_decoderNbThreads_valueChanged(int value);
    void on_decoderTimeBudget_valueChanged(double value);
    void on_osdEnable_toggled(bool checked);
    void on_osdDepth_valueChanged(int value);
    void on_osdLDPCThreshold_valueChanged(int value);
    void on_verifyOSD_stateChanged(int state);
    void on_addBand_clicked();
    void on_deleteBand_clicked();
    void on_moveBandUp_clicked();
    void on_moveBandDown_clicked();
    void on_restoreBandPresets_clicked();
    void textCellChanged(int row, int col);
    void baseFrequencyCellChanged();
    void offsetFrequencyCellChanged();
};


#endif // PLUGINS_CHANNELRX_DEMODFT8_FT8DEMODSETTINGSDIALOG_H_
