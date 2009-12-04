/****************************************************************************************
 * Copyright (c) 2009 Nathan Sala <sala.nathan@gmail.com>                               *
 * Copyright (c) 2009 Ludovic Deveaux <deveaux.ludovic31@gmail.com>                     *
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

#ifndef LASTFMEVENT_H
#define LASTFMEVENT_H

#include <QString>
#include <QDate>
#include <QStringList>
#include <KUrl>

class LastFmEvent
{

private:
    QStringList m_artists;
    QString m_name;
    QString m_date;
    KUrl m_smallImageUrl;
    KUrl m_url;

public:
    typedef QList< LastFmEvent > LastFmEventList ;
    LastFmEvent();
    LastFmEvent( const LastFmEvent& );
    ~LastFmEvent();
    LastFmEvent(QStringList artists, QString name, QString date, KUrl smallImageUrl, KUrl url);
    QStringList artists() const;
    QString name() const;
    QString date() const;
    KUrl smallImageUrl() const;
    KUrl url() const;
};

#endif // LASTFMEVENT_H

Q_DECLARE_METATYPE(LastFmEvent)
Q_DECLARE_METATYPE(LastFmEvent::LastFmEventList)
