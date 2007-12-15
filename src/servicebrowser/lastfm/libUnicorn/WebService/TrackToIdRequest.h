/***************************************************************************
 *   Copyright (C) 2005 - 2007 by                                          *
 *      Max Howell, Last.fm Ltd <max@last.fm>                              *
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
 *   51 Franklin Steet, Fifth Floor, Boston, MA  02110-1301, USA.          *
 ***************************************************************************/


#ifndef TRACK_TO_ID_REQUEST_H
#define TRACK_TO_ID_REQUEST_H

#include "fwd.h"
#include "UnicornDllExportMacro.h"

#include "metadata.h"


/** @author <max@last.fm> */

class UNICORN_DLLEXPORT TrackToIdRequest : public Request
{
    PROP_GET( int, id );
    PROP_GET( bool, isStreamable );
    PROP_GET_SET( Track, track, Track );

public:
    TrackToIdRequest( Track );

    virtual void start();

private:
    virtual void success( QByteArray );
};

#endif
