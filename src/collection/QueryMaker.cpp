/****************************************************************************************
 * Copyright (c) 2007 Maximilian Kossick <maximilian.kossick@googlemail.com>            *
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

#include "QueryMaker.h"

QueryMaker::QueryMaker() : QObject()
{
}

QueryMaker::~QueryMaker()
{
}

int
QueryMaker::resultCount() const
{
    return 1;
}

QueryMaker*
QueryMaker::setAlbumQueryMode( AlbumQueryMode mode )
{
    Q_UNUSED( mode )
    return this;
}

int QueryMaker::validFilterMask()
{
    return AllFilters;
}

QueryMaker*
QueryMaker::setAutoDelete( bool autoDelete )
{
    if( autoDelete )
        connect( this, SIGNAL( queryDone() ), this, SLOT( deleteLater() ) );
    else
        disconnect( this, SIGNAL( queryDone() ), this, SLOT( deleteLater() ) );
    return this;
}

#include "QueryMaker.moc"

