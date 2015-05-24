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
#include "LastFmEvent.h"

#include <QUrl>
#include <Plasma/ScrollWidget>

#include <QGraphicsWidget>

class KDateTime;
class QLabel;
class QGraphicsLinearLayout;
class QGraphicsProxyWidget;
class QPixmap;
class QPointF;
class QSignalMapper;
namespace Plasma {
    class Label;
    class PushButton;
}

class UpcomingEventsWidget : public QGraphicsWidget
{
    Q_OBJECT

    public:
        /**
         * UpcomingEventsWidget constructor
         * @param QGraphicsWidget*, like QGraphicsWidget constructor
         */
        UpcomingEventsWidget( const LastFmEventPtr &event,
                              QGraphicsItem *parent = 0,
                              Qt::WindowFlags wFlags = 0 );
        ~UpcomingEventsWidget();

        /**
         * The upcoming event associated with this widget
         */
        LastFmEventPtr eventPtr() const
        { return m_event; }

        /**
         *Set the event's image in Plasma::Label from an url
         *@param QUrl, image's url to be displayed
         */
        void setImage( const QUrl &url );

        /**
         * Set attendance for this event
         * @param count number of attendees
         */
        void setAttendance( int count );

        /**
         *Set the event's participants text in Plasma::Label from a QString
         *@param QString, participant's text to be displayed
         */
        void setParticipants( const QStringList &participants );

        /**
         *Set the event's date in Plasma::Label from a KDateTime
         *@param KDateTime, date to be displayed
         */
        void setDate( const KDateTime &date );

        /**
         *Set the event's name in Plasma::Label from a QString
         *@param QString, name's text to be displayed
         */
        void setName( const QString &name );

        /**
         *Set the event's location in a Plasma::Label from a QString
         *@param QString, location's text to be displayed
         */
        void setLocation( const LastFmLocationPtr &location );

        /**
         * Set event venue
         * @param venue Last.fm's venue
         */
        void setVenue( const LastFmVenuePtr &venue );

        /**
         *Set the event's url in Plasma::Label from a QUrl
         *@param QUrl, url to be displayed
         */
        void setUrl( const QUrl &url );

        /**
         * Set the events tags
         * @param tags list of tags
         */
        void setTags( const QStringList &tags );

    protected:
        Plasma::PushButton *m_mapButton;

    private:
        Plasma::PushButton *m_urlButton;
        QGraphicsProxyWidget *m_attendance;
        QGraphicsProxyWidget *m_date;
        QGraphicsProxyWidget *m_location;
        QGraphicsProxyWidget *m_name;
        QGraphicsProxyWidget *m_participants;
        QGraphicsProxyWidget *m_tags;
        QGraphicsProxyWidget *m_venue;
        QLabel *m_image;
        QUrl m_imageUrl;
        const LastFmEventPtr m_event;

        QGraphicsProxyWidget *createLabel( const QString &text = QString(),
                                           QSizePolicy::Policy hPolicy = QSizePolicy::Expanding );

        friend class UpcomingEventsListWidget;

    private slots:
        void loadImage();
        void openUrl();
};

class UpcomingEventsListWidget : public Plasma::ScrollWidget
{
    Q_OBJECT
    Q_PROPERTY( QString name READ name WRITE setName )
    Q_PROPERTY( LastFmEvent::List events READ events )

public:
    explicit UpcomingEventsListWidget( QGraphicsWidget *parent = 0 );
    ~UpcomingEventsListWidget();

    int count() const;
    bool isEmpty() const;

    void addEvent( const LastFmEventPtr &event );
    void addEvents( const LastFmEvent::List &events );

    LastFmEvent::List events() const;
    QString name() const;
    void setName( const QString &name );

    void clear();

signals:
    void mapRequested( QObject *widget );
    void eventAdded( const LastFmEventPtr &event );
    void eventRemoved( const LastFmEventPtr &event );

private:
    QString m_name;
    LastFmEvent::List m_events;
    QMap<uint, UpcomingEventsWidget*> m_sortMap;
    QGraphicsLinearLayout *m_layout;
    QSignalMapper *m_sigmap;
    Q_DISABLE_COPY( UpcomingEventsListWidget )
};

#endif /* UPCOMINGEVENTSWIDGET_H */
