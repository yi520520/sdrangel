///////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2023-2024 Jon Beniston, M7RCE                                   //
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

#ifndef INCLUDE_GUI_COLORDIALOG_H
#define INCLUDE_GUI_COLORDIALOG_H

#include <QColor>
#include <QDialog>

#include "export.h"

class QColorDialog;
class QPushButton;

class SDRGUI_API ColorDialog : public QDialog {
    Q_OBJECT

public:
    explicit ColorDialog(const QColor &initial, QWidget *parent = nullptr);
    QColor selectedColor() const;
    bool noColorSelected() const;

public slots:
    virtual void accept() override;
    void noColorClicked();

private:

    QColorDialog *m_colorDialog;
    QPushButton *m_noColorButton;
    QPushButton *m_cancelButton;
    QPushButton *m_okButton;

    bool m_noColorSelected;
};

#endif // INCLUDE_GUI_COLORDIALOG_H
