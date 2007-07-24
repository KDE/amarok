/*
   Copyright (C) 2007 Maximilian Kossick <maximilian.kossick@googlemail.com>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
*/

#include "MetaUtility.h"

#include <QChar>

QString
Meta::msToPrettyTime( int ms )
{
    return Meta::secToPrettyTime( ms / 1000 );
}

QString
Meta::secToPrettyTime( int seconds )
{
    int minutes = ( seconds / 60 ) % 60;
    int hours = seconds / 3600;
    QString s = QChar( ':' );
    s.append( ( seconds % 60 ) < 10 ? QString( "0%1" ).arg( seconds % 60 ) : QString::number( seconds % 60 ) ); //seconds

    if( hours )
    {
        s.prepend( minutes < 10 ? QString( "0%1" ).arg( minutes ) : QString::number( minutes ) );
        s.prepend( ':' );
    }
    else
    {
        s.prepend( QString::number( minutes ) );
        return s;
    }

    //don't zeroPad the last one, as it can be greater than 2 digits
    s.prepend( QString::number( hours ) );

    return s;
}
