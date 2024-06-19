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

#include "amarok_export.h"
#include "browsers/BrowserDefines.h"
#include "core/collections/Collection.h"
#include "core/meta/forward_declarations.h"

#include <QList>

class CollectionTreeItemModelBase;
class QAction;

class AMAROK_EXPORT CollectionTreeItem : public QObject
{
    Q_OBJECT

    public:
        enum Type
        {
            Root,
            Collection,
            VariousArtist,
            NoLabel,
            Data
        };
        Q_ENUM( Type )

        explicit CollectionTreeItem( CollectionTreeItemModelBase *model ); //root node
        CollectionTreeItem( const Meta::DataPtr &data, CollectionTreeItem *parent, CollectionTreeItemModelBase *model  ); //data node
        CollectionTreeItem( Collections::Collection *parentCollection, CollectionTreeItem *parent, CollectionTreeItemModelBase *model  ); //collection node
        //this ctor creates a "Various Artists" and "No Labels" nodes. do not use it for anything else
        CollectionTreeItem( Type type, const Meta::DataList &data, CollectionTreeItem *parent, CollectionTreeItemModelBase *model  ); //various artist node

        ~CollectionTreeItem() override;

        CollectionTreeItem* parent() const { return m_parent; }

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
        bool isArtistItem() const;
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
        void addMatch( Collections::QueryMaker *qm, CategoryId::CatMenuId levelCategory ) const;

        bool operator<( const CollectionTreeItem &other ) const;

        const Meta::DataPtr data() const;
        Collections::Collection* parentCollection() const { return m_parentCollection ? m_parentCollection : (m_parent ? m_parent->parentCollection() : nullptr); }

        Meta::TrackList descendentTracks();

        bool allDescendentTracksLoaded() const;

        //required to mark a tree item as dirty if the model has to require its children

        Type type() const;
        bool requiresUpdate() const;
        void setRequiresUpdate( bool updateRequired );

    Q_SIGNALS:
        void dataUpdated();

    private Q_SLOTS:
        void tracksCounted( QStringList res );
        void collectionUpdated();

    private:
        /** Returns a list of collection actions.
            Collection actions are shown on top of the collection tree item as icons (decorations)
        */
        QList<QAction *> decoratorActions() const;
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

Q_DECLARE_METATYPE( CollectionTreeItem* )
Q_DECLARE_METATYPE( QList<CollectionTreeItem*> )

#endif
