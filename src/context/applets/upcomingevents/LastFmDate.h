/*
   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef LASTFMDATE_H
#define LASTFMDATE_H

#include <QDate>
#include <QObject>
#include <QString>


class LastFmDate : public QObject
{
    Q_OBJECT

private:
    QDate* m_date;

public:
    LastFmDate(const QString &dateString);
    QString toString() const;
    int day() const;
    int month() const;
    int year() const;
};

#endif // LASTFMDATE_H
