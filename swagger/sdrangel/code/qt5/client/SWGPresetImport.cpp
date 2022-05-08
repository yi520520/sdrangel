/**
 * SDRangel
 * This is the web REST/JSON API of SDRangel SDR software. SDRangel is an Open Source Qt5/OpenGL 3.0+ (4.3+ in Windows) GUI and server Software Defined Radio and signal analyzer in software. It supports Airspy, BladeRF, HackRF, LimeSDR, PlutoSDR, RTL-SDR, SDRplay RSP1 and FunCube    ---   Limitations and specifcities:    * In SDRangel GUI the first Rx device set cannot be deleted. Conversely the server starts with no device sets and its number of device sets can be reduced to zero by as many calls as necessary to /sdrangel/deviceset with DELETE method.   * Preset import and export from/to file is a server only feature.   * Device set focus is a GUI only feature.   * The following channels are not implemented (status 501 is returned): ATV and DATV demodulators, Channel Analyzer NG, LoRa demodulator   * The device settings and report structures contains only the sub-structure corresponding to the device type. The DeviceSettings and DeviceReport structures documented here shows all of them but only one will be or should be present at a time   * The channel settings and report structures contains only the sub-structure corresponding to the channel type. The ChannelSettings and ChannelReport structures documented here shows all of them but only one will be or should be present at a time    --- 
 *
 * OpenAPI spec version: 6.0.0
 * Contact: f4exb06@gmail.com
 *
 * NOTE: This class is auto generated by the swagger code generator program.
 * https://github.com/swagger-api/swagger-codegen.git
 * Do not edit the class manually.
 */


#include "SWGPresetImport.h"

#include "SWGHelpers.h"

#include <QJsonDocument>
#include <QJsonArray>
#include <QObject>
#include <QDebug>

namespace SWGSDRangel {

SWGPresetImport::SWGPresetImport(QString* json) {
    init();
    this->fromJson(*json);
}

SWGPresetImport::SWGPresetImport() {
    preset = nullptr;
    m_preset_isSet = false;
    file_path = nullptr;
    m_file_path_isSet = false;
}

SWGPresetImport::~SWGPresetImport() {
    this->cleanup();
}

void
SWGPresetImport::init() {
    preset = new SWGPresetIdentifier();
    m_preset_isSet = false;
    file_path = new QString("");
    m_file_path_isSet = false;
}

void
SWGPresetImport::cleanup() {
    if(preset != nullptr) { 
        delete preset;
    }
    if(file_path != nullptr) { 
        delete file_path;
    }
}

SWGPresetImport*
SWGPresetImport::fromJson(QString &json) {
    QByteArray array (json.toStdString().c_str());
    QJsonDocument doc = QJsonDocument::fromJson(array);
    QJsonObject jsonObject = doc.object();
    this->fromJsonObject(jsonObject);
    return this;
}

void
SWGPresetImport::fromJsonObject(QJsonObject &pJson) {
    ::SWGSDRangel::setValue(&preset, pJson["preset"], "SWGPresetIdentifier", "SWGPresetIdentifier");
    
    ::SWGSDRangel::setValue(&file_path, pJson["filePath"], "QString", "QString");
    
}

QString
SWGPresetImport::asJson ()
{
    QJsonObject* obj = this->asJsonObject();

    QJsonDocument doc(*obj);
    QByteArray bytes = doc.toJson();
    delete obj;
    return QString(bytes);
}

QJsonObject*
SWGPresetImport::asJsonObject() {
    QJsonObject* obj = new QJsonObject();
    if((preset != nullptr) && (preset->isSet())){
        toJsonValue(QString("preset"), preset, obj, QString("SWGPresetIdentifier"));
    }
    if(file_path != nullptr && *file_path != QString("")){
        toJsonValue(QString("filePath"), file_path, obj, QString("QString"));
    }

    return obj;
}

SWGPresetIdentifier*
SWGPresetImport::getPreset() {
    return preset;
}
void
SWGPresetImport::setPreset(SWGPresetIdentifier* preset) {
    this->preset = preset;
    this->m_preset_isSet = true;
}

QString*
SWGPresetImport::getFilePath() {
    return file_path;
}
void
SWGPresetImport::setFilePath(QString* file_path) {
    this->file_path = file_path;
    this->m_file_path_isSet = true;
}


bool
SWGPresetImport::isSet(){
    bool isObjectUpdated = false;
    do{
        if(preset && preset->isSet()){
            isObjectUpdated = true; break;
        }
        if(file_path && *file_path != QString("")){
            isObjectUpdated = true; break;
        }
    }while(false);
    return isObjectUpdated;
}
}

