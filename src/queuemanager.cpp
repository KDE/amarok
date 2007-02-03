/***************************************************************************
 * copyright            : (C) 2005 Seb Ruiz <me@sebruiz.net>               *
 **************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#define DEBUG_PREFIX "QueueManager"
#include "debug.h"

#include "amarok.h"
#include "amarokconfig.h"     //check if dynamic mode
#include "playlist.h"
#include "queuemanager.h"
//Added by qt3to4:
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QPaintEvent>
#include <Q3ValueList>
#include <QKeyEvent>
#include <QDragEnterEvent>

#include <kapplication.h>
#include <kguiitem.h>
#include <klocale.h>
#include <kpushbutton.h>
#include <kurldrag.h>
#include <kwin.h>

#include <QPainter>
#include <q3ptrlist.h>
#include <q3simplerichtext.h>
#include <QToolTip>
#include <q3vbox.h>

//////////////////////////////////////////////////////////////////////////////////////////
/// CLASS QueueItem
//////////////////////////////////////////////////////////////////////////////////////////
void
QueueItem::paintCell( QPainter *p, const QColorGroup &cg, int column, int width, int align )
{
    K3ListViewItem::paintCell( p, cg, column, width, align );

    QString str = QString::number( ( static_cast<K3ListView *>( listView() ) )->itemIndex( this ) + 1 );

    //draw the symbol's outline
          uint fw = p->fontMetrics().width( str ) + 2;
    const uint w  = 16; //keep this even
    const uint h  = height() - 2;

    p->setBrush( cg.highlight() );
    p->setPen( cg.highlight().dark() ); //TODO blend with background color
    p->drawEllipse( width - fw - w/2, 1, w, h );
    p->drawRect( width - fw, 1, fw, h );
    p->setPen( cg.highlight() );
    p->drawLine( width - fw, 2, width - fw, h - 1 );

    fw += 2; //add some more padding
    p->setPen( cg.highlightedText() );
    p->drawText( width - fw, 2, fw, h-1, Qt::AlignCenter, str );
}


//////////////////////////////////////////////////////////////////////////////////////////
/// CLASS QueueList
//////////////////////////////////////////////////////////////////////////////////////////

QueueList::QueueList( QWidget *parent, const char *name )
            : K3ListView( parent, name )
{
    addColumn( i18n("Name") );
    setResizeMode( Q3ListView::LastColumn );
    setSelectionMode( Q3ListView::Extended );
    setSorting( -1 );

    setAcceptDrops( true );
    setDragEnabled( true );
    setDropVisualizer( true );    //the visualizer (a line marker) is drawn when dragging over tracks
    setDropVisualizerWidth( 3 );
}

void
QueueList::viewportPaintEvent( QPaintEvent *e )
{
    if( e ) K3ListView::viewportPaintEvent( e );

    if( !childCount() && e )
    {
        QPainter p( viewport() );
        QString minimumText(i18n(
                "<div align=center>"
                "<h3>The Queue Manager</h3>"
                    "To create a queue, "
                    "<b>drag</b> tracks from the playlist, and "
                    "<b>drop</b> them here.<br><br>"
                    "Drag and drop tracks within the manager to resort queue orders."
                "</div>" ) );
        Q3SimpleRichText t( minimumText, QApplication::font() );

        if ( t.width()+30 >= viewport()->width() || t.height()+30 >= viewport()->height() )
            //too big, giving up
            return;

        const uint w = t.width();
        const uint h = t.height();
        const uint x = (viewport()->width() - w - 30) / 2 ;
        const uint y = (viewport()->height() - h - 30) / 2 ;

        p.setBrush( colorGroup().background() );
        p.drawRoundRect( x, y, w+30, h+30, (8*200)/w, (8*200)/h );
        t.draw( &p, x+15, y+15, QRect(), colorGroup() );
    }
}

void
QueueList::keyPressEvent( QKeyEvent *e )
{
    switch( e->key() ) {

        case Key_Delete:    //remove
            removeSelected();
            break;

        case CTRL+Key_Up:
            moveSelectedUp();
            break;

        case CTRL+Key_Down:
            moveSelectedDown();
            break;
    }
}

bool
QueueList::hasSelection()
{
    Q3ListViewItemIterator it( this, Q3ListViewItemIterator::Selected );

    if( !it.current() )
        return false;

    return true;
}

Q3PtrList<Q3ListViewItem>
QueueList::selectedItems()
{
    Q3PtrList<Q3ListViewItem> selected;
    Q3ListViewItemIterator it( this, Q3ListViewItemIterator::Selected );

    for( ; it.current(); ++it )
        selected.append( it.current() );

    return selected;
}

void
QueueList::moveSelectedUp() // SLOT
{
    Q3PtrList<Q3ListViewItem> selected = selectedItems();
    bool item_moved = false;

    // Whilst it would be substantially faster to do this: ((*it)->itemAbove())->move( *it ),
    // this would only work for sequentially ordered items
    for( Q3ListViewItem *item = selected.first(); item; item = selected.next() )
    {
        if( item == itemAtIndex(0) )
            continue;

        Q3ListViewItem *after;

        item == itemAtIndex(1) ?
            after = 0:
            after = ( item->itemAbove() )->itemAbove();

        moveItem( item, 0, after );
        item_moved = true;
    }

    ensureItemVisible( selected.first() );

    if( item_moved )
        emit changed();
}

void
QueueList::moveSelectedDown() // SLOT
{
    Q3PtrList<Q3ListViewItem> list = selectedItems();
    bool item_moved = false;

    for( Q3ListViewItem *item  = list.last(); item; item = list.prev() )
    {
        Q3ListViewItem *after = item->nextSibling();

        if( !after )
            continue;

        moveItem( item, 0, after );
        item_moved = true;
    }

    ensureItemVisible( list.last() );

    if( item_moved )
        emit changed();
}

void
QueueList::removeSelected() //SLOT
{
    setSelected( currentItem(), true );

    bool item_removed = false;
    Q3PtrList<Q3ListViewItem> selected = selectedItems();

    for( Q3ListViewItem *item = selected.first(); item; item = selected.next() )
    {
        delete item;
        item_removed = true;
    }

    if( isEmpty() )
        QueueManager::instance()->updateButtons();

    if( item_removed )
        emit changed();
}

void
QueueList::clear() // SLOT
{
    K3ListView::clear();
    emit changed();
}

void
QueueList::contentsDragEnterEvent( QDragEnterEvent *e )
{
    debug() << "contentsDrageEnterEvent()" << endl;
    e->accept( e->source() == reinterpret_cast<K3ListView*>( Playlist::instance() )->viewport() );
}

void
QueueList::contentsDragMoveEvent( QDragMoveEvent *e )
{
    debug() << "contentsDrageMoveEvent()" << endl;
    K3ListView::contentsDragMoveEvent( e );

    // Must be overloaded for dnd to work
    e->accept( ( e->source() == reinterpret_cast<K3ListView*>( Playlist::instance() )->viewport() ) ||
                 e->source() == viewport() );
}

void
QueueList::contentsDropEvent( QDropEvent *e )
{
    debug() << "contentsDragDropEvent()" << endl;
    if( e->source() == viewport() )
    {
        K3ListView::contentsDropEvent( e );
        emit changed();
    }
    else
    {
        Q3ListViewItem *parent = 0;
        Q3ListViewItem *after;

        findDrop( e->pos(), parent, after );

        QueueManager::instance()->addItems( after );
    }
}


//////////////////////////////////////////////////////////////////////////////////////////
/// CLASS QueueManager
//////////////////////////////////////////////////////////////////////////////////////////

QueueManager *QueueManager::s_instance = 0;

QueueManager::QueueManager( QWidget *parent, const char *name )
    : KDialog( parent )
{
    setModal( false );
    setButtons( Ok|Apply|Cancel );
    setDefaultButton( Ok );
    showButtonSeparator( true );

    s_instance = this;

    // Gives the window a small title bar, and skips a taskbar entry
    KWin::setType( winId(), NET::Utility );
    KWin::setState( winId(), NET::SkipTaskbar );

    kapp->setTopWidget( this );
    setCaption( KDialog::makeStandardCaption( i18n("Queue Manager") ) );
    setInitialSize( QSize( 400, 260 ) );

    Q3VBox *mainBox = new Q3VBox( this );
    setMainWidget( mainBox );

    Q3HBox *box = new Q3HBox( mainWidget() );
    box->setSpacing( 5 );
    m_listview = new QueueList( box );

    Q3VBox *buttonBox = new Q3VBox( box );
    m_up     = new KPushButton( KGuiItem( QString::null, "up" ), buttonBox );
    m_down   = new KPushButton( KGuiItem( QString::null, "down" ), buttonBox );
    m_remove = new KPushButton( KGuiItem( QString::null, Amarok::icon( "dequeue_track" ) ), buttonBox );
    m_add    = new KPushButton( KGuiItem( QString::null, Amarok::icon( "queue_track" ) ), buttonBox );
    m_clear  = new KPushButton( KGuiItem( QString::null, Amarok::icon( "playlist_clear" ) ), buttonBox );

    QToolTip::add( m_up,     i18n( "Move up" ) );
    QToolTip::add( m_down,   i18n( "Move down" ) );
    QToolTip::add( m_remove, i18n( "Remove" ) );
    QToolTip::add( m_add,    i18n( "Enqueue track" ) );
    QToolTip::add( m_clear,  i18n( "Clear queue" ) );

    m_up->setEnabled( false );
    m_down->setEnabled( false );
    m_remove->setEnabled( false );
    m_add->setEnabled( false );
    m_clear->setEnabled( false );

    connect( m_up,     SIGNAL( clicked() ), m_listview, SLOT( moveSelectedUp() ) );
    connect( m_down,   SIGNAL( clicked() ), m_listview, SLOT( moveSelectedDown() ) );
    connect( m_remove, SIGNAL( clicked() ), this,       SLOT( removeSelected() ) );
    connect( m_add,    SIGNAL( clicked() ), this,       SLOT( addItems() ) );
    connect( m_clear,  SIGNAL( clicked() ), m_listview, SLOT( clear() ) );

    Playlist *pl = Playlist::instance();
    connect( pl,         SIGNAL( selectionChanged() ),    SLOT( updateButtons() ) );
    connect( m_listview, SIGNAL( selectionChanged() ),    SLOT( updateButtons() ) );
    connect( pl,         SIGNAL( queueChanged(const PLItemList &, const PLItemList &) ),
                         SLOT( changeQueuedItems(const PLItemList &, const PLItemList &) ) );
    connect( this,       SIGNAL( applyClicked()), SLOT( applyNow() ) );
    connect( m_listview, SIGNAL( changed() ), this, SLOT ( changed() ) );
    s_instance->enableButtonApply(false);

    insertItems();
}

QueueManager::~QueueManager()
{
    s_instance = 0;
}

void
QueueManager::applyNow()
{
    Playlist *pl = Playlist::instance();
    pl->changeFromQueueManager( newQueue() );
    s_instance->enableButtonApply(false);
}

void
QueueManager::addItems( Q3ListViewItem *after )
{
    /*
        HACK!!!!! We can know which items where dragged since they should still be selected
        I do this, because:
        - Dragging items from the playlist provides urls
        - Providing urls, requires iterating through the entire list in order to find which
          item was selected.  Possibly a very expensive task - worst case: O(n)
        - After a drag, those items are still selected in the playlist, so we can find out
          which PlaylistItems were dragged by selectedItems();
    */
    if( !after )
        after = m_listview->lastChild();

    Q3PtrList<Q3ListViewItem> list = Playlist::instance()->selectedItems();

    bool item_added = false;
    for( Q3ListViewItem *item = list.first(); item; item = list.next() )
    {
        #define item static_cast<PlaylistItem*>(item)
        Q3ValueList<PlaylistItem*> current = m_map.values();

        if( current.find( item ) == current.end() ) //avoid duplication
        {
            QString title = i18n("%1 - %2").arg( item->artist(), item->title() );

            after = new QueueItem( m_listview, after, title );
            m_map[ after ] = item;
            item_added = true;
        }
        #undef item
    }

    if( item_added )
        emit m_listview->changed();
}

void
QueueManager::changeQueuedItems( const PLItemList &in, const PLItemList &out ) //SLOT
{
    Q3PtrListIterator<PlaylistItem> it(in);
    for( it.toFirst(); it; ++it ) addQueuedItem( *it );
    it = Q3PtrListIterator<PlaylistItem>(out);
    for( it.toFirst(); it; ++it ) removeQueuedItem( *it );
}

void
QueueManager::addQueuedItem( PlaylistItem *item )
{
    Playlist *pl = Playlist::instance();
    if( !pl ) return; //should never happen

    const int index = pl->m_nextTracks.findRef( item );

    Q3ListViewItem *after;
    if( !index ) after = 0;
    else
    {
        int find = m_listview->childCount();
        if( index - 1 <= find )
            find = index - 1;
        after = m_listview->itemAtIndex( find );
    }

    Q3ValueList<PlaylistItem*>         current = m_map.values();
    Q3ValueListIterator<PlaylistItem*> newItem = current.find( item );

    QString title = i18n("%1 - %2").arg( item->artist(), item->title() );

    if( newItem == current.end() ) //avoid duplication
    {
        after = new QueueItem( m_listview, after, title );
        m_map[ after ] = item;
    }
}

void
QueueManager::removeQueuedItem( PlaylistItem *item )
{
    Playlist *pl = Playlist::instance();
    if( !pl ) return; //should never happen

    const int index = pl->m_nextTracks.findRef( item );

    Q3ListViewItem *after;
    if( !index ) after = 0;
    else
    {
        int find = m_listview->childCount();
        if( index - 1 <= find )
            find = index - 1;
        after = m_listview->itemAtIndex( find );
    }

    Q3ValueList<PlaylistItem*>         current = m_map.values();
    Q3ValueListIterator<PlaylistItem*> newItem = current.find( item );

    QString title = i18n("%1 - %2").arg( item->artist(), item->title() );

    Q3ListViewItem *removableItem = m_listview->findItem( title, 0 );

    if( removableItem )
    {
        //Remove the key from the map, so we can re-queue the item
        QMapIterator<Q3ListViewItem*, PlaylistItem*> end(  m_map.end() );
        for( QMapIterator<Q3ListViewItem*, PlaylistItem*> it = m_map.begin(); it != end; ++it )
        {
            if( it.data() == item )
            {
                m_map.remove( it );

                //Remove the item from the queuelist
                m_listview->takeItem( removableItem );
                delete removableItem;
                return;
            }
        }
    }
}

/// Playlist uses this to determine the altered queue and reflect the changes.

Q3PtrList<PlaylistItem>
QueueManager::newQueue()
{
    Q3PtrList<PlaylistItem> queue;
    for( Q3ListViewItem *key = m_listview->firstChild(); key; key = key->nextSibling() )
    {
        queue.append( m_map[ key ] );
    }
    return queue;
}

void
QueueManager::insertItems()
{
    Q3PtrList<PlaylistItem> list = Playlist::instance()->m_nextTracks;
    Q3ListViewItem *last = 0;

    for( PlaylistItem *item = list.first(); item; item = list.next() )
    {
        QString title = i18n("%1 - %2").arg( item->artist(), item->title() );

        last = new QueueItem( m_listview, last, title );
        m_map[ last ] = item;
    }

    updateButtons();
}

void
QueueManager::changed() // SLOT
{
  s_instance->enableButtonApply(true);
}


void
QueueManager::removeSelected() //SLOT
{
    Q3PtrList<Q3ListViewItem>  selected = m_listview->selectedItems();

    bool item_removed = false;

    for( Q3ListViewItem *item = selected.first(); item; item = selected.next() )
    {
        //Remove the key from the map, so we can re-queue the item
        QMapIterator<Q3ListViewItem*, PlaylistItem*> it = m_map.find( item );

        m_map.remove( it );

        //Remove the item from the queuelist
        m_listview->takeItem( item );
        delete item;
        item_removed = true;
    }

    if( item_removed )
        emit m_listview->changed();
}

void
QueueManager::updateButtons() //SLOT
{
    const bool enablePL = !Playlist::instance()->selectedItems().isEmpty();
    const bool emptyLV  = m_listview->isEmpty();
    const bool enableQL = m_listview->hasSelection() && !emptyLV;

    m_up->setEnabled( enableQL );
    m_down->setEnabled( enableQL );
    m_remove->setEnabled( enableQL );
    m_add->setEnabled( enablePL );
    m_clear->setEnabled( !emptyLV );
}

#include "queuemanager.moc"

