/***************************************************************************
 * copyright            : (C) 2007 Ian Monroe <ian@monroe.nu>              *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License version 2        *
 *   as published by the Free Software Foundation.                         *
 ***************************************************************************/

#include "meta.h"

#include <QAbstractTableModel>
#include <QVector>

#include <klocale.h>


//PORT rename to Playlist when the playlist class is removed
namespace PlaylistNS {

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
        public:
            Model( QObject* parent = 0 );
        //required by QAbstractTabelModel
            int rowCount(const QModelIndex &parent) const;
            int columnCount(const QModelIndex &parent) const;
            QVariant data(const QModelIndex &index, int role) const;
        //overriding QAbstractItemModel
            bool removeRows( int position, int rows );
            QVariant headerData(int section, Qt::Orientation orientation, int role) const;
        //other methods
            void setColumns( QVector< Column > columns ) { m_columns = columns; }
            void insertTracks( int row, Meta::TrackList list ); //doesn't override
            void testData();
            static QString prettyColumnName( Column index );
        private:
            
            Meta::TrackList m_tracks;
            QVector< Column > m_columns;
    };
}
