/**
 * SDRangel
 * This is the web REST/JSON API of SDRangel SDR software. SDRangel is an Open Source Qt5/OpenGL 3.0+ (4.3+ in Windows) GUI and server Software Defined Radio and signal analyzer in software. It supports Airspy, BladeRF, HackRF, LimeSDR, PlutoSDR, RTL-SDR, SDRplay RSP1 and FunCube    ---   Limitations and specifcities:    * In SDRangel GUI the first Rx device set cannot be deleted. Conversely the server starts with no device sets and its number of device sets can be reduced to zero by as many calls as necessary to /sdrangel/deviceset with DELETE method.   * Preset import and export from/to file is a server only feature.   * Device set focus is a GUI only feature.   * The following channels are not implemented (status 501 is returned): ATV and DATV demodulators, Channel Analyzer NG, LoRa demodulator   * The device settings and report structures contains only the sub-structure corresponding to the device type. The DeviceSettings and DeviceReport structures documented here shows all of them but only one will be or should be present at a time   * The channel settings and report structures contains only the sub-structure corresponding to the channel type. The ChannelSettings and ChannelReport structures documented here shows all of them but only one will be or should be present at a time    --- 
 *
 * OpenAPI spec version: 7.0.0
 * Contact: f4exb06@gmail.com
 *
 * NOTE: This class is auto generated by the swagger code generator program.
 * https://github.com/swagger-api/swagger-codegen.git
 * Do not edit the class manually.
 */


#include "SWGFreqScannerFrequency.h"

#include "SWGHelpers.h"

#include <QJsonDocument>
#include <QJsonArray>
#include <QObject>
#include <QDebug>

namespace SWGSDRangel {

SWGFreqScannerFrequency::SWGFreqScannerFrequency(QString* json) {
    init();
    this->fromJson(*json);
}

SWGFreqScannerFrequency::SWGFreqScannerFrequency() {
    frequency = 0L;
    m_frequency_isSet = false;
    enabled = 0;
    m_enabled_isSet = false;
    notes = nullptr;
    m_notes_isSet = false;
    channel = nullptr;
    m_channel_isSet = false;
    channel_bandwidth = nullptr;
    m_channel_bandwidth_isSet = false;
    threshold = nullptr;
    m_threshold_isSet = false;
    squelch = nullptr;
    m_squelch_isSet = false;
}

SWGFreqScannerFrequency::~SWGFreqScannerFrequency() {
    this->cleanup();
}

void
SWGFreqScannerFrequency::init() {
    frequency = 0L;
    m_frequency_isSet = false;
    enabled = 0;
    m_enabled_isSet = false;
    notes = new QString("");
    m_notes_isSet = false;
    channel = new QString("");
    m_channel_isSet = false;
    channel_bandwidth = new QString("");
    m_channel_bandwidth_isSet = false;
    threshold = new QString("");
    m_threshold_isSet = false;
    squelch = new QString("");
    m_squelch_isSet = false;
}

void
SWGFreqScannerFrequency::cleanup() {


    if(notes != nullptr) { 
        delete notes;
    }
    if(channel != nullptr) { 
        delete channel;
    }
    if(channel_bandwidth != nullptr) { 
        delete channel_bandwidth;
    }
    if(threshold != nullptr) { 
        delete threshold;
    }
    if(squelch != nullptr) { 
        delete squelch;
    }
}

SWGFreqScannerFrequency*
SWGFreqScannerFrequency::fromJson(QString &json) {
    QByteArray array (json.toStdString().c_str());
    QJsonDocument doc = QJsonDocument::fromJson(array);
    QJsonObject jsonObject = doc.object();
    this->fromJsonObject(jsonObject);
    return this;
}

void
SWGFreqScannerFrequency::fromJsonObject(QJsonObject &pJson) {
    ::SWGSDRangel::setValue(&frequency, pJson["frequency"], "qint64", "");
    
    ::SWGSDRangel::setValue(&enabled, pJson["enabled"], "qint32", "");
    
    ::SWGSDRangel::setValue(&notes, pJson["notes"], "QString", "QString");
    
    ::SWGSDRangel::setValue(&channel, pJson["channel"], "QString", "QString");
    
    ::SWGSDRangel::setValue(&channel_bandwidth, pJson["channelBandwidth"], "QString", "QString");
    
    ::SWGSDRangel::setValue(&threshold, pJson["threshold"], "QString", "QString");
    
    ::SWGSDRangel::setValue(&squelch, pJson["squelch"], "QString", "QString");
    
}

QString
SWGFreqScannerFrequency::asJson ()
{
    QJsonObject* obj = this->asJsonObject();

    QJsonDocument doc(*obj);
    QByteArray bytes = doc.toJson();
    delete obj;
    return QString(bytes);
}

QJsonObject*
SWGFreqScannerFrequency::asJsonObject() {
    QJsonObject* obj = new QJsonObject();
    if(m_frequency_isSet){
        obj->insert("frequency", QJsonValue(frequency));
    }
    if(m_enabled_isSet){
        obj->insert("enabled", QJsonValue(enabled));
    }
    if(notes != nullptr && *notes != QString("")){
        toJsonValue(QString("notes"), notes, obj, QString("QString"));
    }
    if(channel != nullptr && *channel != QString("")){
        toJsonValue(QString("channel"), channel, obj, QString("QString"));
    }
    if(channel_bandwidth != nullptr && *channel_bandwidth != QString("")){
        toJsonValue(QString("channelBandwidth"), channel_bandwidth, obj, QString("QString"));
    }
    if(threshold != nullptr && *threshold != QString("")){
        toJsonValue(QString("threshold"), threshold, obj, QString("QString"));
    }
    if(squelch != nullptr && *squelch != QString("")){
        toJsonValue(QString("squelch"), squelch, obj, QString("QString"));
    }

    return obj;
}

qint64
SWGFreqScannerFrequency::getFrequency() {
    return frequency;
}
void
SWGFreqScannerFrequency::setFrequency(qint64 frequency) {
    this->frequency = frequency;
    this->m_frequency_isSet = true;
}

qint32
SWGFreqScannerFrequency::getEnabled() {
    return enabled;
}
void
SWGFreqScannerFrequency::setEnabled(qint32 enabled) {
    this->enabled = enabled;
    this->m_enabled_isSet = true;
}

QString*
SWGFreqScannerFrequency::getNotes() {
    return notes;
}
void
SWGFreqScannerFrequency::setNotes(QString* notes) {
    this->notes = notes;
    this->m_notes_isSet = true;
}

QString*
SWGFreqScannerFrequency::getChannel() {
    return channel;
}
void
SWGFreqScannerFrequency::setChannel(QString* channel) {
    this->channel = channel;
    this->m_channel_isSet = true;
}

QString*
SWGFreqScannerFrequency::getChannelBandwidth() {
    return channel_bandwidth;
}
void
SWGFreqScannerFrequency::setChannelBandwidth(QString* channel_bandwidth) {
    this->channel_bandwidth = channel_bandwidth;
    this->m_channel_bandwidth_isSet = true;
}

QString*
SWGFreqScannerFrequency::getThreshold() {
    return threshold;
}
void
SWGFreqScannerFrequency::setThreshold(QString* threshold) {
    this->threshold = threshold;
    this->m_threshold_isSet = true;
}

QString*
SWGFreqScannerFrequency::getSquelch() {
    return squelch;
}
void
SWGFreqScannerFrequency::setSquelch(QString* squelch) {
    this->squelch = squelch;
    this->m_squelch_isSet = true;
}


bool
SWGFreqScannerFrequency::isSet(){
    bool isObjectUpdated = false;
    do{
        if(m_frequency_isSet){
            isObjectUpdated = true; break;
        }
        if(m_enabled_isSet){
            isObjectUpdated = true; break;
        }
        if(notes && *notes != QString("")){
            isObjectUpdated = true; break;
        }
        if(channel && *channel != QString("")){
            isObjectUpdated = true; break;
        }
        if(channel_bandwidth && *channel_bandwidth != QString("")){
            isObjectUpdated = true; break;
        }
        if(threshold && *threshold != QString("")){
            isObjectUpdated = true; break;
        }
        if(squelch && *squelch != QString("")){
            isObjectUpdated = true; break;
        }
    }while(false);
    return isObjectUpdated;
}
}

