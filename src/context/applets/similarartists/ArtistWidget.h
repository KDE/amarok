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


//Qt
#include <QWidget>
#include <QString>

#ifndef ARTIST_WIDGET_H
#define ARTIST_WIDGET_H

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
         * Change the photo of the artist
         * @param photo The new artist photo 
         */
        void setPhoto( const QPixmap &photo);

        /**
         * Change the artist name and the url which permit to display a page
         * which contains informations about this artist
         * @param nom The name of this artist
         * @param url The url of the artist about page
         */
        void setArtist( const QString &nom, const QString &url);

        /**
         * Change the genre of the artist
         * @param genre The new artist genres
         */
        void setGenres( const QString &genres);

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
        
};

#endif // ARTIST_WIDGET_H
