///////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2019-2020, 2022 Edouard Griffiths, F4EXB <f4exb06@gmail.com>    //
// Copyright (C) 2020-2023 Jon Beniston, M7RCE <jon@beniston.com>                //
// Copyright (C) 2020 Vort <vvort@yandex.ru>                                     //
// Copyright (C) 2022 Jiří Pinkava <jiri.pinkava@rossum.ai>                      //
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

#ifndef INCLUDE_FEATURE_GS232CONTROLLERWORKER_H_
#define INCLUDE_FEATURE_GS232CONTROLLERWORKER_H_

#include <QObject>
#include <QTimer>
#include <QSerialPort>
#include <QTcpSocket>

#include "util/message.h"
#include "util/messagequeue.h"

#include "gs232controllersettings.h"
#include "controllerprotocol.h"

class GS232Controller;

class GS232ControllerWorker : public QObject
{
    Q_OBJECT
public:
    class MsgConfigureGS232ControllerWorker : public Message {
        MESSAGE_CLASS_DECLARATION

    public:
        const GS232ControllerSettings& getSettings() const { return m_settings; }
        const QList<QString>& getSettingsKeys() const { return m_settingsKeys; }
        bool getForce() const { return m_force; }

        static MsgConfigureGS232ControllerWorker* create(const GS232ControllerSettings& settings, const QList<QString>& settingsKeys, bool force)
        {
            return new MsgConfigureGS232ControllerWorker(settings, settingsKeys, force);
        }

    private:
        GS232ControllerSettings m_settings;
        QList<QString> m_settingsKeys;
        bool m_force;

        MsgConfigureGS232ControllerWorker(const GS232ControllerSettings& settings, const QList<QString>& settingsKeys, bool force) :
            Message(),
            m_settings(settings),
            m_settingsKeys(settingsKeys),
            m_force(force)
        { }
    };

    GS232ControllerWorker(GS232Controller *controller);
    ~GS232ControllerWorker();
    void startWork();
    void stopWork();
    bool isRunning() const { return m_running; }
    MessageQueue *getInputMessageQueue() { return &m_inputMessageQueue; }
    void setMessageQueueToFeature(MessageQueue *messageQueue) { m_msgQueueToFeature = messageQueue; }

private:

    GS232Controller *m_controller;
    MessageQueue m_inputMessageQueue;  //!< Queue for asynchronous inbound communication
    MessageQueue *m_msgQueueToFeature; //!< Queue to report channel change to main feature object
    GS232ControllerSettings m_settings;
    bool m_running;
    QIODevice *m_device;
    QSerialPort m_serialPort;
    QTcpSocket m_socket;
    QTimer m_pollTimer;

    float m_lastAzimuth;
    float m_lastElevation;

    ControllerProtocol *m_controllerProtocol;

    bool handleMessage(const Message& cmd);
    void applySettings(const GS232ControllerSettings& settings, const QList<QString>& settingsKeys, bool force = false);
    QIODevice *openSerialPort(const GS232ControllerSettings& settings);
    QIODevice *openSocket(const GS232ControllerSettings& settings);
    void setAzimuth(float azimuth);
    void setAzimuthElevation(float azimuth, float elevation);
    void sendToSkyMap(float azimuth, float elevation);

private slots:
    void handleInputMessages();
    void readData();
    void update();
};

#endif // INCLUDE_FEATURE_GS232CONTROLLERWORKER_H_

