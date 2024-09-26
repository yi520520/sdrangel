///////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2016-2022 Edouard Griffiths, F4EXB <f4exb06@gmail.com>          //
// Copyright (C) 2020-2022 Jon Beniston, M7RCE <jon@beniston.com>                //
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

#ifndef INCLUDE_IEEE_802_15_4_MODGUI_H
#define INCLUDE_IEEE_802_15_4_MODGUI_H

#include "channel/channelgui.h"
#include "dsp/channelmarker.h"
#include "util/movingaverage.h"
#include "util/messagequeue.h"
#include "settings/rollupstate.h"

#include "ieee_802_15_4_mod.h"
#include "ieee_802_15_4_modsettings.h"

class PluginAPI;
class DeviceUISet;
class BasebandSampleSource;
class SpectrumVis;
class ScopeVis;

namespace Ui {
    class IEEE_802_15_4_ModGUI;
}

class IEEE_802_15_4_ModGUI : public ChannelGUI {
    Q_OBJECT

public:
    static IEEE_802_15_4_ModGUI* create(PluginAPI* pluginAPI, DeviceUISet *deviceUISet, BasebandSampleSource *channelTx);
    virtual void destroy();

    void setName(const QString& name);
    QString getName() const;
    virtual qint64 getCenterFrequency() const;
    virtual void setCenterFrequency(qint64 centerFrequency);

    void resetToDefaults();
    QByteArray serialize() const;
    bool deserialize(const QByteArray& data);
    virtual MessageQueue *getInputMessageQueue() { return &m_inputMessageQueue; }
    virtual void setWorkspaceIndex(int index) { m_settings.m_workspaceIndex = index; };
    virtual int getWorkspaceIndex() const { return m_settings.m_workspaceIndex; };
    virtual void setGeometryBytes(const QByteArray& blob) { m_settings.m_geometryBytes = blob; };
    virtual QByteArray getGeometryBytes() const { return m_settings.m_geometryBytes; };
    virtual QString getTitle() const { return m_settings.m_title; };
    virtual QColor getTitleColor() const  { return m_settings.m_rgbColor; };
    virtual void zetHidden(bool hidden) { m_settings.m_hidden = hidden; }
    virtual bool getHidden() const { return m_settings.m_hidden; }
    virtual ChannelMarker& getChannelMarker() { return m_channelMarker; }
    virtual int getStreamIndex() const { return m_settings.m_streamIndex; }
    virtual void setStreamIndex(int streamIndex) { m_settings.m_streamIndex = streamIndex; }

public slots:
    void channelMarkerChangedByCursor();

private:
    Ui::IEEE_802_15_4_ModGUI* ui;
    PluginAPI* m_pluginAPI;
    DeviceUISet* m_deviceUISet;
    ChannelMarker m_channelMarker;
    RollupState m_rollupState;
    IEEE_802_15_4_ModSettings m_settings;
    qint64 m_deviceCenterFrequency;
    bool m_doApplySettings;
    SpectrumVis* m_spectrumVis;
    ScopeVis* m_scopeVis;
    int m_basebandSampleRate;

    IEEE_802_15_4_Mod* m_IEEE_802_15_4_Mod;
    MovingAverageUtil<double, double, 2> m_channelPowerDbAvg; // Less than other mods, as frames are short

    MessageQueue m_inputMessageQueue;

    explicit IEEE_802_15_4_ModGUI(PluginAPI* pluginAPI, DeviceUISet *deviceUISet, BasebandSampleSource *channelTx, QWidget* parent = 0);
    virtual ~IEEE_802_15_4_ModGUI();

    void checkSampleRate();
    void transmit();
    void blockApplySettings(bool block);
    void applySettings(bool force = false);
    void displaySettings();
    void displayRFBandwidth(int bandwidth);
    void displayChipRate(const IEEE_802_15_4_ModSettings& settings);
    QString getDisplayValueWithMultiplier(int value);
    bool handleMessage(const Message& message);
    void makeUIConnections();
    void updateAbsoluteCenterFrequency();

    void leaveEvent(QEvent*);
    void enterEvent(EnterEventType*);

private slots:
    void handleSourceMessages();

    void on_deltaFrequency_changed(qint64 value);
    void on_phy_currentIndexChanged(int value);
    void on_rfBW_valueChanged(int index);
    void on_gain_valueChanged(int value);
    void on_channelMute_toggled(bool checked);
    void on_txButton_clicked();
    void on_frame_editingFinished();
    void on_frame_returnPressed();
    void on_repeat_toggled(bool checked);
    void repeatSelect(const QPoint& p);
    void txSettingsSelect(const QPoint& p);
    void on_udpEnabled_clicked(bool checked);
    void on_udpAddress_editingFinished();
    void on_udpPort_editingFinished();

    void onWidgetRolled(QWidget* widget, bool rollDown);
    void onMenuDialogCalled(const QPoint& p);

    void tick();
};

#endif /* INCLUDE_IEEE_802_15_4_MODGUI_H */
