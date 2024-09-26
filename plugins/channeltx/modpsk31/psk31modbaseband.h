///////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2019-2021 Edouard Griffiths, F4EXB <f4exb06@gmail.com>          //
// Copyright (C) 2020, 2023 Jon Beniston, M7RCE <jon@beniston.com>               //
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

#ifndef INCLUDE_PSK31MODBASEBAND_H
#define INCLUDE_PSK31MODBASEBAND_H

#include <QObject>
#include <QRecursiveMutex>

#include "dsp/samplesourcefifo.h"
#include "util/message.h"
#include "util/messagequeue.h"

#include "psk31modsource.h"

class UpChannelizer;
class ChannelAPI;

class PSK31Baseband : public QObject
{
    Q_OBJECT
public:
    class MsgConfigurePSK31Baseband : public Message {
        MESSAGE_CLASS_DECLARATION

    public:
        const PSK31Settings& getSettings() const { return m_settings; }
        bool getForce() const { return m_force; }

        static MsgConfigurePSK31Baseband* create(const PSK31Settings& settings, bool force)
        {
            return new MsgConfigurePSK31Baseband(settings, force);
        }

    private:
        PSK31Settings m_settings;
        bool m_force;

        MsgConfigurePSK31Baseband(const PSK31Settings& settings, bool force) :
            Message(),
            m_settings(settings),
            m_force(force)
        { }
    };

    PSK31Baseband();
    ~PSK31Baseband();
    void reset();
    void pull(const SampleVector::iterator& begin, unsigned int nbSamples);
    MessageQueue *getInputMessageQueue() { return &m_inputMessageQueue; } //!< Get the queue for asynchronous inbound communication
    void setMessageQueueToGUI(MessageQueue* messageQueue) { m_source.setMessageQueueToGUI(messageQueue); }
    double getMagSq() const { return m_source.getMagSq(); }
    int getChannelSampleRate() const;
    void setSpectrumSampleSink(BasebandSampleSink* sampleSink) { m_source.setSpectrumSink(sampleSink); }
    void setChannel(ChannelAPI *channel);
    int getSourceChannelSampleRate() const { return m_source.getChannelSampleRate(); }

signals:
    /**
     * Level changed
     * \param rmsLevel RMS level in range 0.0 - 1.0
     * \param peakLevel Peak level in range 0.0 - 1.0
     * \param numSamples Number of audio samples analyzed
     */
    void levelChanged(qreal rmsLevel, qreal peakLevel, int numSamples);

private:
    SampleSourceFifo m_sampleFifo;
    UpChannelizer *m_channelizer;
    PSK31Source m_source;
    MessageQueue m_inputMessageQueue; //!< Queue for asynchronous inbound communication
    PSK31Settings m_settings;
    QRecursiveMutex m_mutex;

    void processFifo(SampleVector& data, unsigned int iBegin, unsigned int iEnd);
    bool handleMessage(const Message& cmd);
    void applySettings(const PSK31Settings& settings, bool force = false);

private slots:
    void handleInputMessages();
    void handleData(); //!< Handle data when samples have to be processed
};


#endif // INCLUDE_PSK31MODBASEBAND_H
