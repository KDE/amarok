/***************************************************************************
 * copyright            : (C) 2007 Ian Monroe <ian@monroe.nu>              *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License version 2        *
 *   as published by the Free Software Foundation.                         *
 ***************************************************************************/

#ifndef AMAROK_PLAYLISTMODEL_H
#define AMAROK_PLAYLISTMODEL_H

#include "meta.h"

#include <QAbstractTableModel>
#include <QHash>
#include <QVector>

#include <klocale.h>

class QModelIndex;
class QueryMaker;


//PORT rename to Playlist when the playlist class is removed
namespace PlaylistNS {

class TrackAdvancer;

    enum Column
    {
        Album  = 0,
        AlbumArtist,
        Artist,
        Bitrate,
        Bpm,
        Comment,
        Composer,
        Directory,
        DiscNumber,
        Filename,
        Filesize,
        Genre,
        LastPlayed,
        Length,
        Mood,
        PlayCount,
        Rating,
        SampleRate,
        Score,
        Title,
        TrackNumber,
        Type,
        Year,
        NUM_COLUMNS
    };


    class Model : public QAbstractTableModel
    {
        Q_OBJECT
        public:
        //TODO remove singleton when playlist controller class is created
            static Model *instance() {
                if(!s_instance)  s_instance = new Model;
                return s_instance;
            }
            ~Model();
        //required by QAbstractTabelModel
            int rowCount(const QModelIndex &parent = QModelIndex() ) const;
            int columnCount(const QModelIndex &parent = QModelIndex() ) const;
            QVariant data(const QModelIndex &index, int role) const;
        //overriding QAbstractItemModel
            bool removeRows( int position, int rows );
            QVariant headerData(int section, Qt::Orientation orientation, int role) const;
            Qt::DropActions supportedDropActions() const;
        //other methods
            void setColumns( QVector< Column > columns ) { m_columns = columns; }
            ///
            void insertTracks( int row, Meta::TrackList list ); //doesn't override
            void insertTracks( int row, QueryMaker *qm );
            int activeRow() const { return m_activeRow; }
            void setActiveRow( int row );
            Meta::TrackPtr activeTrack() const { return m_tracks[ m_activeRow ]; }
        //    Qt::ItemFlags flags(const QModelIndex &index) const;
            void testData();

        public slots:
            void play( const QModelIndex& index );
        private slots:
            void trackFinished(); //! what to do when a track finishes
            void queryDone();
            void newResultReady( const QString &collectionId, const Meta::TrackList &tracks );

        private:
            static QString prettyColumnName( Column index ); //!takes a Column enum and returns its string name
            Model( QObject* parent = 0 ); //! Singleton

            Meta::TrackList m_tracks; //! list of tracks in order currently in the playlist
            QVector< Column > m_columns;
            int m_activeRow; //! the row being played
            TrackAdvancer* m_advancer; //! the strategy of what to do when a track finishes playing
            QHash<QueryMaker*, int> m_queryMap; //!maps queries to the row where the results should be inserted

            static Model* s_instance; //! instance variable

    };
}

#endif
