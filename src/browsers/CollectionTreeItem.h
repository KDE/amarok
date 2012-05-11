/****************************************************************************************
 * Copyright (c) 2007 Alexandre Pereira de Oliveira <aleprj@gmail.com>                  *
 * Copyright (c) 2007-2009 Maximilian Kossick <maximilian.kossick@googlemail.com>       *
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

#ifndef COLLECTIONTREEITEM_H
#define COLLECTIONTREEITEM_H

#include "BrowserDefines.h"
#include "core/collections/Collection.h"
#include "core/meta/Meta.h"

#include <QList>


namespace CustomRoles
{
    enum CustomRolesId
    {
        SortRole = Qt::UserRole + 1,
        FilterRole = Qt::UserRole + 2,
        ByLineRole = Qt::UserRole + 3,
        /** Boolean value whether given collection knows about used and total capacity */
        HasCapacityRole = Qt::UserRole + 4,
        /** Number of bytes used by music and other files in collection (float) */
        UsedCapacityRole = Qt::UserRole + 5,
        /** Total capacity of the collection in bytes (float) */
        TotalCapacityRole = Qt::UserRole + 6,
        /** The number of collection actions */
        DecoratorRoleCount = Qt::UserRole + 7,
        /** The collection actions */
        DecoratorRole = Qt::UserRole + 8
    };
}

class CollectionTreeItemModelBase;

class CollectionTreeItem : public QObject
{
    Q_OBJECT
    Q_ENUMS( Type )

    public:
        enum Type
        {
            Root,
            Collection,
            VariousArtist,
            NoLabel,
            Data
        };

        CollectionTreeItem( CollectionTreeItemModelBase *model ); //root node
        CollectionTreeItem( Meta::DataPtr data, CollectionTreeItem *parent, CollectionTreeItemModelBase *model  ); //data node
        CollectionTreeItem( Collections::Collection *parentCollection, CollectionTreeItem *parent, CollectionTreeItemModelBase *model  ); //collection node
        //this ctor creates a "Various Artists" and "No Labels" nodes. do not use it for anything else
        CollectionTreeItem( Type type, const Meta::DataList &data, CollectionTreeItem *parent, CollectionTreeItemModelBase *model  ); //various artist node

        ~CollectionTreeItem();

        CollectionTreeItem* parent() { return m_parent; }

        void appendChild( CollectionTreeItem *child );
        void removeChild( int index );

        CollectionTreeItem* child( int row );

        int childCount() const { return m_childItems.count(); }
        int columnCount() const { return 1; }
        QList<CollectionTreeItem*> children() const;

        QVariant data( int role ) const;

        int row() const;

        int level() const;

        bool isDataItem() const;
        bool isAlbumItem() const;
        bool isTrackItem() const;
        bool isVariousArtistItem() const;
        bool isNoLabelItem() const;

        Collections::QueryMaker* queryMaker() const;

        /**
         * Call addMatch for this objects data and it's query maker. Handles VariousArtist
         * item and NoLabel item, too.
         *
         * @param qm QueryMaker to add match to
         * @param levelCategory category for level this item is in, one of the values from
         *        CategoryId::CatMenuId enum. Used only for distinction between Artist and
         *        AlbumArtist.
         */
        void addMatch( Collections::QueryMaker* qm, CategoryId::CatMenuId levelCategory ) const;

        bool operator<( const CollectionTreeItem& other ) const;

        const Meta::DataPtr data() const { return m_data; }
        Collections::Collection* parentCollection() const { return m_parentCollection ? m_parentCollection : (m_parent ? m_parent->parentCollection() : 0); }

        KUrl::List urls() const;
        QList<Meta::TrackPtr> descendentTracks();

        bool allDescendentTracksLoaded() const;

        //required to mark a tree item as dirty if the model has to requiry its childre

        Type type() const;
        bool requiresUpdate() const;
        void setRequiresUpdate( bool updateRequired );

    signals:
        void dataUpdated();

    private slots:
        void tracksCounted( QStringList res );
        void collectionUpdated();

    private:
        /** Returns a list of collection actions.
            Collection actions are shown on top of the collection tree item as icons (decorations)
        */
        QList<QAction*> decoratorActions() const;
        void prepareForRemoval();

        Meta::DataPtr m_data;
        CollectionTreeItem *m_parent;
        CollectionTreeItemModelBase *m_model;
        Collections::Collection *m_parentCollection;

        QList<CollectionTreeItem*> m_childItems;
        bool m_updateRequired;
        int  m_trackCount;
        Type m_type;
        //QString m_name;
        mutable bool m_isCounting;
};

#endif
