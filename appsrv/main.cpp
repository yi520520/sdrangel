///////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2012 maintech GmbH, Otto-Hahn-Str. 15, 97204 Hoechberg, Germany //
// written by Christian Daniel                                                   //
// Copyright (C) 2015-2020 Edouard Griffiths, F4EXB <f4exb06@gmail.com>          //
// Copyright (C) 2019 Davide Gerhard <rainbow@irh.it>                            //
// Copyright (C) 2023 Jon Beniston, M7RCE <jon@beniston.com>                     //
// Copyright (C) 2023 Daniele Forsi <iu5hkx@gmail.com>                           //
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

#include <QCoreApplication>
#include <QSysInfo>

#include <signal.h>
#include <vector>

#include "loggerwithfile.h"
#include "mainparser.h"
#include "mainserver.h"
#include "remotetcpsinkstarter.h"
#include "dsp/dsptypes.h"

void handler(int sig) {
    fprintf(stderr, "quit the application by signal(%d).\n", sig);
    QCoreApplication::quit();
}

#ifndef _WIN32
void catchUnixSignals(const std::vector<int>& quitSignals) {
    sigset_t blocking_mask;
    sigemptyset(&blocking_mask);

    for (std::vector<int>::const_iterator it = quitSignals.begin(); it != quitSignals.end(); ++it) {
        sigaddset(&blocking_mask, *it);
    }

    struct sigaction sa;
    sa.sa_handler = handler;
    sa.sa_mask    = blocking_mask;
    sa.sa_flags   = 0;

    for (std::vector<int>::const_iterator it = quitSignals.begin(); it != quitSignals.end(); ++it) {
        sigaction(*it, &sa, 0);
    }
}
#endif

static int runQtApplication(int argc, char* argv[], qtwebapp::LoggerWithFile *logger)
{
    QCoreApplication a(argc, argv);

    QCoreApplication::setOrganizationName("f4exb");
    QCoreApplication::setApplicationName("SDRangelSrv");
    QCoreApplication::setApplicationVersion(SDRANGEL_VERSION);

#ifndef _WIN32
    int catchSignals[] = {SIGQUIT, SIGINT, SIGTERM, SIGHUP};
    std::vector<int> vsig(catchSignals, catchSignals + sizeof(catchSignals) / sizeof(int));
    catchUnixSignals(vsig);
#endif

    MainParser parser;
    parser.parse(a);

#if QT_VERSION >= QT_VERSION_CHECK(5, 4, 0)
    qInfo("%s %s Qt %s %db %s %s DSP Rx:%db Tx:%db PID %lld",
          qPrintable(QCoreApplication::applicationName()),
          qPrintable(QCoreApplication::applicationVersion()),
          qPrintable(QString(QT_VERSION_STR)),
          QT_POINTER_SIZE*8,
          qPrintable(QSysInfo::currentCpuArchitecture()),
          qPrintable(QSysInfo::prettyProductName()),
          SDR_RX_SAMP_SZ,
          SDR_TX_SAMP_SZ,
          QCoreApplication::applicationPid());
#else
    qInfo("%s %s Qt %s %db DSP Rx:%db Tx:%db PID %lld",
          qPrintable(QCoreApplication::applicationName()),
          qPrintable(QCoreApplication::>applicationVersion()),
                     qPrintable(QString(QT_VERSION_STR)),
                     QT_POINTER_SIZE*8,
                     SDR_RX_SAMP_SZ,
                     SDR_TX_SAMP_SZ,
                     QCoreApplication::applicationPid());
#endif

    if (parser.getListDevices())
    {
        // Disable log on console, so we can more easily see device list
        logger->setConsoleMinMessageLevel(QtFatalMsg);
        // Don't pass logger to MainServer, otherwise it can re-enable log output
        logger = nullptr;
    }

    MainServer m(logger, parser, &a);

    // This will cause the application to exit when the main core is finished
    QObject::connect(&m, SIGNAL(finished()), &a, SLOT(quit()));

    if (parser.getListDevices())
    {
        // List available physical devices and exit
        RemoteTCPSinkStarter::listAvailableDevices();
        exit (EXIT_SUCCESS);
    }

    if (parser.getRemoteTCPSink()) {
        RemoteTCPSinkStarter::start(parser);
    }

    return a.exec();
}

int main(int argc, char* argv[])
{
    qtwebapp::LoggerWithFile *logger = new qtwebapp::LoggerWithFile(qApp);
    logger->installMsgHandler();
    int res = runQtApplication(argc, argv, logger);
    delete logger;
    qWarning("SDRangel quit.");
    return res;
}
