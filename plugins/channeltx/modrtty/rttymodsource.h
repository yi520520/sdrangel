///////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2020-2021, 2023 Jon Beniston, M7RCE <jon@beniston.com>          //
// Copyright (C) 2020-2021 Edouard Griffiths, F4EXB <f4exb06@gmail.com>          //
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

#ifndef INCLUDE_RTTYMODSOURCE_H
#define INCLUDE_RTTYMODSOURCE_H

#include <QMutex>
#include <QDebug>
#include <QVector>

#include "dsp/channelsamplesource.h"
#include "dsp/nco.h"
#include "dsp/interpolator.h"
#include "dsp/firfilter.h"
#include "dsp/raisedcosine.h"
#include "util/movingaverage.h"
#include "util/baudot.h"

#include "rttymodsettings.h"

class BasebandSampleSink;
class ChannelAPI;

class RttyModSource : public ChannelSampleSource
{
public:
    RttyModSource();
    virtual ~RttyModSource();

    virtual void pull(SampleVector::iterator begin, unsigned int nbSamples);
    virtual void pullOne(Sample& sample);
    virtual void prefetch(unsigned int nbSamples) { (void) nbSamples; }

    double getMagSq() const { return m_magsq; }
    void getLevels(qreal& rmsLevel, qreal& peakLevel, int& numSamples) const
    {
        rmsLevel = m_rmsLevel;
        peakLevel = m_peakLevelOut;
        numSamples = m_levelNbSamples;
    }
    void setMessageQueueToGUI(MessageQueue* messageQueue) { m_messageQueueToGUI = messageQueue; }
    void setSpectrumSink(BasebandSampleSink *sampleSink) { m_spectrumSink = sampleSink; }
    void applySettings(const RttyModSettings& settings, bool force = false);
    void applyChannelSettings(int channelSampleRate, int channelFrequencyOffset, bool force = false);
    void addTXText(QString data);
    void setChannel(ChannelAPI *channel) { m_channel = channel; }
    int getChannelSampleRate() const { return m_channelSampleRate; }

private:
    int m_channelSampleRate;
    int m_channelFrequencyOffset;
    int m_spectrumRate;
    RttyModSettings m_settings;
    ChannelAPI *m_channel;

    NCO m_carrierNco;
    double m_fmPhase;                   // Double gives cleaner spectrum than Real
    double m_phaseSensitivity;
    Real m_linearGain;
    Complex m_modSample;

    int m_bit;                          // Current bit
    RaisedCosine<Real> m_pulseShape;    // Pulse shaping filter
    Lowpass<Complex> m_lowpass;         // Low pass filter to limit RF bandwidth

    BasebandSampleSink* m_spectrumSink; // Spectrum GUI to display baseband waveform
    SampleVector m_specSampleBuffer;
    static const int m_specSampleBufferSize = 256;
    int m_specSampleBufferIndex;
    Interpolator m_interpolator;        // Interpolator to downsample to spectrum
    Real m_interpolatorDistance;
    Real m_interpolatorDistanceRemain;
    bool m_interpolatorConsumed;

    double m_magsq;
    MovingAverageUtil<double, double, 16> m_movingAverage;

    quint32 m_levelCalcCount;
    qreal m_rmsLevel;
    qreal m_peakLevelOut;
    Real m_peakLevel;
    Real m_levelSum;

    static const int m_levelNbSamples = 480;  // every 10ms assuming 48k Sa/s

    int m_sampleIdx;                    // Sample index in to symbol
    int m_samplesPerSymbol;             // Number of samples per symbol

    QString m_textToTransmit;           // Transmit buffer (before RTTY encoding)

    BaudotEncoder m_rttyEncoder;

    QList<uint8_t> m_bits;              // Bits to transmit
    int m_byteIdx;                      // Index in to m_bits
    int m_bitIdx;                       // Index in to current byte of m_bits
    int m_bitCount;                     // Count of number of valid bits in m_bits
    int m_bitCountTotal;

    QVector<qint16> m_demodBuffer;
    int m_demodBufferFill;

    MessageQueue* m_messageQueueToGUI;

    MessageQueue* getMessageQueueToGUI() { return m_messageQueueToGUI; }

    void encodeText(const QString& data);
    int getBit();                       // Get bit from m_bits
    void addBit(int bit);               // Add bit to m_bits, with zero stuffing
    void initTX();

    void calculateLevel(Real& sample);
    void modulateSample();
    void sampleToSpectrum(Complex sample);

};

#endif // INCLUDE_RTTYMODSOURCE_H
