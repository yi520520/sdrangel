///////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2021, 2023 Jon Beniston, M7RCE <jon@beniston.com>               //
// Copyright (C) 2022-2023 Edouard Griffiths, F4EXB <f4exb06@gmail.com>          //
// Copyright (C) 2022 CRD716 <crd716@gmail.com>                                  //
//                                                                               //
// File recorder in .wav format                                                  //
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

#ifndef INCLUDE_WAV_FILERECORD_H
#define INCLUDE_WAV_FILERECORD_H

#include <string>
#include <iostream>
#include <fstream>
#include <ctime>

#include <QDateTime>
#include <QFile>

#include "dsp/filerecordinterface.h"
#include "export.h"

class Message;

class SDRBASE_API WavFileRecord : public FileRecordInterface {
public:

#pragma pack(push, 1)
    struct Chunk
    {
        char m_id[4];          // "RIFF", "fmt ", "auxi", "data"
        quint32 m_size;
    };
    struct SystemTime {
        quint16 m_year;
        quint16 m_month;
        quint16 m_dayOfWeek;
        quint16 m_day;
        quint16 m_hour;
        quint16 m_minute;
        quint16 m_second;
        quint16 m_milliseconds;
    };
    struct Auxi {
        SystemTime m_startTime;
        SystemTime m_stopTime;
        quint32 m_centerFreq;
        quint32 m_adFrequency;
        quint32 m_ifFrequency;
        quint32 m_bandwidth;
        quint32 m_iqOffset;
        quint32 m_unused2;
        quint32 m_unused3;
        quint32 m_unused4;
        quint32 m_unused5;
        char m_nextFilename[96];
    };
    struct SDRBASE_API Header
    {
        Chunk m_riffHeader;
        char m_type[4];        // "WAVE"
        Chunk m_fmtHeader;
        quint16 m_audioFormat;
        quint16 m_numChannels;
        quint32 m_sampleRate;
        quint32 m_byteRate;
        quint16 m_blockAlign;
        quint16 m_bitsPerSample;
        Chunk m_auxiHeader;
        Auxi m_auxi;
        Chunk m_dataHeader;

        QDateTime getStartTime() const;
    };
#pragma pack(pop)

    WavFileRecord(quint32 sampleRate=0, quint64 centerFrequency=0);
    WavFileRecord(const QString& fileBase);
    virtual ~WavFileRecord();

    quint64 getByteCount() const { return m_byteCount; }
    void setMsShift(qint64 shift) override { m_msShift = shift; }
    virtual int getBytesPerSample() override { return 4; };
    const QString& getCurrentFileName() override { return m_currentFileName; }
    void setMono(bool mono) { m_nbChannels = mono ? 1 : 2; }
    void setSampleRate(quint32 sampleRate) { m_sampleRate = sampleRate; }

    void genUniqueFileName(uint deviceUID, int istream = -1);

    virtual void feed(const SampleVector::const_iterator& begin, const SampleVector::const_iterator& end, bool positiveOnly) override;
    void write(qint16 lSample, qint16 rSample); //!< write a single sample
    void writeMono(qint16 sample); //!< write a single mono sample
    void writeMono(qint16 *samples, int nbSamples); //!< write a buffer of mono samples
    virtual void start() override;
    virtual void stop() override;
    virtual bool handleMessage(const Message& message) override;

    virtual void setFileName(const QString& fileBase) override;
    void setFileBaseIsFileName(bool fileBaseIsFileName) { m_fileBaseIsFileName = fileBaseIsFileName; }
    virtual bool startRecording() override;
    virtual bool stopRecording() override;
    virtual bool isRecording() const override { return m_recordOn; }

    static bool readHeader(std::ifstream& samplefile, Header& header, bool check=true);
    static bool readHeader(QFile& samplefile, Header& header);
    static void writeHeader(std::ofstream& samplefile, Header& header);
    static void writeHeader(QFile& samplefile, Header& header);

    // These functions guess from the filename, not contents
    static bool getCenterFrequency(QString fileName, quint64& centerFrequency);
    static bool getStartTime(QString fileName, QDateTime& startTime);

private:
    QString m_fileBase;
    bool m_fileBaseIsFileName;
    quint32 m_sampleRate;
    quint64 m_centerFrequency;
    bool m_recordOn;
    bool m_recordStart;
#ifdef ANDROID
    QFile m_sampleFile;
#else
    std::ofstream m_sampleFile;
#endif
    QString m_currentFileName;
    quint64 m_byteCount;
    qint64 m_msShift;
    int m_nbChannels;

    void writeHeader();

    static bool checkHeader(Header& header);
};

#endif // INCLUDE_WAV_FILERECORD_H
