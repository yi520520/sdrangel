///////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2022 Edouard Griffiths, F4EXB                                   //
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

#ifndef INCLUDE_FEATURE_LIMERFE_H_
#define INCLUDE_FEATURE_LIMERFE_H_

#include <QNetworkRequest>

#include <string>
#include <map>
#include "lime/limeRFE.h"

#include "feature/feature.h"
#include "util/message.h"

#include "limerfesettings.h"
#include "limerfeusbcalib.h"

class QNetworkReply;
class QNetworkAccessManager;

class LimeRFE : public Feature
{
	Q_OBJECT
public:
    class MsgConfigureLimeRFE : public Message {
        MESSAGE_CLASS_DECLARATION

    public:
        const LimeRFESettings& getSettings() const { return m_settings; }
        bool getForce() const { return m_force; }

        static MsgConfigureLimeRFE* create(const LimeRFESettings& settings, bool force) {
            return new MsgConfigureLimeRFE(settings, force);
        }

    private:
        LimeRFESettings m_settings;
        bool m_force;

        MsgConfigureLimeRFE(const LimeRFESettings& settings, bool force) :
            Message(),
            m_settings(settings),
            m_force(force)
        { }
    };

    LimeRFE(WebAPIAdapterInterface *webAPIAdapterInterface);
    virtual ~LimeRFE();
    virtual void destroy() { delete this; }
    virtual bool handleMessage(const Message& cmd);

    virtual void getIdentifier(QString& id) const { id = objectName(); }
    virtual QString getIdentifier() const { return objectName(); }
    virtual void getTitle(QString& title) const { title = m_settings.m_title; }

    virtual QByteArray serialize() const;
    virtual bool deserialize(const QByteArray& data);

    LimeRFEUSBCalib *getCalib() { return &m_calib; }

    int openDevice(const std::string& serialDeviceName);
    void closeDevice();

    const QStringList& getComPorts() { return m_comPorts; }
    int configure();
    int getState();
    static std::string getError(int errorCode);
    int setRx(LimeRFESettings& settings, bool rxOn);
    int setTx(LimeRFESettings& settings, bool txOn);
    bool turnDevice(int deviceSetIndex, bool on);
    int getFwdPower(int& powerDB);
    int getRefPower(int& powerDB);

    void settingsToState(const LimeRFESettings& settings);
    void stateToSettings(LimeRFESettings& settings);

    static const char* const m_featureIdURI;
    static const char* const m_featureId;

private:
    LimeRFESettings m_settings;
    LimeRFEUSBCalib m_calib;

    QNetworkAccessManager *m_networkManager;
    QNetworkRequest m_networkRequest;
    WebAPIAdapterInterface *m_webAPIAdapterInterface;

    rfe_dev_t *m_rfeDevice;
    rfe_boardState m_rfeBoardState;
    static const std::map<int, std::string> m_errorCodesMap;
    QStringList m_comPorts;

    void start();
    void stop();
    void listComPorts();

private slots:
    void networkManagerFinished(QNetworkReply *reply);

};

#endif
