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
 
#ifndef AUDIOCDQUERYMAKER_H
#define AUDIOCDQUERYMAKER_H

#include "collection/support/MemoryQueryMaker.h"

/**
Simple sublcass to handle various artists in a sane way ( initally just dont add _everything_ to the variosu artist top node... )

	@author Nikolaj Hald Nielsen <nhnFreespirit@gmail.com> 
*/
class AudioCdQueryMaker : public MemoryQueryMaker
{
public:
    AudioCdQueryMaker( MemoryCollection *mc, const QString &collectionId );

    ~AudioCdQueryMaker();

    virtual QueryMaker* setAlbumQueryMode( AlbumQueryMode mode );
    virtual void run();

private:
    AlbumQueryMode m_albumQueryMode;

};

#endif
