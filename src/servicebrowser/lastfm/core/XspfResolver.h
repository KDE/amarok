/***************************************************************************
 *   Copyright (C) 2005 - 2007 by                                          *
 *      Christian Muehlhaeuser, Last.fm Ltd <chris@last.fm>                *
 *      Erik Jalevik, Last.fm Ltd <erik@last.fm>                           *
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

#ifndef XSPFRESOLVER_H
#define XSPFRESOLVER_H

#include "TrackInfo.h"
#include "exceptions.h"
#include "StationUrl.h"

#include <QList>
#include <QDomNode>


/*************************************************************************/ /**
    Takes an XSPF playlist and returns a list of TrackInfo objects complete
    with resolved paths. Paths can resolve to local tracks or Internet
    streams with priority given to local tracks.
******************************************************************************/
class XspfResolver
{
    public:

        /*************************************************************************/ /**
            Thrown by resolveTracks if the XSPF is corrupt.
        ******************************************************************************/
        class ParseException : public LastFmException
        {
        public:
            ParseException( const QString& msg ) : LastFmException( msg ) { }
        };

        /*****************************************************************/ /**
            Resolves entries in an XSPF stream into TrackInfo objects.

            @param xspf - an XSPF playlist
        **********************************************************************/
        QList<TrackInfo>
        resolveTracks( const QByteArray& xspf );
        

    private:
    
        QString
        childText( QDomNode parent,
                   QString tagName );

        // Bit hacky, because a resolver isn't really designed to know these
        // things but in our current implementation, in the XSPF they live.
        PROP_GET( QString, station )
        PROP_GET( int, skipLimit )

};

#endif // XSPFRESOLVER_H
