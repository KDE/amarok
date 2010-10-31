/****************************************************************************************
 * Copyright (c) 2010 Sergey Ivanov <123kash@gmail.com>                                 *
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

#ifndef MUSICBRAINZTAGSMODEL_H
#define MUSICBRAINZTAGSMODEL_H

#include <KLocalizedString>
#include <core/meta/Meta.h>
#include <QAbstractItemModel>

class MusciBrainzTagsItem
{
    public:
        MusciBrainzTagsItem( Meta::TrackPtr track,  const QVariantMap tags = QVariantMap() );
        MusciBrainzTagsItem() {}

        Meta::TrackPtr track();

        Qt::ItemFlags flags();

        QVariant data( int column );
        QVariantMap data();
        void setData( QVariantMap tags );

        bool checked();
        void setChecked( bool checked );

    private:
        Meta::TrackPtr m_track;
        QVariantMap m_data;

        bool m_checked;
};

class MusicBrainzTagsModel : public QAbstractItemModel
{
    Q_OBJECT

    public:
        MusicBrainzTagsModel( Meta::TrackList tracks, QObject *parent = 0 );
        ~MusicBrainzTagsModel();

        QModelIndex index( int row, int column, const QModelIndex &parent = QModelIndex() ) const;
        QModelIndex parent( const QModelIndex &index ) const;

        Qt::ItemFlags flags( const QModelIndex &index ) const;
        QVariant data( const QModelIndex &index, int role = Qt::DisplayRole ) const;
        bool setData( const QModelIndex &index, const QVariant &value, int role );
        QVariant headerData( int section, Qt::Orientation orientation, int role = Qt::DisplayRole ) const;

        int rowCount( const QModelIndex &parent = QModelIndex() ) const;
        int columnCount( const QModelIndex & ) const;

        QMap < Meta::TrackPtr, QVariantMap > getAllChecked(); 

    public slots:
        void addTrack( const Meta::TrackPtr track, const QVariantMap tags );
        void selectAll( int section );

    private:
        QList < MusciBrainzTagsItem > m_items;

        bool m_singleTrackMode;
};

#endif // MUSICBRAINZTAGSMODEL_H
