/****************************************************************************************
 * Copyright (c) 2007 Alexandre Pereira de Oliveira <aleprj@gmail.com>                  *
 * Copyright (c) 2007 Maximilian Kossick <maximilian.kossick@googlemail.com>            *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Pulic License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#ifndef COLLECTIONTREEITEM_H
#define COLLECTIONTREEITEM_H

#include "meta/Meta.h"
#include "Collection.h"

#include <QList>

namespace CustomRoles
{
    enum CustomRolesId
    {
        SortRole = Qt::UserRole + 1,
        FilterRole = Qt::UserRole + 2,
        ByLineRole = Qt::UserRole + 3,
        HasCapacityRole = Qt::UserRole + 4,
        UsedCapacityRole = Qt::UserRole + 5,
        DecoratorsRole = Qt::UserRole + 6
    };
}

class CollectionTreeItem : public QObject
{
    Q_OBJECT

    public:
        CollectionTreeItem( Meta::DataPtr data, CollectionTreeItem *parent );
        CollectionTreeItem( Amarok::Collection *parentCollection, CollectionTreeItem *parent );
        //this ctor creates a "Various Artists" node. do not use it for anything else
        CollectionTreeItem( const Meta::DataList &data, CollectionTreeItem *parent );

        ~CollectionTreeItem();

        CollectionTreeItem* parent() { return m_parent; }

        void appendChild( CollectionTreeItem *child );
        void removeChild( int index );

        CollectionTreeItem *child( int row );

        int childCount() const { return m_childItems.count(); }
        int columnCount() const { return 1; }

        QVariant data( int role ) const;

        int row() const;

        int level() const;

        bool isDataItem() const;
        bool isAlbumItem() const;
        bool isTrackItem() const;

        QueryMaker* queryMaker() const;

        bool operator<( const CollectionTreeItem& other ) const;
        bool childrenLoaded() const { return  m_childrenLoaded; }
        void setChildrenLoaded( bool childrenLoaded );

        const Meta::DataPtr data() const { return m_data; }
        Amarok::Collection* parentCollection() const { return m_parentCollection; }

        KUrl::List urls() const;
        QList<Meta::TrackPtr> descendentTracks();

        bool allDescendentTracksLoaded() const;

    signals:
        void dataUpdated();

    private slots:
        void tracksCounted( QString collectionId, QStringList res );
        void collectionUpdated();

    private:
        QString albumYear() const;

        Meta::DataPtr m_data;
        CollectionTreeItem *m_parent;
        Amarok::Collection* m_parentCollection;

        QList<CollectionTreeItem*> m_childItems;
        bool m_childrenLoaded;
        bool m_isVariousArtistsNode;
        int  m_trackCount;
        mutable bool m_isCounting;
};

#endif
