/****************************************************************************************
 * Copyright (c) 2009-2010 Joffrey Clavel <jclavel@clabert.info>                        *
 * Copyright (c) 2024 Tuomas Nurmi <tuomas@norsumanageri.org>                           *
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

#include <QStandardItemModel>
#include <QUrl>
#include <QXmlStreamReader>


/**
 * Represents a similar artist to another
 * @author Joffrey Clavel
 * @version 0.1
 */
class SimilarArtistModel : public QStandardItemModel
{
public:
    explicit SimilarArtistModel( QObject *parent = nullptr );

    QVariant data( const QModelIndex &index, int role ) const override;
    void clearAll();
    void fillFromXml( QXmlStreamReader &xml );
    void fillArtistInfoFromXml( QXmlStreamReader &xml );
    QString currentTarget() const { return m_similarTo; }
    void setCurrentTarget( const QString &target );
    void setCover( const QString &target, const QUrl &cover );

    enum ArtistDataRoles {
        NameRole = Qt::UserRole + 1,
        MatchRole,
        LinkRole,
        ImageRole,
        BioRole,
        ListenerCountRole,
        PlayCountRole,
        OwnPlayCountRole,
        AlbumCoverRole
    };
protected:
    QHash<int, QByteArray> roleNames() const override;
private:
    /**
    * Currently active target artist
    */
    QString m_similarTo;


    class SimilarArtistItem : public QStandardItem
    {
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


        QString bioText() const;

        QString listenerCount() const;
        QString playCount() const;
        QString ownPlayCount() const;
        QUrl albumCover() const;

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

        QString m_bioText;
        QString m_listenerCount;
        QString m_playCount;
        QString m_ownPlayCount;
        QUrl m_albumCover;

        friend class SimilarArtistModel;
    };
};

Q_DECLARE_METATYPE( SimilarArtistModel* )

#endif // SIMILAR_ARTIST_H
