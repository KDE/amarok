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

#include <q3ptrlist.h>
#include <Q3ValueList>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QKeyEvent>
#include <QPainter>
#include <QPaintEvent>
#include <QTextDocument>
#include <QToolTip>

#include <k3urldrag.h>
#include <kapplication.h>
#include <kguiitem.h>
#include <klocale.h>
#include <kpushbutton.h>
#include <kvbox.h>
#include <kwin.h>

#if 0
//////////////////////////////////////////////////////////////////////////////////////////
/// CLASS QueueItem
//////////////////////////////////////////////////////////////////////////////////////////
void
QueueItem::paintCell( QPainter *p, const QColorGroup &cg, int column, int width, int align )
{
    QListWidgetItem::paintCell( p, cg, column, width, align );

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
#endif

//////////////////////////////////////////////////////////////////////////////////////////
/// CLASS QueueList
//////////////////////////////////////////////////////////////////////////////////////////

QueueList::QueueList( QWidget *parent, const char *name )
            : QListWidget( parent )
{
    setObjectName( name );
    setResizeMode( QListWidget::Adjust );
    setSelectionMode( QAbstractItemView::ExtendedSelection );

    setAcceptDrops( true );
    setDragEnabled( true );
    //setDropVisualizer( true );    //the visualizer (a line marker) is drawn when dragging over tracks
    //setDropVisualizerWidth( 3 );
}

void
QueueList::paintEvent( QPaintEvent *e )
{
    if( e ) QListWidget::paintEvent( e );

    if( !count() && e )
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
        QTextDocument t;
        t.setHtml( minimumText );
        const uint w = t.size().width();
        const uint h = t.size().height();
        if ( w+30 >= viewport()->width() || h+30 >= viewport()->height() )
            //too big, giving up
            return;

        const uint x = (viewport()->width() - w - 30) / 2 ;
        const uint y = (viewport()->height() - h - 30) / 2 ;

        p.setBrush( colorGroup().background() );
        p.drawRoundRect( x, y, w+30, h+30, (8*200)/w, (8*200)/h );
       // t.draw( &p, x+15, y+15, QRect(), colorGroup() );
        t.drawContents( &p, QRect() );
    }
}

void
QueueList::keyPressEvent( QKeyEvent *e )
{
    switch( e->key() ) {

        case Qt::Key_Delete:    //remove
            removeSelected();
            break;

        case Qt::ControlModifier+Qt::Key_Up:
            moveSelectedUp();
            break;

        case Qt::ControlModifier+Qt::Key_Down:
            moveSelectedDown();
            break;
    }
}

bool
QueueList::hasSelection()
{
    return selectedItems().size() == 0;
}

void
QueueList::moveSelected( int direction )
{
    QList<QListWidgetItem *> selected = selectedItems();
    bool item_moved = false;

    // Whilst it would be substantially faster to do this: ((*it)->itemAbove())->move( *it ),
    // this would only work for sequentially ordered items
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

void
QueueList::removeSelected() //SLOT
{
    currentItem()->setSelected( true );

    bool item_removed = false;
    QList<QListWidgetItem* > selected = selectedItems();

    foreach( QListWidgetItem* item, selected )
    {
        delete item;
        item_removed = true;
    }

    if( count() == 0 )
        QueueManager::instance()->updateButtons();

    if( item_removed )
        emit changed();
}

void
QueueList::clear() // SLOT
{
    QListWidget::clear();
    emit changed();
}

void
QueueList::dragEnterEvent( QDragEnterEvent *e )
{
    debug() << "dragEnterEvent()" << endl;
    if(  e->source() == reinterpret_cast<K3ListView*>( Playlist::instance() )->viewport() )
        e->acceptProposedAction();
}

void
QueueList::dragMoveEvent( QDragMoveEvent *e )
{
    debug() << "dragMoveEvent()" << endl;
    QListWidget::dragMoveEvent( e );

    //Comment From 1.4: Must be overloaded for dnd to work
    if (( e->source() == reinterpret_cast<K3ListView*>( Playlist::instance() )->viewport() ) ||
                 e->source() == viewport() )
        e->acceptProposedAction();
}

void
QueueList::dropEvent( QDropEvent *e )
{
    debug() << "dragDropEvent()" << endl;
    if( e->source() == viewport() )
    {
        QListWidget::dropEvent( e );
        emit changed();
    }
    else
    {
        QListWidgetItem *after = itemAt( e->pos() );

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

    KVBox *mainBox = new KVBox( this );
    setMainWidget( mainBox );

    KHBox *box = new KHBox( mainWidget() );
    box->setSpacing( 5 );
    m_listview = new QueueList( box );

    KVBox *buttonBox = new KVBox( box );
    m_up     = new KPushButton( KGuiItem( QString::null, "up" ), buttonBox );
    m_down   = new KPushButton( KGuiItem( QString::null, "down" ), buttonBox );
    m_remove = new KPushButton( KGuiItem( QString::null, Amarok::icon( "dequeue_track" ) ), buttonBox );
    m_add    = new KPushButton( KGuiItem( QString::null, Amarok::icon( "queue_track" ) ), buttonBox );
    m_clear  = new KPushButton( KGuiItem( QString::null, Amarok::icon( "playlist_clear" ) ), buttonBox );

    m_up->setToolTip(     i18n( "Move up" ) );
    m_down->setToolTip(   i18n( "Move down" ) );
    m_remove->setToolTip( i18n( "Remove" ) );
    m_add->setToolTip(    i18n( "Enqueue track" ) );
    m_clear->setToolTip(  i18n( "Clear queue" ) );

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
    connect( pl,         SIGNAL( queueChanged(const QList<PlaylistItem*> &, const QList<PlaylistItem*> &) ),
                         SLOT( changeQueuedItems(const QList<PlaylistItem*> &, const QList<PlaylistItem*> &) ) );
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
QueueManager::addItems( QListWidgetItem *after )
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
        after = m_listview->item( m_listview->count() - 1 );

    QList<Q3ListViewItem*> list = Playlist::instance()->selectedItems();

    bool item_added = false;
    for( QList<Q3ListViewItem*>::const_iterator it = list.begin(); it != list.end(); ++it ) {
        #define item static_cast<PlaylistItem*>(*it)
        Q3ValueList<PlaylistItem*> current = m_map.values();

        if( current.find( item ) == current.end() ) //avoid duplication
        {
            QString title = i18n("%1 - %2", item->artist(), item->title() );

            QListWidgetItem* createdItem = new QueueItem( title );
            m_listview->insertItem( m_listview->row( after+1 ), createdItem );
            m_map[ createdItem ] = item;
            item_added = true;
        }
        #undef item
    }

    if( item_added )
        emit m_listview->changed();
}

void
QueueManager::changeQueuedItems( const QList<PlaylistItem*> &in, const QList<PlaylistItem*> &out ) //SLOT
{
    foreach( PlaylistItem* it, in ) 
        addQueuedItem( it );
    foreach( PlaylistItem* it, out )
        removeQueuedItem( it );
}

void
QueueManager::addQueuedItem( PlaylistItem *item )
{
    Playlist *pl = Playlist::instance();
    if( !pl ) return; //should never happen

    const int index = pl->m_nextTracks.indexOf( item );

    QListWidgetItem *after;
    if( !index ) after = 0;
    else
    {
        int find = m_listview->count();
        if( index - 1 <= find )
            find = index - 1;
        after = m_listview->item( find );
    }

    Q3ValueList<PlaylistItem*>         current = m_map.values();
    Q3ValueListIterator<PlaylistItem*> newItem = current.find( item );

    QString title = i18n("%1 - %2", item->artist(), item->title() );

    if( newItem == current.end() ) //avoid duplication
    {
        QueueItem* createdItem = new QueueItem( title );
        m_listview->insertItem( m_listview->row( after +1 ), createdItem );
        m_map[ createdItem ] = item;
    }
}

void
QueueManager::removeQueuedItem( PlaylistItem *item )
{
    Playlist *pl = Playlist::instance();
    if( !pl ) return; //should never happen

    const int index = pl->m_nextTracks.indexOf( item );

    QListWidgetItem *after;
    if( !index ) after = 0;
    else
    {
        int find = m_listview->count();
        if( index - 1 <= find )
            find = index - 1;
        after = m_listview->item( find );
    }

    Q3ValueList<PlaylistItem*>         current = m_map.values();
    Q3ValueListIterator<PlaylistItem*> newItem = current.find( item );

    QString title = i18n("%1 - %2", item->artist(), item->title() );

    QListWidgetItem *removableItem = m_listview->findItems( title, 0 ).first();

    if( removableItem )
    {
        //Remove the key from the map, so we can re-queue the item
        QMap<QListWidgetItem*, PlaylistItem*>::iterator end =  m_map.end();
        for( QMap<QListWidgetItem*, PlaylistItem*>::iterator it = m_map.begin(); it != end; ++it )
        {
            if( it.data() == item )
            {
                m_map.remove( it );

                //Remove the item from the queuelist
                m_listview->takeItem( m_listview->row( removableItem ) );
                delete removableItem;
                return;
            }
        }
    }
}

/// Playlist uses this to determine the altered queue and reflect the changes.

QList<PlaylistItem*>
QueueManager::newQueue()
{
    QList<PlaylistItem*> queue;
    for( int i = 0; i < m_listview->count(); ++i )
    {
        queue.append( m_map[ m_listview->item( i ) ] );
    }
    return queue;
}

void
QueueManager::insertItems()
{
    QList<PlaylistItem*> list = Playlist::instance()->m_nextTracks;
    //QListWidgetItem *last = 0;

    foreach( PlaylistItem *item, list )
    {
        QString title = i18n("%1 - %2", item->artist(), item->title() );

        QueueItem* createdItem = new QueueItem( title );
        debug() << "Inserting " << title << " at " << createdItem + 1 << endl;
        m_listview->addItem( createdItem );
        m_map[ createdItem ] = item;
      //  last = createdItem;
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
    QList<QListWidgetItem *> selected = m_listview->selectedItems();

    bool item_removed = false;

    foreach( QListWidgetItem *item, selected )
    {
        //Remove the key from the map, so we can re-queue the item
        QMap<QListWidgetItem*, PlaylistItem*>::iterator it = m_map.find( item );

        m_map.remove( it );

        //Remove the item from the queuelist
        m_listview->takeItem( m_listview->row( item ) );
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

