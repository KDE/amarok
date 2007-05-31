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
#include <kdemacros.h>

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

    ///Options for insertTracks
    enum AddOptions 
    { 
        Append     = 1,     /// inserts media after the last item in the playlist
        Queue      = 2,     /// inserts media after the currentTrack
        Replace    = 4,     /// clears the playlist first
        DirectPlay = 8,     /// start playback of the first item in the list
        Unique     = 16,    /// don't insert anything already in the playlist
        StartPlay  = 32,    /// start playback of the first item in the list if nothing else playing
        Colorize   = 64,    /// colorize newly added items
        AppendAndPlay = Append | Unique | StartPlay
    };

    class Model : public QAbstractTableModel, Meta::TrackObserver
    {
        Q_OBJECT
        public:
            Model( QObject* parent = 0 ); 
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
            void clear(); ///clear the playlist of all items
            void setColumns( QVector< Column > columns ) { m_columns = columns; }
/**
* Insert tracks into the playlist with some handy options.
* @param list tracks to add
* @param options valid values are Unique || (Append xor Queue xor Replace) || ( DirectPlay xor StartPlay )
**/
            void insertOptioned( Meta::TrackList list, int options );
            void insertTracks( int row, Meta::TrackList list ); //doesn't override
            void insertTracks( int row, QueryMaker *qm );
            int activeRow() const { return m_activeRow; }
            void setActiveRow( int row );
            Meta::TrackPtr activeTrack() const { return m_tracks[ m_activeRow ]; }
        //    Qt::ItemFlags flags(const QModelIndex &index) const;
            void testData();
            ///deprecated function to ease porting to Meta::Track from URLs
            KDE_DEPRECATED void insertMedia( KUrl::List list, int options = Append );
            virtual void metadataChanged( Meta::Track *track );
            void play( int row );
            static Model* s_instance; //! instance variable
        public slots:
            void play( const QModelIndex& index );
        private slots:
            void trackFinished(); //! what to do when a track finishes
            void queryDone();
            void newResultReady( const QString &collectionId, const Meta::TrackList &tracks );
           
        private:
            static QString prettyColumnName( Column index ); //!takes a Column enum and returns its string name

            Meta::TrackList m_tracks; //! list of tracks in order currently in the playlist
            QVector< Column > m_columns;
            int m_activeRow; //! the row being played
            TrackAdvancer* m_advancer; //! the strategy of what to do when a track finishes playing
            QHash<QueryMaker*, int> m_queryMap; //!maps queries to the row where the results should be inserted

    };
}



#endif
