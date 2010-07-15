/****************************************************************************************
 * Copyright (c) 2009-2010 Joffrey Clavel <jclavel@clabert.info>                        *
 * Copyright (c) 2010 Alexandre Mendes <alex.mendes1988@gmail.com>                      *
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

#ifndef ARTIST_WIDGET_H
#define ARTIST_WIDGET_H

#include "core/meta/Meta.h"
#include "network/NetworkAccessManagerProxy.h"
#include "SimilarArtist.h"

//Kde
#include <KUrl>
#include <Plasma/ScrollWidget>

//Qt
#include <QWidget>
#include <QString>

class QGraphicsGridLayout;
class QGraphicsLinearLayout;
class QLabel;

namespace Plasma {
    class Label;
    class PushButton;
}

/**
 * A widget for display an artist with some details
 * @author Joffrey Clavel
 * @version 0.2
 */
class ArtistWidget : public QGraphicsWidget
{
    Q_OBJECT

public:
    /**
     * ArtistWidget constructor
     * @param parent The widget parent
     */
    ArtistWidget( const SimilarArtistPtr &artist,
                  QGraphicsWidget *parent = 0, Qt::WindowFlags wFlags = 0 );

    /**
     * ArtistWidget destructor
     */
    ~ArtistWidget();

    /**
     * Pointer to the similar artist this widget is associated with
     */
    SimilarArtistPtr artist() const;

    /**
     * Clean the widget => the content of the QLabel is empty
     */
    void clear();

    /**
     * Set the artist description
     * @param description The description of this artist
     */
    void setDescription( const QString &description );

    /**
     * Change the most known track of this artist
     * @param topTrack the top track of this artist
     */
    void setTopTrack( const QString &topTrack );

protected:
    void resizeEvent( QGraphicsSceneResizeEvent *event );

private:
    void fetchPhoto();     //!< Fetch the photo of the artist
    void queryArtist();    //!< Query collection about artist

    /**
     * Elide the artist description depending on the widget size
     */
    void elideArtistDescription();

    /**
     * Layout for the formatting of the widget contents
     */
    QGraphicsGridLayout *m_layout;

    /**
     * Image of the artist
     */
    QLabel *m_image;

    /**
     * Label showing the name of the artist
     */
    QLabel *m_nameLabel;

    /**
     * Similarity match percentage
     */
    QLabel *m_match;

    /**
     * Title of the top track
     */
    QString m_topTrackTitle;

    /**
     * Label showing the title of the top track of the artist
     */
    QLabel *m_topTrackLabel;

    /**
     * Meta::LabelPtr to the top track, if it's in a collection
     */
    Meta::TrackPtr m_topTrack;

    /**
     * Button to add the top track to the playlist
     */
    Plasma::PushButton *m_topTrackButton;

    /**
     * Button to add the last.fm simmilar artist station for this artist to the playlist
     */
    Plasma::PushButton *m_lastfmStationButton;

    /**
     * Button to navigate to the artit in the local collection
     */
    Plasma::PushButton *m_navigateButton;

    /**
     * Button to open Last.fm's artist webpage using external browser
     */
    Plasma::PushButton *m_urlButton;

    /**
     * Description of the artist
     */
    Plasma::Label *m_desc;

    /**
     * The description in text of the artist
     */
    QString m_descString;

    const SimilarArtistPtr m_artist;

private slots:
    /**
     * Handle artist photo retrieved from Last.fm
     */
    void setImageFromInternet( const KUrl &url, QByteArray data, NetworkAccessManagerProxy::Error e );

    /**
     * Open an URL
     * @param url The URL of the artist
     */
    void openArtistUrl();

    /**
     * Add top track to the playlist
     */
    void addTopTrackToPlaylist();

    /**
     * Navigate to this artist in the local collection
     */
    void navigateToArtist();

    /**
     * Add this artists last.fm similar artist stream
     */
    void addLastfmArtistStation();

    /**
     * Get results from the query maker
     */
    void resultReady( const QString &collectionId, const Meta::ArtistList &artists );
    void resultReady( const QString &collectionId, const Meta::TrackList &tracks );
};

class ArtistsListWidget : public Plasma::ScrollWidget
{
    Q_OBJECT
    Q_PROPERTY( QString name READ name WRITE setName )

public:
    explicit ArtistsListWidget( QGraphicsWidget *parent = 0 );
    ~ArtistsListWidget();

    int count() const;
    bool isEmpty() const;

    void addItem( ArtistWidget *widget );
    void addArtist( const SimilarArtistPtr &artist );
    void addArtists( const SimilarArtist::List &artists );

    QString name() const;
    void setName( const QString &name );

    void clear();

private:
    void addSeparator();
    int m_separatorCount;
    QString m_name;
    QGraphicsLinearLayout *m_layout;
    Q_DISABLE_COPY( ArtistsListWidget )
};

#endif // ARTIST_WIDGET_H
