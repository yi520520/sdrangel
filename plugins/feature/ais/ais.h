///////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2020, 2022 Edouard Griffiths, F4EXB <f4exb06@gmail.com>         //
// Copyright (C) 2020 Kacper Michajłow <kasper93@gmail.com>                      //
// Copyright (C) 2021-2024 Jon Beniston, M7RCE <jon@beniston.com>                //
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

#ifndef INCLUDE_FEATURE_AIS_H_
#define INCLUDE_FEATURE_AIS_H_

#include <QNetworkRequest>
#include <QSet>

#include "feature/feature.h"
#include "util/message.h"
#include "availablechannelorfeaturehandler.h"

#include "aissettings.h"

class WebAPIAdapterInterface;
class QNetworkAccessManager;
class QNetworkReply;

namespace SWGSDRangel {
    class SWGDeviceState;
}

class AIS : public Feature
{
	Q_OBJECT
public:
    class MsgConfigureAIS : public Message {
        MESSAGE_CLASS_DECLARATION

    public:
        const AISSettings& getSettings() const { return m_settings; }
        const QList<QString>& getSettingsKeys() const { return m_settingsKeys; }
        bool getForce() const { return m_force; }

        static MsgConfigureAIS* create(const AISSettings& settings, const QList<QString>& settingsKeys, bool force) {
            return new MsgConfigureAIS(settings, settingsKeys, force);
        }

    private:
        AISSettings m_settings;
        QList<QString> m_settingsKeys;
        bool m_force;

        MsgConfigureAIS(const AISSettings& settings, const QList<QString>& settingsKeys, bool force) :
            Message(),
            m_settings(settings),
            m_settingsKeys(settingsKeys),
            m_force(force)
        { }
    };

    AIS(WebAPIAdapterInterface *webAPIAdapterInterface);
    virtual ~AIS();
    virtual void destroy() { delete this; }
    virtual bool handleMessage(const Message& cmd);

    virtual void getIdentifier(QString& id) const { id = objectName(); }
    virtual QString getIdentifier() const { return objectName(); }
    virtual void getTitle(QString& title) const { title = m_settings.m_title; }

    virtual QByteArray serialize() const;
    virtual bool deserialize(const QByteArray& data);

    virtual int webapiSettingsGet(
            SWGSDRangel::SWGFeatureSettings& response,
            QString& errorMessage);

    virtual int webapiSettingsPutPatch(
            bool force,
            const QStringList& featureSettingsKeys,
            SWGSDRangel::SWGFeatureSettings& response,
            QString& errorMessage);

    static void webapiFormatFeatureSettings(
        SWGSDRangel::SWGFeatureSettings& response,
        const AISSettings& settings);

    static void webapiUpdateFeatureSettings(
            AISSettings& settings,
            const QStringList& featureSettingsKeys,
            SWGSDRangel::SWGFeatureSettings& response);

    static const char* const m_featureIdURI;
    static const char* const m_featureId;

private:
    AISSettings m_settings;
    AvailableChannelOrFeatureHandler m_availableChannelHandler;
    AvailableChannelOrFeatureList m_availableChannels;

    QNetworkAccessManager *m_networkManager;
    QNetworkRequest m_networkRequest;

    void start();
    void stop();
    void applySettings(const AISSettings& settings, const QList<QString>& settingsKeys, bool force = false);
    void webapiReverseSendSettings(const QList<QString>& featureSettingsKeys, const AISSettings& settings, bool force);

private slots:
    void networkManagerFinished(QNetworkReply *reply);
    void handleChannelMessageQueue(MessageQueue* messageQueue);
};

#endif // INCLUDE_FEATURE_AIS_H_
