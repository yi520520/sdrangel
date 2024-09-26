///////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2020-2024 Jon Beniston, M7RCE <jon@beniston.com>                //
// Copyright (C) 2020-2022 Edouard Griffiths, F4EXB <f4exb06@gmail.com>          //
// Copyright (C) 2020 Kacper Michajłow <kasper93@gmail.com>                      //
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

#include <cmath>

#include <QDebug>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QBuffer>
#include <QSerialPortInfo>

#include "SWGFeatureSettings.h"
#include "SWGFeatureReport.h"
#include "SWGFeatureActions.h"
#include "SWGDeviceState.h"
#include "SWGTargetAzimuthElevation.h"

#include "feature/featureset.h"
#include "settings/serializable.h"
#include "maincore.h"

#include "gs232controller.h"
#include "gs232controllerworker.h"
#include "gs232controllerreport.h"
#include "dfmprotocol.h"

MESSAGE_CLASS_DEFINITION(GS232Controller::MsgConfigureGS232Controller, Message)
MESSAGE_CLASS_DEFINITION(GS232Controller::MsgStartStop, Message)
MESSAGE_CLASS_DEFINITION(GS232Controller::MsgReportWorker, Message)
MESSAGE_CLASS_DEFINITION(GS232Controller::MsgReportAvailableChannelOrFeatures, Message)
MESSAGE_CLASS_DEFINITION(GS232Controller::MsgScanAvailableChannelOrFeatures, Message)
MESSAGE_CLASS_DEFINITION(GS232Controller::MsgReportSerialPorts, Message)

const char* const GS232Controller::m_featureIdURI = "sdrangel.feature.gs232controller";
const char* const GS232Controller::m_featureId = "GS232Controller";

GS232Controller::GS232Controller(WebAPIAdapterInterface *webAPIAdapterInterface) :
    Feature(m_featureIdURI, webAPIAdapterInterface),
    m_thread(nullptr),
    m_worker(nullptr),
    m_availableChannelOrFeatureHandler(GS232ControllerSettings::m_pipeURIs),
    m_selectedPipe(nullptr),
    m_currentAzimuth(0.0f),
    m_currentElevation(0.0f)
{
    qDebug("GS232Controller::GS232Controller: webAPIAdapterInterface: %p", webAPIAdapterInterface);
    setObjectName(m_featureId);
    m_state = StIdle;
    m_errorMessage = "GS232Controller error";
    m_networkManager = new QNetworkAccessManager();
    QObject::connect(
        m_networkManager,
        &QNetworkAccessManager::finished,
        this,
        &GS232Controller::networkManagerFinished
    );

    QObject::connect(
        &m_availableChannelOrFeatureHandler,
        &AvailableChannelOrFeatureHandler::channelsOrFeaturesChanged,
        this,
        &GS232Controller::channelsOrFeaturesChanged
    );
    QObject::connect(
        &m_availableChannelOrFeatureHandler,
        &AvailableChannelOrFeatureHandler::messageEnqueued,
        this,
        &GS232Controller::handlePipeMessageQueue
    );
    m_availableChannelOrFeatureHandler.scanAvailableChannelsAndFeatures();

    connect(&m_timer, &QTimer::timeout, this, &GS232Controller::scanSerialPorts);
    m_timer.start(5000);
}

GS232Controller::~GS232Controller()
{
    m_timer.stop();
    disconnect(&m_timer, &QTimer::timeout, this, &GS232Controller::scanSerialPorts);
    QObject::disconnect(
        &m_availableChannelOrFeatureHandler,
        &AvailableChannelOrFeatureHandler::channelsOrFeaturesChanged,
        this,
        &GS232Controller::channelsOrFeaturesChanged
    );
    QObject::disconnect(
        &m_availableChannelOrFeatureHandler,
        &AvailableChannelOrFeatureHandler::messageEnqueued,
        this,
        &GS232Controller::handlePipeMessageQueue
    );
    QObject::disconnect(
        m_networkManager,
        &QNetworkAccessManager::finished,
        this,
        &GS232Controller::networkManagerFinished
    );
    delete m_networkManager;
    stop();
}

void GS232Controller::start()
{
    qDebug("GS232Controller::start");

    m_thread = new QThread();
    m_worker = new GS232ControllerWorker(this);
    m_worker->moveToThread(m_thread);
    QObject::connect(m_thread, &QThread::started, m_worker, &GS232ControllerWorker::startWork);
    QObject::connect(m_thread, &QThread::finished, m_worker, &QObject::deleteLater);
    QObject::connect(m_thread, &QThread::finished, m_thread, &QThread::deleteLater);
    m_worker->setMessageQueueToFeature(getInputMessageQueue());
    m_thread->start();
    m_state = StRunning;

    GS232ControllerWorker::MsgConfigureGS232ControllerWorker *msg =
        GS232ControllerWorker::MsgConfigureGS232ControllerWorker::create(m_settings, QList<QString>(), true);
    m_worker->getInputMessageQueue()->push(msg);
}

void GS232Controller::stop()
{
    qDebug("GS232Controller::stop");
    m_state = StIdle;
    if (m_thread)
    {
        m_thread->quit();
        m_thread->wait();
        m_thread = nullptr;
        m_worker = nullptr;
    }
}

bool GS232Controller::handleMessage(const Message& cmd)
{
    if (MsgConfigureGS232Controller::match(cmd))
    {
        MsgConfigureGS232Controller& cfg = (MsgConfigureGS232Controller&) cmd;
        qDebug() << "GS232Controller::handleMessage: MsgConfigureGS232Controller";
        applySettings(cfg.getSettings(), cfg.getSettingsKeys(), cfg.getForce());

        return true;
    }
    else if (MsgStartStop::match(cmd))
    {
        MsgStartStop& cfg = (MsgStartStop&) cmd;
        qDebug() << "GS232Controller::handleMessage: MsgStartStop: start:" << cfg.getStartStop();

        if (cfg.getStartStop()) {
            start();
        } else {
            stop();
        }

        return true;
    }
    else if (MsgReportWorker::match(cmd))
    {
        MsgReportWorker& report = (MsgReportWorker&) cmd;
        if (report.getMessage() == "Connected")
            m_state = StRunning;
        else if (report.getMessage() == "Disconnected")
            m_state = StIdle;
        else
        {
            m_state = StError;
            m_errorMessage = report.getMessage();
        }
        return true;
    }
    else if (MsgScanAvailableChannelOrFeatures::match(cmd))
    {
        notifyUpdate({}, {});
        return true;
    }
    else if (GS232ControllerReport::MsgReportAzAl::match(cmd))
    {
        GS232ControllerReport::MsgReportAzAl& report = (GS232ControllerReport::MsgReportAzAl&) cmd;
        // Save state for Web report/getOnTarget
        m_currentAzimuth = report.getAzimuth();
        m_currentElevation = report.getElevation();
        // Forward to GUI
        if (getMessageQueueToGUI()) {
            getMessageQueueToGUI()->push(new GS232ControllerReport::MsgReportAzAl(report));
        }
        return true;
    }
    else if (MainCore::MsgTargetAzimuthElevation::match(cmd))
    {
        // New source from another plugin
        if ((m_state == StRunning) && m_settings.m_track)
        {
            MainCore::MsgTargetAzimuthElevation& msg = (MainCore::MsgTargetAzimuthElevation&) cmd;
            // Is it from the selected pipe?
            if (msg.getPipeSource() == m_selectedPipe)
            {
                if (getMessageQueueToGUI())
                {
                    // Forward to GUI - which will then send us updated settings
                    getMessageQueueToGUI()->push(new MainCore::MsgTargetAzimuthElevation(msg));
                }
                else
                {
                    // No GUI, so save source - applySettings will propagate to worker
                    SWGSDRangel::SWGTargetAzimuthElevation *swgTarget = msg.getSWGTargetAzimuthElevation();
                    m_settings.m_azimuth = swgTarget->getAzimuth();
                    m_settings.m_elevation = swgTarget->getElevation();
                    applySettings(m_settings, QList<QString>{"azimuth", "elevation"});
                }
            }
        }
        return true;
    }
    else if (DFMProtocol::MsgReportDFMStatus::match(cmd))
    {
        // Forward to GUI
        if (getMessageQueueToGUI())
        {
            DFMProtocol::MsgReportDFMStatus& report = (DFMProtocol::MsgReportDFMStatus&) cmd;
            getMessageQueueToGUI()->push(new DFMProtocol::MsgReportDFMStatus(report));
        }
        return true;
    }
    else
    {
        return false;
    }
}

// Calculate whether last received az/el was on target
bool GS232Controller::getOnTarget() const
{
    float targetAziumth, targetElevation;
    m_settings.calcTargetAzEl(targetAziumth, targetElevation);
    float readTolerance = m_settings.m_tolerance + 0.0f;
    bool onTarget =   (std::fabs(m_currentAzimuth - targetAziumth) <= readTolerance)
                   && (std::fabs(m_currentElevation - targetElevation) <= readTolerance);
    return onTarget;
}

QByteArray GS232Controller::serialize() const
{
    return m_settings.serialize();
}

bool GS232Controller::deserialize(const QByteArray& data)
{
    if (m_settings.deserialize(data))
    {
        MsgConfigureGS232Controller *msg = MsgConfigureGS232Controller::create(m_settings, QList<QString>(), true);
        m_inputMessageQueue.push(msg);
        return true;
    }
    else
    {
        m_settings.resetToDefaults();
        MsgConfigureGS232Controller *msg = MsgConfigureGS232Controller::create(m_settings, QList<QString>(), true);
        m_inputMessageQueue.push(msg);
        return false;
    }
}

void GS232Controller::applySettings(const GS232ControllerSettings& settings, const QList<QString>& settingsKeys, bool force)
{
    qDebug() << "GS232Controller::applySettings:" << settings.getDebugString(settingsKeys, force) << " force: " << force;

    if (settingsKeys.contains("source")
        || (!settings.m_source.isEmpty() && (m_selectedPipe == nullptr)) // Change in available pipes
        || force)
    {
        m_availableChannelOrFeatureHandler.deregisterPipes(m_selectedPipe, {"target"});
        m_selectedPipe = m_availableChannelOrFeatureHandler.registerPipes(settings.m_source, {"target"});
    }

    if (m_worker) {
        GS232ControllerWorker::MsgConfigureGS232ControllerWorker *msg = GS232ControllerWorker::MsgConfigureGS232ControllerWorker::create(
            settings, settingsKeys, force
            );
        m_worker->getInputMessageQueue()->push(msg);
    }

    if (settings.m_useReverseAPI)
    {
        bool fullUpdate = (settingsKeys.contains("useReverseAPI") && settings.m_useReverseAPI) ||
                settingsKeys.contains("reverseAPIAddress") ||
                settingsKeys.contains("reverseAPIPort") ||
                settingsKeys.contains("reverseAPIFeatureSetIndex") ||
                settingsKeys.contains("m_reverseAPIFeatureIndex");
        webapiReverseSendSettings(settingsKeys, settings, fullUpdate || force);
    }

    if (force) {
        m_settings = settings;
    } else {
        m_settings.applySettings(settingsKeys, settings);
    }
}

int GS232Controller::webapiRun(bool run,
    SWGSDRangel::SWGDeviceState& response,
    QString& errorMessage)
{
    (void) errorMessage;
    getFeatureStateStr(*response.getState());
    MsgStartStop *msg = MsgStartStop::create(run);
    getInputMessageQueue()->push(msg);
    return 202;
}

int GS232Controller::webapiSettingsGet(
    SWGSDRangel::SWGFeatureSettings& response,
    QString& errorMessage)
{
    (void) errorMessage;
    response.setGs232ControllerSettings(new SWGSDRangel::SWGGS232ControllerSettings());
    response.getGs232ControllerSettings()->init();
    webapiFormatFeatureSettings(response, m_settings);
    return 200;
}

int GS232Controller::webapiSettingsPutPatch(
    bool force,
    const QStringList& featureSettingsKeys,
    SWGSDRangel::SWGFeatureSettings& response,
    QString& errorMessage)
{
    (void) errorMessage;
    GS232ControllerSettings settings = m_settings;
    webapiUpdateFeatureSettings(settings, featureSettingsKeys, response);

    MsgConfigureGS232Controller *msg = MsgConfigureGS232Controller::create(settings, featureSettingsKeys, force);
    m_inputMessageQueue.push(msg);

    if (m_guiMessageQueue) // forward to GUI if any
    {
        MsgConfigureGS232Controller *msgToGUI = MsgConfigureGS232Controller::create(settings, featureSettingsKeys, force);
        m_guiMessageQueue->push(msgToGUI);
    }

    webapiFormatFeatureSettings(response, settings);

    return 200;
}

int GS232Controller::webapiReportGet(
    SWGSDRangel::SWGFeatureReport& response,
    QString& errorMessage)
{
    (void) errorMessage;
    response.setGs232ControllerReport(new SWGSDRangel::SWGGS232ControllerReport());
    response.getGs232ControllerReport()->init();
    webapiFormatFeatureReport(response);
    return 200;
}

int GS232Controller::webapiActionsPost(
    const QStringList& featureActionsKeys,
    SWGSDRangel::SWGFeatureActions& query,
    QString& errorMessage)
{
    SWGSDRangel::SWGGS232ControllerActions *swgGS232ControllerActions = query.getGs232ControllerActions();

    if (swgGS232ControllerActions)
    {
        if (featureActionsKeys.contains("run"))
        {
            bool featureRun = swgGS232ControllerActions->getRun() != 0;
            MsgStartStop *msg = MsgStartStop::create(featureRun);
            getInputMessageQueue()->push(msg);
            return 202;
        }
        else
        {
            errorMessage = "Unknown action";
            return 400;
        }
    }
    else
    {
        errorMessage = "Missing GS232ControllerActions in query";
        return 400;
    }
}

void GS232Controller::webapiFormatFeatureSettings(
    SWGSDRangel::SWGFeatureSettings& response,
    const GS232ControllerSettings& settings)
{
    response.getGs232ControllerSettings()->setAzimuth(settings.m_azimuth);
    response.getGs232ControllerSettings()->setElevation(settings.m_elevation);
    response.getGs232ControllerSettings()->setSerialPort(new QString(settings.m_serialPort));
    response.getGs232ControllerSettings()->setBaudRate(settings.m_baudRate);
    response.getGs232ControllerSettings()->setHost(new QString(settings.m_host));
    response.getGs232ControllerSettings()->setPort(settings.m_port);
    response.getGs232ControllerSettings()->setTrack(settings.m_track);
    response.getGs232ControllerSettings()->setSource(new QString(settings.m_source));
    response.getGs232ControllerSettings()->setAzimuthOffset(settings.m_azimuthOffset);
    response.getGs232ControllerSettings()->setElevationOffset(settings.m_elevationOffset);
    response.getGs232ControllerSettings()->setAzimuthMin(settings.m_azimuthMin);
    response.getGs232ControllerSettings()->setAzimuthMax(settings.m_azimuthMax);
    response.getGs232ControllerSettings()->setElevationMin(settings.m_elevationMin);
    response.getGs232ControllerSettings()->setElevationMax(settings.m_elevationMax);
    response.getGs232ControllerSettings()->setTolerance(settings.m_tolerance);
    response.getGs232ControllerSettings()->setProtocol(settings.m_protocol);
    response.getGs232ControllerSettings()->setPrecision(settings.m_precision);
    response.getGs232ControllerSettings()->setCoordinates((int)settings.m_coordinates);
    response.getGs232ControllerSettings()->setInputController(new QString(settings.m_inputController));
    response.getGs232ControllerSettings()->setInputSensitivity(settings.m_inputControllerSettings.m_lowSensitivity);

    if (response.getGs232ControllerSettings()->getTitle()) {
        *response.getGs232ControllerSettings()->getTitle() = settings.m_title;
    } else {
        response.getGs232ControllerSettings()->setTitle(new QString(settings.m_title));
    }

    response.getGs232ControllerSettings()->setRgbColor(settings.m_rgbColor);
    response.getGs232ControllerSettings()->setUseReverseApi(settings.m_useReverseAPI ? 1 : 0);

    if (response.getGs232ControllerSettings()->getReverseApiAddress()) {
        *response.getGs232ControllerSettings()->getReverseApiAddress() = settings.m_reverseAPIAddress;
    } else {
        response.getGs232ControllerSettings()->setReverseApiAddress(new QString(settings.m_reverseAPIAddress));
    }

    response.getGs232ControllerSettings()->setReverseApiPort(settings.m_reverseAPIPort);
    response.getGs232ControllerSettings()->setReverseApiFeatureSetIndex(settings.m_reverseAPIFeatureSetIndex);
    response.getGs232ControllerSettings()->setReverseApiFeatureIndex(settings.m_reverseAPIFeatureIndex);

    if (settings.m_rollupState)
    {
        if (response.getGs232ControllerSettings()->getRollupState())
        {
            settings.m_rollupState->formatTo(response.getGs232ControllerSettings()->getRollupState());
        }
        else
        {
            SWGSDRangel::SWGRollupState *swgRollupState = new SWGSDRangel::SWGRollupState();
            settings.m_rollupState->formatTo(swgRollupState);
            response.getGs232ControllerSettings()->setRollupState(swgRollupState);
        }
    }
}

void GS232Controller::webapiUpdateFeatureSettings(
    GS232ControllerSettings& settings,
    const QStringList& featureSettingsKeys,
    SWGSDRangel::SWGFeatureSettings& response)
{
    if (featureSettingsKeys.contains("azimuth")) {
        settings.m_azimuth = response.getGs232ControllerSettings()->getAzimuth();
    }
    if (featureSettingsKeys.contains("elevation")) {
        settings.m_elevation = response.getGs232ControllerSettings()->getElevation();
    }
    if (featureSettingsKeys.contains("serialPort")) {
        settings.m_serialPort = *response.getGs232ControllerSettings()->getSerialPort();
    }
    if (featureSettingsKeys.contains("baudRate")) {
        settings.m_baudRate = response.getGs232ControllerSettings()->getBaudRate();
    }
    if (featureSettingsKeys.contains("host")) {
        settings.m_host = *response.getGs232ControllerSettings()->getHost();
    }
    if (featureSettingsKeys.contains("port")) {
        settings.m_port = response.getGs232ControllerSettings()->getPort();
    }
    if (featureSettingsKeys.contains("track")) {
        settings.m_track = response.getGs232ControllerSettings()->getTrack() != 0;
    }
    if (featureSettingsKeys.contains("source")) {
        settings.m_source = *response.getGs232ControllerSettings()->getSource();
    }
    if (featureSettingsKeys.contains("azimuthOffset")) {
        settings.m_azimuthOffset = response.getGs232ControllerSettings()->getAzimuthOffset();
    }
    if (featureSettingsKeys.contains("elevationOffset")) {
        settings.m_elevationOffset = response.getGs232ControllerSettings()->getElevationOffset();
    }
    if (featureSettingsKeys.contains("azimuthMin")) {
        settings.m_azimuthMin = response.getGs232ControllerSettings()->getAzimuthMin();
    }
    if (featureSettingsKeys.contains("azimuthMax")) {
        settings.m_azimuthMax = response.getGs232ControllerSettings()->getAzimuthMax();
    }
    if (featureSettingsKeys.contains("elevationMin")) {
        settings.m_elevationMin = response.getGs232ControllerSettings()->getElevationMin();
    }
    if (featureSettingsKeys.contains("elevationMax")) {
        settings.m_elevationMax = response.getGs232ControllerSettings()->getElevationMax();
    }
    if (featureSettingsKeys.contains("tolerance")) {
        settings.m_tolerance = response.getGs232ControllerSettings()->getTolerance();
    }
    if (featureSettingsKeys.contains("protocol")) {
        settings.m_protocol = (GS232ControllerSettings::Protocol)response.getGs232ControllerSettings()->getProtocol();
    }
    if (featureSettingsKeys.contains("precision")) {
        settings.m_precision = response.getGs232ControllerSettings()->getPrecision();
    }
    if (featureSettingsKeys.contains("coordinates")) {
        settings.m_coordinates = (GS232ControllerSettings::Coordinates)response.getGs232ControllerSettings()->getCoordinates();
    }
    if (featureSettingsKeys.contains("inputController")) {
        settings.m_inputController = *response.getGs232ControllerSettings()->getInputController();
    }
    if (featureSettingsKeys.contains("inputSensitivity")) {
        settings.m_inputControllerSettings.m_lowSensitivity = response.getGs232ControllerSettings()->getInputSensitivity();
    }
    if (featureSettingsKeys.contains("title")) {
        settings.m_title = *response.getGs232ControllerSettings()->getTitle();
    }
    if (featureSettingsKeys.contains("rgbColor")) {
        settings.m_rgbColor = response.getGs232ControllerSettings()->getRgbColor();
    }
    if (featureSettingsKeys.contains("useReverseAPI")) {
        settings.m_useReverseAPI = response.getGs232ControllerSettings()->getUseReverseApi() != 0;
    }
    if (featureSettingsKeys.contains("reverseAPIAddress")) {
        settings.m_reverseAPIAddress = *response.getGs232ControllerSettings()->getReverseApiAddress();
    }
    if (featureSettingsKeys.contains("reverseAPIPort")) {
        settings.m_reverseAPIPort = response.getGs232ControllerSettings()->getReverseApiPort();
    }
    if (featureSettingsKeys.contains("reverseAPIFeatureSetIndex")) {
        settings.m_reverseAPIFeatureSetIndex = response.getGs232ControllerSettings()->getReverseApiFeatureSetIndex();
    }
    if (featureSettingsKeys.contains("reverseAPIFeatureIndex")) {
        settings.m_reverseAPIFeatureIndex = response.getGs232ControllerSettings()->getReverseApiFeatureIndex();
    }
    if (settings.m_rollupState && featureSettingsKeys.contains("rollupState")) {
        settings.m_rollupState->updateFrom(featureSettingsKeys, response.getGs232ControllerSettings()->getRollupState());
    }
}

void GS232Controller::webapiReverseSendSettings(const QList<QString>& featureSettingsKeys, const GS232ControllerSettings& settings, bool force)
{
    SWGSDRangel::SWGFeatureSettings *swgFeatureSettings = new SWGSDRangel::SWGFeatureSettings();
    // swgFeatureSettings->setOriginatorFeatureIndex(getIndexInDeviceSet());
    // swgFeatureSettings->setOriginatorFeatureSetIndex(getDeviceSetIndex());
    swgFeatureSettings->setFeatureType(new QString("GS232Controller"));
    swgFeatureSettings->setGs232ControllerSettings(new SWGSDRangel::SWGGS232ControllerSettings());
    SWGSDRangel::SWGGS232ControllerSettings *swgGS232ControllerSettings = swgFeatureSettings->getGs232ControllerSettings();

    // transfer data that has been modified. When force is on transfer all data except reverse API data

    if (featureSettingsKeys.contains("azimuth") || force) {
        swgGS232ControllerSettings->setAzimuth(settings.m_azimuth);
    }
    if (featureSettingsKeys.contains("elevation") || force) {
        swgGS232ControllerSettings->setElevation(settings.m_elevation);
    }
    if (featureSettingsKeys.contains("serialPort") || force) {
        swgGS232ControllerSettings->setSerialPort(new QString(settings.m_serialPort));
    }
    if (featureSettingsKeys.contains("baudRate") || force) {
        swgGS232ControllerSettings->setBaudRate(settings.m_baudRate);
    }
    if (featureSettingsKeys.contains("host") || force) {
        swgGS232ControllerSettings->setHost(new QString(settings.m_host));
    }
    if (featureSettingsKeys.contains("port") || force) {
        swgGS232ControllerSettings->setPort(settings.m_port);
    }
    if (featureSettingsKeys.contains("track") || force) {
        swgGS232ControllerSettings->setTrack(settings.m_track);
    }
    if (featureSettingsKeys.contains("source") || force) {
        swgGS232ControllerSettings->setSource(new QString(settings.m_source));
    }
    if (featureSettingsKeys.contains("azimuthOffset") || force) {
        swgGS232ControllerSettings->setAzimuthOffset(settings.m_azimuthOffset);
    }
    if (featureSettingsKeys.contains("elevationOffset") || force) {
        swgGS232ControllerSettings->setElevationOffset(settings.m_elevationOffset);
    }
    if (featureSettingsKeys.contains("azimuthMin") || force) {
        swgGS232ControllerSettings->setAzimuthMin(settings.m_azimuthMin);
    }
    if (featureSettingsKeys.contains("azimuthMax") || force) {
        swgGS232ControllerSettings->setAzimuthMax(settings.m_azimuthMax);
    }
    if (featureSettingsKeys.contains("elevationMin") || force) {
        swgGS232ControllerSettings->setElevationMin(settings.m_elevationMin);
    }
    if (featureSettingsKeys.contains("elevationMax") || force) {
        swgGS232ControllerSettings->setElevationMax(settings.m_elevationMax);
    }
    if (featureSettingsKeys.contains("tolerance") || force) {
        swgGS232ControllerSettings->setTolerance(settings.m_tolerance);
    }
    if (featureSettingsKeys.contains("protocol") || force) {
        swgGS232ControllerSettings->setProtocol((int)settings.m_protocol);
    }
    if (featureSettingsKeys.contains("precision") || force) {
        swgGS232ControllerSettings->setPrecision(settings.m_precision);
    }
    if (featureSettingsKeys.contains("coordinates") || force) {
        swgGS232ControllerSettings->setCoordinates(settings.m_coordinates);
    }
    if (featureSettingsKeys.contains("inputController") || force) {
        swgGS232ControllerSettings->setInputController(new QString(settings.m_inputController));
    }
    if (featureSettingsKeys.contains("inputSensitivity") || force) {
        swgGS232ControllerSettings->setInputSensitivity(settings.m_inputControllerSettings.m_lowSensitivity);
    }
    if (featureSettingsKeys.contains("title") || force) {
        swgGS232ControllerSettings->setTitle(new QString(settings.m_title));
    }
    if (featureSettingsKeys.contains("rgbColor") || force) {
        swgGS232ControllerSettings->setRgbColor(settings.m_rgbColor);
    }

    QString channelSettingsURL = QString("http://%1:%2/sdrangel/featureset/%3/feature/%4/settings")
            .arg(settings.m_reverseAPIAddress)
            .arg(settings.m_reverseAPIPort)
            .arg(settings.m_reverseAPIFeatureSetIndex)
            .arg(settings.m_reverseAPIFeatureIndex);
    m_networkRequest.setUrl(QUrl(channelSettingsURL));
    m_networkRequest.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QBuffer *buffer = new QBuffer();
    buffer->open((QBuffer::ReadWrite));
    buffer->write(swgFeatureSettings->asJson().toUtf8());
    buffer->seek(0);

    // Always use PATCH to avoid passing reverse API settings
    QNetworkReply *reply = m_networkManager->sendCustomRequest(m_networkRequest, "PATCH", buffer);
    buffer->setParent(reply);

    delete swgFeatureSettings;
}

void GS232Controller::webapiFormatFeatureReport(SWGSDRangel::SWGFeatureReport& response)
{
    response.getGs232ControllerReport()->setSources(new QList<QString*>());

    for (const auto& item : m_availableChannelOrFeatures)
    {
        QString itemText = item.getLongId();
        response.getGs232ControllerReport()->getSources()->append(new QString(itemText));
    }

    response.getGs232ControllerReport()->setSerialPorts(new QList<QString*>());
    for (const auto& serialPort : m_serialPorts) {
        response.getGs232ControllerReport()->getSerialPorts()->append(new QString(serialPort));
    }

    float azimuth, elevation;
    m_settings.calcTargetAzEl(azimuth, elevation);
    response.getGs232ControllerReport()->setTargetAzimuth(azimuth);
    response.getGs232ControllerReport()->setTargetElevation(elevation);
    response.getGs232ControllerReport()->setCurrentAzimuth(m_currentAzimuth);
    response.getGs232ControllerReport()->setCurrentElevation(m_currentElevation);
    response.getGs232ControllerReport()->setOnTarget(getOnTarget());
    response.getGs232ControllerReport()->setRunningState(getState());
}

void GS232Controller::networkManagerFinished(QNetworkReply *reply)
{
    QNetworkReply::NetworkError replyError = reply->error();

    if (replyError)
    {
        qWarning() << "GS232Controller::networkManagerFinished:"
                << " error(" << (int) replyError
                << "): " << replyError
                << ": " << reply->errorString();
    }
    else
    {
        QString answer = reply->readAll();
        answer.chop(1); // remove last \n
        qDebug("GS232Controller::networkManagerFinished: reply:\n%s", answer.toStdString().c_str());
    }

    reply->deleteLater();
}

void GS232Controller::channelsOrFeaturesChanged(const QStringList& renameFrom, const QStringList& renameTo)
{
    m_availableChannelOrFeatures = m_availableChannelOrFeatureHandler.getAvailableChannelOrFeatureList();
    notifyUpdate(renameFrom, renameTo);
}

void GS232Controller::notifyUpdate(const QStringList& renameFrom, const QStringList& renameTo)
{
    if (getMessageQueueToGUI())
    {
        MsgReportAvailableChannelOrFeatures *msg = MsgReportAvailableChannelOrFeatures::create(renameFrom, renameTo);
        msg->getItems() = m_availableChannelOrFeatures;
        getMessageQueueToGUI()->push(msg);
    }
}

void GS232Controller::handlePipeMessageQueue(MessageQueue* messageQueue)
{
    Message* message;

    while ((message = messageQueue->pop()) != nullptr)
    {
        if (handleMessage(*message)) {
            delete message;
        }
    }
}

void GS232Controller::scanSerialPorts()
{
    // This can take 4ms on Windows, so we don't want to have it in webapiFormatFeatureReport
    // as polling of target az/el by other plugins will be slowed down
    QList<QSerialPortInfo> serialPortInfos = QSerialPortInfo::availablePorts();
    QListIterator<QSerialPortInfo> i(serialPortInfos);
    QStringList serialPorts;
    while (i.hasNext())
    {
        QSerialPortInfo info = i.next();
        serialPorts.append(info.portName());
    }
    if (m_serialPorts != serialPorts)
    {
        if (getMessageQueueToGUI())
        {
            MsgReportSerialPorts *msg = MsgReportSerialPorts::create(serialPorts);
            getMessageQueueToGUI()->push(msg);
        }
        m_serialPorts = serialPorts;
    }
}

