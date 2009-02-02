/***************************************************************************
 *   Copyright (c) 2009  Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/
 
#ifndef PLAYLISTLAYOUTEDITDIALOG_H
#define PLAYLISTLAYOUTEDITDIALOG_H

#include "widgets/PlaylistItemEditWidget.h"
#include <QDialog>

#include "ui_PlaylistLayoutEditDialog.h"

/**
Dialog for editing playlist layouts

	@author Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>
*/
class PlaylistLayoutEditDialog : public QDialog, private Ui::PlaylistLayoutEditDialog
{
    Q_OBJECT
public:
    PlaylistLayoutEditDialog( QWidget *parent = 0 );
    ~PlaylistLayoutEditDialog();

    void setLayout( const QString &layoutName );

private slots:

    void preview();
    virtual void accept();
        

private:

    PlaylistItemEditWidget * m_headEdit;
    PlaylistItemEditWidget * m_bodyEdit;
    PlaylistItemEditWidget * m_singleEdit;

    QString m_layoutName;

};

#endif
