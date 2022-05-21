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

#include <QNetworkAccessManager>
#include <QNetworkReply>

#include "SWGDeviceState.h"
#include "SWGErrorResponse.h"

#include "util/simpleserializer.h"
#include "util/serialutil.h"
#include "webapi/webapiadapterinterface.h"

#include "limerfe.h"

MESSAGE_CLASS_DEFINITION(LimeRFE::MsgConfigureLimeRFE, Message)

const char* const LimeRFE::m_featureIdURI = "sdrangel.feature.limerfe";
const char* const LimeRFE::m_featureId = "LimeRFE";

const std::map<int, std::string> LimeRFE::m_errorCodesMap = {
    { 0, "OK"},
    {-4, "Error synchronizing communication"},
    {-3, "Non-configurable GPIO pin specified. Only pins 4 and 5 are configurable."},
    {-2, "Problem with .ini configuration file"},
    {-1, "Communication error"},
    { 1, "Wrong TX connector - not possible to route TX of the selecrted channel to the specified port"},
    { 2, "Wrong RX connector - not possible to route RX of the selecrted channel to the specified port"},
    { 3, "Mode TXRX not allowed - when the same port is selected for RX and TX, it is not allowed to use mode RX & TX"},
    { 4, "Wrong mode for cellular channel - Cellular FDD bands (1, 2, 3, and 7) are only allowed mode RX & TX, while TDD band 38 is allowed only RX or TX mode"},
    { 5, "Cellular channels must be the same both for RX and TX"},
    { 6, "Requested channel code is wrong"}
};

LimeRFE::LimeRFE(WebAPIAdapterInterface *webAPIAdapterInterface) :
    Feature(m_featureIdURI, webAPIAdapterInterface),
    m_webAPIAdapterInterface(webAPIAdapterInterface),
    m_rfeDevice(nullptr)
{
    setObjectName(m_featureId);
    m_state = StIdle;
    m_errorMessage = "LimeRFE error";
    m_networkManager = new QNetworkAccessManager();
    QObject::connect(
        m_networkManager,
        &QNetworkAccessManager::finished,
        this,
        &LimeRFE::networkManagerFinished
    );
    listComPorts();
}

LimeRFE::~LimeRFE()
{
    QObject::disconnect(
        m_networkManager,
        &QNetworkAccessManager::finished,
        this,
        &LimeRFE::networkManagerFinished
    );
    delete m_networkManager;
    closeDevice();
}

void LimeRFE::start()
{
    qDebug("LimeRFE::start");
    m_state = StRunning;
}

void LimeRFE::stop()
{
    qDebug("LimeRFE::stop");
    m_state = StIdle;
}

void LimeRFE::listComPorts()
{
    m_comPorts.clear();
    std::vector<std::string> comPorts;
    SerialUtil::getComPorts(comPorts, "ttyUSB[0-9]+"); // regex is for Linux only

    for (std::vector<std::string>::const_iterator it = comPorts.begin(); it != comPorts.end(); ++it) {
        m_comPorts.push_back(QString(it->c_str()));
    }
}

bool LimeRFE::handleMessage(const Message& cmd)
{
    (void) cmd;
    return false;
}

QByteArray LimeRFE::serialize() const
{
    SimpleSerializer s(1);

    s.writeBlob(1, m_settings.serialize());
    s.writeBlob(2, m_calib.serialize());

    return s.final();
}

bool LimeRFE::deserialize(const QByteArray& data)
{
    SimpleDeserializer d(data);

    if (!d.isValid())
    {
        m_settings.resetToDefaults();
        return false;
    }

    if (d.getVersion() == 1)
    {
        QByteArray bytetmp;
        bool ret;

        d.readBlob(1, &bytetmp);

        if (m_settings.deserialize(bytetmp))
        {
            MsgConfigureLimeRFE *msg = MsgConfigureLimeRFE::create(m_settings, true);
            m_inputMessageQueue.push(msg);
            ret = true;
        }
        else
        {
            m_settings.resetToDefaults();
            MsgConfigureLimeRFE *msg = MsgConfigureLimeRFE::create(m_settings, true);
            m_inputMessageQueue.push(msg);
            ret = false;
        }

        d.readBlob(2, &bytetmp);

        if (!m_calib.deserialize(bytetmp)) {
            ret = false;
        }

        return ret;
    }
    else
    {
        return false;
    }
}

int LimeRFE::openDevice(const std::string& serialDeviceName)
{
    closeDevice();

    rfe_dev_t *rfeDevice = RFE_Open(serialDeviceName.c_str(), nullptr);

    if (rfeDevice != (void *) -1)
    {
        m_rfeDevice = rfeDevice;
        return 0;
    }
    else
    {
        return -1;
    }
}

void LimeRFE::closeDevice()
{
    if (m_rfeDevice)
    {
        RFE_Close(m_rfeDevice);
        m_rfeDevice = nullptr;
    }
}

int LimeRFE::configure()
{
    if (!m_rfeDevice) {
        return -1;
    }

    qDebug() << "LimeRFE::configure: "
        << "attValue: " << (int) m_rfeBoardState.attValue
        << "channelIDRX: " << (int) m_rfeBoardState.channelIDRX
        << "channelIDTX: " << (int) m_rfeBoardState.channelIDTX
        << "mode: " << (int) m_rfeBoardState.mode
        << "notchOnOff: " << (int) m_rfeBoardState.notchOnOff
        << "selPortRX: " << (int) m_rfeBoardState.selPortRX
        << "selPortTX: " << (int) m_rfeBoardState.selPortTX
        << "enableSWR: " << (int) m_rfeBoardState.enableSWR
        << "sourceSWR: " << (int) m_rfeBoardState.sourceSWR;

    int rc = RFE_ConfigureState(m_rfeDevice, m_rfeBoardState);

    if (rc != 0) {
        qInfo("LimeRFE::configure: %s", getError(rc).c_str());
    } else {
        qDebug() << "LimeRFE::configure: done";
    }

    return rc;
}

int LimeRFE::getState()
{
    if (!m_rfeDevice) {
        return -1;
    }

    int rc = RFE_GetState(m_rfeDevice, &m_rfeBoardState);

    qDebug() << "LimeRFE::getState: "
        << "attValue: " << (int) m_rfeBoardState.attValue
        << "channelIDRX: " << (int) m_rfeBoardState.channelIDRX
        << "channelIDTX: " << (int) m_rfeBoardState.channelIDTX
        << "mode: " << (int) m_rfeBoardState.mode
        << "notchOnOff: " << (int) m_rfeBoardState.notchOnOff
        << "selPortRX: " << (int) m_rfeBoardState.selPortRX
        << "selPortTX: " << (int) m_rfeBoardState.selPortTX
        << "enableSWR: " << (int) m_rfeBoardState.enableSWR
        << "sourceSWR: " << (int) m_rfeBoardState.sourceSWR;

    if (rc != 0) {
        qInfo("LimeRFE::getState: %s", getError(rc).c_str());
    }

    return rc;
}

std::string LimeRFE::getError(int errorCode)
{
    std::map<int, std::string>::const_iterator it = m_errorCodesMap.find(errorCode);

    if (it == m_errorCodesMap.end()) {
        return "Unknown error";
    } else {
        return it->second;
    }
}

int LimeRFE::setRx(LimeRFESettings& settings, bool rxOn)
{
    if (!m_rfeDevice) {
        return -1;
    }

    int mode = rxOn && settings.m_txOn ?
        RFE_MODE_TXRX : rxOn ?
            RFE_MODE_RX : settings.m_txOn ?
                RFE_MODE_TX :  RFE_MODE_NONE;

    int rc = RFE_Mode(m_rfeDevice, mode);

    if (rc == 0) {
        settings.m_rxOn = rxOn;
    }

    return rc;
}

int LimeRFE::setTx(LimeRFESettings& settings, bool txOn)
{
    if (!m_rfeDevice) {
        return -1;
    }

    int mode = txOn && settings.m_rxOn ?
        RFE_MODE_TXRX : txOn ?
            RFE_MODE_TX : settings.m_rxOn ?
                RFE_MODE_RX :  RFE_MODE_NONE;

    int rc = RFE_Mode(m_rfeDevice, mode);

    if (rc == 0) {
        settings.m_txOn = txOn;
    }

    return rc;
}

bool LimeRFE::turnDevice(int deviceSetIndex, bool on)
{
    qDebug("LimeRFE::turnDevice %d: %s", deviceSetIndex, on ? "on" : "off");
    SWGSDRangel::SWGDeviceState response;
    SWGSDRangel::SWGErrorResponse error;
    int httpCode;

    if (on) {
        httpCode = m_webAPIAdapterInterface->devicesetDeviceRunPost(deviceSetIndex, response, error);
    } else {
        httpCode = m_webAPIAdapterInterface->devicesetDeviceRunDelete(deviceSetIndex, response, error);
    }

    if (httpCode/100 == 2)
    {
        qDebug("LimeRFE::turnDevice: %s success", on ? "on" : "off");
        return true;
    }
    else
    {
        qWarning("LimeRFE::turnDevice: error: %s", qPrintable(*error.getMessage()));
        return false;
    }
}

int LimeRFE::getFwdPower(int& powerDB)
{
    if (!m_rfeDevice) {
        return -1;
    }

    int power;
    int rc = RFE_ReadADC(m_rfeDevice, RFE_ADC1, &power);

    if (rc == 0) {
        powerDB = power;
    }

    return rc;
}

int LimeRFE::getRefPower(int& powerDB)
{
    if (!m_rfeDevice) {
        return -1;
    }

    int power;
    int rc = RFE_ReadADC(m_rfeDevice, RFE_ADC2, &power);

    if (rc == 0) {
        powerDB = power;
    }

    return rc;
}

void LimeRFE::settingsToState(const LimeRFESettings& settings)
{
    if (settings.m_rxChannels == LimeRFESettings::ChannelGroups::ChannelsCellular)
    {
        if (settings.m_rxCellularChannel == LimeRFESettings::CellularChannel::CellularBand1)
        {
            m_rfeBoardState.channelIDRX = RFE_CID_CELL_BAND01;
            m_rfeBoardState.mode = RFE_MODE_TXRX;
        }
        else if (settings.m_rxCellularChannel == LimeRFESettings::CellularChannel::CellularBand2)
        {
            m_rfeBoardState.channelIDRX = RFE_CID_CELL_BAND02;
            m_rfeBoardState.mode = RFE_MODE_TXRX;
        }
        else if (settings.m_rxCellularChannel == LimeRFESettings::CellularChannel::CellularBand3)
        {
            m_rfeBoardState.channelIDRX = RFE_CID_CELL_BAND03;
            m_rfeBoardState.mode = RFE_MODE_TXRX;
        }
        else if (settings.m_rxCellularChannel == LimeRFESettings::CellularChannel::CellularBand38)
        {
            m_rfeBoardState.channelIDRX = RFE_CID_CELL_BAND38;
        }
        else if (settings.m_rxCellularChannel == LimeRFESettings::CellularChannel::CellularBand7)
        {
            m_rfeBoardState.channelIDRX = RFE_CID_CELL_BAND07;
            m_rfeBoardState.mode = RFE_MODE_TXRX;
        }

        m_rfeBoardState.selPortRX = RFE_PORT_1;
        m_rfeBoardState.selPortTX = RFE_PORT_1;
        m_rfeBoardState.channelIDTX = m_rfeBoardState.channelIDRX;
    }
    else
    {
        m_rfeBoardState.mode = settings.m_rxOn && settings.m_txOn ?
            RFE_MODE_TXRX :
            settings.m_rxOn ?
                RFE_MODE_RX :
                settings.m_txOn ?
                    RFE_MODE_TX :
                    RFE_MODE_NONE;

        if (settings.m_rxChannels == LimeRFESettings::ChannelGroups::ChannelsWideband)
        {
            if (settings.m_rxWidebandChannel == LimeRFESettings::WidebandChannel::WidebandLow) {
                m_rfeBoardState.channelIDRX = RFE_CID_WB_1000;
            } else if (settings.m_rxWidebandChannel == LimeRFESettings::WidebandChannel::WidebandHigh) {
                m_rfeBoardState.channelIDRX = RFE_CID_WB_4000;
            }
        }
        else if (settings.m_rxChannels == LimeRFESettings::ChannelGroups::ChannelsHAM)
        {
            if (settings.m_rxHAMChannel == LimeRFESettings::HAMChannel::HAM_30M) {
                m_rfeBoardState.channelIDRX = RFE_CID_HAM_0030;
            } else if (settings.m_rxHAMChannel == LimeRFESettings::HAMChannel::HAM_50_70MHz) {
                m_rfeBoardState.channelIDRX = RFE_CID_HAM_0070;
            } else if (settings.m_rxHAMChannel == LimeRFESettings::HAMChannel::HAM_144_146MHz) {
                m_rfeBoardState.channelIDRX = RFE_CID_HAM_0145;
            } else if (settings.m_rxHAMChannel == LimeRFESettings::HAMChannel::HAM_220_225MHz) {
                m_rfeBoardState.channelIDRX = RFE_CID_HAM_0220;
            } else if (settings.m_rxHAMChannel == LimeRFESettings::HAMChannel::HAM_430_440MHz) {
                m_rfeBoardState.channelIDRX = RFE_CID_HAM_0435;
            } else if (settings.m_rxHAMChannel == LimeRFESettings::HAMChannel::HAM_902_928MHz) {
                m_rfeBoardState.channelIDRX = RFE_CID_HAM_0920;
            } else if (settings.m_rxHAMChannel == LimeRFESettings::HAMChannel::HAM_1240_1325MHz) {
                m_rfeBoardState.channelIDRX = RFE_CID_HAM_1280;
            } else if (settings.m_rxHAMChannel == LimeRFESettings::HAMChannel::HAM_2300_2450MHz) {
                m_rfeBoardState.channelIDRX = RFE_CID_HAM_2400;
            } else if (settings.m_rxHAMChannel == LimeRFESettings::HAMChannel::HAM_3300_3500MHz) {
                m_rfeBoardState.channelIDRX = RFE_CID_HAM_3500;
            }
        }

        if (settings.m_rxPort == LimeRFESettings::RxPort::RxPortJ3) {
            m_rfeBoardState.selPortRX = RFE_PORT_1;
        } else if (settings.m_rxPort == LimeRFESettings::RxPort::RxPortJ5) {
            m_rfeBoardState.selPortRX = RFE_PORT_3;
        }

        if (settings.m_txRxDriven)
        {
            m_rfeBoardState.channelIDTX = m_rfeBoardState.channelIDRX;
        }
        else
        {
            if (settings.m_txChannels == LimeRFESettings::ChannelGroups::ChannelsWideband)
            {
                if (settings.m_txWidebandChannel == LimeRFESettings::WidebandChannel::WidebandLow) {
                    m_rfeBoardState.channelIDTX = RFE_CID_WB_1000;
                } else if (settings.m_txWidebandChannel == LimeRFESettings::WidebandChannel::WidebandHigh) {
                    m_rfeBoardState.channelIDTX = RFE_CID_WB_4000;
                }
            }
            else if (settings.m_txChannels == LimeRFESettings::ChannelGroups::ChannelsHAM)
            {
                if (settings.m_txHAMChannel == LimeRFESettings::HAMChannel::HAM_30M) {
                    m_rfeBoardState.channelIDTX = RFE_CID_HAM_0030;
                } else if (settings.m_txHAMChannel == LimeRFESettings::HAMChannel::HAM_50_70MHz) {
                    m_rfeBoardState.channelIDTX = RFE_CID_HAM_0070;
                } else if (settings.m_txHAMChannel == LimeRFESettings::HAMChannel::HAM_144_146MHz) {
                    m_rfeBoardState.channelIDTX = RFE_CID_HAM_0145;
                } else if (settings.m_txHAMChannel == LimeRFESettings::HAMChannel::HAM_220_225MHz) {
                    m_rfeBoardState.channelIDTX = RFE_CID_HAM_0220;
                } else if (settings.m_txHAMChannel == LimeRFESettings::HAMChannel::HAM_430_440MHz) {
                    m_rfeBoardState.channelIDTX = RFE_CID_HAM_0435;
                } else if (settings.m_txHAMChannel == LimeRFESettings::HAMChannel::HAM_902_928MHz) {
                    m_rfeBoardState.channelIDTX = RFE_CID_HAM_0920;
                } else if (settings.m_txHAMChannel == LimeRFESettings::HAMChannel::HAM_1240_1325MHz) {
                    m_rfeBoardState.channelIDTX = RFE_CID_HAM_1280;
                } else if (settings.m_txHAMChannel == LimeRFESettings::HAMChannel::HAM_2300_2450MHz) {
                    m_rfeBoardState.channelIDTX = RFE_CID_HAM_2400;
                } else if (settings.m_txHAMChannel == LimeRFESettings::HAMChannel::HAM_3300_3500MHz) {
                    m_rfeBoardState.channelIDTX = RFE_CID_HAM_3500;
                }
            }
        }

        if (settings.m_txPort == LimeRFESettings::TxPort::TxPortJ3) {
            m_rfeBoardState.selPortTX = RFE_PORT_1;
        } else if (settings.m_txPort == LimeRFESettings::TxPort::TxPortJ4) {
            m_rfeBoardState.selPortTX = RFE_PORT_2;
        } else if (settings.m_txPort == LimeRFESettings::TxPort::TxPortJ5) {
            m_rfeBoardState.selPortTX = RFE_PORT_3;
        }
    }

    m_rfeBoardState.attValue = settings.m_attenuationFactor > 7 ? 7 : settings.m_attenuationFactor;
    m_rfeBoardState.notchOnOff = settings.m_amfmNotch;
    m_rfeBoardState.enableSWR = settings.m_swrEnable ? RFE_SWR_ENABLE : RFE_SWR_DISABLE;

    if (settings.m_swrSource == LimeRFESettings::SWRSource::SWRExternal) {
        m_rfeBoardState.sourceSWR = RFE_SWR_SRC_EXT;
    } else if (settings.m_swrSource == LimeRFESettings::SWRSource::SWRCellular) {
        m_rfeBoardState.sourceSWR = RFE_SWR_SRC_CELL;
    }
}

void LimeRFE::stateToSettings(LimeRFESettings& settings)
{
    if (m_rfeBoardState.channelIDRX == RFE_CID_CELL_BAND01)
    {
        settings.m_rxChannels = LimeRFESettings::ChannelGroups::ChannelsCellular;
        settings.m_rxCellularChannel = LimeRFESettings::CellularChannel::CellularBand1;
    }
    else if (m_rfeBoardState.channelIDRX == RFE_CID_CELL_BAND02)
    {
        settings.m_rxChannels = LimeRFESettings::ChannelGroups::ChannelsCellular;
        settings.m_rxCellularChannel = LimeRFESettings::CellularChannel::CellularBand2;
    }
    else if (m_rfeBoardState.channelIDRX == RFE_CID_CELL_BAND03)
    {
        settings.m_rxChannels = LimeRFESettings::ChannelGroups::ChannelsCellular;
        settings.m_rxCellularChannel = LimeRFESettings::CellularChannel::CellularBand3;
    }
    else if (m_rfeBoardState.channelIDRX == RFE_CID_CELL_BAND07)
    {
        settings.m_rxChannels = LimeRFESettings::ChannelGroups::ChannelsCellular;
        settings.m_rxCellularChannel = LimeRFESettings::CellularChannel::CellularBand7;
    }
    else if (m_rfeBoardState.channelIDRX == RFE_CID_CELL_BAND38)
    {
        settings.m_rxChannels = LimeRFESettings::ChannelGroups::ChannelsCellular;
        settings.m_rxCellularChannel = LimeRFESettings::CellularChannel::CellularBand38;
    }
    else if (m_rfeBoardState.channelIDRX == RFE_CID_WB_1000)
    {
        settings.m_rxChannels = LimeRFESettings::ChannelGroups::ChannelsWideband;
        settings.m_rxWidebandChannel = LimeRFESettings::WidebandChannel::WidebandLow;
    }
    else if (m_rfeBoardState.channelIDRX == RFE_CID_WB_4000)
    {
        settings.m_rxChannels = LimeRFESettings::ChannelGroups::ChannelsWideband;
        settings.m_rxWidebandChannel = LimeRFESettings::WidebandChannel::WidebandHigh;
    }
    else if (m_rfeBoardState.channelIDRX == RFE_CID_HAM_0030)
    {
        settings.m_rxChannels = LimeRFESettings::ChannelGroups::ChannelsHAM;
        settings.m_rxHAMChannel = LimeRFESettings::HAMChannel::HAM_30M;
    }
    else if (m_rfeBoardState.channelIDRX == RFE_CID_HAM_0070)
    {
        settings.m_rxChannels = LimeRFESettings::ChannelGroups::ChannelsHAM;
        settings.m_rxHAMChannel = LimeRFESettings::HAMChannel::HAM_50_70MHz;
    }
    else if (m_rfeBoardState.channelIDRX == RFE_CID_HAM_0145)
    {
        settings.m_rxChannels = LimeRFESettings::ChannelGroups::ChannelsHAM;
        settings.m_rxHAMChannel = LimeRFESettings::HAMChannel::HAM_144_146MHz;
    }
    else if (m_rfeBoardState.channelIDRX == RFE_CID_HAM_0220)
    {
        settings.m_rxChannels = LimeRFESettings::ChannelGroups::ChannelsHAM;
        settings.m_rxHAMChannel = LimeRFESettings::HAMChannel::HAM_220_225MHz;
    }
    else if (m_rfeBoardState.channelIDRX == RFE_CID_HAM_0435)
    {
        settings.m_rxChannels = LimeRFESettings::ChannelGroups::ChannelsHAM;
        settings.m_rxHAMChannel = LimeRFESettings::HAMChannel::HAM_430_440MHz;
    }
    else if (m_rfeBoardState.channelIDRX == RFE_CID_HAM_0920)
    {
        settings.m_rxChannels = LimeRFESettings::ChannelGroups::ChannelsHAM;
        settings.m_rxHAMChannel = LimeRFESettings::HAMChannel::HAM_902_928MHz;
    }
    else if (m_rfeBoardState.channelIDRX == RFE_CID_HAM_1280)
    {
        settings.m_rxChannels = LimeRFESettings::ChannelGroups::ChannelsHAM;
        settings.m_rxHAMChannel = LimeRFESettings::HAMChannel::HAM_1240_1325MHz;
    }
    else if (m_rfeBoardState.channelIDRX == RFE_CID_HAM_2400)
    {
        settings.m_rxChannels = LimeRFESettings::ChannelGroups::ChannelsHAM;
        settings.m_rxHAMChannel = LimeRFESettings::HAMChannel::HAM_2300_2450MHz;
    }
    else if (m_rfeBoardState.channelIDRX == RFE_CID_HAM_3500)
    {
        settings.m_rxChannels = LimeRFESettings::ChannelGroups::ChannelsHAM;
        settings.m_rxHAMChannel = LimeRFESettings::HAMChannel::HAM_3300_3500MHz;
    }

    if (m_rfeBoardState.selPortRX == RFE_PORT_1) {
        settings.m_rxPort = LimeRFESettings::RxPort::RxPortJ3;
    } else if (m_rfeBoardState.selPortRX == RFE_PORT_3) {
        settings.m_rxPort = LimeRFESettings::RxPort::RxPortJ5;
    }

    if (m_rfeBoardState.channelIDTX == RFE_CID_CELL_BAND01)
    {
        settings.m_txChannels = LimeRFESettings::ChannelGroups::ChannelsCellular;
        settings.m_txCellularChannel = LimeRFESettings::CellularChannel::CellularBand1;
    }
    else if (m_rfeBoardState.channelIDTX == RFE_CID_CELL_BAND02)
    {
        settings.m_txChannels = LimeRFESettings::ChannelGroups::ChannelsCellular;
        settings.m_txCellularChannel = LimeRFESettings::CellularChannel::CellularBand2;
    }
    else if (m_rfeBoardState.channelIDTX == RFE_CID_CELL_BAND03)
    {
        settings.m_txChannels = LimeRFESettings::ChannelGroups::ChannelsCellular;
        settings.m_txCellularChannel = LimeRFESettings::CellularChannel::CellularBand3;
    }
    else if (m_rfeBoardState.channelIDTX == RFE_CID_CELL_BAND07)
    {
        settings.m_txChannels = LimeRFESettings::ChannelGroups::ChannelsCellular;
        settings.m_txCellularChannel = LimeRFESettings::CellularChannel::CellularBand7;
    }
    else if (m_rfeBoardState.channelIDTX == RFE_CID_CELL_BAND38)
    {
        settings.m_txChannels = LimeRFESettings::ChannelGroups::ChannelsCellular;
        settings.m_txCellularChannel = LimeRFESettings::CellularChannel::CellularBand38;
    }
    else if (m_rfeBoardState.channelIDTX == RFE_CID_WB_1000)
    {
        settings.m_txChannels = LimeRFESettings::ChannelGroups::ChannelsWideband;
        settings.m_txWidebandChannel = LimeRFESettings::WidebandChannel::WidebandLow;
    }
    else if (m_rfeBoardState.channelIDTX == RFE_CID_WB_4000)
    {
        settings.m_txChannels = LimeRFESettings::ChannelGroups::ChannelsWideband;
        settings.m_txWidebandChannel = LimeRFESettings::WidebandChannel::WidebandHigh;
    }
    else if (m_rfeBoardState.channelIDTX == RFE_CID_HAM_0030)
    {
        settings.m_txChannels = LimeRFESettings::ChannelGroups::ChannelsHAM;
        settings.m_txHAMChannel = LimeRFESettings::HAMChannel::HAM_30M;
    }
    else if (m_rfeBoardState.channelIDTX == RFE_CID_HAM_0070)
    {
        settings.m_txChannels = LimeRFESettings::ChannelGroups::ChannelsHAM;
        settings.m_txHAMChannel = LimeRFESettings::HAMChannel::HAM_50_70MHz;
    }
    else if (m_rfeBoardState.channelIDTX == RFE_CID_HAM_0145)
    {
        settings.m_txChannels = LimeRFESettings::ChannelGroups::ChannelsHAM;
        settings.m_txHAMChannel = LimeRFESettings::HAMChannel::HAM_144_146MHz;
    }
    else if (m_rfeBoardState.channelIDTX == RFE_CID_HAM_0220)
    {
        settings.m_txChannels = LimeRFESettings::ChannelGroups::ChannelsHAM;
        settings.m_txHAMChannel = LimeRFESettings::HAMChannel::HAM_220_225MHz;
    }
    else if (m_rfeBoardState.channelIDTX == RFE_CID_HAM_0435)
    {
        settings.m_txChannels = LimeRFESettings::ChannelGroups::ChannelsHAM;
        settings.m_txHAMChannel = LimeRFESettings::HAMChannel::HAM_430_440MHz;
    }
    else if (m_rfeBoardState.channelIDTX == RFE_CID_HAM_0920)
    {
        settings.m_txChannels = LimeRFESettings::ChannelGroups::ChannelsHAM;
        settings.m_txHAMChannel = LimeRFESettings::HAMChannel::HAM_902_928MHz;
    }
    else if (m_rfeBoardState.channelIDTX == RFE_CID_HAM_1280)
    {
        settings.m_txChannels = LimeRFESettings::ChannelGroups::ChannelsHAM;
        settings.m_txHAMChannel = LimeRFESettings::HAMChannel::HAM_1240_1325MHz;
    }
    else if (m_rfeBoardState.channelIDTX == RFE_CID_HAM_2400)
    {
        settings.m_txChannels = LimeRFESettings::ChannelGroups::ChannelsHAM;
        settings.m_txHAMChannel = LimeRFESettings::HAMChannel::HAM_2300_2450MHz;
    }
    else if (m_rfeBoardState.channelIDTX == RFE_CID_HAM_3500)
    {
        settings.m_txChannels = LimeRFESettings::ChannelGroups::ChannelsHAM;
        settings.m_txHAMChannel = LimeRFESettings::HAMChannel::HAM_3300_3500MHz;
    }

    if (m_rfeBoardState.selPortTX == RFE_PORT_1) {
        settings.m_txPort = LimeRFESettings::TxPort::TxPortJ3;
    } else if (m_rfeBoardState.selPortTX == RFE_PORT_2) {
        settings.m_txPort = LimeRFESettings::TxPort::TxPortJ4;
    } else if (m_rfeBoardState.selPortTX == RFE_PORT_3) {
        settings.m_txPort = LimeRFESettings::TxPort::TxPortJ5;
    }

    settings.m_attenuationFactor = m_rfeBoardState.attValue;
    settings.m_amfmNotch =  m_rfeBoardState.notchOnOff == RFE_NOTCH_ON;

    if (m_rfeBoardState.mode == RFE_MODE_RX)
    {
        settings.m_rxOn = true;
        settings.m_txOn = false;
    }
    else if (m_rfeBoardState.mode == RFE_MODE_TX)
    {
        settings.m_rxOn = false;
        settings.m_txOn = true;
    }
    else if (m_rfeBoardState.mode == RFE_MODE_NONE)
    {
        settings.m_rxOn = false;
        settings.m_txOn = false;
    }
    else if (m_rfeBoardState.mode == RFE_MODE_TXRX)
    {
        settings.m_rxOn = true;
        settings.m_txOn = true;
    }

    settings.m_swrEnable = m_rfeBoardState.enableSWR == RFE_SWR_ENABLE;
    settings.m_swrSource = m_rfeBoardState.sourceSWR == RFE_SWR_SRC_CELL ?
        LimeRFESettings::SWRSource::SWRCellular :
        LimeRFESettings::SWRSource::SWRExternal;
}

void LimeRFE::networkManagerFinished(QNetworkReply *reply)
{
    QNetworkReply::NetworkError replyError = reply->error();

    if (replyError)
    {
        qWarning() << "LimeRFE::networkManagerFinished:"
                << " error(" << (int) replyError
                << "): " << replyError
                << ": " << reply->errorString();
    }
    else
    {
        QString answer = reply->readAll();
        answer.chop(1); // remove last \n
        qDebug("LimeRFE::networkManagerFinished: reply:\n%s", answer.toStdString().c_str());
    }

    reply->deleteLater();
}
