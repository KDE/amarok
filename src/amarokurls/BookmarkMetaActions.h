/***************************************************************************
 *   Copyright (c) 2008  Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>    *
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
 
#ifndef BOOKMARKMETAACTIONS_H
#define BOOKMARKMETAACTIONS_H

#include <GlobalCollectionActions.h>

/**
	@author Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>
*/
class BookmarkAlbumAction : public GlobalCollectionAlbumAction
{
    Q_OBJECT
public:
    BookmarkAlbumAction( QObject *parent );

    private slots:
        void slotTriggered();

};

class BookmarkArtistAction : public GlobalCollectionArtistAction
{
    Q_OBJECT
    public:
        BookmarkArtistAction( QObject *parent );

    private slots:
        void slotTriggered();

};

#endif
