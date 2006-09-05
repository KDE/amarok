/***************************************************************************
 *   Copyright (C) 2005 by Gábor Lehel <illissius@gmail.com>               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Steet, Fifth Floor, Boston, MA  02111-1307, USA.          *
 ***************************************************************************/

#include "amarok.h"
#include "collectiondb.h"
#include "metabundle.h"
#include "playlist.h"
#include "playlistitem.h"
#include "queueLabel.h"
#include "statusbar.h"

#include <qapplication.h>
#include <qlabel.h>
#include <qpainter.h>
#include <qpixmap.h>
#include <qtimer.h>

#include <kactioncollection.h>
#include <kglobal.h>
#include <kiconloader.h>
#include <kpopupmenu.h>
#include <kstringhandler.h>

static const uint MAX_TO_SHOW = 20;

QueueLabel::QueueLabel( QWidget *parent, const char *name )
    : QLabel( parent, name )
    , m_timer( this )
    , m_tooltip( 0 )
    , m_tooltipShowing( false )
    , m_tooltipHidden( false )
{
    connect( this,                 SIGNAL( queueChanged( const PLItemList &, const PLItemList & ) ),
             Playlist::instance(), SIGNAL( queueChanged( const PLItemList &, const PLItemList & ) ) );

    connect( CollectionDB::instance(), SIGNAL( coverChanged( const QString &, const QString & ) ),
             this, SLOT( slotCoverChanged( const QString &, const QString & ) ) );

    setNum( 0 );
}


void QueueLabel::update() //SLOT
{
    PLItemList &queue = Playlist::instance()->m_nextTracks;
    setNum( queue.count() );
    if( isVisible() )
        getCover( queue.getFirst()->artist(), queue.getFirst()->album() );
}

void QueueLabel::slotCoverChanged( const QString &artist, const QString &album ) //SLOT
{
    PLItemList &queue = Playlist::instance()->m_nextTracks;
    if( isVisible() && queue.getFirst()->artist().string() == artist && queue.getFirst()->album().string() == album )
        getCover( artist, album );
}

void QueueLabel::getCover( const QString &artist, const QString &album )
{
    m_cover = CollectionDB::instance()->albumImage( artist, album, 50 );
    if( m_cover == CollectionDB::instance()->notAvailCover( 50 ) )
        m_cover = KGlobal::iconLoader()->iconPath( "goto", -KIcon::SizeHuge );
}

void QueueLabel::setNum( int num )
{
    if( num <= 0 )
    {
        clear();
        hide();
    }
    else
    {
        show();

        const QString text = QString::number( num );
        const int h = 18;
        QFont f = font();
        f.setPixelSize( h - 2 );
        f.setBold( true );
        const int w = kMax( h, QFontMetrics( f ).width( text ) + h/4 + 2 );

        QPixmap pix( w, h );
        QPainter p( &pix );

        p.setBrush( colorGroup().background() );
        p.setPen( colorGroup().background() );
        p.drawRect( pix.rect() );

        p.setBrush( colorGroup().highlight() );
        p.setPen( colorGroup().highlight().dark() );
        if( w > h )
        {
            p.drawPie( 0, 0, h, h, 90*16, 180*16 );
            p.drawPie( w-1 -h, 0, h, h, -90*16, 180*16 );
            p.drawLine( h/2-1, 0, w-1 - h/2, 0 );
            p.drawLine( h/2-1, h-1, w-1 - h/2, h-1 );
            p.setPen( colorGroup().highlight() );
            p.drawRect( h/2-1, 1, w - h + 1, h-2 );
        }
        else
            p.drawEllipse( pix.rect() );

        p.setFont( f );
        p.setPen( colorGroup().highlightedText() );
        p.setBrush( colorGroup().highlight().dark() );
        p.drawText( pix.rect(), Qt::AlignCenter | Qt::SingleLine, text );

        p.end();
        setPixmap( pix );
    }
}

void QueueLabel::enterEvent( QEvent* )
{
    m_tooltipHidden = false;
    QTimer::singleShot( 1000, this, SLOT(aboutToShow()) );
}

void QueueLabel::leaveEvent( QEvent* )
{
    hideToolTip();
}


void QueueLabel::aboutToShow()
{
    if( hasMouse() && !m_tooltipHidden )
        showToolTip();
}

void QueueLabel::mousePressEvent( QMouseEvent* mouseEvent )
{
    hideToolTip();

    if( m_timer.isActive() )  // if the user clicks again when (right after) the menu is open,
    {                         // (s)he probably wants to close it
        m_timer.stop();
        return;
    }

    Playlist *pl = Playlist::instance();
    PLItemList &queue = pl->m_nextTracks;
    if( queue.isEmpty() )
        return;

    int length = 0;
    for( QPtrListIterator<PlaylistItem> it( queue ); *it; ++it )
    {
        const int s = (*it)->length();
        if( s > 0 ) length += s;
    }

    QPtrList<KPopupMenu> menus;
    menus.setAutoDelete( true );
    KPopupMenu *menu = new KPopupMenu;
    menus.append( menu );

    const uint count = queue.count();
    if( length )
        menu->insertTitle( i18n( "1 Queued Track (%1)", "%n Queued Tracks (%1)", count )
                           .arg( MetaBundle::prettyLength( length, true ) ) );
    else
        menu->insertTitle( i18n( "1 Queued Track", "%n Queued Tracks", count ) );
    Amarok::actionCollection()->action( "queue_manager" )->plug( menu );
    menu->insertItem( SmallIconSet( Amarok::icon( "rewind" ) ),
                      count > 1 ? i18n( "&Dequeue All Tracks" ) : i18n( "&Dequeue Track" ), 0 );
    menu->insertSeparator();

    uint i = 1;
    QPtrListIterator<PlaylistItem> it( queue );
    it.toFirst();

    while( i <= count )
    {
        for( uint n = kMin( i + MAX_TO_SHOW - 1, count ); i <= n; ++i, ++it )
            menu->insertItem(
                KStringHandler::rsqueeze( i18n( "%1. %2" ).arg( i ).arg( veryNiceTitle( *it ) ), 50 ), i );

        if( i < count )
        {
            menus.append( new KPopupMenu );
            menu->insertSeparator();
            menu->insertItem( i18n( "1 More Track", "%n More Tracks", count - i + 1 ), menus.getLast() );
            menu = menus.getLast();
        }
    }

    menu = menus.getFirst();

    int mx, my;
    const int   mw      = menu->sizeHint().width(),
                mh      = menu->sizeHint().height(),
                sy      = mapFrom( Amarok::StatusBar::instance(), QPoint( 0, 0 ) ).y(),
                sheight = Amarok::StatusBar::instance()->height();
    const QRect dr      = QApplication::desktop()->availableGeometry( this );

    if( mapYToGlobal( sy ) - mh > dr.y() )
       my = mapYToGlobal( sy ) - mh;
    else if( mapYToGlobal( sy + sheight ) + mh < dr.y() + dr.height() )
       my = mapYToGlobal( sy + sheight );
    else
       my = mapToGlobal( mouseEvent->pos() ).y();

    mx = mapXToGlobal( 0 ) - ( mw - width() ) / 2;

    int id = menu->exec( QPoint( mx, my ) );
    if( id < 0 )
        m_timer.start( 50, true );
    else if( id == 0 ) //dequeue
    {
        const PLItemList dequeued = queue;
        while( !queue.isEmpty() )
            pl->queue( queue.getLast(), true );
        emit queueChanged( PLItemList(), dequeued );
    }
    else
    {
        PlaylistItem *selected = queue.at( id - 1 );
        if( selected )
            pl->ensureItemCentered( selected );
    }
}

void QueueLabel::showToolTip()
{
    if( m_tooltipShowing )
        return;

    m_tooltipShowing = true;

    Playlist     *pl    = Playlist::instance();
    const uint    count = pl->m_nextTracks.count();
    PlaylistItem *item  = pl->m_nextTracks.getFirst();

    if( !item )
        return;

    QString text;

    if( count > 1 )
    {
        int length = 0;
        for( QPtrListIterator<PlaylistItem> it( pl->m_nextTracks ); *it; ++it )
        {
            const int s = (*it)->length();
            if( s > 0 ) length += s;
        }
        if( length )
            text += QString("<center>%1</center>")
                    .arg( i18n( "1 track (%1)", "%n tracks (%1)", count )
                          .arg( MetaBundle::prettyLength( length, true ) ) );
    }

    text += i18n( "Next: %1" ).arg( veryNiceTitle( item, true /*bold*/ ) );

    m_tooltip = new KDE::PopupMessage( parentWidget()->parentWidget(), this, 0 );
    m_tooltip->setShowCloseButton( false );
    m_tooltip->setShowCounter( false );
    m_tooltip->setMaskEffect( KDE::PopupMessage::Plain );
    m_tooltip->setText( text );
    m_tooltip->setImage( m_cover );

    m_tooltip->reposition(); //make sure it is in the correct location

    m_tooltip->display();
}

void QueueLabel::hideToolTip()
{
    if( m_tooltip && m_tooltipShowing )
        m_tooltip->close();

    m_tooltipHidden = true;
    m_tooltipShowing = false;
}

QString QueueLabel::veryNiceTitle( PlaylistItem* item, bool bold ) const
{
    const QString artist = item->artist()->stripWhiteSpace(),
                  title =  item->title().stripWhiteSpace();
    if( !artist.isEmpty() && !title.isEmpty() )
       return ( bold ? i18n( "<b>%1</b> by <b>%2</b>" ) : i18n( "%1 by %2" ) ).arg( title ).arg( artist );
    else
       return QString( "<b>%1</b>" ).arg( MetaBundle::prettyTitle( item->filename() ) );
}


#include "queueLabel.moc"
