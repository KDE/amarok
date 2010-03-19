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

//Kde
#include<KUrl>
#include <KIO/Job>
#include <ksqueezedtextlabel.h>

//Qt
#include <QWidget>
#include <QString>


class QLabel;
class QGraphicsScene;
class QGridLayout;
class QPushButton;

/**
 * A widget for display an artist with some details
 * @author Joffrey Clavel
 * @version 0.2
 */
class ArtistWidget : public QWidget
{
    Q_OBJECT
public:

    /**
     * ArtistWidget constructor
     * @param parent The widget parent
     */
    ArtistWidget( QWidget *parent =0 );
    /**
     * ArtistWidget destructor
     */
    ~ArtistWidget();

    /**
     * Change the photo of the artist with a QPixmap
     * @param photo The new artist photo
     */
    void setPhoto( const QPixmap &photo );

    /**
     * Change the photo of the artist with a photo load from an Url
     * @param photo The url of the new artist photo
     */
    void setPhoto( const KUrl &urlPhoto );

    /**
     * Change the artist name and the url which allows to display a page
     * which contains informations about this artist
     * @param nom The name of this artist
     * @param url The url of the artist about page
     */
    void setArtist( const QString &nom, const KUrl &url );

    /**
     * Change the match pourcentage of the artist
     * @param match The match of the artist
     */
    void setMatch( const int match );

    /**
     * Change the artist description which contains informations about this artist
     * @param desc The description of this artist
     */
    void setDescription( const QString &description );

    /**
     * Change the most known track of this artist
     * @param topTrack the top track of this artist
     */
    void setTopTrack( const QString &topTrack );

    void resizeEvent(QResizeEvent *event);

    /**
     * Clean the widget => the content of the QLabel is empty
     */
    void clear();

private:

    /**
     * Elide the artist description depending on the widget size
     */
    void elideArtistDescription();

    /**
     * Layout for the formatting of the widget contents
     */
    QGridLayout *m_layout;

    //elements of the widget
    /**
     * Image of the artist
     */
    QLabel *m_image;


    /**
     * Name of the artist
     */
    QString m_name;
    
    /**
     * Label showing the name of the artist
     */
    QLabel *m_nameLabel;

    /**
     * Genre of the artist's music
     */
    QLabel *m_genre;

    /**
     * Top track of the artist
     */
    QLabel *m_topTrack;

    /**
     * Button to navigate to the artit in the local collection
     */
     QPushButton *m_navigateButton;

    /**
     * Description of the artist
     */
    QLabel *m_desc;

    /**
     * The description in text of the artist
     */
    QString m_descString;

    /**
     * Job of the image from the web
     */
    KJob *m_imageJob;



private slots:
    /**
     * Put the image of the artist in the QPixMap
     * @param job, pointer to the job which get the pixmap from the web
     */
    void setImageFromInternet( KJob *job );

    /**
     * Open an URL
     * @param url The URL of the artist
     */
    void openUrl( const QString &url );

    /**
     * Navigate to this artist in the local collection
     */
    void navigateToArtist();
};

#endif // ARTIST_WIDGET_H
