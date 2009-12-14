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

#ifndef SIMILAR_ARTIST_H
#define SIMILAR_ARTIST_H

//Qt
#include<QString>

//Kde
#include <KUrl>

/**
 * Represents a similar artist to another
 * @author Joffrey Clavel
 * @version 0.1
 */
class SimilarArtist
{
    public:
        SimilarArtist();
        SimilarArtist(const QString &name, const int match, const KUrl &url,
                      const KUrl &urlImage, const QString &similarTo);

        QString name() const;
        int match() const;
        KUrl url() const;
        KUrl urlImage() const;
        
        typedef QList<SimilarArtist> SimilarArtistsList ;
        
    private:
        QString m_name;
        int m_match;
        KUrl m_url;
        KUrl m_urlImage;
        QString m_similarTo;
        
};

#endif // SIMILAR_ARTIST_H

Q_DECLARE_METATYPE(SimilarArtist)
Q_DECLARE_METATYPE(SimilarArtist::SimilarArtistsList)