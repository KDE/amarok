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

#include "amarokconfig.h"
#include "core/support/Debug.h"
#include "browsers/CollectionTreeView.h"
#include "browsers/collectionbrowser/CollectionWidget.h"
#include "browsers/collectionbrowser/CollectionBrowserTreeView.h"
#include "browsers/CollectionTreeItem.h"
#include "browsers/CollectionTreeItemModelBase.h"
#include "core-impl/collections/support/CollectionManager.h"
#include "core-impl/collections/support/TextualQueryFilter.h"
#include "MainWindow.h"
#include "ScriptingDefines.h"
#include "widgets/SearchWidget.h"

#include <QMenu>
#include <QMetaEnum>
#include <QQmlEngine>
#include <QJSEngine>
#include <QSortFilterProxyModel>

Q_DECLARE_METATYPE( QAction* )
Q_DECLARE_METATYPE( QList<QAction*> )

using namespace AmarokScript;

QMap<QString, AmarokCollectionViewScript*> AmarokCollectionViewScript::s_instances;
QPointer<Selection> AmarokCollectionViewScript::s_selection;

AmarokCollectionViewScript::AmarokCollectionViewScript( AmarokScriptEngine *engine, const QString &scriptName )
    : QObject( engine )
    , m_collectionWidget( The::mainWindow()->collectionBrowser() )
    , m_engine( engine )
    , m_scriptName( scriptName )
    , m_categoryEnum( metaObject()->enumerator( metaObject()->indexOfEnumerator("Category") ) )
{
    QJSValue scriptObject = engine->newQObject( this );
    QJSValue windowObject = engine->globalObject().property( QStringLiteral("Amarok") ).property( QStringLiteral("Window") );
    Q_ASSERT( !windowObject.isUndefined() );
    windowObject.setProperty( QStringLiteral("CollectionView"), scriptObject );
    const QMetaEnum typeEnum = CollectionTreeItem::staticMetaObject.enumerator( CollectionTreeItem::staticMetaObject.indexOfEnumerator( "Type" ) );
    Q_ASSERT( typeEnum.isValid() );
    scriptObject.setProperty( QStringLiteral("Type"), engine->enumObject( typeEnum ) );
    Q_ASSERT( m_categoryEnum.isValid() );
    scriptObject.setProperty( QStringLiteral("Category"), engine->enumObject( m_categoryEnum ) );

    qRegisterMetaType<CollectionTreeItem*>();
    QMetaType::registerConverter<CollectionTreeItem*, QJSValue>( [=] (CollectionTreeItem* item) { return CollectionViewItem::toScriptValue( m_engine, item ); } );
    QMetaType::registerConverter<QJSValue, CollectionTreeItem*>( [=] (QJSValue jsValue) {
        CollectionTreeItem* item;
        fromScriptValue<CollectionTreeItem*, CollectionViewItem>( jsValue, item );
        return item;
    } );
    engine->registerArrayType< QList<CollectionTreeItem*> >();
    engine->registerArrayType<QActionList>();
    s_instances[m_scriptName] = this;
    connect( The::mainWindow()->collectionBrowser()->searchWidget(), &SearchWidget::filterChanged, this, &AmarokCollectionViewScript::filterChanged );
}

AmarokCollectionViewScript::~AmarokCollectionViewScript()
{
    s_instances.remove( m_scriptName );
    if( s_instances.isEmpty() )
        delete s_selection.data();
}

void
AmarokCollectionViewScript::setFilter( const QString &filter )
{
    m_collectionWidget->setFilter( filter );
}

QString
AmarokCollectionViewScript::filter() const
{
    return m_collectionWidget->filter();
}

QActionList
AmarokCollectionViewScript::actions()
{
    QJSValue actions = m_actionFunction.call( QJSValueList() << selectionScriptValue() );
    QActionList actionList = m_engine->fromScriptValue<QActionList>( actions );
    debug() << "Received " << actionList.size() << " actions";
    return actionList;
}

void
AmarokCollectionViewScript::setAction( const QJSValue &value )
{
    m_actionFunction = value;
}

void
AmarokCollectionViewScript::createScriptedActions( QMenu &menu, const QModelIndexList &indices )
{
    debug() << "Checking for scripted actions";
    if( s_selection )
        delete s_selection.data();
    if( s_instances.isEmpty() )
        return;
    s_selection = new Selection( indices );

    for( const QString &scriptName : s_instances.keys() )
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

QJSValue
AmarokCollectionViewScript::selectionScriptValue()
{
    QQmlEngine::setObjectOwnership( s_selection.data(), QQmlEngine::CppOwnership);
    return m_engine->newQObject( s_selection.data() );

}

Selection*
AmarokCollectionViewScript::selection()
{
    return s_selection.data();
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

bool
AmarokCollectionViewScript::mergedView() const
{
    return m_collectionWidget->viewMode() == CollectionWidget::UnifiedCollection;
}

void
AmarokCollectionViewScript::setMergedView( bool merged )
{
    CollectionWidget::instance()->toggleView( merged );
}

QList<int>
AmarokCollectionViewScript::levels() const
{
    QList<int> levels;
    for( CategoryId::CatMenuId level : m_collectionWidget->currentView()->levels() )
        levels << level;
    return levels;
}

void
AmarokCollectionViewScript::setLevel( int level, int type )
{
    if( m_categoryEnum.valueToKey( type ) ) {
        m_collectionWidget->currentView()->setLevel( level, CategoryId::CatMenuId( type ) );
        return;
    }
    m_engine->throwError( QJSValue::TypeError, QStringLiteral("Invalid category!") );
}

void
AmarokCollectionViewScript::setLevels( const QList<int> &levels )
{
    QList<CategoryId::CatMenuId> catLevels;
    for( int level : levels )
    {
        if( !m_categoryEnum.valueToKey( level ) )
        {
            m_engine->throwError( QJSValue::TypeError, QStringLiteral("Invalid category!") );
            return;
        }
        catLevels << CategoryId::CatMenuId( level );
    }
    m_collectionWidget->setLevels( catLevels );
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
CollectionViewItem::data() const
{
    return m_item;
}

QJSValue
CollectionViewItem::toScriptValue( QJSEngine *engine, CollectionTreeItem* const &item )
{
    CollectionViewItem *proto = new CollectionViewItem( item, AmarokCollectionViewScript::selection() );
    QJSValue val = engine->newQObject( proto );
    return val;
}

Meta::TrackPtr
CollectionViewItem::track()
{
    return Meta::TrackPtr::dynamicCast( m_item->data() );
}

bool
CollectionViewItem::isAlbumItem() const
{
    return m_item->isAlbumItem();
}

bool
CollectionViewItem::isDataItem() const
{
    return m_item->isDataItem();
}

bool
CollectionViewItem::isNoLabelItem() const
{
    return m_item->isNoLabelItem();
}

bool
CollectionViewItem::isVariousArtistItem() const
{
    return m_item->isVariousArtistItem();
}

bool
CollectionViewItem::childrenLoaded() const
{
    return m_item->isTrackItem() || !m_item->requiresUpdate();
}

void
CollectionViewItem::loadChildren()
{
    if( !m_item->requiresUpdate() )
        return;
    CollectionTreeItemModelBase *model = getModel();
    connect( model, &CollectionTreeItemModelBase::dataChanged,
             this, &CollectionViewItem::slotDataChanged );
    model->ensureChildrenLoaded( m_item );
}

void
CollectionViewItem::slotDataChanged( const QModelIndex &topLeft, const QModelIndex &bottomRight )
{
    Q_UNUSED( bottomRight )
    if( static_cast<CollectionTreeItem*>( topLeft.internalPointer() ) != m_item )
        return;
    Q_EMIT loaded( m_item );
    Q_ASSERT( disconnect( qobject_cast<QAbstractItemModel*>(sender()), &QAbstractItemModel::dataChanged, this, nullptr ) );
}

Collections::QueryMaker*
CollectionViewItem::queryMaker()
{
    Collections::QueryMaker *qm = nullptr;
    if( The::mainWindow()->collectionBrowser()->viewMode() == CollectionWidget::NormalCollections )
        qm = m_item->queryMaker();
    else
        qm = CollectionManager::instance()->queryMaker();
    addFilter( qm );
    return qm;
}

void
CollectionViewItem::addFilter( Collections::QueryMaker *queryMaker )
{
    if( !queryMaker )
        return;
    CollectionTreeItemModelBase *model = getModel();
    for( CollectionTreeItem *tmp = m_item; tmp; tmp = tmp->parent() )
        tmp->addMatch( queryMaker, model->levelCategory( tmp->level() - 1 ) );
    Collections::addTextualFilter( queryMaker, model->currentFilter() );
}

CollectionTreeItemModelBase*
CollectionViewItem::getModel()
{
    QSortFilterProxyModel *proxyModel = dynamic_cast<QSortFilterProxyModel*>( The::mainWindow()->collectionBrowser()->currentView()->model() );
    return dynamic_cast<CollectionTreeItemModelBase*>( proxyModel ? proxyModel->sourceModel() : nullptr );
}

///////////////////////////////////////////////////////////
// Selection
///////////////////////////////////////////////////////////

bool
Selection::singleCollection() const
{
    return CollectionTreeView::onlyOneCollection( m_indices );
}

QList<CollectionTreeItem*>
Selection::selectedItems()
{
    QList<CollectionTreeItem*> collectionItems;
    for( const QModelIndex &index : m_indices )
        collectionItems << static_cast<CollectionTreeItem*>( index.internalPointer() );
    return collectionItems;
}

Selection::Selection( const QModelIndexList &indices )
    : QObject( nullptr )
    , m_indices( indices )
{}

Collections::QueryMaker*
Selection::queryMaker()
{
    QList<CollectionTreeItem*> items=selectedItems();
    return The::mainWindow()->collectionBrowser()->currentView()->createMetaQueryFromItems( QSet<CollectionTreeItem*>(items.begin(), items.end()), true );
}
