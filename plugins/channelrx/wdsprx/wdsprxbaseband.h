///////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2019-2022 Edouard Griffiths, F4EXB <f4exb06@gmail.com>          //
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

#ifndef INCLUDE_WDSPRXBASEBAND_H
#define INCLUDE_WDSPRXBASEBAND_H

#include <QObject>
#include <QRecursiveMutex>

#include "dsp/samplesinkfifo.h"
#include "dsp/downchannelizer.h"
#include "util/message.h"
#include "util/messagequeue.h"

#include "wdsprxsink.h"

class ChannelAPI;
class SpectrumVis;

class WDSPRxBaseband : public QObject
{
    Q_OBJECT
public:
    class MsgConfigureWDSPRxBaseband : public Message {
        MESSAGE_CLASS_DECLARATION

    public:
        const WDSPRxSettings& getSettings() const { return m_settings; }
        bool getForce() const { return m_force; }

        static MsgConfigureWDSPRxBaseband* create(const WDSPRxSettings& settings, bool force)
        {
            return new MsgConfigureWDSPRxBaseband(settings, force);
        }

    private:
        WDSPRxSettings m_settings;
        bool m_force;

        MsgConfigureWDSPRxBaseband(const WDSPRxSettings& settings, bool force) :
            Message(),
            m_settings(settings),
            m_force(force)
        { }
    };

    WDSPRxBaseband();
    ~WDSPRxBaseband();
    void reset();
    void feed(const SampleVector::const_iterator& begin, const SampleVector::const_iterator& end);
    MessageQueue *getInputMessageQueue() { return &m_inputMessageQueue; } //!< Get the queue for asynchronous inbound communication
    int getChannelSampleRate() const;
	void setSpectrumSink(SpectrumVis* spectrumSink) { m_spectrumVis = spectrumSink; m_sink.setSpectrumSink(spectrumSink); }
    double getMagSq() const { return m_sink.getMagSq(); }
    void getMagSqLevels(double& avg, double& peak, int& nbSamples) { m_sink.getMagSqLevels(avg, peak, nbSamples); }
    unsigned int getAudioSampleRate() const { return m_audioSampleRate; }
    bool getAudioActive() const { return m_sink.getAudioActive(); }
    void setBasebandSampleRate(int sampleRate);
    void setMessageQueueToGUI(MessageQueue *messageQueue) { m_messageQueueToGUI = messageQueue; }
    void setChannel(ChannelAPI *channel);
    void setFifoLabel(const QString& label) { m_sampleFifo.setLabel(label); }
    void setAudioFifoLabel(const QString& label) { m_sink.setAudioFifoLabel(label); }

private:
    SampleSinkFifo m_sampleFifo;
    DownChannelizer m_channelizer;
    WDSPRxSink m_sink;
	MessageQueue m_inputMessageQueue; //!< Queue for asynchronous inbound communication
    WDSPRxSettings m_settings;
    unsigned int m_audioSampleRate;
    int m_channelSampleRate;
    MessageQueue *m_messageQueueToGUI;
    SpectrumVis *m_spectrumVis;
    QRecursiveMutex m_mutex;

    bool handleMessage(const Message& cmd);
    void applySettings(const WDSPRxSettings& settings, bool force = false);
    MessageQueue *getMessageQueueToGUI() { return m_messageQueueToGUI; }

private slots:
    void handleInputMessages();
    void handleData(); //!< Handle data when samples have to be processed
};

#endif // INCLUDE_WDSPRXBASEBAND_H
