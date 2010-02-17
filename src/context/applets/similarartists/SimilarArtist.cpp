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

/**
 * Create an empty similar artist
 */
SimilarArtist::SimilarArtist() {}

/**
 * Create a similar artist with data
 * @param name  The name of this similar artist
 * @param match The match pourcent (between 0 and 100) of the similarity
 * between this artist and the artist similarTo
 * @param url   A url of this artist on the web, for example on last.fm
 * @param urlImage  A url of an image of this artist, for example on last.fm
 * @param similarTo The name of the artist similar to this artist
 * @param topTrack The most known artist track.
 */
SimilarArtist::SimilarArtist( const QString &name, const int match, const KUrl &url,
                              const KUrl &urlImage, const QString &similarTo,
                              const QString &description,
                              const QString &topTrack )
{
    m_name = name;
    m_match = match;
    m_url = url;
    m_urlImage = urlImage;
    m_similarTo = similarTo;
    m_description = description;
    m_topTrack = topTrack;

    static bool metaTypeRegistered = false;
    if ( !metaTypeRegistered )
    {
        qRegisterMetaType<SimilarArtist>( "SimilarArtists" );
        metaTypeRegistered = true;
    }
}

 /**
  * @return The name of this artist
  */
QString
SimilarArtist::name() const
{
    return m_name;
}

/**
 * @return the pourcent of match of this artist, betwwen 0 and 100
 */
int
SimilarArtist::match() const
{
    return m_match;
}

/**
 * @return a url on the web for this artist, for example on last.fm
 */
KUrl
SimilarArtist::url() const
{
    return m_url;
}

 /**
  * @return a url on the web for an image oh this artist, for example on last.fm
  */
KUrl
SimilarArtist::urlImage() const
{
    return m_urlImage;
}

/**
 * @return the description of this artist
 */
QString
SimilarArtist::description() const
{
    return m_description;
}

/**
 * Set the description of this artist
 * @param desc the description
 */
void
SimilarArtist::setDescription(const QString description)
{
    m_description=description;
}

/**
 * @return the most known artist track
 */
QString
SimilarArtist::topTrack() const
{
    return m_topTrack;
}


/**
 * Set the most known artist track
 * @param track the top track
 */
void
SimilarArtist::setTopTrack(const QString track)
{
    m_topTrack=track;
}

