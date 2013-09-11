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

// #include "browsers/BrowserDefines.h"
// #include "core/meta/forward_declarations.h"
#include "core/meta/Meta.h"

#include <QAction>
#include <QMap>
#include <QObject>
#include <QMetaType>
#include <QModelIndexList>
#include <QScriptValue>

class CollectionWidget;
class CollectionTreeItem;
namespace Collections
{
    class Collection;
    class QueryMaker;
}
class KMenu;
class QScriptEngine;
typedef QList<QAction *> QActionList;

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
        Q_ENUMS( Category )

        Q_PROPERTY( QString filter READ filter WRITE setFilter )
        Q_PROPERTY( QScriptValue selection READ selectionScriptValue )
        Q_PROPERTY( bool showYears READ showYears WRITE setShowYears )
        Q_PROPERTY( bool showTrackNumbers READ showTrackNumbers WRITE setShowTrackNumbers )
        Q_PROPERTY( bool showCovers READ showCovers WRITE setShowCovers )

        // Q_PROPERTY( QList<Category> levels READ levels WRITE setLevels )

        public:
            AmarokCollectionViewScript( AmarokScriptEngine *scriptEngine, const QString &scriptName );
            ~AmarokCollectionViewScript();
            static void createScriptedActions( KMenu &menu, const QModelIndexList &indices );
            QActionList actions();
            static Selection *selection();
            /*
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
            */
            enum ContextCategory {
                Track = 1,
                Collection = 2
            };

        public slots:
            // void setLevel( int level, Category type );
            /**
             * Toggle between unified and
             */
            void toggleView( bool merged );

            /**
             * Set a function returning a QActionList here.
             */
            void setAction( const QScriptValue &value );

        private:
            QString filter() const;
            void setFilter( const QString &filter );
            //QList<Category> levels() const;
            //void setLevels( const QList<Category> &levels );
            QScriptValue selectionScriptValue();
            void setShowYears( bool shown );
            void setShowTrackNumbers( bool shown );
            void setShowCovers( bool shown );
            bool showYears();
            bool showTrackNumbers();
            bool showCovers();


            static QMap< QString, AmarokCollectionViewScript*> s_instances;
            static Selection *s_selection;
            QScriptValue m_actionFunction;
            CollectionWidget *m_collectionWidget;
            AmarokScriptEngine *m_engine;
            QString m_scriptName;

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
        Q_PROPERTY( int row READ row )
        Q_PROPERTY( int level READ level )
        Q_PROPERTY( int childCount READ childCount )
        Q_PROPERTY( bool isCollection READ isCollection )
        Q_PROPERTY( bool isTrack READ isTrackItem )
        Q_PROPERTY( Collections::Collection* parentCollection READ parentCollection )

        public:
            CollectionViewItem( CollectionTreeItem *item, QObject *parent = 0 );
            CollectionTreeItem* data();
            static QScriptValue toScriptValue( QScriptEngine *engine, CollectionTreeItem* const &item );

        public slots:
            CollectionTreeItem* child( int row );

        private:
            CollectionTreeItem* parent() const;
            QList<CollectionTreeItem*> children() const;
            int row() const;
            int level() const;
            // bool isAlbumItem() const;
            bool isTrackItem() const;
            int childCount() const;
            Meta::TrackList descendentTracks();
            bool isCollection() const;
            Collections::Collection* parentCollection() const;

            CollectionTreeItem* m_item;
    };

    class Selection : public QObject
    {
        Q_OBJECT

        /**
         * Whether the selected items belongs to a single collection.
         */
        Q_PROPERTY( bool singleCollection READ singleCollection )

        /**
         * Number of collections in the selection.
         */
        Q_PROPERTY( int collectionCount READ collectionCount )

        /**
         * Get the selected lis tof items.
         */
        Q_PROPERTY( QList<CollectionTreeItem*> selectedItems READ selectedItems )

        public slots:
            /**
             * Get a QueryMaker for the selected items.
             */
            Collections::QueryMaker* queryMaker();

        private:
            Selection( const QModelIndexList &indices );
            bool singleCollection() const;
            int collectionCount() const;
            QList<CollectionTreeItem*> selectedItems();

            QModelIndexList m_indices;

        friend AmarokCollectionViewScript;
    };
}

#endif
