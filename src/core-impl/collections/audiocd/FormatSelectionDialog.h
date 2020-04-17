/****************************************************************************************
 * Copyright (c) 2009 Nikolaj Hald Nielsen <nhn@kde.org>                                *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/
 
#ifndef FORMATSELECTIONDIALOG_H
#define FORMATSELECTIONDIALOG_H

#include <QDialog>

#include "ui_FormatSelectionDialog.h"

/**
A dialog for selecting the format of of tracks imported from a AudioCdCollection

	@author 
*/
class FormatSelectionDialog
    : public QDialog
    , private Ui::FormatSelectionDialog
{
    Q_OBJECT
public:
    explicit FormatSelectionDialog( QWidget *parent = nullptr );

    ~FormatSelectionDialog() override;

public Q_SLOTS:
    void accept() override;

    virtual void showAdvancedSettings();

Q_SIGNALS:
    void formatSelected( int );

private Q_SLOTS:
    void selectionChanged( bool checked );

private:
    int m_selectedFormat;

};

#endif
