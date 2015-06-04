/****************************************************************************************
 * Copyright (c) 2010 Sergey Ivanov <123kash@gmail.com>                                 *
 * Copyright (c) 2013 Alberto Villa <avilla@FreeBSD.org>                                *
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

#include "core/meta/forward_declarations.h"

#include <QAbstractItemModel>

class MusicBrainzTagsItem;

class MusicBrainzTagsModel : public QAbstractItemModel
{
    Q_OBJECT

    public:
        enum {
            SortRole = Qt::UserRole,
            TracksRole,
            ArtistsRole,
            ReleasesRole,
            ChosenStateRole
        };

        enum ChosenState {
            Unchosen,
            Chosen
        };

        explicit MusicBrainzTagsModel( QObject *parent = 0 );
        ~MusicBrainzTagsModel();

        QModelIndex index( int row, int column,
                           const QModelIndex &parent = QModelIndex() ) const;
        QModelIndex parent( const QModelIndex &index ) const;

        QVariant data( const QModelIndex &index, int role = Qt::DisplayRole ) const;
        bool setData( const QModelIndex &index, const QVariant &value, int role );
        Qt::ItemFlags flags( const QModelIndex &index ) const;
        QVariant headerData( int section, Qt::Orientation orientation,
                             int role = Qt::DisplayRole ) const;

        int rowCount( const QModelIndex &parent = QModelIndex() ) const;
        int columnCount( const QModelIndex &parent = QModelIndex() ) const;

        QMap<Meta::TrackPtr, QVariantMap> chosenItems() const;
        void chooseBestMatchesFromRelease( const QStringList &releases );

    public Q_SLOTS:
        void addTrack( const Meta::TrackPtr track, const QVariantMap tags );

        void chooseBestMatches();
        void clearChoices();

    private:
        MusicBrainzTagsItem *m_rootItem;
};

#endif // MUSICBRAINZTAGSMDOEL_H
