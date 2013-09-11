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

#define DEBUG_PREFIX "CollectionViewScript"
#include "AmarokCollectionViewScript.h"

#include "core/support/Debug.h"
#include "ScriptingDefines.h"
#include "browsers/CollectionTreeView.h"
#include "browsers/collectionbrowser/CollectionWidget.h"
#include <browsers/collectionbrowser/CollectionBrowserTreeView.h>
#include "browsers/CollectionTreeItem.h"
#include "MainWindow.h"

#include "amarokconfig.h"

#include <KMenu>

#include <QMetaEnum>
#include <QScriptEngine>

Q_DECLARE_METATYPE( QAction* )
Q_DECLARE_METATYPE( QList<QAction*> )

using namespace AmarokScript;

QMap<QString, AmarokCollectionViewScript*> AmarokCollectionViewScript::s_instances;
Selection *AmarokCollectionViewScript::s_selection = 0;

AmarokCollectionViewScript::AmarokCollectionViewScript( AmarokScriptEngine *engine, const QString &scriptName )
    : QObject( engine )
    , m_collectionWidget( The::mainWindow()->collectionBrowser() )
    , m_engine( engine )
    , m_scriptName( scriptName )
{
    QScriptValue scriptObject = engine->newQObject( this, QScriptEngine::AutoOwnership,
                                                    QScriptEngine::ExcludeSuperClassContents );
    QScriptValue windowObject = engine->globalObject().property( "Amarok" ).property( "Window" );
    Q_ASSERT( !windowObject.isUndefined() );
    windowObject.setProperty( "CollectionView", scriptObject );
    /*const QMetaEnum categoryEnum = metaObject()->enumerator( metaObject()->indexOfEnumerator("Category") );
    Q_ASSERT( categoryEnum.isValid() );
    scriptObject.setProperty( "Category", engine->enumObject( categoryEnum ) );*/
    qScriptRegisterMetaType<CollectionTreeItem*>( engine, CollectionViewItem::toScriptValue, fromScriptValue<CollectionTreeItem*, CollectionViewItem> );
    engine->registerArrayType< QList<CollectionTreeItem*> >();
    engine->registerArrayType<QActionList>();
    s_instances[m_scriptName] = this;
}

AmarokCollectionViewScript::~AmarokCollectionViewScript()
{
    s_instances.remove( m_scriptName );
}

void
AmarokCollectionViewScript::setFilter( const QString &filter )
{
    return m_collectionWidget->setFilter( filter );
}

QString
AmarokCollectionViewScript::filter() const
{
    return m_collectionWidget->filter();
}

QActionList
AmarokCollectionViewScript::actions()
{
    QScriptValue actions = m_actionFunction.call( QScriptValue(), QScriptValueList() << selectionScriptValue() );
    QActionList actionList = m_engine->fromScriptValue<QActionList>( actions );
    debug() << "Received " << actionList.size() << " actions";
    return actionList;
}

void
AmarokCollectionViewScript::setAction( const QScriptValue &value )
{
    m_actionFunction = value;
}

void
AmarokCollectionViewScript::createScriptedActions( KMenu &menu, const QModelIndexList &indices )
{
    debug() << "Checking for scripted actions";
    if( s_selection ) delete s_selection;
    debug() << "here";
    if( s_instances.isEmpty() )
        return;
    s_selection = new Selection( indices );

    debug() << "here2";
    foreach( const QString &scriptName, s_instances.keys() )
    {
        if( s_instances[scriptName] )
        {
            debug() << "Adding actions for script " << scriptName;
            menu.addSeparator();
            foreach( QAction *action, s_instances[scriptName]->actions() )
            {
                if( !action )
                {
                    debug() << "Null action received from script " << scriptName;
                    continue;
                }
                action->setParent( &menu );
                menu.addAction( action );
            }
        }
    }
}

QScriptValue
AmarokCollectionViewScript::selectionScriptValue()
{
    return m_engine->newQObject( s_selection, QScriptEngine::QtOwnership,
                                QScriptEngine::ExcludeSuperClassContents );
}

Selection*
AmarokCollectionViewScript::selection()
{
    return s_selection;
}

void
AmarokCollectionViewScript::setShowCovers( bool shown )
{
    CollectionWidget::instance()->slotShowCovers( shown );
}

void
AmarokCollectionViewScript::setShowTrackNumbers( bool shown )
{
    CollectionWidget::instance()->slotShowTrackNumbers( shown );
}

void
AmarokCollectionViewScript::setShowYears( bool shown )
{
    CollectionWidget::instance()->slotShowYears( shown );
}

bool
AmarokCollectionViewScript::showCovers()
{
    return AmarokConfig::showAlbumArt();
}

bool
AmarokCollectionViewScript::showTrackNumbers()
{
    return AmarokConfig::showTrackNumbers();
}

bool
AmarokCollectionViewScript::showYears()
{
    return AmarokConfig::showYears();
}

void
AmarokCollectionViewScript::toggleView( bool merged )
{
    CollectionWidget::instance()->toggleView( merged );
}

///////////////////////////////////////////////////////////
// CollectionViewItem
///////////////////////////////////////////////////////////

CollectionTreeItem*
CollectionViewItem::child( int row )
{
    return m_item->child( row );
}

int
CollectionViewItem::childCount() const
{
    return m_item->childCount();
}

QList<CollectionTreeItem*>
CollectionViewItem::children() const
{
    return m_item->children();
}

CollectionViewItem::CollectionViewItem( CollectionTreeItem *item, QObject *parent )
    : QObject( parent )
    , m_item( item )
{}

bool
CollectionViewItem::isTrackItem() const
{
    return m_item->isTrackItem();
}

int
CollectionViewItem::level() const
{
    return m_item->level();
}

CollectionTreeItem*
CollectionViewItem::parent() const
{
    return m_item->parent();
}

Collections::Collection*
CollectionViewItem::parentCollection() const
{
    return m_item->parentCollection();
}

int
CollectionViewItem::row() const
{
    return m_item->row();
}

bool
CollectionViewItem::isCollection() const
{
    return m_item->type() == CollectionTreeItem::Collection;
}

CollectionTreeItem*
CollectionViewItem::data()
{
    return m_item;
}

QScriptValue
CollectionViewItem::toScriptValue( QScriptEngine *engine, CollectionTreeItem* const &item )
{
    CollectionViewItem *proto = new CollectionViewItem( item, AmarokCollectionViewScript::selection() );
    QScriptValue val = engine->newQObject( proto, QScriptEngine::AutoOwnership,
                                            QScriptEngine::ExcludeSuperClassContents );
    return val;
}

///////////////////////////////////////////////////////////
// Selection
///////////////////////////////////////////////////////////

bool
Selection::singleCollection() const
{
    return CollectionTreeView::onlyOneCollection( m_indices );
}

int
Selection::collectionCount() const
{
    return 0; // ANM-TODO
}

QList<CollectionTreeItem*>
Selection::selectedItems()
{
    QList<CollectionTreeItem*> collectionItems;
    foreach( const QModelIndex &index, m_indices )
    {
        collectionItems << static_cast<CollectionTreeItem*>( index.internalPointer() );
    }
    return collectionItems;
}

Selection::Selection( const QModelIndexList &indices )
    : QObject( 0 )
    , m_indices( indices )
{}

Collections::QueryMaker*
Selection::queryMaker()
{
    return The::mainWindow()->collectionBrowser()->currentView()->createMetaQueryFromItems( selectedItems().toSet(), true );
}
