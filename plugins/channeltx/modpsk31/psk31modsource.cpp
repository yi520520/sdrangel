///////////////////////////////////////////////////////////////////////////////////
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

#include <cctype>
#include <QDebug>

#include "dsp/basebandsamplesink.h"
#include "dsp/datafifo.h"
#include "psk31mod.h"
#include "psk31modsource.h"
#include "util/messagequeue.h"
#include "maincore.h"

PSK31Source::PSK31Source() :
    m_channelSampleRate(48000),
    m_channelFrequencyOffset(0),
    m_spectrumRate(2000),
    m_spectrumSink(nullptr),
    m_specSampleBufferIndex(0),
    m_magsq(0.0),
    m_levelCalcCount(0),
    m_peakLevel(0.0f),
    m_levelSum(0.0f),
    m_byteIdx(0),
    m_bitIdx(0),
    m_bitCount(0)
 {
    m_bits.append(0);
    m_lowpass.create(301, m_channelSampleRate, 100.0 / 2.0);
    m_pulseShape.create(0.5, 6, m_channelSampleRate / 31.25, true);

    m_demodBuffer.resize(1<<12);
    m_demodBufferFill = 0;

    m_specSampleBuffer.resize(m_specSampleBufferSize);
    m_interpolatorDistanceRemain = 0;
    m_interpolatorConsumed = false;
    m_interpolatorDistance = (Real)m_channelSampleRate / (Real)m_spectrumRate;
    m_interpolator.create(48, m_spectrumRate, m_spectrumRate / 2.2, 3.0);

    applySettings(m_settings, true);
    applyChannelSettings(m_channelSampleRate, m_channelFrequencyOffset, true);
}

PSK31Source::~PSK31Source()
{
}

void PSK31Source::pull(SampleVector::iterator begin, unsigned int nbSamples)
{
    std::for_each(
        begin,
        begin + nbSamples,
        [this](Sample& s) {
            pullOne(s);
        }
    );
}

void PSK31Source::pullOne(Sample& sample)
{
    if (m_settings.m_channelMute)
    {
        sample.m_real = 0.0f;
        sample.m_imag = 0.0f;
        return;
    }

    // Calculate next sample
    modulateSample();

    // Shift to carrier frequency
    Complex ci = m_modSample;
    ci *= m_carrierNco.nextIQ();

    // Calculate power
    double magsq = ci.real() * ci.real() + ci.imag() * ci.imag();
    m_movingAverage(magsq);
    m_magsq = m_movingAverage.asDouble();

    // Convert from float to fixed point
    sample.m_real = (FixReal) (ci.real() * SDR_TX_SCALEF);
    sample.m_imag = (FixReal) (ci.imag() * SDR_TX_SCALEF);
}

void PSK31Source::sampleToSpectrum(Complex sample)
{
    if (m_spectrumSink)
    {
        Complex out;
        if (m_interpolator.decimate(&m_interpolatorDistanceRemain, sample, &out))
        {
            m_interpolatorDistanceRemain += m_interpolatorDistance;
            Real r = std::real(out) * SDR_TX_SCALEF;
            Real i = std::imag(out) * SDR_TX_SCALEF;
            m_specSampleBuffer[m_specSampleBufferIndex++] = Sample(r, i);
            if (m_specSampleBufferIndex == m_specSampleBufferSize)
            {
                m_spectrumSink->feed(m_specSampleBuffer.begin(), m_specSampleBuffer.end(), false);
                m_specSampleBufferIndex = 0;
            }
        }
    }
}

void PSK31Source::modulateSample()
{
    Real mod;

    if (m_sampleIdx == 0)
    {
        if (m_bitCount == 0)
        {
            if (!m_textToTransmit.isEmpty())
            {
                // Encode a character at a time, so we get a TxReport after each character
                QString s = m_textToTransmit.left(1);
                m_textToTransmit = m_textToTransmit.mid(1);
                encodeText(s);
            }
            else
            {
                encodeIdle();
            }
            initTX();
        }

        m_bit = getBit();

        // Differential encoding
        m_prevSymbol = m_symbol;
        m_symbol = (m_bit ^ m_symbol) ? 0 : 1;
    }

    // PSK
    if (m_settings.m_pulseShaping)
    {
        if (m_sampleIdx == 1) {
            mod = m_pulseShape.filter(m_symbol ? 1.0f : -1.0f);
        } else {
            mod = m_pulseShape.filter(0.0f);
        }
    }
    else
    {
        mod = m_symbol ? 1.0f : -1.0f;
    }

    m_sampleIdx++;
    if (m_sampleIdx >= m_samplesPerSymbol) {
        m_sampleIdx = 0;
    }

    if (!m_settings.m_rfNoise)
    {
        m_modSample.real(m_linearGain * mod);
        m_modSample.imag(0.0f);
    }
    else
    {
        // Noise to test filter frequency response
        m_modSample.real(m_linearGain * ((Real)rand()/((Real)RAND_MAX)-0.5f));
        m_modSample.imag(m_linearGain * ((Real)rand()/((Real)RAND_MAX)-0.5f));
    }

    // Apply low pass filter to limit RF BW
    m_modSample = m_lowpass.filter(m_modSample);

    // Display in spectrum analyser
    sampleToSpectrum(m_modSample);

    Real s = std::real(m_modSample);
    calculateLevel(s);

    // Send to demod analyser
    m_demodBuffer[m_demodBufferFill] = mod * std::numeric_limits<int16_t>::max();
    ++m_demodBufferFill;

    if (m_demodBufferFill >= m_demodBuffer.size())
    {
        QList<ObjectPipe*> dataPipes;
        MainCore::instance()->getDataPipes().getDataPipes(m_channel, "demod", dataPipes);

        if (dataPipes.size() > 0)
        {
            QList<ObjectPipe*>::iterator it = dataPipes.begin();

            for (; it != dataPipes.end(); ++it)
            {
                DataFifo *fifo = qobject_cast<DataFifo*>((*it)->m_element);

                if (fifo) {
                    fifo->write((quint8*) &m_demodBuffer[0], m_demodBuffer.size() * sizeof(qint16), DataFifo::DataTypeI16);
                }
            }
        }

        m_demodBufferFill = 0;
    }
}

void PSK31Source::calculateLevel(Real& sample)
{
    if (m_levelCalcCount < m_levelNbSamples)
    {
        m_peakLevel = std::max(std::fabs(m_peakLevel), sample);
        m_levelSum += sample * sample;
        m_levelCalcCount++;
    }
    else
    {
        m_rmsLevel = sqrt(m_levelSum / m_levelNbSamples);
        m_peakLevelOut = m_peakLevel;
        m_peakLevel = 0.0f;
        m_levelSum = 0.0f;
        m_levelCalcCount = 0;
    }
}

void PSK31Source::applySettings(const PSK31Settings& settings, bool force)
{
    if ((settings.m_baud != m_settings.m_baud) || force)
    {
        m_samplesPerSymbol = m_channelSampleRate / settings.m_baud;
        qDebug() << "m_samplesPerSymbol: " << m_samplesPerSymbol << " (" << m_channelSampleRate << "/" << settings.m_baud << ")";
    }

    if ((settings.m_lpfTaps != m_settings.m_lpfTaps) || (settings.m_rfBandwidth != m_settings.m_rfBandwidth) || force)
    {
        qDebug() << "PSK31Source::applySettings: Creating new lpf with taps " << settings.m_lpfTaps << " rfBW " << settings.m_rfBandwidth;
        m_lowpass.create(settings.m_lpfTaps, m_channelSampleRate, settings.m_rfBandwidth / 2.0);
    }
    if ((settings.m_beta != m_settings.m_beta) || (settings.m_symbolSpan != m_settings.m_symbolSpan) || (settings.m_baud != m_settings.m_baud) || force)
    {
        qDebug() << "PSK31Source::applySettings: Recreating pulse shaping filter: "
                << " beta: " << settings.m_beta
                << " symbolSpan: " << settings.m_symbolSpan
                << " channelSampleRate:" << m_channelSampleRate
                << " baud:" << settings.m_baud;
        m_pulseShape.create(settings.m_beta, settings.m_symbolSpan, m_channelSampleRate/settings.m_baud, true);
    }

    m_settings = settings;

    m_linearGain = powf(10.0f,  m_settings.m_gain/20.0f);
}

void PSK31Source::applyChannelSettings(int channelSampleRate, int channelFrequencyOffset, bool force)
{
    qDebug() << "PSK31Source::applyChannelSettings:"
            << " channelSampleRate: " << channelSampleRate
            << " channelFrequencyOffset: " << channelFrequencyOffset
            << " rfBandwidth: " << m_settings.m_rfBandwidth;

    if ((channelFrequencyOffset != m_channelFrequencyOffset)
     || (channelSampleRate != m_channelSampleRate) || force)
    {
        m_carrierNco.setFreq(channelFrequencyOffset, channelSampleRate);
    }

    if ((m_channelSampleRate != channelSampleRate) || force)
    {
        qDebug() << "PSK31Source::applyChannelSettings: Recreating filters";
        m_lowpass.create(m_settings.m_lpfTaps, channelSampleRate, m_settings.m_rfBandwidth / 2.0);
        qDebug() << "PSK31Source::applyChannelSettings: Recreating bandpass filter: "
                << " channelSampleRate:" << channelSampleRate;
        qDebug() << "PSK31Source::applyChannelSettings: Recreating pulse shaping filter: "
                << " beta: " << m_settings.m_beta
                << " symbolSpan: " << m_settings.m_symbolSpan
                << " channelSampleRate:" << m_channelSampleRate
                << " baud:" << m_settings.m_baud;
        m_pulseShape.create(m_settings.m_beta, m_settings.m_symbolSpan, channelSampleRate/m_settings.m_baud, true);
    }

    if ((m_channelSampleRate != channelSampleRate) || force)
    {
        m_interpolatorDistanceRemain = 0;
        m_interpolatorConsumed = false;
        m_interpolatorDistance = (Real) channelSampleRate / (Real) m_spectrumRate;
        m_interpolator.create(48, m_spectrumRate, m_spectrumRate / 2.2, 3.0);
    }

    m_channelSampleRate = channelSampleRate;
    m_channelFrequencyOffset = channelFrequencyOffset;
    m_samplesPerSymbol = m_channelSampleRate / m_settings.m_baud;
    qDebug() << "m_samplesPerSymbol: " << m_samplesPerSymbol << " (" << m_channelSampleRate << "/" << m_settings.m_baud << ")";

    QList<ObjectPipe*> pipes;
    MainCore::instance()->getMessagePipes().getMessagePipes(m_channel, "reportdemod", pipes);

    if (pipes.size() > 0)
    {
        for (const auto& pipe : pipes)
        {
            MessageQueue* messageQueue = qobject_cast<MessageQueue*>(pipe->m_element);
            MainCore::MsgChannelDemodReport *msg = MainCore::MsgChannelDemodReport::create(m_channel, m_channelSampleRate);
            messageQueue->push(msg);
        }
    }
}

int PSK31Source::getBit()
{
    int bit;

    if (m_bitCount > 0)
    {
        bit = (m_bits[m_byteIdx] >> m_bitIdx) & 1;
        m_bitIdx++;
        m_bitCount--;
        if (m_bitIdx == 8)
        {
            m_byteIdx++;
            m_bitIdx = 0;
        }
    }
    else
    {
        qDebug() << "PSK31Source::getBit: Called when empty";
        bit = 1;
    }

    return bit;
}

void PSK31Source::addBit(int bit)
{
    m_bits[m_byteIdx] |= bit << m_bitIdx;
    m_bitIdx++;
    m_bitCount++;
    if (m_bitIdx == 8)
    {
        m_byteIdx++;
        if (m_bits.size() <= m_byteIdx) {
            m_bits.append(0);
        }
        m_bitIdx = 0;
    }
}

void PSK31Source::initTX()
{
    m_byteIdx = 0;
    m_bitIdx = 0;
}

void PSK31Source::addTXText(QString text)
{
    int count = m_settings.m_repeat ? m_settings.m_repeatCount : 1;

    for (int i = 0; i < count; i++) {

        QString s = text;

        if (m_settings.m_prefixCRLF) {
            s.prepend("\r\r\n");
        }
        if (m_settings.m_postfixCRLF) {
            s.append("\r\r\n");
        }

        m_textToTransmit.append(s);
    }
}

void PSK31Source::encodeText(const QString& text)
{
    // PSK31 varicode encoding
    m_byteIdx = 0;
    m_bitIdx = 0;
    m_bitCount = 0;
    for (int i = 0; i < m_bits.size(); i++) {
        m_bits[i] = 0;
    }

    for (int i = 0; i < text.size(); i++)
    {
        unsigned bits;
        unsigned bitCount;

        m_psk31Encoder.encode(text[i], bits, bitCount);
        for (unsigned int j = 0; j < bitCount; j++)
        {
            int txBit = (bits >> j) & 1;
            addBit(txBit);
        }
    }

    if (getMessageQueueToGUI())
    {
        PSK31::MsgReportTx* msg = PSK31::MsgReportTx::create(text, m_textToTransmit.size());
        getMessageQueueToGUI()->push(msg);
    }
}

void PSK31Source::encodeIdle()
{
    m_byteIdx = 0;
    m_bitIdx = 0;
    m_bitCount = 0;
    for (int i = 0; i < m_bits.size(); i++) {
        m_bits[i] = 0;
    }
    addBit(0);
    addBit(0);
}
