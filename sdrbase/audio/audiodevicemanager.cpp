///////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2012 maintech GmbH, Otto-Hahn-Str. 15, 97204 Hoechberg, Germany //
// written by Christian Daniel                                                   //
// Copyright (C) 2017-2020, 2022-2023 Edouard Griffiths, F4EXB <f4exb06@gmail.com> //
// Copyright (C) 2022 Jon Beniston, M7RCE <jon@beniston.com>                     //
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

#include "audio/audiodevicemanager.h"
#include "util/simpleserializer.h"
#include "util/messagequeue.h"
#include "dsp/dspcommands.h"

#include <QThread>
#include <QDataStream>
#include <QSet>
#include <QDebug>

const float AudioDeviceManager::m_defaultAudioInputVolume = 1.0f;
const QString AudioDeviceManager::m_defaultUDPAddress = "127.0.0.1";
const QString AudioDeviceManager::m_defaultDeviceName = "System default device";

QDataStream& operator<<(QDataStream& ds, const AudioDeviceManager::InputDeviceInfo& info)
{
    ds << info.sampleRate << info.volume;
    return ds;
}

QDataStream& operator>>(QDataStream& ds, AudioDeviceManager::InputDeviceInfo& info)
{
    ds >> info.sampleRate >> info.volume;
    return ds;
}

QDataStream& operator<<(QDataStream& ds, const AudioDeviceManager::OutputDeviceInfo& info)
{
    ds << info.sampleRate
        << info.udpAddress
        << info.udpPort
        << info.copyToUDP
        << info.udpUseRTP
        << (int) info.udpChannelMode
        << (int) info.udpChannelCodec
        << info.udpDecimationFactor
        << info.fileRecordName
        << info.recordSilenceTime;
    return ds;
}

QDataStream& operator>>(QDataStream& ds, AudioDeviceManager::OutputDeviceInfo& info)
{
    int intChannelMode;
    int intChannelCodec;

    ds >> info.sampleRate
        >> info.udpAddress
        >> info.udpPort
        >> info.copyToUDP
        >> info.udpUseRTP
        >> intChannelMode
        >> intChannelCodec
        >> info.udpDecimationFactor
        >> info.fileRecordName
        >> info.recordSilenceTime;
    info.udpChannelMode = (AudioOutputDevice::UDPChannelMode) intChannelMode;
    info.udpChannelCodec = (AudioOutputDevice::UDPChannelCodec) intChannelCodec;
    return ds;
}

AudioDeviceManager::AudioDeviceManager()
{
    qDebug("AudioDeviceManager::AudioDeviceManager: scan input devices");
    {
        auto &devicesInfo = AudioDeviceInfo::availableInputDevices();

        for (int i = 0; i < devicesInfo.size(); i++) {
            qDebug("AudioDeviceManager::AudioDeviceManager: input device #%d: %s", i, qPrintable(devicesInfo[i].deviceName()));
        }
    }
    qDebug("AudioDeviceManager::AudioDeviceManager: scan output devices");

    {
        auto &devicesInfo = AudioDeviceInfo::availableOutputDevices();

        for (int i = 0; i < devicesInfo.size(); i++) {
            qDebug("AudioDeviceManager::AudioDeviceManager: output device #%d: %s", i, qPrintable(devicesInfo[i].deviceName()));
        }
    }
    m_defaultInputStarted = false;
    m_defaultOutputStarted = false;

    connect(&m_inputMessageQueue, SIGNAL(messageEnqueued()), this, SLOT(handleInputMessages()), Qt::QueuedConnection);
}

AudioDeviceManager::~AudioDeviceManager()
{
    QMap<int, AudioOutputDevice*>::iterator aoit = m_audioOutputs.begin();

    for (; aoit != m_audioOutputs.end(); ++aoit) {
        (*aoit)->getInputMessageQueue()->push(AudioOutputDevice::MsgStop::create());
    }

    QMap<int, QThread*>::iterator otit = m_audioOutputThreads.begin();

    for (; otit != m_audioOutputThreads.end(); ++otit)
    {
        (*otit)->exit();
        (*otit)->wait();
    }

    QMap<int, AudioInputDevice*>::iterator aiit = m_audioInputs.begin();

    for (; aiit != m_audioInputs.end(); ++aiit) {
        (*aiit)->getInputMessageQueue()->push(AudioInputDevice::MsgStop::create());
    }

    QMap<int, QThread*>::iterator itit = m_audioInputThreads.begin();

    for (; itit != m_audioInputThreads.end(); ++itit)
    {
        (*itit)->exit();
        (*itit)->wait();
    }
}

bool AudioDeviceManager::getOutputDeviceName(int outputDeviceIndex, QString &deviceName) const
{
    if (outputDeviceIndex < 0)
    {
        deviceName = m_defaultDeviceName;
        return true;
    }
    else
    {
        if (outputDeviceIndex < AudioDeviceInfo::availableOutputDevices().size())
        {
            deviceName = AudioDeviceInfo::availableOutputDevices()[outputDeviceIndex].deviceName();
            return true;
        }
        else
        {
            return false;
        }
    }
}

bool AudioDeviceManager::getInputDeviceName(int inputDeviceIndex, QString &deviceName) const
{
    if (inputDeviceIndex < 0)
    {
        deviceName = m_defaultDeviceName;
        return true;
    }
    else
    {
        if (inputDeviceIndex < AudioDeviceInfo::availableInputDevices().size())
        {
            deviceName = AudioDeviceInfo::availableInputDevices()[inputDeviceIndex].deviceName();
            return true;
        }
        else
        {
            return false;
        }
    }
}

int AudioDeviceManager::getOutputDeviceIndex(const QString &deviceName) const
{
    for (int i = 0; i < AudioDeviceInfo::availableOutputDevices().size(); i++)
    {
        //qDebug("AudioDeviceManager::getOutputDeviceIndex: %d: %s|%s", i, qPrintable(deviceName), qPrintable(AudioDeviceInfo::availableOutputDevices()[i].deviceName()));
        if (deviceName == AudioDeviceInfo::availableOutputDevices()[i].deviceName()) {
            return i;
        }
    }

    return -1; // system default
}

int AudioDeviceManager::getInputDeviceIndex(const QString &deviceName) const
{
    for (int i = 0; i < AudioDeviceInfo::availableInputDevices().size(); i++)
    {
        //qDebug("AudioDeviceManager::getInputDeviceIndex: %d: %s|%s", i, qPrintable(deviceName), qPrintable(AudioDeviceInfo::availableInputDevices()[i].deviceName()));
        if (deviceName == AudioDeviceInfo::availableInputDevices()[i].deviceName()) {
            return i;
        }
    }

    return -1; // system default
}


void AudioDeviceManager::resetToDefaults()
{
}

QByteArray AudioDeviceManager::serialize() const
{
    qDebug("AudioDeviceManager::serialize");
    debugAudioInputInfos();
    debugAudioOutputInfos();

    SimpleSerializer s(1);
    QByteArray data;

    serializeInputMap(data);
    s.writeBlob(1, data);
    serializeOutputMap(data);
    s.writeBlob(2, data);

    return s.final();
}

void AudioDeviceManager::serializeInputMap(QByteArray& data) const
{
    QDataStream *stream = new QDataStream(&data, QIODevice::WriteOnly);
    *stream << m_audioInputInfos;
    delete stream;
}

void AudioDeviceManager::serializeOutputMap(QByteArray& data) const
{
    QDataStream *stream = new QDataStream(&data, QIODevice::WriteOnly);
    *stream << m_audioOutputInfos;
    delete stream;
}

bool AudioDeviceManager::deserialize(const QByteArray& data)
{
    qDebug("AudioDeviceManager::deserialize");

    SimpleDeserializer d(data);

    if(!d.isValid()) {
        resetToDefaults();
        return false;
    }

    if(d.getVersion() == 1)
    {
        QByteArray data;

        d.readBlob(1, &data);
        deserializeInputMap(data);
        d.readBlob(2, &data);
        deserializeOutputMap(data);

        debugAudioInputInfos();
        debugAudioOutputInfos();

        return true;
    }
    else
    {
        resetToDefaults();
        return false;
    }
}

void AudioDeviceManager::deserializeInputMap(QByteArray& data)
{
    QDataStream readStream(&data, QIODevice::ReadOnly);
    readStream >> m_audioInputInfos;
}

void AudioDeviceManager::deserializeOutputMap(QByteArray& data)
{
    QDataStream readStream(&data, QIODevice::ReadOnly);
    readStream >> m_audioOutputInfos;
}

void AudioDeviceManager::addAudioSink(AudioFifo* audioFifo, MessageQueue *sampleSinkMessageQueue, int outputDeviceIndex)
{
    qDebug("AudioDeviceManager::addAudioSink: %d: %p", outputDeviceIndex, audioFifo);

    if (m_audioOutputs.find(outputDeviceIndex) == m_audioOutputs.end())
    {
        QThread *thread = new QThread();
        AudioOutputDevice *audioOutputDevice = new AudioOutputDevice();
        m_audioOutputs[outputDeviceIndex] = audioOutputDevice;
        m_audioOutputThreads[outputDeviceIndex] = thread;

        if (outputDeviceIndex < 0) {
            audioOutputDevice->setDeviceName("System default");
        } else {
            audioOutputDevice->setDeviceName(AudioDeviceInfo::availableOutputDevices()[outputDeviceIndex].deviceName());
        }

        qDebug("AudioDeviceManager::addAudioSink: new AudioOutputDevice on thread: %p", thread);
        audioOutputDevice->setManagerMessageQueue(&m_inputMessageQueue);
        audioOutputDevice->moveToThread(thread);

        QObject::connect(
            thread,
            &QThread::finished,
            audioOutputDevice,
            &QObject::deleteLater
        );
        QObject::connect(
            thread,
            &QThread::finished,
            thread,
            &QThread::deleteLater
        );

        thread->start();
    }

    if ((m_audioOutputs[outputDeviceIndex]->getNbFifos() == 0) &&
       ((outputDeviceIndex != -1) || !m_defaultOutputStarted))
    {
        startAudioOutput(outputDeviceIndex);
    }

    if (m_audioSinkFifos.find(audioFifo) == m_audioSinkFifos.end()) // new FIFO
    {
        m_audioOutputs[outputDeviceIndex]->addFifo(audioFifo);
        m_audioSinkFifos[audioFifo] = outputDeviceIndex; // register audio FIFO
        m_audioFifoToSinkMessageQueues[audioFifo] = sampleSinkMessageQueue;
        m_outputDeviceSinkMessageQueues[outputDeviceIndex].append(sampleSinkMessageQueue);
    }
    else
    {
        int audioOutputDeviceIndex = m_audioSinkFifos[audioFifo];

        if (audioOutputDeviceIndex != outputDeviceIndex) // change of audio device
        {
            // remove from current
            m_audioOutputs[audioOutputDeviceIndex]->removeFifo(audioFifo);
            if ((audioOutputDeviceIndex != -1) && (m_audioOutputs[audioOutputDeviceIndex]->getNbFifos() == 0)) {
                stopAudioOutput(audioOutputDeviceIndex);
            }

            m_audioOutputs[outputDeviceIndex]->addFifo(audioFifo); // add to new
            m_audioSinkFifos[audioFifo] = outputDeviceIndex; // new index
            m_outputDeviceSinkMessageQueues[audioOutputDeviceIndex].removeOne(m_audioFifoToSinkMessageQueues[audioFifo]);
            m_outputDeviceSinkMessageQueues[outputDeviceIndex].append(sampleSinkMessageQueue);
            m_audioFifoToSinkMessageQueues[audioFifo] = sampleSinkMessageQueue;
        }
    }
}

void AudioDeviceManager::removeAudioSink(AudioFifo* audioFifo)
{
    qDebug("AudioDeviceManager::removeAudioSink: %p", audioFifo);

    if (m_audioSinkFifos.find(audioFifo) == m_audioSinkFifos.end())
    {
        qWarning("AudioDeviceManager::removeAudioSink: audio FIFO %p not found", audioFifo);
        return;
    }

    int audioOutputDeviceIndex = m_audioSinkFifos[audioFifo];
    m_audioOutputs[audioOutputDeviceIndex]->removeFifo(audioFifo);

    if ((audioOutputDeviceIndex != -1) && (m_audioOutputs[audioOutputDeviceIndex]->getNbFifos() == 0)) {
        stopAudioOutput(audioOutputDeviceIndex);
    }

    m_audioSinkFifos.remove(audioFifo); // unregister audio FIFO
    m_outputDeviceSinkMessageQueues[audioOutputDeviceIndex].removeOne(m_audioFifoToSinkMessageQueues[audioFifo]);
    m_audioFifoToSinkMessageQueues.remove(audioFifo);
}

void AudioDeviceManager::addAudioSource(AudioFifo* audioFifo, MessageQueue *sampleSourceMessageQueue, int inputDeviceIndex)
{
    qDebug("AudioDeviceManager::addAudioSource: %d: %p", inputDeviceIndex, audioFifo);

    if (m_audioInputs.find(inputDeviceIndex) == m_audioInputs.end())
    {
        QThread *thread = new QThread();
        AudioInputDevice *audioInputDevice = new AudioInputDevice();
        m_audioInputs[inputDeviceIndex] = audioInputDevice;
        m_audioInputThreads[inputDeviceIndex] = thread;

        if (inputDeviceIndex < 0) {
            audioInputDevice->setDeviceName("System default");
        } else {
            audioInputDevice->setDeviceName(AudioDeviceInfo::availableOutputDevices()[inputDeviceIndex].deviceName());
        }

        qDebug("AudioDeviceManager::addAudioSource: new AudioInputDevice on thread: %p", thread);
        audioInputDevice->setManagerMessageQueue(&m_inputMessageQueue);
        audioInputDevice->moveToThread(thread);

        QObject::connect(
            thread,
            &QThread::finished,
            audioInputDevice,
            &QObject::deleteLater
        );
        QObject::connect(
            thread,
            &QThread::finished,
            thread,
            &QThread::deleteLater
        );

        thread->start();
    }

    if ((m_audioInputs[inputDeviceIndex]->getNbFifos() == 0) &&
       ((inputDeviceIndex != -1) || !m_defaultInputStarted))
    {
        startAudioInput(inputDeviceIndex);
    }

    if (m_audioSourceFifos.find(audioFifo) == m_audioSourceFifos.end()) // new FIFO
    {
        m_audioInputs[inputDeviceIndex]->addFifo(audioFifo);
        m_audioSourceFifos[audioFifo] = inputDeviceIndex; // register audio FIFO
        m_audioFifoToSourceMessageQueues[audioFifo] = sampleSourceMessageQueue;
        m_inputDeviceSourceMessageQueues[inputDeviceIndex].append(sampleSourceMessageQueue);
    }
    else
    {
        int audioInputDeviceIndex = m_audioSourceFifos[audioFifo];

        if (audioInputDeviceIndex != inputDeviceIndex) // change of audio device
        {
            // remove from current
            m_audioInputs[audioInputDeviceIndex]->removeFifo(audioFifo);
            if ((audioInputDeviceIndex != -1) && (m_audioInputs[audioInputDeviceIndex]->getNbFifos() == 0)) {
                stopAudioInput(audioInputDeviceIndex);
            }

            m_audioInputs[inputDeviceIndex]->addFifo(audioFifo); // add to new
            m_audioSourceFifos[audioFifo] = inputDeviceIndex; // new index
            m_outputDeviceSinkMessageQueues[audioInputDeviceIndex].removeOne(m_audioFifoToSourceMessageQueues[audioFifo]);
            m_inputDeviceSourceMessageQueues[inputDeviceIndex].append(sampleSourceMessageQueue);
            m_audioFifoToSourceMessageQueues[audioFifo] = sampleSourceMessageQueue;
        }
    }
}

void AudioDeviceManager::removeAudioSource(AudioFifo* audioFifo)
{
    qDebug("AudioDeviceManager::removeAudioSource: %p", audioFifo);

    if (m_audioSourceFifos.find(audioFifo) == m_audioSourceFifos.end())
    {
        qWarning("AudioDeviceManager::removeAudioSource: audio FIFO %p not found", audioFifo);
        return;
    }

    int audioInputDeviceIndex = m_audioSourceFifos[audioFifo];
    m_audioInputs[audioInputDeviceIndex]->removeFifo(audioFifo);

    if ((audioInputDeviceIndex != -1) && (m_audioInputs[audioInputDeviceIndex]->getNbFifos() == 0)) {
        stopAudioInput(audioInputDeviceIndex);
    }

    m_audioSourceFifos.remove(audioFifo); // unregister audio FIFO
    m_inputDeviceSourceMessageQueues[audioInputDeviceIndex].removeOne(m_audioFifoToSourceMessageQueues[audioFifo]);
    m_audioFifoToSourceMessageQueues.remove(audioFifo);
}

void AudioDeviceManager::startAudioOutput(int outputDeviceIndex)
{
    unsigned int sampleRate;
    QString udpAddress;
    quint16 udpPort;
    bool copyAudioToUDP;
    bool udpUseRTP;
    AudioOutputDevice::UDPChannelMode udpChannelMode;
    AudioOutputDevice::UDPChannelCodec udpChannelCodec;
    uint32_t decimationFactor;
    QString deviceName;

    if (getOutputDeviceName(outputDeviceIndex, deviceName))
    {
        if (m_audioOutputInfos.find(deviceName) == m_audioOutputInfos.end())
        {
            sampleRate = m_defaultAudioSampleRate;
            udpAddress = m_defaultUDPAddress;
            udpPort = m_defaultUDPPort;
            copyAudioToUDP = false;
            udpUseRTP = false;
            udpChannelMode = AudioOutputDevice::UDPChannelLeft;
            udpChannelCodec = AudioOutputDevice::UDPCodecL16;
            decimationFactor = 1;
        }
        else
        {
            sampleRate = m_audioOutputInfos[deviceName].sampleRate;
            udpAddress = m_audioOutputInfos[deviceName].udpAddress;
            udpPort = m_audioOutputInfos[deviceName].udpPort;
            copyAudioToUDP = m_audioOutputInfos[deviceName].copyToUDP;
            udpUseRTP = m_audioOutputInfos[deviceName].udpUseRTP;
            udpChannelMode = m_audioOutputInfos[deviceName].udpChannelMode;
            udpChannelCodec = m_audioOutputInfos[deviceName].udpChannelCodec;
            decimationFactor = m_audioOutputInfos[deviceName].udpDecimationFactor;
        }

        AudioOutputDevice::MsgStart *msg = AudioOutputDevice::MsgStart::create(outputDeviceIndex, sampleRate);
        m_audioOutputs[outputDeviceIndex]->getInputMessageQueue()->push(msg);

        m_audioOutputInfos[deviceName].udpAddress = udpAddress;
        m_audioOutputInfos[deviceName].udpPort = udpPort;
        m_audioOutputInfos[deviceName].copyToUDP = copyAudioToUDP;
        m_audioOutputInfos[deviceName].udpUseRTP = udpUseRTP;
        m_audioOutputInfos[deviceName].udpChannelMode = udpChannelMode;
        m_audioOutputInfos[deviceName].udpChannelCodec = udpChannelCodec;
        m_audioOutputInfos[deviceName].udpDecimationFactor = decimationFactor;
        m_defaultOutputStarted |= (outputDeviceIndex == -1);
    }
    else
    {
        qWarning("AudioDeviceManager::startAudioOutput: unknown device index %d", outputDeviceIndex);
    }
}

void AudioDeviceManager::stopAudioOutput(int outputDeviceIndex)
{
    AudioOutputDevice::MsgStop *msg = AudioOutputDevice::MsgStop::create();
    m_audioOutputs[outputDeviceIndex]->getInputMessageQueue()->push(msg);
}

void AudioDeviceManager::startAudioInput(int inputDeviceIndex)
{
    unsigned int sampleRate;
    float volume;
    QString deviceName;

    if (getInputDeviceName(inputDeviceIndex, deviceName))
    {
        if (m_audioInputInfos.find(deviceName) == m_audioInputInfos.end())
        {
            sampleRate = m_defaultAudioSampleRate;
            volume = m_defaultAudioInputVolume;
        }
        else
        {
            sampleRate = m_audioInputInfos[deviceName].sampleRate;
            volume = m_audioInputInfos[deviceName].volume;
        }

        AudioInputDevice::MsgStart *msg = AudioInputDevice::MsgStart::create(inputDeviceIndex, sampleRate);
        m_audioInputs[inputDeviceIndex]->getInputMessageQueue()->push(msg);

        m_audioInputs[inputDeviceIndex]->setVolume(volume);
        m_audioInputInfos[deviceName].volume = volume;
        m_defaultInputStarted |= (inputDeviceIndex == -1);
    }
    else
    {
        qWarning("AudioDeviceManager::startAudioInput: unknown device index %d", inputDeviceIndex);
    }
}

void AudioDeviceManager::stopAudioInput(int inputDeviceIndex)
{
    AudioInputDevice::MsgStop *msg = AudioInputDevice::MsgStop::create();
    m_audioInputs[inputDeviceIndex]->getInputMessageQueue()->push(msg);
}

bool AudioDeviceManager::getInputDeviceInfo(const QString& deviceName, InputDeviceInfo& deviceInfo) const
{
    if (m_audioInputInfos.find(deviceName) == m_audioInputInfos.end())
    {
        return false;
    }
    else
    {
        deviceInfo = m_audioInputInfos[deviceName];
        return true;
    }
}

bool AudioDeviceManager::getOutputDeviceInfo(const QString& deviceName, OutputDeviceInfo& deviceInfo) const
{
    if (m_audioOutputInfos.find(deviceName) == m_audioOutputInfos.end())
    {
        return false;
    }
    else
    {
        deviceInfo = m_audioOutputInfos[deviceName];
        return true;
    }
}

int AudioDeviceManager::getInputSampleRate(int inputDeviceIndex)
{
    QString deviceName;

    if (!getInputDeviceName(inputDeviceIndex, deviceName))
    {
        qDebug("AudioDeviceManager::getInputSampleRate: unknown device index %d", inputDeviceIndex);
        return m_defaultAudioSampleRate;
    }

    InputDeviceInfo deviceInfo;

    if (!getInputDeviceInfo(deviceName, deviceInfo))
    {
        qDebug("AudioDeviceManager::getInputSampleRate: unknown device %s", qPrintable(deviceName));
        return m_defaultAudioSampleRate;
    }
    else
    {
        if (deviceInfo.sampleRate > 0)
        {
            return deviceInfo.sampleRate;
        }
        else
        {
            qDebug("AudioDeviceManager::getInputSampleRate: device %s has invalid sample rate", qPrintable(deviceName));
            return m_defaultAudioSampleRate;
        }
    }
}

int AudioDeviceManager::getOutputSampleRate(int outputDeviceIndex)
{
    QString deviceName;

    if (!getOutputDeviceName(outputDeviceIndex, deviceName))
    {
        qDebug("AudioDeviceManager::getOutputSampleRate: unknown device index %d", outputDeviceIndex);
        return m_defaultAudioSampleRate;
    }

    OutputDeviceInfo deviceInfo;

    if (!getOutputDeviceInfo(deviceName, deviceInfo))
    {
        qDebug("AudioDeviceManager::getOutputSampleRate: unknown device %s", qPrintable(deviceName));
        return m_defaultAudioSampleRate;
    }
    else
    {
        if (deviceInfo.sampleRate > 0)
        {
            return deviceInfo.sampleRate;
        }
        else
        {
            qDebug("AudioDeviceManager::getOutputSampleRate: device %s has invalid sample rate", qPrintable(deviceName));
            return m_defaultAudioSampleRate;
        }
    }
}


void AudioDeviceManager::setInputDeviceInfo(int inputDeviceIndex, const InputDeviceInfo& deviceInfo)
{
    QString deviceName;

    if (!getInputDeviceName(inputDeviceIndex, deviceName))
    {
        qWarning("AudioDeviceManager::setInputDeviceInfo: unknown device index %d", inputDeviceIndex);
        return;
    }

    InputDeviceInfo oldDeviceInfo;

    if (!getInputDeviceInfo(deviceName, oldDeviceInfo))
    {
        qDebug("AudioDeviceManager::setInputDeviceInfo: unknown device %s", qPrintable(deviceName));
    }

    m_audioInputInfos[deviceName] = deviceInfo;

    if (m_audioInputs.find(inputDeviceIndex) == m_audioInputs.end()) { // no FIFO registered yet hence no audio input has been allocated yet
        return;
    }

    AudioInputDevice *audioInput = m_audioInputs[inputDeviceIndex];

    if (oldDeviceInfo.sampleRate != deviceInfo.sampleRate)
    {
        AudioInputDevice::MsgStop *msgStop = AudioInputDevice::MsgStop::create();
        audioInput->getInputMessageQueue()->push(msgStop);

        AudioInputDevice::MsgStart *msgStart = AudioInputDevice::MsgStart::create(inputDeviceIndex, deviceInfo.sampleRate);
        audioInput->getInputMessageQueue()->push(msgStart);
    }

    audioInput->setVolume(deviceInfo.volume);
}

void AudioDeviceManager::setOutputDeviceInfo(int outputDeviceIndex, const OutputDeviceInfo& deviceInfo)
{
    QString deviceName;

    if (!getOutputDeviceName(outputDeviceIndex, deviceName))
    {
        qWarning("AudioDeviceManager::setOutputDeviceInfo: unknown device index %d", outputDeviceIndex);
        return;
    }

    OutputDeviceInfo oldDeviceInfo;

    if (!getOutputDeviceInfo(deviceName, oldDeviceInfo))
    {
        qInfo("AudioDeviceManager::setOutputDeviceInfo: unknown device %s", qPrintable(deviceName));
    }

    m_audioOutputInfos[deviceName] = deviceInfo;

    if (m_audioOutputs.find(outputDeviceIndex) == m_audioOutputs.end())
    {
        qWarning("AudioDeviceManager::setOutputDeviceInfo: index: %d device: %s no FIFO registered yet hence no audio output has been allocated yet",
                outputDeviceIndex, qPrintable(deviceName));
        return;
    }

    AudioOutputDevice *audioOutput = m_audioOutputs[outputDeviceIndex];

    if (oldDeviceInfo.sampleRate != deviceInfo.sampleRate)
    {
        AudioOutputDevice::MsgStop *msgStop = AudioOutputDevice::MsgStop::create();
        audioOutput->getInputMessageQueue()->push(msgStop);

        AudioOutputDevice::MsgStart *msgStart = AudioOutputDevice::MsgStart::create(outputDeviceIndex, deviceInfo.sampleRate);
        audioOutput->getInputMessageQueue()->push(msgStart);
    }

    audioOutput->setUdpCopyToUDP(deviceInfo.copyToUDP);
    audioOutput->setUdpDestination(deviceInfo.udpAddress, deviceInfo.udpPort);
    audioOutput->setUdpUseRTP(deviceInfo.udpUseRTP);
    audioOutput->setUdpChannelMode(deviceInfo.udpChannelMode);
    audioOutput->setUdpChannelFormat(deviceInfo.udpChannelCodec, deviceInfo.udpChannelMode == AudioOutputDevice::UDPChannelStereo, deviceInfo.sampleRate);
    audioOutput->setUdpDecimation(deviceInfo.udpDecimationFactor);
    audioOutput->setFileRecordName(deviceInfo.fileRecordName);
    audioOutput->setRecordToFile(deviceInfo.recordToFile);
    audioOutput->setRecordSilenceTime(deviceInfo.recordSilenceTime);

    qDebug("AudioDeviceManager::setOutputDeviceInfo: index: %d device: %s updated",
            outputDeviceIndex, qPrintable(deviceName));
}

void AudioDeviceManager::unsetOutputDeviceInfo(int outputDeviceIndex)
{
    QString deviceName;

    if (!getOutputDeviceName(outputDeviceIndex, deviceName))
    {
        qWarning("AudioDeviceManager::unsetOutputDeviceInfo: unknown device index %d", outputDeviceIndex);
        return;
    }

    OutputDeviceInfo oldDeviceInfo;

    if (!getOutputDeviceInfo(deviceName, oldDeviceInfo))
    {
        qDebug("AudioDeviceManager::unsetOutputDeviceInfo: unregistered device %s", qPrintable(deviceName));
        return;
    }

    m_audioOutputInfos.remove(deviceName);

    if (m_audioOutputs.find(outputDeviceIndex) == m_audioOutputs.end()) { // no FIFO registered yet hence no audio output has been allocated yet
        return;
    }

    stopAudioOutput(outputDeviceIndex);
    startAudioOutput(outputDeviceIndex);
}

void AudioDeviceManager::unsetInputDeviceInfo(int inputDeviceIndex)
{
    QString deviceName;

    if (!getInputDeviceName(inputDeviceIndex, deviceName))
    {
        qWarning("AudioDeviceManager::unsetInputDeviceInfo: unknown device index %d", inputDeviceIndex);
        return;
    }

    InputDeviceInfo oldDeviceInfo;

    if (!getInputDeviceInfo(deviceName, oldDeviceInfo))
    {
        qDebug("AudioDeviceManager::unsetInputDeviceInfo: unregistered device %s", qPrintable(deviceName));
        return;
    }

    m_audioInputInfos.remove(deviceName);

    if (m_audioInputs.find(inputDeviceIndex) == m_audioInputs.end()) { // no FIFO registered yet hence no audio input has been allocated yet
        return;
    }

    stopAudioInput(inputDeviceIndex);
    startAudioInput(inputDeviceIndex);
}

void AudioDeviceManager::inputInfosCleanup()
{
    QSet<QString> deviceNames;
    deviceNames.insert(m_defaultDeviceName);
    QList<AudioDeviceInfo>::const_iterator itd = AudioDeviceInfo::availableInputDevices().begin();

    for (; itd != AudioDeviceInfo::availableInputDevices().end(); ++itd)
    {
        qDebug("AudioDeviceManager::inputInfosCleanup: device: %s", qPrintable(itd->deviceName()));
        deviceNames.insert(itd->deviceName());
    }

    QMap<QString, InputDeviceInfo>::iterator itm = m_audioInputInfos.begin();

    for (; itm != m_audioInputInfos.end();)
    {
        if (!deviceNames.contains(itm.key()))
        {
            qDebug("AudioDeviceManager::inputInfosCleanup: removing key: %s", qPrintable(itm.key()));
            m_audioInputInfos.erase(itm++);
        }
        else
        {
            ++itm;
        }
    }
}

void AudioDeviceManager::outputInfosCleanup()
{
    QSet<QString> deviceNames;
    deviceNames.insert(m_defaultDeviceName);
    QList<AudioDeviceInfo>::const_iterator itd = AudioDeviceInfo::availableOutputDevices().begin();

    for (; itd != AudioDeviceInfo::availableOutputDevices().end(); ++itd)
    {
        qDebug("AudioDeviceManager::outputInfosCleanup: device: %s", qPrintable(itd->deviceName()));
        deviceNames.insert(itd->deviceName());
    }

    QMap<QString, OutputDeviceInfo>::iterator itm = m_audioOutputInfos.begin();

    for (; itm != m_audioOutputInfos.end();)
    {
        if (!deviceNames.contains(itm.key()))
        {
            qDebug("AudioDeviceManager::outputInfosCleanup: removing key: %s", qPrintable(itm.key()));
            m_audioOutputInfos.erase(itm++);
        }
        else
        {
            ++itm;
        }
    }
}

bool AudioDeviceManager::setInputDeviceVolume(float volume, int inputDeviceIndex)
{
    if (m_audioInputs.find(inputDeviceIndex) == m_audioInputs.end()) { // no FIFO registered yet hence no audio input has been allocated yet
        return false;
    }

    m_audioInputs[inputDeviceIndex]->setVolume(volume);
    return true;
}

bool AudioDeviceManager::setOutputDeviceVolume(float volume, int outputDeviceIndex)
{
    if (m_audioOutputs.find(outputDeviceIndex) == m_audioOutputs.end()) { // no FIFO registered yet hence no audio output has been allocated yet
        return false;
    }

    m_audioOutputs[outputDeviceIndex]->setVolume(volume);
    return true;

}

void AudioDeviceManager::debugAudioInputInfos() const
{
    QMap<QString, InputDeviceInfo>::const_iterator it = m_audioInputInfos.begin();

    for (; it != m_audioInputInfos.end(); ++it)
    {
        qDebug() << "AudioDeviceManager::debugAudioInputInfos:"
                << " name: " << it.key()
                << " sampleRate: " << it.value().sampleRate
                << " volume: " << it.value().volume;
    }
}

void AudioDeviceManager::debugAudioOutputInfos() const
{
    QMap<QString, OutputDeviceInfo>::const_iterator it = m_audioOutputInfos.begin();

    for (; it != m_audioOutputInfos.end(); ++it)
    {
        qDebug() << "AudioDeviceManager::debugAudioOutputInfos:"
                << " name: " << it.key()
                << " sampleRate: " << it.value().sampleRate
                << " udpAddress: " << it.value().udpAddress
                << " udpPort: " << it.value().udpPort
                << " copyToUDP: " << it.value().copyToUDP
                << " udpUseRTP: " << it.value().udpUseRTP
                << " udpChannelMode: " << (int) it.value().udpChannelMode
                << " udpChannelCodec: " << (int) it.value().udpChannelCodec
                << " decimationFactor: " << it.value().udpDecimationFactor;
    }
}

bool AudioDeviceManager::handleMessage(const Message& msg)
{
    if (AudioOutputDevice::MsgReportSampleRate::match(msg))
    {
        AudioOutputDevice::MsgReportSampleRate& report = (AudioOutputDevice::MsgReportSampleRate&) msg;
        int deviceIndex = report.getDeviceIndex();
        const QString& deviceName = report.getDeviceName();
        int sampleRate = report.getSampleRate();
        qDebug("AudioDeviceManager::handleMessage: AudioOutputDevice::MsgReportSampleRate: device(%d) %s: rate: %d",
            deviceIndex, qPrintable(deviceName), sampleRate);
        m_audioOutputInfos[deviceName].sampleRate = sampleRate;

        // send message to attached channels
        for (auto& messageQueue : m_outputDeviceSinkMessageQueues[deviceIndex])
        {
            DSPConfigureAudio *msg = new DSPConfigureAudio(m_audioOutputInfos[deviceName].sampleRate, DSPConfigureAudio::AudioOutput);
            messageQueue->push(msg);
        }

        return true;
    }
    else if (AudioInputDevice::MsgReportSampleRate::match(msg))
    {
        AudioInputDevice::MsgReportSampleRate& report = (AudioInputDevice::MsgReportSampleRate&) msg;
        int deviceIndex = report.getDeviceIndex();
        const QString& deviceName = report.getDeviceName();
        int sampleRate = report.getSampleRate();
        qDebug("AudioDeviceManager::handleMessage: AudioInputDevice::MsgReportSampleRate: device(%d) %s: rate: %d",
            deviceIndex, qPrintable(deviceName), sampleRate);
        m_audioInputInfos[deviceName].sampleRate = sampleRate;

        // send message to attached channels
        for (auto& messageQueue : m_inputDeviceSourceMessageQueues[deviceIndex])
        {
            DSPConfigureAudio *msg = new DSPConfigureAudio(m_audioInputInfos[deviceName].sampleRate, DSPConfigureAudio::AudioInput);
            messageQueue->push(msg);
        }

        return true;
    }

    return false;
}

void AudioDeviceManager::handleInputMessages()
{
	Message* message;

	while ((message = m_inputMessageQueue.pop()) != nullptr)
	{
		if (handleMessage(*message)) {
			delete message;
		}
	}
}
