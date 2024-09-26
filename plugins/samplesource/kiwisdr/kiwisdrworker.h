///////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2012 maintech GmbH, Otto-Hahn-Str. 15, 97204 Hoechberg, Germany //
// written by Christian Daniel                                                   //
// Copyright (C) 2014 John Greb <hexameron@spam.no>                              //
// Copyright (C) 2015-2019, 2022 Edouard Griffiths, F4EXB <f4exb06@gmail.com>    //
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

#ifndef _KIWISDR_KIWISDRWORKER_H_
#define _KIWISDR_KIWISDRWORKER_H_

#include <QTimer>
#include <QtWebSockets/QtWebSockets>

#include "dsp/samplesinkfifo.h"
#include "util/message.h"

class MessageQueue;

class KiwiSDRWorker : public QObject {
	Q_OBJECT

public:
	class MsgReportSampleRate : public Message {
		MESSAGE_CLASS_DECLARATION

	public:
		int getSampleRate() const { return m_sampleRate; }

		static MsgReportSampleRate* create(int sampleRate) {
			return new MsgReportSampleRate(sampleRate);
		}

	private:
		int m_sampleRate;

		MsgReportSampleRate(int sampleRate) :
			Message(),
			m_sampleRate(sampleRate)
		{ }
	};

	class MsgReportPosition : public Message {
		MESSAGE_CLASS_DECLARATION

	public:
		float getLatitude() const { return m_latitude; }
		float getLongitude() const { return m_longitude; }
		float getAltitude() const { return m_altitude; }

		static MsgReportPosition* create(float latitude, float longitude, float altitude) {
			return new MsgReportPosition(latitude, longitude, altitude);
		}

	private:
		float m_latitude;
		float m_longitude;
		float m_altitude;

		MsgReportPosition(float latitude, float longitude, float altitude) :
			Message(),
			m_latitude(latitude),
			m_longitude(longitude),
			m_altitude(altitude)
		{ }
	};

	KiwiSDRWorker(SampleSinkFifo* sampleFifo);
    int getStatus() const { return m_status; }
    void setInputMessageQueue(MessageQueue *messageQueue) { m_inputMessageQueue = messageQueue; }

private:
	QTimer m_timer;
	QWebSocket m_webSocket;

	SampleVector m_samplesBuf;
	SampleSinkFifo* m_sampleFifo;

	QString m_serverAddress;
	uint64_t m_centerFrequency;
    int m_sampleRate;
    MessageQueue *m_inputMessageQueue;

	uint32_t m_gain;
	bool m_useAGC;

    int m_status; //!< See GUI for status number detail

	void sendCenterFrequency();
	void sendGain();

signals:
	void updateStatus(int status);

public slots:
	void onCenterFrequencyChanged(quint64 centerFrequency);
	void onServerAddressChanged(QString serverAddress);
	void onGainChanged(quint32 gain, bool useAGC);

private slots:
	void onConnected();
    void onDisconnected();
	void onBinaryMessageReceived(const QByteArray &message);
	void onSocketError(QAbstractSocket::SocketError error);

    void tick();
};

#endif // _KIWISDR_KIWISDRWORKER_H_
