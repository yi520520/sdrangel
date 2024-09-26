///////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2021 Jon Beniston, M7RCE                                        //
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

#ifndef INCLUDE_ADSBOSMTEMPLATE_SERVER_H_
#define INCLUDE_ADSBOSMTEMPLATE_SERVER_H_

#include <QTcpServer>
#include <QTcpSocket>
#include <QDebug>
#include <QRegularExpression>

// Redirect OSM maps so we can support Street, Satellite and an Aviation map (CartoDB position)
class ADSBOSMTemplateServer : public QTcpServer
{
    Q_OBJECT
private:
    QString m_thunderforestAPIKey;
    QString m_maptilerAPIKey;

public:
    // port - port to listen on / is listening on. Use 0 for any free port.
    ADSBOSMTemplateServer(const QString &maptilerAPIKey, quint16 &port, QObject* parent = 0) :
        QTcpServer(parent),
        m_maptilerAPIKey(maptilerAPIKey)
    {
        listen(QHostAddress::Any, port);
        port = serverPort();
    }

    void incomingConnection(qintptr socket) override
    {
        QTcpSocket* s = new QTcpSocket(this);
        connect(s, SIGNAL(readyRead()), this, SLOT(readClient()));
        connect(s, SIGNAL(disconnected()), this, SLOT(discardClient()));
        s->setSocketDescriptor(socket);
        //addPendingConnection(socket);
    }

private slots:
    void readClient()
    {
        QTcpSocket* socket = (QTcpSocket*)sender();
        if (socket->canReadLine())
        {
            QString line = socket->readLine();
            QStringList tokens = QString(line).split(QRegularExpression("[ \r\n][ \r\n]*"));
            if (tokens[0] == "GET")
            {
                bool hires = tokens[1].contains("hires");
                QString hiresURL = hires ? "@2x" : "";
                QString xml;
                if ((tokens[1] == "/street") || (tokens[1] == "/street-hires"))
                {
                    xml = QString("\
                    {\
                        \"UrlTemplate\" : \"https://maps.wikimedia.org/osm-intl/%z/%x/%y%1.png\",\
                        \"ImageFormat\" : \"png\",\
                        \"QImageFormat\" : \"Indexed8\",\
                        \"ID\" : \"wmf-intl-%2x\",\
                        \"MaximumZoomLevel\" : 18,\
                        \"MapCopyRight\" : \"<a href='https://wikimediafoundation.org/wiki/Terms_of_Use'>WikiMedia Foundation</a>\",\
                        \"DataCopyRight\" : \"<a href='http://www.openstreetmap.org/copyright'>OpenStreetMap</a> contributors\"\
                    }").arg(hiresURL).arg(hires ? 1 : 2);
                }
                else if (tokens[1] == "/satellite")
                {
                    xml = QString("\
                    {\
                        \"Enabled\" : true,\
                        \"UrlTemplate\" : \"https://api.maptiler.com/tiles/satellite/%z/%x/%y%1.jpg?key=%2\",\
                        \"ImageFormat\" : \"jpg\",\
                        \"QImageFormat\" : \"RGB888\",\
                        \"ID\" : \"usgs-l7\",\
                        \"MaximumZoomLevel\" : 20,\
                        \"MapCopyRight\" : \"<a href='http://maptiler.com'>Maptiler</a>\",\
                        \"DataCopyRight\" : \"<a href='http://maptiler.com'>Maptiler</a>\"\
                    }").arg(hiresURL).arg(m_maptilerAPIKey);
                }
                else if (tokens[1].contains("transit"))
                {
                    QStringList map({"/night-transit", "/night-transit-hires", "/transit", "/transit-hires"});
                    QStringList mapId({"thf-nighttransit", "thf-nighttransit-hires", "thf-transit", "thf-transit-hires"});
                    QStringList mapUrl({"dark_nolabels", "dark_nolabels", "light_nolabels", "light_nolabels"});

                    // Use CartoDB maps without labels for aviation maps
                    int idx = map.indexOf(tokens[1]);
                    xml = QString("\
                    {\
                        \"UrlTemplate\" : \"http://1.basemaps.cartocdn.com/%2/%z/%x/%y.png%1\",\
                        \"ImageFormat\" : \"png\",\
                        \"QImageFormat\" : \"Indexed8\",\
                        \"ID\" : \"%3\",\
                        \"MaximumZoomLevel\" : 20,\
                        \"MapCopyRight\" : \"<a href='https://carto.com'>CartoDB</a>\",\
                        \"DataCopyRight\" : \"<a href='https://carto.com'>CartoDB</a>\"\
                    }").arg(hiresURL).arg(mapUrl[idx]).arg(mapId[idx]);
                }
                else
                {
                    QStringList map({"/cycle", "/cycle-hires", "/hiking", "/hiking-hires", "/night-transit", "/night-transit-hires", "/terrain", "/terrain-hires", "/transit", "/transit-hires"});
                    QStringList mapId({"thf-cycle", "thf-cycle-hires", "thf-hike", "thf-hike-hires", "thf-nighttransit", "thf-nighttransit-hires", "thf-landsc", "thf-landsc-hires", "thf-transit", "thf-transit-hires"});
                    QStringList mapUrl({"cycle", "cycle", "outdoors", "outdoors", "transport-dark", "transport-dark", "landscape", "landscape", "transport", "transport"});

                    int idx = map.indexOf(tokens[1]);
                    if (idx != -1)
                    {
                        xml = QString("\
                        {\
                            \"UrlTemplate\" : \"http://a.tile.thunderforest.com/%1/%z/%x/%y%4.png?apikey=%2\",\
                            \"ImageFormat\" : \"png\",\
                            \"QImageFormat\" : \"Indexed8\",\
                            \"ID\" : \"%3\",\
                            \"MaximumZoomLevel\" : 20,\
                            \"MapCopyRight\" : \"<a href='http://www.thunderforest.com/'>Thunderforest</a>\",\
                            \"DataCopyRight\" : \"<a href='http://www.openstreetmap.org/copyright'>OpenStreetMap</a> contributors\"\
                        }").arg(mapUrl[idx]).arg("3e1f614f78a345459931ba3c898e975e").arg(mapId[idx]).arg(hiresURL);
                    }
                }
                QTextStream os(socket);
                os.setAutoDetectUnicode(true);
                os << "HTTP/1.0 200 Ok\r\n"
                    "Content-Type: text/html; charset=\"utf-8\"\r\n"
                    "\r\n"
                    << xml << "\n";
                socket->close();

                if (socket->state() == QTcpSocket::UnconnectedState) {
                    delete socket;
                }
            }
        }
    }

    void discardClient()
    {
        QTcpSocket* socket = (QTcpSocket*)sender();
        socket->deleteLater();
    }

};

#endif
