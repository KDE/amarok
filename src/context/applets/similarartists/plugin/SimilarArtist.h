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

#ifndef SIMILAR_ARTIST_H
#define SIMILAR_ARTIST_H

//Kde
#include <KSharedPtr>
#include <QUrl>

//Qt
#include <QSharedData>
#include <QString>
#include <QXmlStreamReader>

class SimilarArtist;
typedef KSharedPtr<SimilarArtist> SimilarArtistPtr;

/**
 * Represents a similar artist to another
 * @author Joffrey Clavel
 * @version 0.1
 */
class SimilarArtist : public QSharedData
{
public:
    typedef QList<SimilarArtistPtr> List;

    /**
     * Create an empty similar artist
     */
    SimilarArtist();

    /**
     * Create a similar artist with data
     * @param name  The name of this similar artist
     * @param match The match percent (between 0 and 100) of the similarity
     * between this artist and the artist similarTo
     * @param url   A url of this artist on the web, for example on last.fm
     * @param urlImage  A url of an image of this artist, for example on last.fm
     * @param similarTo The name of the artist similar to this artist
     */
    SimilarArtist( const QString &name, const int match, const QUrl &url,
                   const QUrl &urlImage, const QString &similarTo );

    SimilarArtist( const SimilarArtist &other );

    /**
     * @return The name of this artist
     */
    QString name() const;

    /**
     * @return the percent of match of this artist, between 0 and 100
     */
    int match() const;

    /**
     * @return a url on the web for this artist, for example on last.fm
     */
    QUrl url() const;

    /**
     * @return a url on the web for an image oh this artist, for example on last.fm
     */
    QUrl urlImage() const;

    /**
     * @return the artist this similar artist is related to
     */
    QString similarTo() const;

    /**
     * Set the artist this similar artist is related to
     * @param artist artist name
     */
    void setSimilarTo( const QString &artist );

    static SimilarArtist::List listFromXml( QXmlStreamReader &xml );

private:
    /**
     * The name of this artist
     */
    QString m_name;

    /**
     * The match of this artist to the artist similarTo, between 0 and 100
     */
    int m_match;

    /**
     * A url of this artist on the web
     */
    QUrl m_url;

    /**
     * A image url of this artist on the web
     */
    QUrl m_urlImage;

    /**
     * The name of the artist similar to this artist
     */
    QString m_similarTo;
};

Q_DECLARE_METATYPE( SimilarArtist )
Q_DECLARE_METATYPE( SimilarArtistPtr )
Q_DECLARE_METATYPE( SimilarArtist::List )

#endif // SIMILAR_ARTIST_H
