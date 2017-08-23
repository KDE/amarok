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

#ifndef MUSICBRAINZTAGSITEM_H
#define MUSICBRAINZTAGSITEM_H

#include "core/meta/forward_declarations.h"

#include <QReadWriteLock>
#include <QVariant>

class MusicBrainzTagsItem
{
    public:
        explicit MusicBrainzTagsItem( MusicBrainzTagsItem *parent = 0,
                                      const Meta::TrackPtr track = Meta::TrackPtr(),
                                      const QVariantMap tags = QVariantMap() );
        ~MusicBrainzTagsItem();

        MusicBrainzTagsItem *parent() const;
        MusicBrainzTagsItem *child( const int row ) const;
        void appendChild( MusicBrainzTagsItem *child );
        int childCount() const;
        int row() const;

        Meta::TrackPtr track() const;
        float score() const;
        QVariantMap data() const;
        QVariant data( const int column ) const;
        void setData( const QVariantMap &tags );
        void mergeData( const QVariantMap &tags );
        bool dataContains( const QString &key ) const;
        QVariant dataValue( const QString &key ) const;

        bool isChosen() const;
        void setChosen( bool chosen );
        MusicBrainzTagsItem *chosenItem() const;
        bool chooseBestMatch();
        bool chooseBestMatchFromRelease( const QStringList &releases );
        void clearChoices();

        bool isSimilar( const QVariantMap &tags ) const;

        bool operator==( const MusicBrainzTagsItem *item ) const;
        bool operator==( const Meta::TrackPtr &track) const;

    private:
        void setParent( MusicBrainzTagsItem *parent );
        void dataInsert( const QString &key, const QVariant &value );
        void recalcSimilarityRate();

        MusicBrainzTagsItem *m_parent;
        QList<MusicBrainzTagsItem *> m_childItems;

        Meta::TrackPtr m_track;
        QVariantMap m_data;

        bool m_chosen;

        mutable QReadWriteLock m_dataLock;
        mutable QReadWriteLock m_parentLock;
        mutable QReadWriteLock m_childrenLock;
};

#endif // MUSICBRAINZTAGSITEM_H
