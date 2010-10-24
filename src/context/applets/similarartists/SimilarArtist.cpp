/****************************************************************************************
 * Copyright (c) 2009-2010 Joffrey Clavel <jclavel@clabert.info>                        *
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

#include "SimilarArtist.h"

SimilarArtist::SimilarArtist() {}

SimilarArtist::SimilarArtist( const QString &name, const int match, const KUrl &url,
                              const KUrl &urlImage, const QString &similarTo,
                              const QString &description,
                              const QString &topTrack )
    : m_name( name )
    , m_match( match )
    , m_url( url )
    , m_urlImage( urlImage )
    , m_description( description )
    , m_topTrack( topTrack )
    , m_similarTo( similarTo )
{

    static bool metaTypeRegistered = false;
    if ( !metaTypeRegistered )
    {
        qRegisterMetaType<SimilarArtist>( "SimilarArtists" );
        metaTypeRegistered = true;
    }
}

SimilarArtist::SimilarArtist( const SimilarArtist &other )
    : QSharedData( other )
    , m_name( other.m_name )
    , m_match( other.m_match )
    , m_url( other.m_url )
    , m_urlImage( other.m_urlImage )
    , m_description( other.m_description )
    , m_topTrack( other.m_topTrack )
    , m_similarTo( other.m_similarTo )
{
}

QString
SimilarArtist::name() const
{
    return m_name;
}

int
SimilarArtist::match() const
{
    return m_match;
}

KUrl
SimilarArtist::url() const
{
    return m_url;
}

KUrl
SimilarArtist::urlImage() const
{
    return m_urlImage;
}

QString
SimilarArtist::description() const
{
    return m_description;
}

void
SimilarArtist::setDescription(const QString &description)
{
    m_description=description;
}

QString
SimilarArtist::topTrack() const
{
    return m_topTrack;
}

void
SimilarArtist::setTopTrack(const QString &track)
{
    m_topTrack=track;
}
