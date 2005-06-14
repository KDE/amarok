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

#include "debug.h"
#include "queuemanager.h"

#include <kapplication.h>
#include <kguiitem.h>
#include <klocale.h>
#include <kurldrag.h>

#include <qpainter.h>
#include <qptrlist.h>
#include <qsimplerichtext.h>
#include <qvbox.h>

//////////////////////////////////////////////////////////////////////////////////////////
/// CLASS QueueList
//////////////////////////////////////////////////////////////////////////////////////////

QueueList::QueueList( QWidget *parent, const char *name )
            : KListView( parent, name )
{
    addColumn( i18n("Name") );
    setResizeMode( QListView::LastColumn );
    setSelectionMode( QListView::Extended );
    setSorting( -1 );

    setAcceptDrops( true );
    setDragEnabled( true );
    setDropVisualizer( true );    //the visualizer (a line marker) is drawn when dragging over tracks
    setDropVisualizerWidth( 3 );
}

void
QueueList::viewportPaintEvent( QPaintEvent *e )
{
    if( e ) KListView::viewportPaintEvent( e );

    if( !childCount() && e )
    {
        QPainter p( viewport() );
        QString minimumText(i18n(
                "<div align=center>"
                "<h3>The Queue Manager</h3>"
                    "<br>To create a queue, "
                    "<b>drag</b> tracks from the playlist window, "
                    "<b>drop</b> them here to queue them.<br>"
                    "Drag and drop tracks within the manager to resort queue orders."
                "</div>" ) );
        QSimpleRichText *t = new QSimpleRichText( minimumText, QApplication::font() );

        if ( t->width()+30 >= viewport()->width() || t->height()+30 >= viewport()->height() ) {
            // too big for the window, so let's cut part of the text
            delete t;
            t = new QSimpleRichText( minimumText, QApplication::font());
            if ( t->width()+30 >= viewport()->width() || t->height()+30 >= viewport()->height() ) {
                //still too big, giving up
                return;
            }
        }

        const uint w = t->width();
        const uint h = t->height();
        const uint x = (viewport()->width() - w - 30) / 2 ;
        const uint y = (viewport()->height() - h - 30) / 2 ;

        p.setBrush( colorGroup().background() );
        p.drawRoundRect( x, y, w+30, h+30, (8*200)/w, (8*200)/h );
        t->draw( &p, x+15, y+15, QRect(), colorGroup() );
        delete t;
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

void
QueueList::removeSelected()
{
    setSelected( currentItem(), true );

    QPtrList<QListViewItem> selected;
    QListViewItemIterator it( this, QListViewItemIterator::Selected);

    for( ; it.current(); ++it )
        selected.append( it.current() );

    for( QListViewItem *item = selected.first(); item; item = selected.next() )
        delete item;
}

bool
QueueList::hasSelection()
{
    QListViewItemIterator it( this, QListViewItemIterator::Selected);

    if( !it.current() )
        return false;

    return true;
}

void
QueueList::moveSelectedUp() // SLOT
{
    QPtrList<QListViewItem> selected;
    QListViewItemIterator it( this, QListViewItemIterator::Selected);

    for( ; it.current(); ++it )
        selected.append( it.current() );

    // Whilst it would be substantially faster to do this: ((*it)->itemAbove())->move( *it ),
    // this would only work for sequentially ordered items
    for( QListViewItem *item = selected.first(); item; item = selected.next() )
    {
        if( item == itemAtIndex(0) )
            continue;

        QListViewItem *after;

        item == itemAtIndex(1) ?
            after = 0:
            after = ( item->itemAbove() )->itemAbove();

        moveItem( item, 0, after );
    }
}

void
QueueList::moveSelectedDown() // SLOT
{
    QListViewItemIterator it( this, QListViewItemIterator::Selected);

    QPtrList<QListViewItem> list;

    for( ; it.current(); ++it )
        list.append( *it );

    for( QListViewItem *item  = list.last(); item; item = list.prev() )
    {
        QListViewItem *after = item->nextSibling();

        if( !after )
            continue;

        moveItem( item, 0, after );
    }
}

void
QueueList::contentsDragEnterEvent( QDragEnterEvent *e )
{
    e->accept( e->source() != viewport() && KURLDrag::canDecode( e ) );
}

void
QueueList::contentsDropEvent( QDropEvent *e )
{
    if( e->source() == viewport() )
        KListView::contentsDropEvent( e );

//     else
//     {
//         KURL::List list;
//         QMap<QString, QString> map;
//         if( KURLDrag::decode( e, list, map ) ) {
//             if( parent ) {
//                 //insert the dropped tracks
//                 PlaylistEntry *playlist = (PlaylistEntry *)parent;
//                 playlist->insertTracks( after, list, map );
//             }
//             else //dropped on a playlist item
//             {
//                 PlaylistEntry *playlist = (PlaylistEntry *)item;
//                 //append the dropped tracks
//                 playlist->insertTracks( 0, list, map );
//             }
//         }
//         else
//             e->ignore();
//     }

}

//////////////////////////////////////////////////////////////////////////////////////////
/// CLASS QueueManager
//////////////////////////////////////////////////////////////////////////////////////////

QueueManager::QueueManager( QWidget *parent, const char *name )
                    : KDialogBase( parent, name, false, i18n("Queue Manager"), Ok|Cancel )
{
    setWFlags( WX11BypassWM | WStyle_StaysOnTop );

    makeVBoxMainWidget();

    QHBox *box = new QHBox( mainWidget() );
    box->setSpacing( 5 );
    m_listview = new QueueList( box );

    QVBox *buttonBox = new QVBox( box );
    m_up     = new KPushButton( KGuiItem( QString::null, "up"), buttonBox );
    m_down   = new KPushButton( KGuiItem( QString::null, "down"), buttonBox  );
    m_remove = new KPushButton( KGuiItem( QString::null, "edittrash"), buttonBox );

    m_up->setEnabled( false );
    m_down->setEnabled( false );
    m_remove->setEnabled( false );

    connect( m_up,     SIGNAL( clicked() ), m_listview, SLOT( moveSelectedUp() ) );
    connect( m_down,   SIGNAL( clicked() ), m_listview, SLOT( moveSelectedDown() ) );
    connect( m_remove, SIGNAL( clicked() ), m_listview, SLOT( removeSelected() ) );

    connect( m_listview, SIGNAL( selectionChanged() ),  SLOT( updateButtons()  ) );

    insertItems();

    show();
}

QPtrList<PlaylistItem>
QueueManager::newQueue()
{
    QPtrList<PlaylistItem> queue;
    for( QListViewItem *key = m_listview->firstChild(); key; key = key->nextSibling() )
    {
        queue.append( m_map[ key ] );
    }
    return queue;
}

void
QueueManager::insertItems()
{
    QPtrList<PlaylistItem> list = Playlist::instance()->m_nextTracks;
    QListViewItem *last = 0;

    for( PlaylistItem *item = list.first(); item; item = list.next() )
    {
        QString title = item->artist();
        title.append( i18n(" - " ) );
        title.append( item->title() );

        last = new QListViewItem( m_listview, last, title );
        m_map[ last ] = item;
    }
}

void
QueueManager::updateButtons() //SLOT
{
    bool enable = false;
    if( m_listview->hasSelection() ) enable = true;

    m_up->setEnabled( enable );
    m_down->setEnabled( enable );
    m_remove->setEnabled( enable );
}

#include "queuemanager.moc"

