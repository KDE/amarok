/****************************************************************************************
 * Copyright (c) 2010 Andreas Hartmetz <ahartmetz@gmail.com>                            *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) version 3 or        *
 * any later version accepted by the membership of KDE e.V. (or its successor approved  *
 * by the membership of KDE e.V.), which shall act as a proxy defined in Section 14 of  *
 * version 3 of the license.                                                            *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#ifndef AMAROK_PLAYLISTQUEUEEDITOR_H
#define AMAROK_PLAYLISTQUEUEEDITOR_H

#include <QDialog>

#include "ui_PlaylistQueueEditor.h"

class PlaylistQueueEditor : public QDialog
{
    Q_OBJECT
public:
    PlaylistQueueEditor();

private Q_SLOTS:
    void queueChanged();
    void moveUp();
    void moveDown();
    void dequeueTrack();
    void clear();
    void updateQueueFromList();

private:
    void updateView();
    quint64 currentId();
    void setCurrentId( quint64 id );
    bool m_blockViewUpdates;
    Ui::PlaylistQueueEditor m_ui;
};
#endif
