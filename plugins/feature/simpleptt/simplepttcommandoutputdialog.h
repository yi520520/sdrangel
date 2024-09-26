///////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2012 maintech GmbH, Otto-Hahn-Str. 15, 97204 Hoechberg, Germany //
// written by Christian Daniel                                                   //
// Copyright (C) 2015-2019, 2023 Edouard Griffiths, F4EXB <f4exb06@gmail.com>    //
// Copyright (C) 2015 John Greb <hexameron@spam.no>                              //
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

#ifndef SDRGUI_GUI_COMMANDOUTPUTDIALOG_H_
#define SDRGUI_GUI_COMMANDOUTPUTDIALOG_H_

#include <QDialog>
#include <QProcess>

namespace Ui {
    class SimplePTTCommandOutputDialog;
}

class Command;

class SimplePTTCommandOutputDialog : public QDialog {
    Q_OBJECT

public:
    enum StatusIndicator
    {
        StatusIndicatorUnknown,
        StatusIndicatorOK,
        StatusIndicatorKO
    };

    explicit SimplePTTCommandOutputDialog(QWidget* parent = 0);
    ~SimplePTTCommandOutputDialog();
    void setErrorText(const QProcess::ProcessError& processError);
    void setExitText(const QProcess::ExitStatus& processExit);
    void setExitCode(int exitCode);
    void setLog(const QString& log);
    void setStatusIndicator(StatusIndicator indicator);
    void setEndTime(const QDateTime& dt);

private:
    Ui::SimplePTTCommandOutputDialog* ui;

};


#endif /* SDRGUI_GUI_COMMANDOUTPUTDIALOG_H_ */
