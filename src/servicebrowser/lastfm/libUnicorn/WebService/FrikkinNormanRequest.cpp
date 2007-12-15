/***************************************************************************
 *   Copyright (C) 2005 - 2007 by                                          *
 *      Last.fm Ltd <client@last.fm>                                       *
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

#include "FrikkinNormanRequest.h"

#include "logger.h"

#include <QDebug>
#include <QDateTime>

FrikkinNormanRequest::FrikkinNormanRequest() :
    Request(TypeFrikkinNorman,"FrikkinNorman")
{
}

void
FrikkinNormanRequest::start()
{
    QString path = "/fingerprint/fp.php?fid=" + fpId();

    get( path );
}

void
FrikkinNormanRequest::success( QByteArray data )
{
    qDebug() << "FrikkinNorman" << fpId() << ":\n" << data;

    QString s( data );
    QStringList l = s.split( "\t" );
    if ( l.size() > 2 )
    {
        QString artist = l.at( 1 );
        QString track = l.at( 2 );

        uint unixTime = QDateTime::currentDateTime().toTime_t();

        QString normanism;
        switch ( unixTime % 7 )
        {
            case 0: normanism = "It's frikkin: "; break;
            case 1: normanism = "'Oly cow! It's: "; break;
            case 2: normanism = "Ezzally, this is: "; break;
            case 3: normanism = "I do the 'elicopter to: "; break;
            case 4: normanism = "My algorritm sez: "; break;
            case 5: normanism = "Fingerprinting iz like mehking love to a beautiful woman: "; break;
            case 6: normanism = "I took my skipper licence to: "; break;
        }

        setMetadata( normanism + artist + " - " + track );
    }
    else
    {
        setMetadata( "SLAPP-EH! Norman fails." );
    }
}
