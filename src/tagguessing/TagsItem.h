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

#ifndef TAGGUESSINGTAGSITEM_H
#define TAGGUESSINGTAGSITEM_H

#include "core/meta/forward_declarations.h"

#include <QReadWriteLock>
#include <QVariant>

namespace TagGuessing {

    class TagsItem
    {
    public:
        explicit TagsItem( TagsItem *parent = 0,
                           const Meta::TrackPtr track = Meta::TrackPtr(),
                           const QVariantMap tags = QVariantMap() );
        ~TagsItem();

        TagsItem *parent() const;
        TagsItem *child( const int row ) const;
        void appendChild( TagsItem *child );
        int childCount() const;
        int row() const;

        Meta::TrackPtr track() const;
        float score() const;
        QVariantMap data() const;
        QVariant data( const int column ) const;
        void setData( const QVariantMap &tags );
        bool dataContains( const QString &key ) const;
        QVariant dataValue( const QString &key ) const;

        bool isChosen() const;
        void setChosen( bool chosen );
        TagsItem *chosenItem() const;
        bool chooseBestMatch();
        bool chooseBestMatchFromRelease( const QStringList &releases );
        void clearChoices();

        bool operator==( const TagsItem *item ) const;

    private:
        void setParent( TagsItem *parent );
        void mergeWith( TagsItem *item );
        void dataInsert( const QString &key, const QVariant &value );

        TagsItem *m_parent;
        QList<TagsItem *> m_childItems;

        Meta::TrackPtr m_track;
        QVariantMap m_data;

        bool m_chosen;

        mutable QReadWriteLock m_dataLock;
        mutable QReadWriteLock m_parentLock;
        mutable QReadWriteLock m_childrenLock;
    };
}

#endif // TAGGUESSINGTAGSITEM_H
