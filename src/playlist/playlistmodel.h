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
        //TODO remove singleton when playlist controller class is created
        static Model *instance() {
            if(!s_instance)  s_instance = new Model;
            return s_instance;
        }
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
            void insertTracks( int row, Meta::TrackList list ); //doesn't override
        //    Qt::ItemFlags flags(const QModelIndex &index) const;
            void testData();
            
        private:
            static QString prettyColumnName( Column index );
            Model( QObject* parent = 0 ); //singleton
            Meta::TrackList m_tracks;
            QVector< Column > m_columns;
            static Model* s_instance;
    };
}
