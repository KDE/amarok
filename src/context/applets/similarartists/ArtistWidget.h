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

#ifndef ARTIST_WIDGET_H
#define ARTIST_WIDGET_H


//Qt
#include <QWidget>
#include <QString>

//Kde
#include<KUrl>
#include <KIO/Job>

class QLabel;
class QGraphicsScene;
class QGridLayout;

 /**
  * A widget for display an artist with some details
  * @author Joffrey Clavel
  * @version 0.1
  */
class ArtistWidget : public QWidget
{
    Q_OBJECT
    public:

        ArtistWidget(QWidget *parent = 0);
        ~ArtistWidget();

        /**
         * Change the photo of the artist with a QPixmap
         * @param photo The new artist photo 
         */
        void setPhoto( const QPixmap &photo);

        /**
         * Change the photo of the artist with a photo load from an Url
         * @param photo The url of the new artist photo
         */
        void setPhoto( const KUrl &urlPhoto);

        /**
         * Change the artist name and the url which permit to display a page
         * which contains informations about this artist
         * @param nom The name of this artist
         * @param url The url of the artist about page
         */
        void setArtist( const QString &nom, const KUrl &url);

        /**
         * Change the match pourcentage of the artist
         * @param match The match of this artist
         */
        void setMatch( const int match);

        /**
         * Clean the widget => the content of the QLabel are empty
         */
        void clear();

    private:

        /**
         * Layout for the formatting of the widget contents
         */
        QGridLayout *m_layout;

        //elements of the widget
        QLabel *m_image;
        QLabel *m_name;
        QLabel *m_genre;

        KJob *m_imageJob;
        

     private slots:
        void setImageFromInternet( KJob* job);
};

#endif // ARTIST_WIDGET_H
