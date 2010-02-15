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
#include <KUrl>

//Qt
#include<QString>


/**
 * Represents a similar artist to another
 * @author Joffrey Clavel
 * @version 0.1
 */
class SimilarArtist
{
public:
    
    /**
     * Create an empty similar artist
     */
    SimilarArtist();

    /**
     * Create a similar artist with data
     * @param name  The name of this similar artist
     * @param match The match pourcent (between 0 and 100) of the similarity
     * between this artist and the artist similarTo
     * @param url   A url of this artist on the web, for example on last.fm
     * @param urlImage  A url of an image of this artist, for example on last.fm
     * @param similarTo The name of the artist similar to this artist
     * @param desc The description of this artist, NULL as default
     */
    SimilarArtist( const QString &name, const int match, const KUrl &url,
                   const KUrl &urlImage, const QString &similarTo,
                   const QString &description=QString() );

    /**
     * @return The name of this artist
     */
    QString name() const;

    /**
     * @return the pourcent of match of this artist, betwwen 0 and 100
     */
    int match() const;

    /**
     * @return a url on the web for this artist, for example on last.fm
     */
    KUrl url() const;

    /**
     * @return a url on the web for an image oh this artist, for example on last.fm
     */
    KUrl urlImage() const;

    /**
     * @return the description of this artist
     */
    QString description() const;

    /**
     * Set the description of this artist
     * @param desc the description
     */
    void setDescription(const QString description);

    /**
     * Define a new type for help the communication
     * between the data engine SimilarArtists and the applet SimilarArtists
     */
    typedef QList<SimilarArtist> SimilarArtistsList ;

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
    KUrl m_url;

    /**
     * A image url of this artist on the web
     */
    KUrl m_urlImage;

    /**
     * The description of this artist
     */
    QString m_description;

    /**
     * The name of the artist similar to this artist
     */
    QString m_similarTo;

};

#endif // SIMILAR_ARTIST_H

Q_DECLARE_METATYPE( SimilarArtist )
Q_DECLARE_METATYPE( SimilarArtist::SimilarArtistsList )