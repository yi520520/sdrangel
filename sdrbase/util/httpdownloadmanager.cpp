///////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2020-2023 Jon Beniston, M7RCE <jon@beniston.com>                //
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

#include "httpdownloadmanager.h"

#include <QDebug>
#include <QFile>
#include <QDateTime>
#include <QRegularExpression>

HttpDownloadManager::HttpDownloadManager()
{
    connect(&manager, &QNetworkAccessManager::finished, this, &HttpDownloadManager::downloadFinished);
}

QNetworkReply *HttpDownloadManager::download(const QUrl &url, const QString &filename)
{
    QNetworkRequest request(url);
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
    QNetworkReply *reply = manager.get(request);

    connect(reply, &QNetworkReply::sslErrors, this, &HttpDownloadManager::sslErrors);

    qDebug() << "HttpDownloadManager: Downloading from " << url << " to " << filename;
    m_downloads.append(reply);
    m_filenames.append(filename);
    return reply;
}

// Indicate if we have any downloads in progress
bool HttpDownloadManager::downloading() const
{
    return m_filenames.size() > 0;
}

qint64 HttpDownloadManager::fileAgeInDays(const QString& filename)
{
    QFile file(filename);
    if (file.exists())
    {
        QDateTime modified = file.fileTime(QFileDevice::FileModificationTime);
        if (modified.isValid())
            return modified.daysTo(QDateTime::currentDateTime());
        else
            return -1;
    }
    return -1;
}

// Get default directory to write downloads to
QString HttpDownloadManager::downloadDir()
{
    // Get directory to store app data in
    QStringList locations = QStandardPaths::standardLocations(QStandardPaths::AppDataLocation);
    // First dir is writable
    return locations[0];
}

void HttpDownloadManager::sslErrors(const QList<QSslError> &sslErrors)
{
    for (const QSslError &error : sslErrors)
    {
        qCritical() << "HttpDownloadManager: SSL error" << (int)error.error() << ": " << error.errorString();
#ifdef ANDROID
        // On Android 6 (but not on 12), we always seem to get: "The issuer certificate of a locally looked up certificate could not be found"
        // which causes downloads to fail, so ignore
        if (error.error() == QSslError::UnableToGetLocalIssuerCertificate)
        {
            QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
            QList<QSslError> errorsThatCanBeIgnored;
            errorsThatCanBeIgnored << QSslError(QSslError::UnableToGetLocalIssuerCertificate, error.certificate());
            reply->ignoreSslErrors(errorsThatCanBeIgnored);
        }
#endif
    }
}

bool HttpDownloadManager::isHttpRedirect(QNetworkReply *reply)
{
    int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    // 304 is file not changed, but maybe we did
    return (status >= 301 && status <= 308);
}

bool HttpDownloadManager::writeToFile(const QString &filename, const QByteArray &data)
{
    QFile file(filename);

    // Make sure directory to save the file in exists
    QFileInfo fileInfo(filename);
    QDir dir = fileInfo.absoluteDir();
    if (!dir.exists())
        dir.mkpath(".");

    if (file.open(QIODevice::WriteOnly))
    {
        file.write(data);
        file.close();
        return true;
    }
    else
    {
        qCritical() << "HttpDownloadManager: Could not open " << filename << " for writing: " << file.errorString();
        return false;
    }
}

void HttpDownloadManager::downloadFinished(QNetworkReply *reply)
{
    QString url = reply->url().toEncoded().constData();
    int idx = m_downloads.indexOf(reply);
    QString filename = m_filenames[idx];
    bool success = false;
    bool retry = false;

    if (!reply->error())
    {
        if (!isHttpRedirect(reply))
        {
            QByteArray data = reply->readAll();

            // Google drive can redirect downloads to a virus scan warning page
            // We need to use URL with confirm code and retry
            if (url.startsWith("https://drive.google.com/uc?export=download")
                && data.startsWith("<!DOCTYPE html>")
                && !filename.endsWith(".html")
               )
            {
                QRegularExpression regexp("action=\\\"(.*?)\\\"");
                QRegularExpressionMatch match = regexp.match(data);
                if (match.hasMatch())
                {
                    m_downloads.removeAll(reply);
                    m_filenames.remove(idx);

                    QString action = match.captured(1);
                    action = action.replace("&amp;", "&");
                    qDebug() << "HttpDownloadManager: Skipping Google drive warning - downloading " << action;
                    QUrl newUrl(action);
                    QNetworkReply *newReply = download(newUrl, filename);

                    // Indicate that we are retrying, so progress dialogs can be updated
                    emit retryDownload(filename, reply, newReply);

                    retry = true;
                }
                else
                {
                    qDebug() << "HttpDownloadManager: Can't find action URL in Google Drive page\nURL: " << url << "\nData:\n" << data;
                }
            }
            else if (url.startsWith("https://drive.usercontent.google.com/download")
                && data.startsWith("<!DOCTYPE html>")
                && !filename.endsWith(".html")
               )
            {
                QRegularExpression regexpAction("action=\\\"(.*?)\\\"");
                QRegularExpressionMatch matchAction = regexpAction.match(data);
                QRegularExpression regexpId("name=\"id\" value=\"([\\w-]+)\"");
                QRegularExpressionMatch matchId = regexpId.match(data);
                QRegularExpression regexpUuid("name=\"uuid\" value=\"([\\w-]+)\"");
                QRegularExpressionMatch matchUuid = regexpUuid.match(data);
                QRegularExpression regexpAt("name=\"at\" value=\"([\\w-]+\\:)\"");
                QRegularExpressionMatch matchAt = regexpAt.match(data);

                if (matchAction.hasMatch() && matchId.hasMatch() && matchUuid.hasMatch())
                {
                    m_downloads.removeAll(reply);
                    m_filenames.remove(idx);

                    QString newURLString = matchAction.captured(1)
                        + "?id=" + matchId.captured(1)
                        + "&export=download"
                        + "&authuser=0"
                        + "&confirm=t"
                        + "&uuid=" + matchUuid.captured(1)
                        ;
                    if (matchAt.hasMatch()) {
                        newURLString = newURLString + "at=" + matchAt.captured(1);
                    }

                    qDebug() << "HttpDownloadManager: Skipping Google drive warning - downloading " << newURLString;
                    QUrl newUrl(newURLString);
                    QNetworkReply *newReply = download(newUrl, filename);

                    // Indicate that we are retrying, so progress dialogs can be updated
                    emit retryDownload(filename, reply, newReply);

                    retry = true;
                }
                else
                {
                    qDebug() << "HttpDownloadManager: Can't find action URL in Google Drive page\nURL: " << url << "\nData:\n" << data;
                }
            }
            else if (writeToFile(filename, data))
            {
                success = true;
                qDebug() << "HttpDownloadManager: Download from " << url << " to " << filename << " finished.";
            }
        }
        else
        {
            qDebug() << "HttpDownloadManager: Request to download " << url << " was redirected.";
        }
    }
    else
    {
        qCritical() << "HttpDownloadManager: Download of " << url << " failed: " << reply->errorString();
    }

    if (!retry)
    {
        m_downloads.removeAll(reply);
        m_filenames.remove(idx);
        emit downloadComplete(filename, success, url, reply->errorString());
    }
    reply->deleteLater();
}
