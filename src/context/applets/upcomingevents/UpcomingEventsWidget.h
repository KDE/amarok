/****************************************************************************************
 * Copyright (c) 2009-2010 Ludovic Deveaux <deveaux.ludovic31@gmail.com>                *
 * Copyright (c) 2010 Hormiere Guillaume <hormiere.guillaume@gmail.com>                 *
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

#ifndef UPCOMING_EVENTS_WIDGET_H
#define UPCOMING_EVENTS_WIDGET_H

// Kde include
#include<KUrl>

// Qt include
#include <QWidget>
#include <QDate>
#include <QPixmap>
#include <QScrollArea>

class KDateTime;
class KUrl;
class KJob;

class QLabel;
class QGridLayout;

class UpcomingEventsWidget : public QWidget
{
    Q_OBJECT
    
    public:
        /**
         * UpcomingEventsWidget constructor
         * @param QWidget*, like QWidget constructor
         */
        UpcomingEventsWidget( QWidget * parent = 0 );
        ~UpcomingEventsWidget ();

        // Getters
        /**
         *@return the image QLabel pointer
         */
        QLabel  *image() const;
        /**
         *@return the participants QLabel pointer
         */
        QLabel  *participants() const;
        /**
         *@return the date QLabel pointer
         */
        QLabel  *date() const;
        /**
         *@return the name QLabel pointer
         */
        QLabel  *name() const;
        /**
         *@return the location QLabel pointer
         */
        QLabel  *location() const;
        /**
         *@return the url QLabel pointer
         */
        QLabel  *url() const;

        // Setters
        /**
         *Set the event's image in QLabel from an url
         *@param KUrl, image's url to be displayed
         */
        void    setImage( const KUrl &urlImage );
        /**
         *Set the event's participants text in QLabel from a QString
         *@param QString, participant's text to be displayed
         */
        void    setParticipants( const QString &participants );
        /**
         *Set the event's date in QLabel from a KDateTime
         *@param KDateTime, date to be displayed
         */
        void    setDate( const KDateTime &date );
        /**
         *Set the event's name in QLabel from a QString
         *@param QString, name's text to be displayed
         */
        void    setName( const QString &name );
        /**
         *Set the event's location in a QLabel from a QString
         *@param QString, location's text to be displayed
         */
        void    setLocation( const QString &location );
        /**
         *Set the event's url in QLabel from a KUrl
         *@param KUrl, url to be displayed
         */
        void    setUrl( const KUrl &url );

    private:

        QGridLayout *m_layout;
        QLabel *m_image;
        QLabel *m_participants;
        QLabel *m_date;
        QLabel *m_location;
        QLabel *m_name;
        QLabel *m_url;
        QFrame *m_frame;

    private slots:
        /**
         *SLOTS
         *Get pixmap from the KJob and set it into image's QLabel
         *@param KJob*, pointer to the job which get the pixmap from the web
         */
        void    loadImage( KJob *job );
        /**
         *SLOTS
         *Open the event's url
         *@param QString, contain the Url
         */
        void    openUrl( QString );
};

#endif /* UPCOMINGEVENTSWIDGET_H */
