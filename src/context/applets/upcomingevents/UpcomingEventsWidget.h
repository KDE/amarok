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

#include "NetworkAccessManagerProxy.h"

#include <KUrl>

#include <QGraphicsWidget>

class KDateTime;
class QLabel;
class QPixmap;
namespace Plasma {
    class Label;
}

class UpcomingEventsWidget : public QGraphicsWidget
{
    Q_OBJECT
    
    public:
        /**
         * UpcomingEventsWidget constructor
         * @param QGraphicsWidget*, like QGraphicsWidget constructor
         */
        UpcomingEventsWidget( QGraphicsItem *parent = 0, Qt::WindowFlags wFlags = 0 );
        ~UpcomingEventsWidget();

        // Getters
        /**
         *@return the image
         */
        const QPixmap *image() const;
        /**
         *@return the participants Plasma::Label pointer
         */
        Plasma::Label  *participants() const;
        /**
         *@return the date Plasma::Label pointer
         */
        Plasma::Label  *date() const;
        /**
         *@return the name Plasma::Label pointer
         */
        Plasma::Label  *name() const;
        /**
         *@return the location Plasma::Label pointer
         */
        Plasma::Label  *location() const;
        /**
         *@return the url Plasma::Label pointer
         */
        Plasma::Label  *url() const;

        // Setters
        /**
         *Set the event's image in Plasma::Label from an url
         *@param KUrl, image's url to be displayed
         */
        void    setImage( const KUrl &urlImage );
        /**
         *Set the event's participants text in Plasma::Label from a QString
         *@param QString, participant's text to be displayed
         */
        void    setParticipants( const QString &participants );
        /**
         *Set the event's date in Plasma::Label from a KDateTime
         *@param KDateTime, date to be displayed
         */
        void    setDate( const KDateTime &date );
        /**
         *Set the event's name in Plasma::Label from a QString
         *@param QString, name's text to be displayed
         */
        void    setName( const QString &name );
        /**
         *Set the event's location in a Plasma::Label from a QString
         *@param QString, location's text to be displayed
         */
        void    setLocation( const QString &location );
        /**
         *Set the event's url in Plasma::Label from a KUrl
         *@param KUrl, url to be displayed
         */
        void    setUrl( const KUrl &url );

    private:
        QLabel *m_image;
        Plasma::Label *m_participants;
        Plasma::Label *m_date;
        Plasma::Label *m_location;
        Plasma::Label *m_name;
        Plasma::Label *m_url;
        KUrl m_imageUrl;

    private slots:
        /**
         *SLOTS
         *Get pixmap from the internet and set it into image's Plasma::Label
         */
        void loadImage( const KUrl &url, QByteArray data, NetworkAccessManagerProxy::Error e );
};

#endif /* UPCOMINGEVENTSWIDGET_H */
