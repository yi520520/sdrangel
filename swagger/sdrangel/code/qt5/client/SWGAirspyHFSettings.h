/**
 * SDRangel
 * This is the web REST/JSON API of SDRangel SDR software. SDRangel is an Open Source Qt5/OpenGL 3.0+ (4.3+ in Windows) GUI and server Software Defined Radio and signal analyzer in software. It supports Airspy, BladeRF, HackRF, LimeSDR, PlutoSDR, RTL-SDR, SDRplay RSP1 and FunCube    ---   Limitations and specifcities:    * In SDRangel GUI the first Rx device set cannot be deleted. Conversely the server starts with no device sets and its number of device sets can be reduced to zero by as many calls as necessary to /sdrangel/deviceset with DELETE method.   * Preset import and export from/to file is a server only feature.   * Device set focus is a GUI only feature.   * The following channels are not implemented (status 501 is returned): ATV and DATV demodulators, Channel Analyzer NG, LoRa demodulator   * The device settings and report structures contains only the sub-structure corresponding to the device type. The DeviceSettings and DeviceReport structures documented here shows all of them but only one will be or should be present at a time   * The channel settings and report structures contains only the sub-structure corresponding to the channel type. The ChannelSettings and ChannelReport structures documented here shows all of them but only one will be or should be present at a time    --- 
 *
 * OpenAPI spec version: 4.11.3
 * Contact: f4exb06@gmail.com
 *
 * NOTE: This class is auto generated by the swagger code generator program.
 * https://github.com/swagger-api/swagger-codegen.git
 * Do not edit the class manually.
 */

/*
 * SWGAirspyHFSettings.h
 *
 * AirspyHF
 */

#ifndef SWGAirspyHFSettings_H_
#define SWGAirspyHFSettings_H_

#include <QJsonObject>


#include <QString>

#include "SWGObject.h"
#include "export.h"

namespace SWGSDRangel {

class SWG_API SWGAirspyHFSettings: public SWGObject {
public:
    SWGAirspyHFSettings();
    SWGAirspyHFSettings(QString* json);
    virtual ~SWGAirspyHFSettings();
    void init();
    void cleanup();

    virtual QString asJson () override;
    virtual QJsonObject* asJsonObject() override;
    virtual void fromJsonObject(QJsonObject &json) override;
    virtual SWGAirspyHFSettings* fromJson(QString &jsonString) override;

    qint64 getCenterFrequency();
    void setCenterFrequency(qint64 center_frequency);

    qint32 getLOppmTenths();
    void setLOppmTenths(qint32 l_oppm_tenths);

    qint32 getDevSampleRateIndex();
    void setDevSampleRateIndex(qint32 dev_sample_rate_index);

    qint32 getLog2Decim();
    void setLog2Decim(qint32 log2_decim);

    qint32 getTransverterMode();
    void setTransverterMode(qint32 transverter_mode);

    qint64 getTransverterDeltaFrequency();
    void setTransverterDeltaFrequency(qint64 transverter_delta_frequency);

    qint32 getBandIndex();
    void setBandIndex(qint32 band_index);

    QString* getFileRecordName();
    void setFileRecordName(QString* file_record_name);

    qint32 getUseReverseApi();
    void setUseReverseApi(qint32 use_reverse_api);

    QString* getReverseApiAddress();
    void setReverseApiAddress(QString* reverse_api_address);

    qint32 getReverseApiPort();
    void setReverseApiPort(qint32 reverse_api_port);

    qint32 getReverseApiDeviceIndex();
    void setReverseApiDeviceIndex(qint32 reverse_api_device_index);

    qint32 getUseAgc();
    void setUseAgc(qint32 use_agc);

    qint32 getAgcHigh();
    void setAgcHigh(qint32 agc_high);

    qint32 getUseDsp();
    void setUseDsp(qint32 use_dsp);

    qint32 getUseLna();
    void setUseLna(qint32 use_lna);

    qint32 getAttenuatorSteps();
    void setAttenuatorSteps(qint32 attenuator_steps);

    qint32 getDcBlock();
    void setDcBlock(qint32 dc_block);

    qint32 getIqCorrection();
    void setIqCorrection(qint32 iq_correction);


    virtual bool isSet() override;

private:
    qint64 center_frequency;
    bool m_center_frequency_isSet;

    qint32 l_oppm_tenths;
    bool m_l_oppm_tenths_isSet;

    qint32 dev_sample_rate_index;
    bool m_dev_sample_rate_index_isSet;

    qint32 log2_decim;
    bool m_log2_decim_isSet;

    qint32 transverter_mode;
    bool m_transverter_mode_isSet;

    qint64 transverter_delta_frequency;
    bool m_transverter_delta_frequency_isSet;

    qint32 band_index;
    bool m_band_index_isSet;

    QString* file_record_name;
    bool m_file_record_name_isSet;

    qint32 use_reverse_api;
    bool m_use_reverse_api_isSet;

    QString* reverse_api_address;
    bool m_reverse_api_address_isSet;

    qint32 reverse_api_port;
    bool m_reverse_api_port_isSet;

    qint32 reverse_api_device_index;
    bool m_reverse_api_device_index_isSet;

    qint32 use_agc;
    bool m_use_agc_isSet;

    qint32 agc_high;
    bool m_agc_high_isSet;

    qint32 use_dsp;
    bool m_use_dsp_isSet;

    qint32 use_lna;
    bool m_use_lna_isSet;

    qint32 attenuator_steps;
    bool m_attenuator_steps_isSet;

    qint32 dc_block;
    bool m_dc_block_isSet;

    qint32 iq_correction;
    bool m_iq_correction_isSet;

};

}

#endif /* SWGAirspyHFSettings_H_ */
