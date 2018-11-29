/****************************************************************************************
 * Copyright (c) 2013 Anmol Ahuja <darthcodus@gmail.com>                                *
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

#ifndef AMAROK_COLLECTIONVIEW_SCRIPT_H
#define AMAROK_COLLECTIONVIEW_SCRIPT_H

#include "browsers/BrowserDefines.h"
// #include "core/meta/forward_declarations.h"
#include "core/meta/Meta.h"

#include <QAction>
#include <QMap>
#include <QObject>
#include <QMetaEnum>
#include <QMetaType>
#include <QModelIndexList>
#include <QScriptValue>

class CollectionTreeItemModelBase;
class CollectionWidget;
class CollectionTreeItem;
namespace Collections
{
    class Collection;
    class QueryMaker;
}
class QMenu;
class QScriptEngine;
typedef QList<QAction*> QActionList;

namespace AmarokScript
{
    class AmarokScriptEngine;
    class Selection;
    class CollectionViewItem;

    // SCRIPTDOX Amarok.Window.CollectionView
    /**
     * Must call Importer.loadAmarokBinding( "collectionview" ); first;
     */
    class AmarokCollectionViewScript : public QObject
    {
        Q_OBJECT

        Q_PROPERTY( QString filter READ filter WRITE setFilter )
        Q_PROPERTY( QScriptValue selection READ selectionScriptValue )
        Q_PROPERTY( bool showYears READ showYears WRITE setShowYears )
        Q_PROPERTY( bool showTrackNumbers READ showTrackNumbers WRITE setShowTrackNumbers )
        Q_PROPERTY( bool showCovers READ showCovers WRITE setShowCovers )

        /**
         * Set levels like:
         *
         *      Importer.loadAmarokBinding("collectionview");
         *      var cat = Amarok.Window.CollectionView.Category;
         *      Amarok.Window.CollectionView.levels = [cat.Album,cat.Artist,cat.Year];
         */
        Q_PROPERTY( QList<int> levels READ levels WRITE setLevels )
        Q_PROPERTY( bool mergedView READ mergedView WRITE setMergedView )

        public:
            AmarokCollectionViewScript( AmarokScriptEngine *scriptEngine, const QString &scriptName );
            ~AmarokCollectionViewScript();
            static void createScriptedActions( QMenu &menu, const QModelIndexList &indices );
            QActionList actions();
            static Selection *selection();
            // SCRIPTDOX ENUM Amarok.CollectionView.Category
            enum Category {
                None = CategoryId::None,
                Album = CategoryId::Album,
                Artist = CategoryId::Artist,
                AlbumArtist = CategoryId::AlbumArtist,
                Composer = CategoryId::Composer,
                Genre = CategoryId::Genre,
                Year = CategoryId::Year,
                Label = CategoryId::Label
            };
            Q_ENUM( Category )

        public Q_SLOTS:
            void setLevel( int level, int type );

            /**
             * Set a function returning a QActionList here.
             */
            void setAction( const QScriptValue &value );

        Q_SIGNALS:
            void filterChanged( QString );

        private:
            QString filter() const;
            void setFilter( const QString &filter );
            QList<int> levels() const;
            void setLevels( const QList<int> &levels );
            QScriptValue selectionScriptValue();
            void setShowYears( bool shown );
            void setShowTrackNumbers( bool shown );
            void setShowCovers( bool shown );
            bool showYears();
            bool showTrackNumbers();
            bool showCovers();
            bool mergedView() const;
            void setMergedView( bool merged );

            static QMap<QString, AmarokCollectionViewScript*> s_instances;
            static QPointer<Selection> s_selection;
            QScriptValue m_actionFunction;
            CollectionWidget *m_collectionWidget;
            AmarokScriptEngine *m_engine;
            QString m_scriptName;
            const QMetaEnum m_categoryEnum;

        friend class AmarokWindowScript;
    };

    /**
     * Represents an item from the CollectionTreeView
     */
    class CollectionViewItem : public QObject
    {
        Q_OBJECT

        Q_PROPERTY( CollectionTreeItem* parent READ parent )
        Q_PROPERTY( int childCount READ childCount )
        Q_PROPERTY( QList<CollectionTreeItem*> children READ children )
        Q_PROPERTY( int row READ row )
        Q_PROPERTY( int level READ level )
        Q_PROPERTY( Collections::Collection* parentCollection READ parentCollection )
        Q_PROPERTY( bool isCollection READ isCollection )
        Q_PROPERTY( bool isDataItem READ isDataItem )
        Q_PROPERTY( bool isAlbumItem READ isAlbumItem )
        Q_PROPERTY( bool isTrackItem READ isTrackItem )
        Q_PROPERTY( bool isVariousArtistItem READ isVariousArtistItem )
        Q_PROPERTY( bool isNoLabelItem READ isNoLabelItem )

        /**
         * Returns true if all of this item's children have been loaded.
         */
        Q_PROPERTY( bool childrenLoaded READ childrenLoaded )

        public:
            explicit CollectionViewItem( CollectionTreeItem *item, QObject *parent = 0 );
            CollectionTreeItem* data() const;
            static QScriptValue toScriptValue( QScriptEngine *engine, CollectionTreeItem* const &item );

            /**
             * @return QueryMaker representing this item's descendant tracks ( or just the item if a track )
             */
            Q_INVOKABLE Collections::QueryMaker* queryMaker();

            /**
             * Add a filter representing this item to @p queryMaker.
             * For example, for obtaining a specific collection's items in the merged view.
             *
             * @param queryMaker the Query maker.
             */
            Q_INVOKABLE void addFilter( Collections::QueryMaker *queryMaker );

            /**
             * Return the child item at @p row.
             *
             * @param row the number of the row.
             */
            Q_INVOKABLE CollectionTreeItem* child( int row );

            /**
             * Load children if they haven't already been loaded.
             * Emit loaded( CollectionTreeItem* ) on completion.
             */
            Q_INVOKABLE void loadChildren();

            /**
             * Return the track item represented by this collection item,
             * invalid track if !isTrack
             */
            Q_INVOKABLE Meta::TrackPtr track();

        private Q_SLOTS:
            void slotDataChanged( const QModelIndex &topLeft, const QModelIndex &bottomRight );

        Q_SIGNALS:
            void loaded( CollectionTreeItem* );

        private:
            CollectionTreeItem* parent() const;
            QList<CollectionTreeItem*> children() const;
            int row() const;
            int level() const;
            bool isAlbumItem() const;
            bool isTrackItem() const;
            bool isVariousArtistItem() const;
            bool isNoLabelItem() const;
            bool isDataItem() const;
            int childCount() const;
            bool isCollection() const;
            Collections::Collection* parentCollection() const;
            bool childrenLoaded() const;
            CollectionTreeItemModelBase *getModel();

            CollectionTreeItem *m_item;
    };

    class Selection : public QObject
    {
        Q_OBJECT

        /**
         * Whether the selected items belongs to a single collection.
         */
        Q_PROPERTY( bool singleCollection READ singleCollection )

        /**
         * Get the selected lis tof items.
         */
        Q_PROPERTY( QList<CollectionTreeItem*> selectedItems READ selectedItems )

        public Q_SLOTS:
            /**
             * Get a QueryMaker for the selected items.
             */
            Collections::QueryMaker* queryMaker();

        private:
            Selection( const QModelIndexList &indices );
            bool singleCollection() const;
            QList<CollectionTreeItem*> selectedItems();

            QModelIndexList m_indices;

        friend AmarokCollectionViewScript;
    };
}

#endif
