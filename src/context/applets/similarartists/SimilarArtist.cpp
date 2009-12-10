/****************************************************************************************
 * Copyright (c) 2009 Joffrey Clavel <jclavel@clabert.info>                             *
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

SimilarArtist::SimilarArtist(){}

SimilarArtist::SimilarArtist(const QString name, const int match, const KUrl url,
                             const KUrl urlImage, const QString similarTo)
{
    m_name=name;
    m_match=match;
    m_url=url;
    m_urlImage=urlImage;
    m_similarTo=similarTo;

    static bool metaTypeRegistered = false;
    if (!metaTypeRegistered)
    {
        qRegisterMetaType<SimilarArtist>("SimilarArtists");
        metaTypeRegistered = true;
    }
}


QString SimilarArtist::getName() const
{
    return m_name;
}


KUrl SimilarArtist::getUrlImage() const
{
    return m_urlImage;
}
