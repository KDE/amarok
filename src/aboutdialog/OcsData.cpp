/****************************************************************************************
 * Copyright (c) 2009 Téo Mrnjavac <teo.mrnjavac@gmail.com>                             *
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

#include "OcsData.h"

OcsData::OcsData( const QByteArray &providerId )
{
    m_providerId = QString::fromUtf8( providerId );
}

OcsData::~OcsData()
{}

void
OcsData::addAuthor( const QString &username, const KAboutPerson &person )
{
    m_authors.append( QPair< QString, KAboutPerson >( username, person ) );
}

void
OcsData::addCredit( const QString &username, const KAboutPerson &person )
{
    m_credits.append( QPair< QString, KAboutPerson >( username, person ) );
}

