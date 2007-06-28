/***************************************************************************
 * copyright            : (C) 2007 Leo Franchi <lfranchi@gmail.com>        *
 **************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "ContextItemManagerWidget.h"

#include "amarok.h"
#include "amarokconfig.h"
#include "debug.h"
#include "ContextItem.h"
#include "ContextItemManager.h"

#include <QList>
//#include <QTextToolTip>
#include <QWidget>

#include <kapplication.h>
#include <kguiitem.h>
#include <klocale.h>
#include <klistwidget.h>
#include <kpushbutton.h>
#include <kvbox.h>
#include <kwindowsystem.h>

using namespace Context;


/////////////////////////////////////////////////////////////////////////
//// class ContextItemList
/////////////////////////////////////////////////////////////////////////

ContextItemList::ContextItemList( QWidget *parent, const char *name )
: KListWidget( parent )
{
    setObjectName( name );
    setResizeMode( QListWidget::Adjust );
    setSelectionMode( QAbstractItemView::ExtendedSelection );
    
    setAcceptDrops( false );
    setDragEnabled( false ); // for now

}

bool ContextItemList::hasSelection()
{
    return selectedItems().size() == 0;
}

void ContextItemList::moveSelected( int direction )
{
    QList<QListWidgetItem *> selected = selectedItems();
    bool item_moved = false;
    
    foreach( QListWidgetItem* it, selected )
    {
        int position = row( it );
        if( (direction < 0 && position == 0 ) || ( direction > 0 && position == count() - 1 ) )
            continue;
        
        insertItem( position + direction, takeItem( position ) );
        item_moved = true;
    }
    
    scrollToItem( selected.first() ); //apparently this deselects?
    foreach( QListWidgetItem* it, selected )
        it->setSelected( true );
    
    if( item_moved )
        emit changed();
}

//////////////////////////////////////////////////////////////////////////
//// CLASS ContextItemManagerWidget
//////////////////////////////////////////////////////////////////////////


// NOTE: some text should be added to say that changes do not take effect until next track change

ContextItemManagerWidget *ContextItemManagerWidget::s_instance = 0;

ContextItemManagerWidget::ContextItemManagerWidget( QWidget *parent, const char *name, QMap< ContextItem*, bool >* enabled, QStringList order )
: KDialog( parent )
, m_contextItems( enabled )
, m_itemsOrder( order )
{
    DEBUG_BLOCK
        
    // set up widget
    setObjectName( name );
    setModal( false );
    setButtons( Ok|Apply|Cancel );
    setDefaultButton( Ok );
    showButtonSeparator( true );
    
    // Gives the window a small title bar, and skips a taskbar entry
#ifdef Q_WS_X11
    KWindowSystem::setType( winId(), NET::Utility );
    KWindowSystem::setState( winId(), NET::SkipTaskbar );
#endif
    
    kapp->setTopWidget( this );
    setCaption( KDialog::makeStandardCaption( i18n("ContextItemManager") ) );
    setInitialSize( QSize( 400, 260 ) );
    
    KVBox *mainBox = new KVBox( this );
    setMainWidget( mainBox );
    
    KHBox *box = new KHBox( mainWidget() );
    box->setSpacing( 5 );
    m_listview = new ContextItemList( box );
    
    KVBox *buttons = new KVBox( box );
    m_up = new KPushButton( KGuiItem( QString(), "up" ), buttons );
    m_down   = new KPushButton( KGuiItem( QString(), "down" ), buttons );
    m_toggle = new KPushButton( KGuiItem( QString(), "toggle" ), buttons );
    
    m_up->setToolTip(     i18n( "Move Context Item up" ) );
    m_down->setToolTip(   i18n( "Move Context Item down" ) );
    m_toggle->setToolTip( i18n( "Enable Context Item" ) );
    
    connect( m_up,     SIGNAL( clicked() ), m_listview, SLOT( moveSelectedUp() ) );
    connect( m_down,   SIGNAL( clicked() ), m_listview, SLOT( moveSelectedDown() ) );
    connect( m_toggle, SIGNAL( clicked() ), this,       SLOT( toggleState() ) );
    //connect( m_listview, SIGNAL( selectionChanged() ),  this,  SLOT( updateButtons() ) );
    
    connect( this,       SIGNAL( applyClicked()), SLOT( applyNow() ) );
    connect( m_listview, SIGNAL( changed() ), this, SLOT ( changed() ) );
    
    connect( this, SIGNAL( okClicked() ), this, SLOT( applyNow() ) );
    
    enableButtonApply(false);
    
    insertItems();
    
}
    

// iterate through contents of m_contextItems and add them to the listview
void ContextItemManagerWidget::insertItems()
{
    DEBUG_BLOCK
    QList< ContextItem* > keys = m_contextItems->keys();
    foreach( QString item, m_itemsOrder )
    {
        debug() << "inserting context item into widget: " << item << "with value: " << findInContextMap( item ) << endl;
        QListWidgetItem* qitem = new QListWidgetItem( item, m_listview, QListWidgetItem::Type );
        findInContextMap( item ) ? qitem->setBackground( QBrush( Qt::white, Qt::SolidPattern  ) )
                                     : qitem->setBackground( QBrush( Qt::darkRed, Qt::SolidPattern  ) );
    }
}

void ContextItemManagerWidget::insertItem( const QString& name, bool enabled )
{
    QListWidgetItem* item = new QListWidgetItem( name, m_listview, QListWidgetItem::Type );
    enabled ? item->setBackground( QBrush( Qt::darkRed, Qt::SolidPattern  ) )
            : item->setBackground( QBrush( Qt::white, Qt::SolidPattern  ) );
}
    
void ContextItemManagerWidget::toggleState()
{
    DEBUG_BLOCK
    QList< QListWidgetItem* > selected = m_listview->selectedItems();
    
    foreach( QListWidgetItem* item, selected )
    {
        if( findInContextMap( item->text() ) == true ) // item is currently enabled, so we disable it
        {
            insertInContextMap( item->text(), false );
            item->setBackground( QBrush( Qt::darkRed, Qt::SolidPattern  ) );
        } else
        {
            insertInContextMap( item->text(), true );
            item->setBackground( QBrush( Qt::white, Qt::SolidPattern  ) );
        }
    }
    
    enableButtonApply( true );
}

bool ContextItemManagerWidget::findInContextMap( const QString name )
{
    QList< ContextItem* > keys = m_contextItems->keys();
    foreach( ContextItem* key, keys )
    {
        if( key->name() == name )
        {
            return m_contextItems->value( key );
        }
    }
    return false;
}

void ContextItemManagerWidget::insertInContextMap( const QString name, bool val )
{
    QList< ContextItem* > keys = m_contextItems->keys();
    foreach( ContextItem* key, keys )
    {
        if( key->name() == name )
        {
            debug() << "saving to m_contextItems: " << key->name() << ": " << val << endl;
            m_contextItems->insert( key, val );
        }
    }
}

// save ordering of listview into map
void ContextItemManagerWidget::changed()
{
    m_itemsOrder.clear();
    for( int i = 0; i < m_listview->count(); i++ )
        m_itemsOrder << m_listview->item( i )->text();
    
    enableButtonApply( true );
}

    void ContextItemManagerWidget::updateButtons()
{
    const bool emptyLV  = m_listview->isEmpty();
    const bool enableQL = m_listview->hasSelection() && !emptyLV;
    
    m_up->setEnabled( enableQL );
    m_down->setEnabled( enableQL );
    m_toggle->setEnabled( enableQL );
}

void ContextItemManagerWidget::applyNow()
{
    // first save the order
    m_itemsOrder.clear();
    for( int i = 0; i < m_listview->count(); i++ )
        m_itemsOrder << m_listview->item( i )->text();
    Amarok::config( "ContextItemManager" ).writeEntry( "Items", m_itemsOrder );
    // now save the states
    QList< ContextItem* > keys = m_contextItems->keys();
    foreach( ContextItem* item, keys )
        Amarok::config( "ContextItemManager" ).writeEntry( item->name(), m_contextItems->value( item ) );
    
    ContextItemManager::instance()->applyConfig();
    enableButtonApply( false );
}

#include "ContextItemManagerWidget.moc"
