///////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2019-2020, 2022 Edouard Griffiths, F4EXB <f4exb06@gmail.com>    //
// Copyright (C) 2019 Vort <vvort@yandex.ru>                                     //
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

#include <boost/endian/conversion.hpp>
#include "util/messagequeue.h"
#include "kiwisdrworker.h"

MESSAGE_CLASS_DEFINITION(KiwiSDRWorker::MsgReportSampleRate, Message)
MESSAGE_CLASS_DEFINITION(KiwiSDRWorker::MsgReportPosition, Message)

KiwiSDRWorker::KiwiSDRWorker(SampleSinkFifo* sampleFifo) :
	QObject(),
	m_timer(this),
	m_samplesBuf(),
	m_sampleFifo(sampleFifo),
	m_centerFrequency(1450000),
    m_sampleRate(12000),
    m_inputMessageQueue(nullptr),
	m_gain(20),
	m_useAGC(true),
    m_status(0)
{
	connect(&m_timer, SIGNAL(timeout()), this, SLOT(tick()));

	m_webSocket.setParent(this);
	connect(&m_webSocket, &QWebSocket::connected,
		this, &KiwiSDRWorker::onConnected);
	connect(&m_webSocket, &QWebSocket::binaryMessageReceived,
		this, &KiwiSDRWorker::onBinaryMessageReceived);
	connect(&m_webSocket, QOverload<QAbstractSocket::SocketError>::of(&QWebSocket::error),
		this, &KiwiSDRWorker::onSocketError);
    connect(&m_webSocket, &QWebSocket::disconnected,
        this, &KiwiSDRWorker::onDisconnected);
}

void KiwiSDRWorker::onConnected()
{
	m_webSocket.sendTextMessage("SET auth t=kiwi p=#");
}

void KiwiSDRWorker::onDisconnected()
{
    qDebug("KiwiSDRWorker::onDisconnected");
    m_status = 4;
	emit updateStatus(4);
}

void KiwiSDRWorker::onSocketError(QAbstractSocket::SocketError error)
{
	(void) error;
    m_status = 3;
	emit updateStatus(3);
}

void KiwiSDRWorker::sendCenterFrequency()
{
	if (!m_webSocket.isValid())
		return;

	QString freq = QString::number(m_centerFrequency / 1000.0, 'f', 3);
    int bw = (m_sampleRate/2) - 20;
	QString msg = QString("SET mod=iq low_cut=-%1 high_cut=%2 freq=%3").arg(bw).arg(bw).arg(freq);
	m_webSocket.sendTextMessage(msg);
}

void KiwiSDRWorker::sendGain()
{
	if (!m_webSocket.isValid())
		return;

	QString msg("SET agc=");
	msg.append(m_useAGC ? "1" : "0");
	msg.append(" hang=0 thresh=-130 slope=6 decay=1000 manGain=");
	msg.append(QString::number(m_gain));
	m_webSocket.sendTextMessage(msg);
}

void KiwiSDRWorker::onBinaryMessageReceived(const QByteArray &message)
{
	if (message[0] == 'M' && message[1] == 'S' && message[2] == 'G')
	{
		QStringList al = QString::fromUtf8(message).split(' ');

        if ((al.size() > 2) && al[2].startsWith("audio_rate="))
        {
            QStringList rateKeyVal = al[2].split('=');

            if (rateKeyVal.size() > 1)
            {
                bool ok;
                int sampleRate = rateKeyVal[1].toInt(&ok);

                if (ok) {
                    m_sampleRate = sampleRate;
                }

                qDebug("KiwiSDRWorker::onBinaryMessageReceived: sample rate: %d", m_sampleRate);

                if (m_inputMessageQueue) {
                    m_inputMessageQueue->push(MsgReportSampleRate::create(m_sampleRate));
                }

                QString msg = QString("SET AR OK in=%1 out=48000").arg(m_sampleRate);
                m_webSocket.sendTextMessage(msg);
                m_webSocket.sendTextMessage("SERVER DE CLIENT KiwiAngel SND");
                sendGain();
                sendCenterFrequency();
                m_timer.start(5000);
                m_status = 2;
                emit updateStatus(2);
            }
        }
		else if ((al.size() >= 2) && al[1].startsWith("load_cfg="))
		{
			QByteArray urlEncoded = al[1].mid(9).toLatin1();
			QString json =  QUrl::fromPercentEncoding(urlEncoded);
			QJsonDocument doc = QJsonDocument::fromJson(json.toUtf8());

			if (doc.isObject())
			{
				QJsonObject obj = doc.object();

				if (obj.contains("rx_gps"))
				{
					QString gps = obj.value("rx_gps").toString();
					QRegularExpression re("\\((-?[0-9]+(\\.[0-9]+)?), *(-?[0-9]+(\\.[0-9]+)?)\\)");
					QRegularExpressionMatch match = re.match(gps);

					if (match.hasMatch())
					{
						float latitude = match.captured(1).toFloat();
						float longitude = match.captured(3).toFloat();
						float altitude = 0.0f;
						if (obj.contains("rx_asl")) {
							altitude = (float) obj.value("rx_asl").toInt();
						}
						if (m_inputMessageQueue) {
							m_inputMessageQueue->push(MsgReportPosition::create(latitude, longitude, altitude));
						}
					}
				}
			}
			else
			{
				qDebug() << "KiwiSDRWorker::onBinaryMessageReceived - Document is not an object";
			}
		}
	}
	else if (message[0] == 'S' && message[1] == 'N' && message[2] == 'D')
	{
		int dataOffset = 20;
		int sampleCount = 512;
		const int16_t* messageSamples = (const int16_t*)(message.constData() + dataOffset);

		m_samplesBuf.clear();
		for (int i = 0; i < sampleCount; i++)
		{
			m_samplesBuf.push_back(Sample(
				boost::endian::endian_reverse(messageSamples[i * 2]) << (SDR_RX_SAMP_SZ - 16),
				boost::endian::endian_reverse(messageSamples[i * 2 + 1]) << (SDR_RX_SAMP_SZ - 16)
			));
		}

		m_sampleFifo->write(m_samplesBuf.begin(), m_samplesBuf.end());
	}
}

void KiwiSDRWorker::onCenterFrequencyChanged(quint64 centerFrequency)
{
	if (m_centerFrequency == centerFrequency)
		return;

	m_centerFrequency = centerFrequency;
	sendCenterFrequency();
}

void KiwiSDRWorker::onGainChanged(quint32 gain, bool useAGC)
{
	if (m_gain == gain && m_useAGC == useAGC)
		return;

	m_gain = gain;
	m_useAGC = useAGC;

	sendGain();
}

void KiwiSDRWorker::onServerAddressChanged(QString serverAddress)
{
	if (m_serverAddress == serverAddress) {
		return;
    }

	m_serverAddress = serverAddress;

    m_status = 1;
	emit updateStatus(1);

	QString url("ws://");
	url.append(m_serverAddress);
	url.append("/kiwi/");
	url.append(QString::number(QDateTime::currentMSecsSinceEpoch()));
	url.append("/SND");
	m_webSocket.open(QUrl(url));
}

void KiwiSDRWorker::tick()
{
	m_webSocket.sendTextMessage("SET keepalive");
}
