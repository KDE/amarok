/****************************************************************************************
 * Copyright (c) 2007 Maximilian Kossick <maximilian.kossick@googlemail.com>            *
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

#ifndef AMAROK_XESAMCOLLECTIONBUILDER_H
#define AMAROK_XESAMCOLLECTIONBUILDER_H

#include "XesamDbus.h"

#include "amarok_sqlcollection_export.h"

#include <QList>
#include <QMap>
#include <QObject>
#include <QPair>
#include <QString>

class SqlCollection;

class AMAROK_SQLCOLLECTION_EXPORT_TESTS XesamCollectionBuilder : public QObject
{
    Q_OBJECT
    public:
        XesamCollectionBuilder( SqlCollection *collection );
        ~XesamCollectionBuilder();

    private slots:
        void slotHitsAdded( const QString &search, int count );
        void slotHitsModified(const QString &search, const QList<int> &hit_ids);
        void slotHitsRemoved( const QString &search, const QList<int> &hit_ids );
        void searchDone( const QString &search );

    private:
        QString generateXesamQuery() const;
        bool setupXesam();

        void processDirectory( const QList<QList<QVariant> > &data );
        void addTrack( const QList<QVariant> &trackData, int albumArtistId );

        int albumId( const QString &album, int artistId );
        int artistId( const QString &artist );
        int genreId( const QString &genre );
        int yearId( const QString &year );
        int composerId( const QString &composer );
        int urlId( const QString &url );

    private:
        SqlCollection *m_collection;
        org::freedesktop::xesam::search *m_xesam;

        QString m_session;
        QString m_search;

        QMap<QString, int> m_artists;
        QMap<QString, int> m_genre;
        QMap<QString, int> m_year;
        QMap<QString, int> m_composer;
        QMap<QPair<QString, int>, int> m_albums;
};

#endif
