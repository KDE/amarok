/****************************************************************************************
 * Copyright (c) 2010 Ludovic Deveaux <deveaux.ludovic31@gmail.com>                     *
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

//Qt
#include <QWidget>
#include <QDate>
#include <QPixmap>
#include <QScrollArea>

//Kde
#include<KUrl>

class QLabel;
class KUrl;
class QDateTime;
class QGridLayout;
class KJob;

class UpcomingEventsWidget : public QWidget
{
    Q_OBJECT
    
    public:
        UpcomingEventsWidget( QWidget * parent = 0 );
        ~UpcomingEventsWidget();

        // Getters
        QLabel * image() const;
        QLabel * participants() const;
        QLabel * date() const;
        QLabel * name() const;
        QLabel * location() const;
        QLabel * url() const;

        // Setters
        void setImage( const KUrl &urlImage );
        void setParticipants( const QString &participants );
        void setDate( const QDateTime &date );
        void setName( const QString &name );
        void setLocation( const QString &location );
        void setUrl( const KUrl &url );

    private:
        QGridLayout * m_layout;
        
        QLabel * m_image;
        QLabel * m_participants;
        QLabel * m_date;
        QLabel * m_location;
        QLabel * m_name;
        QLabel * m_url;

    private slots:
        void loadImage( KJob * job );
        void openUrl(QString);
};

#endif // UPCOMINGEVENTSWIDGET_H
