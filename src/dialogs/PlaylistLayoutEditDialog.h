/***************************************************************************
 *   Copyright (c) 2009  Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>    *
 *             (c) 2009  Teo Mrnjavac <teo.mrnjavac@gmail.com>             *
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

#include "playlist/layouts/LayoutEditWidget.h"

#include <QDialog>

#include "ui_PlaylistLayoutEditDialog.h"

/**
    Dialog for editing playlist layouts
    @author Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>
*/
class PlaylistLayoutEditDialog : public QDialog, private Ui::PlaylistLayoutEditDialog
{
    Q_OBJECT

    public slots:
        void setLayout( const QString &layoutName );

    public:
        PlaylistLayoutEditDialog( QWidget *parent = 0 );
        ~PlaylistLayoutEditDialog();

    private slots:
        void preview();
        virtual void accept();
        virtual void reject();
        void apply();
        void newLayout();
        void copyLayout();
        void deleteLayout();
        void renameLayout();
        void moveUp();
        void moveDown();
        void toggleDeleteButton();
        void toggleUpDownButtons();

    private:
        Playlist::LayoutEditWidget *m_headEdit;
        Playlist::LayoutEditWidget *m_bodyEdit;
        Playlist::LayoutEditWidget *m_singleEdit;

        QMap<QString, Playlist::PlaylistLayout> *m_layoutsMap;

        QString m_layoutName;

        QString m_firstActiveLayout;
};

#endif
