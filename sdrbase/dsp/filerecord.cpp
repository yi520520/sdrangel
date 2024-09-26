///////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2015-2021 Edouard Griffiths, F4EXB <f4exb06@gmail.com>          //
// Copyright (C) 2018 beta-tester <alpha-beta-release@gmx.net>                   //
// Copyright (C) 2020 Felix Schneider <felix@fx-schneider.de>                    //
// Copyright (C) 2021, 2023 Jon Beniston, M7RCE <jon@beniston.com>               //
// Copyright (C) 2021 Andreas Baulig <free.geronimo@hotmail.de>                  //
// Copyright (C) 2021 Christoph Berg <myon@debian.org>                           //
// Copyright (C) 2022 CRD716 <crd716@gmail.com>                                  //
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

#include <boost/crc.hpp>
#include <boost/cstdint.hpp>

#include <QDebug>
#include <QDateTime>

#include "dsp/dspcommands.h"
#include "util/message.h"

#include "filerecord.h"

FileRecord::FileRecord(quint32 sampleRate, quint64 centerFrequency) :
	FileRecordInterface(),
    m_fileBase("test"),
    m_sampleRate(sampleRate),
    m_centerFrequency(centerFrequency),
	m_recordOn(false),
    m_recordStart(false),
    m_byteCount(0),
    m_msShift(0)
{
	setObjectName("FileRecord");
}

FileRecord::FileRecord(const QString& fileBase) :
    FileRecordInterface(),
    m_fileBase(fileBase),
    m_sampleRate(0),
    m_centerFrequency(0),
    m_recordOn(false),
    m_recordStart(false),
    m_byteCount(0)
{
    setObjectName("FileRecord");
}

FileRecord::~FileRecord()
{
    stopRecording();
}

void FileRecord::setFileName(const QString& fileBase)
{
    if (!m_recordOn)
    {
        m_fileBase = fileBase;
    }
}

void FileRecord::genUniqueFileName(uint deviceUID, int istream)
{
    if (istream < 0) {
        setFileName(QString("rec%1_%2.sdriq").arg(deviceUID).arg(QDateTime::currentDateTimeUtc().toString("yyyy-MM-ddTHH_mm_ss_zzz")));
    } else {
        setFileName(QString("rec%1_%2_%3.sdriq").arg(deviceUID).arg(istream).arg(QDateTime::currentDateTimeUtc().toString("yyyy-MM-ddTHH_mm_ss_zzz")));
    }
}

void FileRecord::feed(const SampleVector::const_iterator& begin, const SampleVector::const_iterator& end, bool positiveOnly)
{
    QMutexLocker mutexLocker(&m_mutex);

    (void) positiveOnly;

    // if no recording is active, send the samples to /dev/null
    if(!m_recordOn)
        return;

    if (begin < end) // if there is something to put out
    {
        if (m_recordStart)
        {
            writeHeader();
            m_recordStart = false;
        }

        m_sampleFile.write(reinterpret_cast<const char*>(&*(begin)), (end - begin)*sizeof(Sample));
        m_byteCount += end - begin;
    }
}

void FileRecord::start()
{
}

void FileRecord::stop()
{
    stopRecording();
}

bool FileRecord::startRecording()
{
    QMutexLocker mutexLocker(&m_mutex);

    if (m_recordOn) {
        stopRecording();
    }

#ifdef ANDROID
    if (!m_sampleFile.isOpen())
#else
    if (!m_sampleFile.is_open())
#endif
    {
    	qDebug() << "FileRecord::startRecording";
#ifdef ANDROID
        // FIXME: No idea how to write to a file where the filename doesn't come from the file picker
        m_currentFileName = m_fileBase + ".sdriq";
        m_sampleFile.setFileName(m_currentFileName);
        if (!m_sampleFile.open(QIODevice::ReadWrite))
        {
            qWarning() << "FileRecord::startRecording: failed to open file: " << m_currentFileName << " error " << m_sampleFile.error();
            return false;
        }
#else
        m_currentFileName = m_fileBase + "." + QDateTime::currentDateTimeUtc().toString("yyyy-MM-ddTHH_mm_ss_zzz") + ".sdriq"; // Don't use QString::arg on Android, as filename can contain %2
        m_sampleFile.open(m_currentFileName.toStdString().c_str(), std::ios::binary);
        if (!m_sampleFile.is_open())
        {
            qWarning() << "FileRecord::startRecording: failed to open file: " << m_currentFileName;
            return false;
        }
#endif
        m_recordOn = true;
        m_recordStart = true;
        m_byteCount = 0;
    }
    return true;
}

bool FileRecord::stopRecording()
{
    QMutexLocker mutexLocker(&m_mutex);

#ifdef ANDROID
    if (m_sampleFile.isOpen())
#else
    if (m_sampleFile.is_open())
#endif
    {
    	qDebug() << "FileRecord::stopRecording";
        m_sampleFile.close();
        m_recordOn = false;
        m_recordStart = false;
#ifdef ANDROID
#else
        if (m_sampleFile.bad())
        {
            qWarning() << "FileRecord::stopRecording: an error occurred while writing to " << m_currentFileName;
            return false;
        }
#endif
    }
    return true;
}

bool FileRecord::handleMessage(const Message& message)
{
	if (DSPSignalNotification::match(message))
	{
        QMutexLocker mutexLocker(&m_mutex);
		DSPSignalNotification& notif = (DSPSignalNotification&) message;
		quint32 sampleRate = notif.getSampleRate();
		qint64 centerFrequency = notif.getCenterFrequency();
		qDebug() << "FileRecord::handleMessage: DSPSignalNotification: inputSampleRate: " << sampleRate
				<< " centerFrequency: " << centerFrequency;

        if (m_recordOn && (m_sampleRate != sampleRate)) {
            startRecording();
        }

        m_sampleRate = sampleRate;
        m_centerFrequency = centerFrequency;

        return true;
	}
    else
    {
        return false;
    }
}

void FileRecord::writeHeader()
{
    Header header;
    header.sampleRate = m_sampleRate;
    header.centerFrequency = m_centerFrequency;
    qint64 ts = QDateTime::currentMSecsSinceEpoch();
    header.startTimeStamp = (quint64)(ts + m_msShift);
    header.sampleSize = SDR_RX_SAMP_SZ;
    header.filler = 0;

    writeHeader(m_sampleFile, header);
}

bool FileRecord::readHeader(std::ifstream& sampleFile, Header& header)
{
    sampleFile.read((char *) &header, sizeof(Header));
    boost::crc_32_type crc32;
    crc32.process_bytes(&header, 28);
    return header.crc32 == crc32.checksum();
}

bool FileRecord::readHeader(QFile& sampleFile, Header& header)
{
    sampleFile.read((char *) &header, sizeof(Header));
    boost::crc_32_type crc32;
    crc32.process_bytes(&header, 28);
    return header.crc32 == crc32.checksum();
}

void FileRecord::writeHeader(std::ofstream& sampleFile, Header& header)
{
    boost::crc_32_type crc32;
    crc32.process_bytes(&header, 28);
    header.crc32 = crc32.checksum();
    sampleFile.write((const char *) &header, sizeof(Header));
}

void FileRecord::writeHeader(QFile& sampleFile, Header& header)
{
    boost::crc_32_type crc32;
    crc32.process_bytes(&header, 28);
    header.crc32 = crc32.checksum();
    sampleFile.write((const char *) &header, sizeof(Header));
}
