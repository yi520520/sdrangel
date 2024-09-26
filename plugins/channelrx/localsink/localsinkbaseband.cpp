///////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2019-2020, 2022 Edouard Griffiths, F4EXB <f4exb06@gmail.com>    //
// Copyright (C) 2022 Jiří Pinkava <jiri.pinkava@rossum.ai>                      //
// Copyright (C) 2023 Jon Beniston, M7RCE <jon@beniston.com>                     //
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

#include <QDebug>

#include "dsp/downchannelizer.h"
#include "dsp/dspcommands.h"
#include "dsp/spectrumvis.h"

#include "localsinkbaseband.h"

MESSAGE_CLASS_DEFINITION(LocalSinkBaseband::MsgConfigureLocalSinkBaseband, Message)
MESSAGE_CLASS_DEFINITION(LocalSinkBaseband::MsgConfigureLocalDeviceSampleSource, Message)
MESSAGE_CLASS_DEFINITION(LocalSinkBaseband::MsgSetSpectrumSampleRateAndFrequency, Message)

LocalSinkBaseband::LocalSinkBaseband() :
    m_localSampleSource(nullptr),
    m_spectrumVis(nullptr)
{
    m_sampleFifo.setSize(SampleSinkFifo::getSizePolicy(48000));
    m_channelizer = new DownChannelizer(&m_sink);

    qDebug("LocalSinkBaseband::LocalSinkBaseband");
    QObject::connect(
        &m_sampleFifo,
        &SampleSinkFifo::dataReady,
        this,
        &LocalSinkBaseband::handleData,
        Qt::QueuedConnection
    );

    connect(&m_inputMessageQueue, SIGNAL(messageEnqueued()), this, SLOT(handleInputMessages()));
    m_sink.start(m_localSampleSource);
}

LocalSinkBaseband::~LocalSinkBaseband()
{
    m_sink.stop();
    delete m_channelizer;
}

void LocalSinkBaseband::reset()
{
    QMutexLocker mutexLocker(&m_mutex);
    m_sampleFifo.reset();
}

void LocalSinkBaseband::feed(const SampleVector::const_iterator& begin, const SampleVector::const_iterator& end)
{
    m_sampleFifo.write(begin, end);
}

void LocalSinkBaseband::handleData()
{
    QMutexLocker mutexLocker(&m_mutex);

    while ((m_sampleFifo.fill() > 0) && (m_inputMessageQueue.size() == 0))
    {
		SampleVector::iterator part1begin;
		SampleVector::iterator part1end;
		SampleVector::iterator part2begin;
		SampleVector::iterator part2end;

        std::size_t count = m_sampleFifo.readBegin(m_sampleFifo.fill(), &part1begin, &part1end, &part2begin, &part2end);

		// first part of FIFO data
        if (part1begin != part1end) {
            m_channelizer->feed(part1begin, part1end);
        }

		// second part of FIFO data (used when block wraps around)
		if(part2begin != part2end) {
            m_channelizer->feed(part2begin, part2end);
        }

		m_sampleFifo.readCommit((unsigned int) count);
    }
}

void LocalSinkBaseband::handleInputMessages()
{
	Message* message;

	while ((message = m_inputMessageQueue.pop()) != nullptr)
	{
		if (handleMessage(*message)) {
			delete message;
		}
	}
}

bool LocalSinkBaseband::handleMessage(const Message& cmd)
{
    if (MsgConfigureLocalSinkBaseband::match(cmd))
    {
        QMutexLocker mutexLocker(&m_mutex);
        MsgConfigureLocalSinkBaseband& cfg = (MsgConfigureLocalSinkBaseband&) cmd;
        qDebug() << "LocalSinkBaseband::handleMessage: MsgConfigureLocalSinkBaseband";

        applySettings(cfg.getSettings(), cfg.getSettingsKeys(), cfg.getForce());

        return true;
    }
    else if (DSPSignalNotification::match(cmd))
    {
        QMutexLocker mutexLocker(&m_mutex);
        DSPSignalNotification& notif = (DSPSignalNotification&) cmd;
        qDebug() << "LocalSinkBaseband::handleMessage: DSPSignalNotification: basebandSampleRate: " << notif.getSampleRate();
        m_sampleFifo.setSize(SampleSinkFifo::getSizePolicy(notif.getSampleRate()));
        m_channelizer->setBasebandSampleRate(notif.getSampleRate(), true); // apply decimation
        m_sink.setSampleRate(getChannelSampleRate());

		return true;
    }
    else if (MsgConfigureLocalDeviceSampleSource::match(cmd))
    {
        QMutexLocker mutexLocker(&m_mutex);
        MsgConfigureLocalDeviceSampleSource& notif  = (MsgConfigureLocalDeviceSampleSource&) cmd;
        qDebug() << "LocalSinkBaseband::handleMessage: MsgConfigureLocalDeviceSampleSource: " << notif.getDeviceSampleSource();
        m_localSampleSource = notif.getDeviceSampleSource();

        if (m_sink.isRunning()) {
            m_sink.start(m_localSampleSource);
        }

        return  true;
    }
    else if (MsgSetSpectrumSampleRateAndFrequency::match(cmd))
    {
        MsgSetSpectrumSampleRateAndFrequency& notif = (MsgSetSpectrumSampleRateAndFrequency&) cmd;

        if (m_spectrumVis)
        {
            DSPSignalNotification *msg = new DSPSignalNotification(notif.getSampleRate(), notif.getCenterFrequency());
            m_spectrumVis->getInputMessageQueue()->push(msg);
        }

        return true;
    }
    else
    {
        return false;
    }
}

void LocalSinkBaseband::applySettings(const LocalSinkSettings& settings, const QList<QString>& settingsKeys, bool force)
{
    qDebug() << "LocalSinkBaseband::applySettings:" << settings.getDebugString(settingsKeys, force) << " force: " << force;

    if (settingsKeys.contains("log2Decim")
     || settingsKeys.contains("filterChainHash") || force)
    {
        m_channelizer->setDecimation(settings.m_log2Decim, settings.m_filterChainHash);
        m_sink.setSampleRate(getChannelSampleRate());
    }

    m_sink.applySettings(settings, settingsKeys, force);
    if (force) {
        m_settings = settings;
    } else {
        m_settings.applySettings(settingsKeys, settings);
    }
}

int LocalSinkBaseband::getChannelSampleRate() const
{
    return m_channelizer->getChannelSampleRate();
}
