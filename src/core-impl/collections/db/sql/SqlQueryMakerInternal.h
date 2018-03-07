/****************************************************************************************
 * Copyright (c) 2009 Maximilian Kossick <maximilian.kossick@googlemail.com>            *
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

#ifndef SQLQUERYMAKERINTERNAL_H
#define SQLQUERYMAKERINTERNAL_H

#include "core/collections/QueryMaker.h"
#include "core/meta/forward_declarations.h"

#include <QObject>
#include <QPointer>
#include <QString>

namespace Collections {

class SqlCollection;

class SqlQueryMakerInternal : public QObject
{
Q_OBJECT
public:
    explicit SqlQueryMakerInternal( SqlCollection *collection );
    virtual ~ SqlQueryMakerInternal();

    void run();
    void setQuery( const QString &query );
    void setQueryType( QueryMaker::QueryType type );
    void setResultAsDataPtrs( bool value );

Q_SIGNALS:
    void newTracksReady( Meta::TrackList );
    void newArtistsReady( Meta::ArtistList );
    void newAlbumsReady( Meta::AlbumList );
    void newGenresReady( Meta::GenreList );
    void newComposersReady( Meta::ComposerList );
    void newYearsReady( Meta::YearList );
    void newResultReady( QStringList );
    void newLabelsReady( Meta::LabelList );

private:
    void handleResult( const QStringList &result );
    void handleTracks( const QStringList &result );
    void handleArtists( const QStringList &result );
    void handleAlbums( const QStringList &result );
    void handleGenres( const QStringList &result );
    void handleComposers( const QStringList &result );
    void handleYears( const QStringList &result );
    void handleLabels( const QStringList &result );

private:
    QPointer<SqlCollection> m_collection;
    QueryMaker::QueryType m_queryType;
    QString m_query;

};

} //namespace Collections

#endif // SQLQUERYMAKERINTERNAL_H
