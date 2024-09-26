///////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2012 maintech GmbH, Otto-Hahn-Str. 15, 97204 Hoechberg, Germany //
// written by Christian Daniel                                                   //
// Copyright (C) 2015-2022 Edouard Griffiths, F4EXB <f4exb06@gmail.com>          //
// Copyright (C) 2015 John Greb <hexameron@spam.no>                              //
// Copyright (C) 2020-2023 Jon Beniston, M7RCE <jon@beniston.com>                //
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

#ifndef INCLUDE_FEATURE_GS232CONTROLLERGUI_H_
#define INCLUDE_FEATURE_GS232CONTROLLERGUI_H_

#include <QTimer>

#include "feature/featuregui.h"
#include "util/messagequeue.h"
#include "settings/rollupstate.h"

#include "gs232controllersettings.h"
#include "dfmstatusdialog.h"
#include "inputcontroller.h"

class PluginAPI;
class FeatureUISet;
class GS232Controller;

namespace Ui {
    class GS232ControllerGUI;
}

class GS232ControllerGUI : public FeatureGUI {
    Q_OBJECT
public:
    static GS232ControllerGUI* create(PluginAPI* pluginAPI, FeatureUISet *featureUISet, Feature *feature);
    virtual void destroy();

    void resetToDefaults();
    QByteArray serialize() const;
    bool deserialize(const QByteArray& data);
    virtual MessageQueue *getInputMessageQueue() { return &m_inputMessageQueue; }
    virtual void setWorkspaceIndex(int index);
    virtual int getWorkspaceIndex() const { return m_settings.m_workspaceIndex; }
    virtual void setGeometryBytes(const QByteArray& blob) { m_settings.m_geometryBytes = blob; }
    virtual QByteArray getGeometryBytes() const { return m_settings.m_geometryBytes; }

private:
    Ui::GS232ControllerGUI* ui;
    PluginAPI* m_pluginAPI;
    FeatureUISet* m_featureUISet;
    GS232ControllerSettings m_settings;
    QList<QString> m_settingsKeys;
    RollupState m_rollupState;
    bool m_doApplySettings;

    GS232Controller* m_gs232Controller;
    MessageQueue m_inputMessageQueue;
    QTimer m_statusTimer;
    int m_lastFeatureState;
    bool m_lastOnTarget;

    DFMStatusDialog m_dfmStatusDialog;

    InputController *m_inputController;
    QTimer m_inputTimer;
    double m_inputCoord1;
    double m_inputCoord2;
    double m_inputAzOffset;
    double m_inputElOffset;
    bool m_inputUpdate;
    static const int m_extraSensitivity = 4.0f; // Otherwise 100% isn't quite fast enough

    explicit GS232ControllerGUI(PluginAPI* pluginAPI, FeatureUISet *featureUISet, Feature *feature, QWidget* parent = nullptr);
    virtual ~GS232ControllerGUI();

    void blockApplySettings(bool block);
    void applySetting(const QString& settingsKey);
    void applySettings(const QStringList& settingsKeys, bool force = false);
    void applyAllSettings();
    void displaySettings();
    void setProtocol(GS232ControllerSettings::Protocol protocol);
    void setPrecision();
    void updateConnectionWidgets();
    void updatePipeList(const AvailableChannelOrFeatureList& sources, const QStringList& renameFrom, const QStringList& renameTo);
    void updateSerialPortList();
    void updateSerialPortList(const QStringList& serialPorts);
    bool handleMessage(const Message& message);
    void makeUIConnections();
    void azElToDisplay(float az, float el, float& coord1, float& coord2) const;
    void displayToAzEl(float coord1, float coord2);
    void updateInputController();

private slots:
    void onMenuDialogCalled(const QPoint &p);
    void onWidgetRolled(QWidget* widget, bool rollDown);
    void handleInputMessages();
    void on_startStop_toggled(bool checked);
    void on_protocol_currentIndexChanged(int index);
    void on_connection_currentIndexChanged(int index);
    void on_serialPort_currentIndexChanged(int index);
    void on_host_editingFinished();
    void on_port_valueChanged(int value);
    void on_baudRate_currentIndexChanged(int index);
    void on_track_stateChanged(int state);
    void on_coord1_valueChanged(double value);
    void on_coord2_valueChanged(double value);
    void on_sources_currentTextChanged(const QString& text);
    void on_azimuthOffset_valueChanged(double value);
    void on_elevationOffset_valueChanged(double value);
    void on_azimuthMin_valueChanged(int value);
    void on_azimuthMax_valueChanged(int value);
    void on_elevationMin_valueChanged(int value);
    void on_elevationMax_valueChanged(int value);
    void on_tolerance_valueChanged(double value);
    void on_precision_valueChanged(int value);
    void on_coordinates_currentIndexChanged(int index);
    void on_dfmTrack_clicked(bool checked=false);
    void on_dfmLubePumps_clicked(bool checked=false);
    void on_dfmBrakes_clicked(bool checked=false);
    void on_dfmDrives_clicked(bool checked=false);
    void on_dfmShowStatus_clicked();
    void updateStatus();
    void on_inputController_currentIndexChanged(int index);
    void on_inputConfigure_clicked();
    void on_highSensitivity_clicked(bool checked);
    void on_enableTargetControl_clicked(bool checked);
    void on_enableOffsetControl_clicked(bool checked);
    void updateInputControllerList();
    void checkInputController();
    void buttonChanged(int button, bool released);
    void inputConfigurationComplete();
};

#endif // INCLUDE_FEATURE_GS232CONTROLLERGUI_H_

