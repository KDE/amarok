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
 
#include "AmarokUrl.h"

#include "Debug.h"

#include "AmarokUrlHandler.h"

AmarokUrl::AmarokUrl()
{
}

AmarokUrl::AmarokUrl( const QString & urlString )
{
    initFromString( urlString );
}


AmarokUrl::~AmarokUrl()
{
}

void AmarokUrl::initFromString( const QString & urlString )
{
    //first, strip amarok://

    QString strippedUrlString = urlString;

    strippedUrlString = strippedUrlString.replace( "amarok://", "" );
    m_fields = strippedUrlString.split( "/" );
    
}

QString AmarokUrl::command()
{
    if ( m_fields.count() != 0 )
        return m_fields[0];
    else
        return QString();
}



int AmarokUrl::numberOfArgs()
{
    if ( m_fields.count() != 0 )
        return m_fields.count() - 1;
    else
        return 0;
}

QString AmarokUrl::arg( int arg )
{
    if ( m_fields.count() != 0 )
        return m_fields[arg + 1];
    else
        return ""; 
}

bool AmarokUrl::run()
{
    DEBUG_BLOCK
    The::amarokUrlHandler()->run( *this );
}


