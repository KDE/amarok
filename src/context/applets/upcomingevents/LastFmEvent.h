/****************************************************************************************
* Copyright (c) 2009   Nathan Sala <sala.nathan@gmail.com>                             *
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
#include <KDateTime>
#include <QStringList>

class LastFmEvent
{
    Q_OBJECT
    
private:
    QStringList m_artists;
    QString m_name;
    QString m_date;

public:
    Event(QStringList artists, QString name, QString date)
        : m_artists(artists), m_name(name), m_date(date);
        
    QStringList artists();
    QString name();
    QString date();
};

#endif // LASTFMEVENT_H
