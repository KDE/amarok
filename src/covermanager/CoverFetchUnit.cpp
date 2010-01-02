/****************************************************************************************
 * Copyright (c) 2009 Rick W. Chen <stuffcorpse@archlinux.us>                           *
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

#include "CoverFetchUnit.h"
#include "CoverFetchQueue.h"

/*
 * CoverFetchUnit
 */

CoverFetchUnit::CoverFetchUnit( Meta::AlbumPtr album,
                                const CoverFetchPayload *url,
                                bool interactive )
    : QSharedData()
    , m_album( album )
    , m_interactive( interactive )
    , m_url( url )
{
}

CoverFetchUnit::CoverFetchUnit( const CoverFetchUnit &cpy )
    : QSharedData( cpy )
{
    m_album = cpy.m_album;
    m_interactive = cpy.m_interactive;

    switch( cpy.m_url->type() )
    {
    case CoverFetchPayload::INFO:
        m_url = new CoverFetchInfoPayload( cpy.m_album );
        break;
    case CoverFetchPayload::ART:
        m_url = new CoverFetchArtPayload( cpy.m_album );
        break;
    default:
        m_url = 0;
    }
}

CoverFetchUnit::~CoverFetchUnit()
{
    delete m_url;
}

Meta::AlbumPtr
CoverFetchUnit::album() const
{
    return m_album;
}

const QStringList &
CoverFetchUnit::errors() const
{
    return m_errors;
}

const CoverFetchPayload *
CoverFetchUnit::url() const
{
    return m_url;
}

bool
CoverFetchUnit::isInteractive() const
{
    return m_interactive;
}

template< typename T >
void
CoverFetchUnit::addError( const T &error )
{
    m_errors << error;
}

CoverFetchUnit &CoverFetchUnit::operator=( const CoverFetchUnit &rhs )
{
    if( this == &rhs )
        return *this;

    switch( rhs.m_url->type() )
    {
    case CoverFetchPayload::INFO:
        m_url = new CoverFetchInfoPayload( rhs.m_album );
        break;
    case CoverFetchPayload::ART:
        m_url = new CoverFetchArtPayload( rhs.m_album );
        break;
    default:
        m_url = 0;
    }

    m_album = rhs.m_album;
    m_interactive = rhs.m_interactive;
    return *this;
}

bool CoverFetchUnit::operator==( const CoverFetchUnit &other ) const
{
    return m_album == other.m_album;
}

bool CoverFetchUnit::operator!=( const CoverFetchUnit &other ) const
{
    return !( *this == other );
}
