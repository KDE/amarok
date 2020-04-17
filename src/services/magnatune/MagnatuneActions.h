/****************************************************************************************
 * Copyright (c) 2008 Nikolaj Hald Nielsen <nhn@kde.org>                                *
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
 
#ifndef MAGNATUNEACTIONS_H
#define MAGNATUNEACTIONS_H

#include "MagnatuneMeta.h"

#include <QAction>

/**
A simple QAction subclass for purchasing or downloading content from Magnatune

	@author Nikolaj Hald Nielsen <nhn@kde.org>
*/
class MagnatuneDownloadAction : public QAction
{
    Q_OBJECT
public:
    MagnatuneDownloadAction( const QString &text, Meta::MagnatuneAlbum * album );

    ~MagnatuneDownloadAction() override;

private Q_SLOTS:
    void slotTriggered();

private:
    Meta::MagnatuneAlbum * m_album;

};

/**
A simple QAction subclass for purchasing or downloading content from Magnatune

    @author Nikolaj Hald Nielsen <nhn@kde.org>
*/
class MagnatuneAddToFavoritesAction : public QAction
{
    Q_OBJECT
public:
    MagnatuneAddToFavoritesAction( const QString &text, Meta::MagnatuneAlbum * album );

    ~MagnatuneAddToFavoritesAction() override;

private Q_SLOTS:
    void slotTriggered();

private:
    Meta::MagnatuneAlbum * m_album;

};

#endif
