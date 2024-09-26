///////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2020, 2022 Edouard Griffiths, F4EXB <f4exb06@gmail.com>         //
// Copyright (C) 2020 Kacper Michajłow <kasper93@gmail.com>                      //
// Copyright (C) 2021-2024 Jon Beniston, M7RCE <jon@beniston.com>                //
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

#ifndef INCLUDE_FEATURE_MAP_H_
#define INCLUDE_FEATURE_MAP_H_

#include <QHash>
#include <QNetworkRequest>
#include <QDateTime>
#include <QRecursiveMutex>

#include "feature/feature.h"
#include "util/message.h"
#include "availablechannelorfeaturehandler.h"

#include "mapsettings.h"

class WebAPIAdapterInterface;
class QNetworkAccessManager;
class QNetworkReply;

namespace SWGSDRangel {
    class SWGDeviceState;
}

class Map : public Feature
{
    Q_OBJECT
public:
    class MsgConfigureMap : public Message {
        MESSAGE_CLASS_DECLARATION

    public:
        const MapSettings& getSettings() const { return m_settings; }
        const QList<QString>& getSettingsKeys() const { return m_settingsKeys; }
        bool getForce() const { return m_force; }

        static MsgConfigureMap* create(const MapSettings& settings, const QList<QString>& settingsKeys, bool force) {
            return new MsgConfigureMap(settings, settingsKeys, force);
        }

    private:
        MapSettings m_settings;
        QList<QString> m_settingsKeys;
        bool m_force;

        MsgConfigureMap(const MapSettings& settings, const QList<QString>& settingsKeys, bool force) :
            Message(),
            m_settings(settings),
            m_settingsKeys(settingsKeys),
            m_force(force)
        { }
    };

    class MsgFind : public Message {
        MESSAGE_CLASS_DECLARATION

    public:
        QString getTarget() const { return m_target; }

        static MsgFind* create(const QString& target) {
            return new MsgFind(target);
        }

    private:
        QString m_target;

        MsgFind(const QString& target) :
            Message(),
            m_target(target)
        {}
    };

    class MsgSetDateTime : public Message {
        MESSAGE_CLASS_DECLARATION

    public:
        QDateTime getDateTime() const { return m_dateTime; }

        static MsgSetDateTime* create(const QDateTime& dateTime) {
            return new MsgSetDateTime(dateTime);
        }

    private:
        QDateTime m_dateTime;

        MsgSetDateTime(const QDateTime& dateTime) :
            Message(),
            m_dateTime(dateTime)
        {}
    };

    class MsgReportAvailableChannelOrFeatures : public Message {
        MESSAGE_CLASS_DECLARATION

    public:
        AvailableChannelOrFeatureList& getItems() { return m_availableChannelOrFeatures; }
        const QStringList& getRenameFrom() const { return m_renameFrom; }
        const QStringList& getRenameTo() const { return m_renameTo; }

        static MsgReportAvailableChannelOrFeatures* create(const QStringList& renameFrom, const QStringList& renameTo) {
            return new MsgReportAvailableChannelOrFeatures(renameFrom, renameTo);
        }

    private:
        AvailableChannelOrFeatureList m_availableChannelOrFeatures;
        QStringList m_renameFrom;
        QStringList m_renameTo;

        MsgReportAvailableChannelOrFeatures(const QStringList& renameFrom, const QStringList& renameTo) :
            Message(),
            m_renameFrom(renameFrom),
            m_renameTo(renameTo)
        {}
    };

    Map(WebAPIAdapterInterface *webAPIAdapterInterface);
    virtual ~Map();
    virtual void destroy() { delete this; }
    virtual bool handleMessage(const Message& cmd);

    virtual void getIdentifier(QString& id) const { id = objectName(); }
    virtual QString getIdentifier() const { return objectName(); }
    virtual void getTitle(QString& title) const { title = m_settings.m_title; }

    virtual QByteArray serialize() const;
    virtual bool deserialize(const QByteArray& data);

    virtual int webapiRun(bool run,
            SWGSDRangel::SWGDeviceState& response,
            QString& errorMessage);

    virtual int webapiSettingsGet(
            SWGSDRangel::SWGFeatureSettings& response,
            QString& errorMessage);

    virtual int webapiSettingsPutPatch(
            bool force,
            const QStringList& featureSettingsKeys,
            SWGSDRangel::SWGFeatureSettings& response,
            QString& errorMessage);

    virtual int webapiReportGet(
            SWGSDRangel::SWGFeatureReport& response,
            QString& errorMessage);

    virtual int webapiActionsPost(
            const QStringList& featureActionsKeys,
            SWGSDRangel::SWGFeatureActions& query,
            QString& errorMessage);

    static void webapiFormatFeatureSettings(
        SWGSDRangel::SWGFeatureSettings& response,
        const MapSettings& settings);

    static void webapiUpdateFeatureSettings(
            MapSettings& settings,
            const QStringList& featureSettingsKeys,
            SWGSDRangel::SWGFeatureSettings& response);

    void setMapDateTime(QDateTime mapDateTime, QDateTime systemDateTime, double multiplier);
    QDateTime getMapDateTime();

    static const char* const m_featureIdURI;
    static const char* const m_featureId;

private:
    MapSettings m_settings;
    AvailableChannelOrFeatureList m_availableChannelOrFeatures;
    AvailableChannelOrFeatureHandler m_availableChannelOrFeatureHandler;

    QNetworkAccessManager *m_networkManager;
    QNetworkRequest m_networkRequest;

    void applySettings(const MapSettings& settings, const QList<QString>& settingsKeys, bool force = false);
    void webapiFormatFeatureReport(SWGSDRangel::SWGFeatureReport& response);
    void webapiReverseSendSettings(const QList<QString>& featureSettingsKeys, const MapSettings& settings, bool force);
    void notifyUpdate(const QStringList& renameFrom, const QStringList& renameTo);

    QDateTime m_mapDateTime;
    QDateTime m_systemDateTime;
    double m_multiplier;
    QRecursiveMutex m_dateTimeMutex;

private slots:
    void networkManagerFinished(QNetworkReply *reply);
    void channelsOrFeaturesChanged(const QStringList& renameFrom, const QStringList& renameTo);
    void handlePipeMessageQueue(MessageQueue* messageQueue);
};

#endif // INCLUDE_FEATURE_MAP_H_
