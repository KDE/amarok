/***************************************************************************
                       kmultitabbar.cpp -  description
                           -------------------
   begin                :  2001
   copyright            : (C) 2001,2002,2003 by Joseph Wenninger <jowenn@kde.org>
                          (C) 2005           by Mark Kretschmann <markey@web.de>
***************************************************************************/

/***************************************************************************
    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
    Boston, MA 02111-1307, USA.
***************************************************************************/

#include "debug.h"
#include "multitabbar.h"
#include "multitabbar.moc"
#include "multitabbar_p.h"
#include "multitabbar_p.moc"

#include <math.h>

#include <qbutton.h>
#include <qfontmetrics.h>
#include <qlayout.h>
#include <qpainter.h>
#include <qpopupmenu.h>
#include <qstyle.h>
#include <qtimer.h>

#include <kapplication.h>
#include <kconfig.h>
#include <kdebug.h>
#include <kiconeffect.h>
#include <kiconloader.h>
#include <klocale.h>
#include <kpopupmenu.h>
#include <kstringhandler.h>

#define NEARBYINT(i) ((int(float(i) + 0.5)))

namespace Amarok { extern KConfig *config( const QString& ); }

class MultiTabBarTabPrivate
{
    public:
        QPixmap pix;
};

class MultiTabBarButtonPrivate
{
    public:
        MultiTabBarButtonPrivate() : finalDropTarget( 0 ) {}
        DropProxyTarget *finalDropTarget;
};


MultiTabBarInternal::MultiTabBarInternal( QWidget *parent, MultiTabBar::MultiTabBarMode bm ) : QScrollView( parent )
{
    m_expandedTabSize = -1;
    m_showActiveTabTexts = false;
    m_tabs.setAutoDelete( true );
    m_barMode = bm;
    setHScrollBarMode( AlwaysOff );
    setVScrollBarMode( AlwaysOff );
    if ( bm == MultiTabBar::Vertical ) {
        box = new QWidget( viewport() );
        mainLayout = new QVBoxLayout( box );
        mainLayout->setAutoAdd( true );
        box->setFixedWidth( 24 );
        setFixedWidth( 24 );
    } else {
        box = new QWidget( viewport() );
        mainLayout = new QHBoxLayout( box );
        mainLayout->setAutoAdd( true );
        box->setFixedHeight( 24 );
        setFixedHeight( 24 );
    }
    addChild( box );
    setFrameStyle( NoFrame );
    viewport() ->setBackgroundMode( Qt::PaletteBackground );
    /*  box->setPaletteBackgroundColor(Qt::red);
        setPaletteBackgroundColor(Qt::green);*/
}

void MultiTabBarInternal::setStyle( enum MultiTabBar::MultiTabBarStyle style )
{
    m_style = style;
    for ( uint i = 0;i < m_tabs.count();i++ )
        m_tabs.at( i ) ->setStyle( m_style );

    if ( ( m_style == MultiTabBar::KDEV3 ) ||
            ( m_style == MultiTabBar::KDEV3ICON ) ||
            ( m_style == MultiTabBar::AMAROK ) ) {
        delete mainLayout;
        mainLayout = 0;
        resizeEvent( 0 );
    } else if ( mainLayout == 0 ) {
        if ( m_barMode == MultiTabBar::Vertical ) {
            box = new QWidget( viewport() );
            mainLayout = new QVBoxLayout( box );
            box->setFixedWidth( 24 );
            setFixedWidth( 24 );
        } else {
            box = new QWidget( viewport() );
            mainLayout = new QHBoxLayout( box );
            box->setFixedHeight( 24 );
            setFixedHeight( 24 );
        }
        addChild( box );
        for ( uint i = 0;i < m_tabs.count();i++ )
            mainLayout->add( m_tabs.at( i ) );
        mainLayout->setAutoAdd( true );

    }
    viewport() ->repaint();
}

void MultiTabBarInternal::drawContents ( QPainter * paint, int clipx, int clipy, int clipw, int cliph )
{
    QScrollView::drawContents ( paint , clipx, clipy, clipw, cliph );

    if ( m_position == MultiTabBar::Right ) {

        paint->setPen( colorGroup().shadow() );
        paint->drawLine( 0, 0, 0, viewport() ->height() );
        paint->setPen( colorGroup().background().dark( 120 ) );
        paint->drawLine( 1, 0, 1, viewport() ->height() );


    } else
        if ( m_position == MultiTabBar::Left ) {
            paint->setPen( colorGroup().light() );
            paint->drawLine( 23, 0, 23, viewport() ->height() );
            paint->drawLine( 22, 0, 22, viewport() ->height() );

            paint->setPen( colorGroup().shadow() );
            paint->drawLine( 0, 0, 0, viewport() ->height() );
        } else
            if ( m_position == MultiTabBar::Bottom ) {
                paint->setPen( colorGroup().shadow() );
                paint->drawLine( 0, 0, viewport() ->width(), 0 );
                paint->setPen( colorGroup().background().dark( 120 ) );
                paint->drawLine( 0, 1, viewport() ->width(), 1 );
            } else {
                paint->setPen( colorGroup().light() );
                paint->drawLine( 0, 23, viewport() ->width(), 23 );
                paint->drawLine( 0, 22, viewport() ->width(), 22 );

                /*                paint->setPen(colorGroup().shadow());
                                paint->drawLine(0,0,0,viewport()->height());*/
            }
}

void MultiTabBarInternal::contentsMousePressEvent( QMouseEvent *ev )
{
    ev->ignore();
}


void MultiTabBarInternal::showTabSelectionMenu(QPoint pos)
{

    KPopupMenu popup;
    popup.insertTitle(  i18n("Browsers") , /*id*/ -1, /*index*/ 1 );
    popup.setCheckable( true );
    for( uint i = 0; i < m_tabs.count(); i++ ) {
        MultiTabBarTab* tab = m_tabs.at( i );
        popup.insertItem( tab->text(), i );
        popup.setItemChecked(i, tab->visible() ? true : false);
    }

    int col = popup.exec(pos);
    if ( col >= 0 )
        setTabVisible( col, !popup.isItemChecked(col) );

}

void MultiTabBarInternal::mousePressEvent( QMouseEvent *ev )
{
    if ( ev->button() != QMouseEvent::RightButton ){
        ev->ignore();
        return;
    }

    // right button pressed
    showTabSelectionMenu(ev->globalPos());

}


#define CALCDIFF(m_tabs,diff,i) if (m_lines>(int)lines) {\
                    /*kdDebug()<<"i="<<i<<" visibleTabCount="<<visibleTabCount()<<" space="<<space<<endl;*/ \
                    uint ulen=0;\
                    diff=0; \
                    for (uint i2=i;i2<visibleTabCount();i2++) {\
                        uint l1=sizePerTab();\
                        if ((ulen+l1)>space){\
                            if (ulen==0) diff=0;\
                            else diff=((float)(space-ulen))/(i2-i);\
                            break;\
                        }\
                        ulen+=l1;\
                    }\
                } else {diff=0; }


void MultiTabBarInternal::resizeEvent( QResizeEvent *ev )
{
    /*  kdDebug()<<"MultiTabBarInternal::resizeEvent"<<endl;
        kdDebug()<<"MultiTabBarInternal::resizeEvent - box geometry"<<box->geometry()<<endl;
        kdDebug()<<"MultiTabBarInternal::resizeEvent - geometry"<<geometry()<<endl;*/

    if ( ev ) QScrollView::resizeEvent( ev );

    if ( ( m_style == MultiTabBar::KDEV3 ) ||
            ( m_style == MultiTabBar::KDEV3ICON ) ||
            ( m_style == MultiTabBar::AMAROK ) ) {
        box->setGeometry( 0, 0, width(), height() );
        int lines = 1;
        uint space;
        float tmp = 0;
        if ( ( m_position == MultiTabBar::Bottom ) || ( m_position == MultiTabBar::Top ) )
            space = width();
        else
            space = height(); // made space for tab management button

        int cnt = 0;
        //CALCULATE LINES
        const uint tabCount = m_tabs.count();

        for ( uint i = 0;i < tabCount;i++ ) {
            if ( ! m_tabs.at( i ) ->visible() ) continue;
            cnt++;
            tmp += sizePerTab();
            if ( tmp > space ) {
                if ( cnt > 1 ) i--;
                else if ( i == ( tabCount - 1 ) ) break;
                cnt = 0;
                tmp = 0;
                lines++;
            }
        }
        //SET SIZE & PLACE
        float diff = 0;
        cnt = 0;

        if ( ( m_position == MultiTabBar::Bottom ) || ( m_position == MultiTabBar::Top ) ) {

            setFixedHeight( lines * 24 );
            box->setFixedHeight( lines * 24 );
            m_lines = height() / 24 - 1;
            lines = 0;
            CALCDIFF( m_tabs, diff, 0 )
            tmp = -diff;

            //kdDebug()<<"m_lines recalculated="<<m_lines<<endl;
            for ( uint i = 0;i < tabCount;i++ ) {
                MultiTabBarTab *tab = m_tabs.at( i );
                if ( ! tab->visible() ) continue;
                cnt++;
                tmp += sizePerTab() + diff;
                if ( tmp > space ) {
                    //kdDebug()<<"about to start new line"<<endl;
                    if ( cnt > 1 ) {
                        CALCDIFF( m_tabs, diff, i )
                        i--;
                    } else {
                        //kdDebug()<<"placing line on old line"<<endl;
                        kdDebug() << "diff=" << diff << endl;
                        tab->removeEventFilter( this );
                        tab->move( NEARBYINT( tmp - sizePerTab() ), lines * 24 );
                        //						tab->setFixedWidth(tab->neededSize()+diff);
                        tab->setFixedWidth( NEARBYINT( tmp + diff ) - tab->x() );;
                        tab->installEventFilter( this );
                        CALCDIFF( m_tabs, diff, ( i + 1 ) )

                    }
                    tmp = -diff;
                    cnt = 0;
                    lines++;
                    //kdDebug()<<"starting new line:"<<lines<<endl;

                } else {
                    //kdDebug()<<"Placing line on line:"<<lines<<" pos: (x/y)=("<<tmp-m_tabs.at(i)->neededSize()<<"/"<<lines*24<<")"<<endl;
                    //kdDebug()<<"diff="<<diff<<endl;
                    tab->removeEventFilter( this );
                    tab->move( NEARBYINT( tmp - sizePerTab() ), lines * 24 );
                    tab->setFixedWidth( NEARBYINT( tmp + diff ) - tab->x() );;

                    //tab->setFixedWidth(tab->neededSize()+diff);
                    tab->installEventFilter( this );

                }
            }
        } else {
            // Left or Right
            setFixedWidth( lines * 24 );
            box->setFixedWidth( lines * 24 );
            m_lines = lines = width() / 24;
            lines = 0;
            CALCDIFF( m_tabs, diff, 0 )
            tmp = -diff;

            for ( uint i = 0;i < tabCount;i++ ) {
                MultiTabBarTab *tab = m_tabs.at( i );
                if ( ! tab->visible() ) continue;
                cnt++;
                tmp += sizePerTab() + diff;
                if ( tmp > space ) {
                    if ( cnt > 1 ) {
                        CALCDIFF( m_tabs, diff, i );
                        tmp = -diff;
                        i--;
                    } else {
                        tab->removeEventFilter( this );
                        tab->move( lines * 24, NEARBYINT( tmp - sizePerTab() ) );
                        tab->setFixedHeight( NEARBYINT( tmp + diff ) - tab->y() );;
                        tab->installEventFilter( this );
                    }
                    cnt = 0;
                    tmp = -diff;
                    lines++;
                } else {
                    tab->removeEventFilter( this );
                    tab->move( lines * 24, NEARBYINT( tmp - sizePerTab() ) );
                    tab->setFixedHeight( NEARBYINT( tmp + diff ) - tab->y() );
                    tab->installEventFilter( this );
                }
            }
        }


        //kdDebug()<<"needed lines:"<<m_lines<<endl;
    } else {
        int size = 0; /*move the calculation into another function and call it only on add tab and tab click events*/
        for ( int i = 0;i < ( int ) m_tabs.count();i++ )
            size += ( m_barMode == MultiTabBar::Vertical ? m_tabs.at( i ) ->height() : m_tabs.at( i ) ->width() );
        if ( ( m_position == MultiTabBar::Bottom ) || ( m_position == MultiTabBar::Top ) )
            box->setGeometry( 0, 0, size, height() );
        else box->setGeometry( 0, 0, width(), size );

    }
}


void MultiTabBarInternal::showActiveTabTexts( bool show )
{
    m_showActiveTabTexts = show;
}

MultiTabBarTab* MultiTabBarInternal::tab( int id ) const
{
    for ( QPtrListIterator<MultiTabBarTab> it( m_tabs );it.current();++it ) {
        if ( it.current() ->id() == id ) return it.current();
    }
    return 0;
}

bool MultiTabBarInternal::eventFilter( QObject *, QEvent *e )
{
    if ( e->type() == QEvent::Resize )
        resizeEvent( 0 );

    //PATCH by markey: Allow switching of tabs with mouse wheel
    if ( e->type() == QEvent::Wheel ) {
        QWheelEvent* event = static_cast<QWheelEvent*>( e );
        const int delta = event->delta() / 120;

        // Determine which tab is currently active
        uint i;
        for( i = 0; i < m_tabs.count(); i++ )
            if ( m_tabs.at( i )->isOn() ) break;

        // Calculate index of the new tab to activate
        int newTab = i - delta;
        while (true) {
            if ( newTab < 0 ) {
                newTab = i;
                break;
            }
            if ( newTab > (int)m_tabs.count() - 1 ) {
                newTab = i;
                break;
            }
            if ( m_tabs.at( newTab )->visible() && m_tabs.at( newTab )->isEnabled() )
                break;
            // try one tab more
            newTab -= delta;
        }

        if ( i < m_tabs.count() && newTab != (int)i )
            m_tabs.at( newTab )->animateClick();

        // Must return true here for the wheel to work properly
        return true;
    }

    return false;
}

int MultiTabBarInternal::appendTab( const QPixmap &pic , int id, const QString& text, const QString& identifier )
{
    MultiTabBarTab * tab;
    m_tabs.append( tab = new MultiTabBarTab( pic, text, id, box, m_position, m_style ) );
    tab->setIdentifier( identifier );
    tab->installEventFilter( this );
    tab->showActiveTabText( m_showActiveTabTexts );
    tab->setVisible( Amarok::config( "BrowserBar" )->readBoolEntry( identifier, true ) );

    if ( m_style == MultiTabBar::KONQSBC ) {
        if ( m_expandedTabSize < tab->neededSize() ) {
            m_expandedTabSize = tab->neededSize();
            for ( uint i = 0;i < m_tabs.count();i++ )
                m_tabs.at( i ) ->setSize( m_expandedTabSize );

        } else tab->setSize( m_expandedTabSize );
    } else tab->updateState();

    if ( tab->visible() ) {
        tab->show();
        resizeEvent( 0 );
    } else tab->hide();

    return 0;
}

void MultiTabBarInternal::removeTab( int id )
{
    for ( uint pos = 0;pos < m_tabs.count();pos++ ) {
        if ( m_tabs.at( pos ) ->id() == id ) {
            m_tabs.remove( pos );
            resizeEvent( 0 );
            break;
        }
    }
}

void MultiTabBarInternal::setTabVisible( int id, bool visible )
{
    for ( uint pos = 0;pos < m_tabs.count();pos++ ) {
        if ( m_tabs.at( pos ) ->id() == id ) {
            MultiTabBarTab* tab = m_tabs.at( pos );

            tab->setVisible( visible );
            Amarok::config( "BrowserBar" )->writeEntry( tab->identifier(), tab->visible() );

            if ( tab->visible() )
                tab->show();
            else {
                tab->hide();
                // if the user wants to hide the currently active tab
                // turn on another tab
                if ( tab->isOn() )
                    for( uint i = 0; i < m_tabs.count(); i++ ) {
                        if ( m_tabs.at( i )->visible() ) {
                            m_tabs.at( i )->animateClick();
                            break;
                        }
                    }
            }
            // redraw the bar
            resizeEvent( 0 );
        }
    }
}

void MultiTabBarInternal::setPosition( enum MultiTabBar::MultiTabBarPosition pos )
{
    m_position = pos;
    for ( uint i = 0;i < m_tabs.count();i++ )
        m_tabs.at( i ) ->setTabsPosition( m_position );
    viewport() ->repaint();
}


uint MultiTabBarInternal::visibleTabCount()
{
    uint visibleTabCount = 0;
    for ( uint i = 0; i < m_tabs.count(); i++ )
        if ( m_tabs.at( i ) ->visible() ) visibleTabCount++;

    return visibleTabCount;
}

uint MultiTabBarInternal::sizePerTab()
{
    uint size;
    if( m_position == MultiTabBar::Left || m_position == MultiTabBar::Right )
                        /* HACK: width() is the "Manage Tabs" button size :-( */
        size = (height() - 3 - width() ) / visibleTabCount();
    else
        size = (width() - 3 ) / visibleTabCount();

    return size;
}


MultiTabBarButton::MultiTabBarButton( const QPixmap& pic, const QString& text, QPopupMenu *popup,
                                      int id, QWidget *parent, MultiTabBar::MultiTabBarPosition pos, MultiTabBar::MultiTabBarStyle style )
        : QPushButton( QIconSet(), text, parent )
        , m_position( pos )
        , m_style( style )
        , m_id( id )
        , m_animCount( 0 )
        , m_animTimer( new QTimer( this ) )
        , m_dragSwitchTimer( new QTimer( this ) )
{
    setAcceptDrops( true );
    setIconSet( pic );
    setText( text );
    if ( popup ) setPopup( popup );
    setFlat( true );
    setFixedHeight( 24 );
    setFixedWidth( 24 );

//     QToolTip::add( this, text );  // Deactivated cause it's annoying
    connect( this, SIGNAL( clicked() ), this, SLOT( slotClicked() ) );
    connect( m_animTimer, SIGNAL( timeout() ), this, SLOT( slotAnimTimer() ) );
    connect( m_dragSwitchTimer, SIGNAL( timeout() ), this, SLOT( slotDragSwitchTimer() ) );
}

MultiTabBarButton::MultiTabBarButton( const QString& text, QPopupMenu *popup,
                                      int id, QWidget *parent, MultiTabBar::MultiTabBarPosition pos, MultiTabBar::MultiTabBarStyle style )
        : QPushButton( QIconSet(), text, parent ), m_style( style )
        , m_animCount( 0 )
        , m_animTimer( new QTimer( this ) )
        , m_dragSwitchTimer( new QTimer( this ) )
{
    d = new MultiTabBarButtonPrivate;
    setAcceptDrops( true );
    setText( text );
    m_position = pos;
    if ( popup ) setPopup( popup );
    setFlat( true );
    setFixedHeight( 24 );
    setFixedWidth( 24 );
    m_id = id;
//     QToolTip::add( this, text );

    connect( this, SIGNAL( clicked() ), this, SLOT( slotClicked() ) );
    connect( m_animTimer, SIGNAL( timeout() ), this, SLOT( slotAnimTimer() ) );
    connect( m_dragSwitchTimer, SIGNAL( timeout() ), this, SLOT( slotDragSwitchTimer() ) );
}

MultiTabBarButton::~MultiTabBarButton()
{
    delete d;
}

int MultiTabBarButton::id() const
{
    return m_id;
}

void MultiTabBarButton::setText( const QString& text )
{
    QPushButton::setText( text );
    m_text = text;
//     QToolTip::add( this, text );
}

void MultiTabBarButton::proxyDrops( DropProxyTarget *finalDropTarget )
{
    d->finalDropTarget = finalDropTarget;
}

void MultiTabBarButton::slotClicked()
{
    emit clicked( m_id );
}

void MultiTabBarButton::setPosition( MultiTabBar::MultiTabBarPosition pos )
{
    m_position = pos;
    repaint();
}

void MultiTabBarButton::setStyle( MultiTabBar::MultiTabBarStyle style )
{
    m_style = style;
    repaint();
}

void MultiTabBarButton::hideEvent( QHideEvent* he )
{
    QPushButton::hideEvent( he );
    MultiTabBar *tb = dynamic_cast<MultiTabBar*>( parentWidget() );
    if ( tb ) tb->updateSeparator();
}

void MultiTabBarButton::showEvent( QShowEvent* he )
{
    QPushButton::showEvent( he );
    MultiTabBar *tb = dynamic_cast<MultiTabBar*>( parentWidget() );
    if ( tb ) tb->updateSeparator();
}

void MultiTabBarButton::enterEvent( QEvent* )
{
    m_animEnter = true;
    m_animCount = 0;

    m_animTimer->start( ANIM_INTERVAL );
}

void MultiTabBarButton::leaveEvent( QEvent* )
{
    // This can happen if you enter and leave the tab quickly
    if ( m_animCount == 0 )
        m_animCount = 1;

    m_animEnter = false;
    m_animTimer->start( ANIM_INTERVAL );
}

void MultiTabBarButton::dragEnterEvent ( QDragEnterEvent *e )
{
    enterEvent ( e );
    e->accept( d->finalDropTarget );
}

void MultiTabBarButton::dragMoveEvent ( QDragMoveEvent * )
{
    if ( !m_dragSwitchTimer->isActive() )
        m_dragSwitchTimer->start( ANIM_INTERVAL * ANIM_MAX + 300, true );
}

void MultiTabBarButton::dragLeaveEvent ( QDragLeaveEvent *e )
{
    m_dragSwitchTimer->stop();
    leaveEvent( e );
}

void MultiTabBarButton::dropEvent( QDropEvent *e )
{
    m_dragSwitchTimer->stop();
    if( d->finalDropTarget )
        d->finalDropTarget->dropProxyEvent( e );
    leaveEvent( e );
}

void MultiTabBarButton::slotDragSwitchTimer()
{
    emit ( initiateDrag ( m_id ) );
}

void MultiTabBarButton::slotAnimTimer()
{
    if ( m_animEnter ) {
        m_animCount += 1;
        repaint( false );
        if ( m_animCount >= ANIM_MAX )
            m_animTimer->stop();
    } else {
        m_animCount -= 1;
        repaint( false );
        if ( m_animCount <= 0 )
            m_animTimer->stop();
    }
}

QSize MultiTabBarButton::sizeHint() const
{
    constPolish();

    int w = 0, h = 0;

    // calculate contents size...
#ifndef QT_NO_ICONSET
    if ( iconSet() && !iconSet() ->isNull() ) {
        int iw = iconSet() ->pixmap( QIconSet::Small, QIconSet::Normal ).width() + 4;
        int ih = iconSet() ->pixmap( QIconSet::Small, QIconSet::Normal ).height();
        w += iw;
        h = QMAX( h, ih );
    }
#endif
    if ( isMenuButton() )
        w += style().pixelMetric( QStyle::PM_MenuButtonIndicator, this );

    if ( pixmap() ) {
        QPixmap * pm = const_cast< QPixmap * >( pixmap() );
        w += pm->width();
        h += pm->height();
    } else {
        QString s( text() );
        bool empty = s.isEmpty();
        if ( empty )
            s = QString::fromLatin1( "XXXX" );
        QFontMetrics fm = fontMetrics();
        QSize sz = fm.size( ShowPrefix, s );
        if ( !empty || !w )
            w += sz.width();
        if ( !empty || !h )
            h = QMAX( h, sz.height() );
    }

//     //PATCH by markey
//     if ( ( m_style == MultiTabBar::AMAROK ) ) {
//         if( m_position == MultiTabBar::Left || m_position == MultiTabBar::Right )
//             w = ( parentWidget()->height() - 3 ) / NUM_TABS;
//         else
//             h = ( parentWidget()->width() - 3 ) / NUM_TABS;
//     }

    return ( style().sizeFromContents( QStyle::CT_ToolButton, this, QSize( w, h ) ).
             expandedTo( QApplication::globalStrut() ) );
}


MultiTabBarTab::MultiTabBarTab( const QPixmap& pic, const QString& text,
                                int id, QWidget *parent, MultiTabBar::MultiTabBarPosition pos,
                                MultiTabBar::MultiTabBarStyle style )
        : MultiTabBarButton( text, 0, id, parent, pos, style ),
        m_visible(true),
        m_showActiveTabText( false )
{
    d = new MultiTabBarTabPrivate();
    setIcon( pic );
    setIdentifier( text );
    m_expandedSize = 24;
    setToggleButton( true );

    // Prevent flicker on redraw
    setWFlags( getWFlags() | Qt::WNoAutoErase );
}

MultiTabBarTab::~MultiTabBarTab()
{
    delete d;
}


void MultiTabBarTab::setTabsPosition( MultiTabBar::MultiTabBarPosition pos )
{
    if ( ( pos != m_position ) && ( ( pos == MultiTabBar::Left ) || ( pos == MultiTabBar::Right ) ) ) {
        if ( !d->pix.isNull() ) {
            QWMatrix temp; // (1.0F, 0.0F, 0.0F, -1.0F, 0.0F, 0.0F);
            temp.rotate( 180 );
            d->pix = d->pix.xForm( temp );
            setIconSet( d->pix );
        }
    }

    setPosition( pos );
    // repaint();
}

void MultiTabBarTab::setIcon( const QString& icon )
{
    QPixmap pic = SmallIcon( icon );
    setIcon( pic );
}

void MultiTabBarTab::setIcon( const QPixmap& icon )
{

    if ( m_style != MultiTabBar::KDEV3 ) {
        if ( ( m_position == MultiTabBar::Left ) || ( m_position == MultiTabBar::Right ) ) {
            QWMatrix rotateMatrix;
            if ( m_position == MultiTabBar::Left )
                rotateMatrix.rotate( 90 );
            else
                rotateMatrix.rotate( -90 );
            QPixmap pic = icon.xForm( rotateMatrix ); //TODO FIX THIS, THIS SHOWS WINDOW
            d->pix = pic;
            setIconSet( pic );
        } else setIconSet( icon );
    }
}

void MultiTabBarTab::slotClicked()
{
    if ( m_animTimer->isActive() ) {
        m_animCount = ANIM_MAX;
        m_animTimer->stop();
        repaint();
    }

    updateState();
    MultiTabBarButton::slotClicked();
}

void MultiTabBarTab::setState( bool b )
{
    setOn( b );
    updateState();
}

void MultiTabBarTab::updateState()
{

    if ( m_style != MultiTabBar::KONQSBC ) {
        if ( ( m_style == MultiTabBar::KDEV3 ) || ( m_style == MultiTabBar::KDEV3ICON ) || ( m_style == MultiTabBar::AMAROK ) || ( isOn() ) ) {
            QPushButton::setText( m_text );
        } else {
            kdDebug() << "MultiTabBarTab::updateState(): setting text to an empty QString***************" << endl;
            QPushButton::setText( QString::null );
        }

        if ( ( m_position == MultiTabBar::Right || m_position == MultiTabBar::Left ) ) {
            setFixedWidth( 24 );
            if ( ( m_style == MultiTabBar::KDEV3 ) || ( m_style == MultiTabBar::KDEV3ICON ) || ( m_style == MultiTabBar::AMAROK ) || ( isOn() ) ) {
                setFixedHeight( MultiTabBarButton::sizeHint().width() );
            } else setFixedHeight( 36 );
        } else {
            setFixedHeight( 24 );
            if ( ( m_style == MultiTabBar::KDEV3 ) || ( m_style == MultiTabBar::KDEV3ICON ) || ( m_style == MultiTabBar::AMAROK ) || ( isOn() ) ) {
                setFixedWidth( MultiTabBarButton::sizeHint().width() );
            } else setFixedWidth( 36 );
        }
    } else {
        if ( ( !isOn() ) || ( !m_showActiveTabText ) ) {
            setFixedWidth( 24 );
            setFixedHeight( 24 );
            return ;
        }
        if ( ( m_position == MultiTabBar::Right || m_position == MultiTabBar::Left ) )
            setFixedHeight( m_expandedSize );
        else
            setFixedWidth( m_expandedSize );
    }
    QApplication::sendPostedEvents( 0, QEvent::Paint | QEvent::Move | QEvent::Resize | QEvent::LayoutHint );
    QApplication::flush();
}

int MultiTabBarTab::neededSize()
{
    return ( ( ( m_style != MultiTabBar::KDEV3 ) ? 24 : 0 ) + QFontMetrics( QFont() ).width( m_text ) + 6 );
}

void MultiTabBarTab::setSize( int size )
{
    m_expandedSize = size;
    updateState();
}

void MultiTabBarTab::showActiveTabText( bool show )
{
    m_showActiveTabText = show;
}

void MultiTabBarTab::drawButtonLabel( QPainter *p )
{
    drawButton( p );
}
void MultiTabBarTab::drawButton( QPainter *paint )
{
    if ( m_style == MultiTabBar::AMAROK ) drawButtonAmarok( paint );
    else if ( m_style != MultiTabBar::KONQSBC ) drawButtonStyled( paint );
    else drawButtonClassic( paint );
}

void MultiTabBarTab::drawButtonStyled( QPainter *paint )
{

    QSize sh;
    const int width = 36; // rotated
    const int height = 24;
    if ( ( m_style == MultiTabBar::KDEV3 ) || ( m_style == MultiTabBar::KDEV3ICON ) || ( m_style == MultiTabBar::AMAROK ) || ( isOn() ) ) {
        if ( ( m_position == MultiTabBar::Left ) || ( m_position == MultiTabBar::Right ) )
            sh = QSize( this->height(), this->width() ); //MultiTabBarButton::sizeHint();
        else
            sh = QSize( this->width(), this->height() );
    } else
        sh = QSize( width, height );

    QPixmap pixmap( sh.width(), height ); ///,sh.height());
    pixmap.fill( eraseColor() );
    QPainter painter( &pixmap );


    QStyle::SFlags st = QStyle::Style_Default;

    st |= QStyle::Style_Enabled;

    if ( isOn() ) st |= QStyle::Style_On;

    style().drawControl( QStyle::CE_PushButton, &painter, this, QRect( 0, 0, pixmap.width(), pixmap.height() ), colorGroup(), st );
    style().drawControl( QStyle::CE_PushButtonLabel, &painter, this, QRect( 0, 0, pixmap.width(), pixmap.height() ), colorGroup(), st );

    switch ( m_position ) {
    case MultiTabBar::Left:
        paint->rotate( -90 );
        paint->drawPixmap( 1 - pixmap.width(), 0, pixmap );
        break;
    case MultiTabBar::Right:
        paint->rotate( 90 );
        paint->drawPixmap( 0, 1 - pixmap.height(), pixmap );
        break;

    default:
        paint->drawPixmap( 0, 0, pixmap );
        break;
    }
    //	style().drawControl(QStyle::CE_PushButtonLabel,painter,this, QRect(0,0,pixmap.width(),pixmap.height()),
    //		colorGroup(),QStyle::Style_Enabled);
}

void MultiTabBarTab::drawButtonClassic( QPainter *paint )
{
    QPixmap pixmap;
    if ( iconSet() )
        pixmap = iconSet() ->pixmap( QIconSet::Small, QIconSet::Normal );
    paint->fillRect( 0, 0, 24, 24, colorGroup().background() );

    if ( !isOn() ) {

        if ( m_position == MultiTabBar::Right ) {
            paint->fillRect( 0, 0, 21, 21, QBrush( colorGroup().background() ) );

            paint->setPen( colorGroup().background().dark( 150 ) );
            paint->drawLine( 0, 22, 23, 22 );

            paint->drawPixmap( 12 - pixmap.width() / 2, 12 - pixmap.height() / 2, pixmap );

            paint->setPen( colorGroup().shadow() );
            paint->drawLine( 0, 0, 0, 23 );
            paint->setPen( colorGroup().background().dark( 120 ) );
            paint->drawLine( 1, 0, 1, 23 );

        } else
            if ( ( m_position == MultiTabBar::Bottom ) || ( m_position == MultiTabBar::Top ) ) {
                paint->fillRect( 0, 1, 23, 22, QBrush( colorGroup().background() ) );

                paint->drawPixmap( 12 - pixmap.width() / 2, 12 - pixmap.height() / 2, pixmap );

                paint->setPen( colorGroup().background().dark( 120 ) );
                paint->drawLine( 23, 0, 23, 23 );


                paint->setPen( colorGroup().light() );
                paint->drawLine( 0, 22, 23, 22 );
                paint->drawLine( 0, 23, 23, 23 );
                paint->setPen( colorGroup().shadow() );
                paint->drawLine( 0, 0, 23, 0 );
                paint->setPen( colorGroup().background().dark( 120 ) );
                paint->drawLine( 0, 1, 23, 1 );

            } else {
                paint->setPen( colorGroup().background().dark( 120 ) );
                paint->drawLine( 0, 23, 23, 23 );
                paint->fillRect( 0, 0, 23, 21, QBrush( colorGroup().background() ) );
                paint->drawPixmap( 12 - pixmap.width() / 2, 12 - pixmap.height() / 2, pixmap );

                paint->setPen( colorGroup().light() );
                paint->drawLine( 23, 0, 23, 23 );
                paint->drawLine( 22, 0, 22, 23 );

                paint->setPen( colorGroup().shadow() );
                paint->drawLine( 0, 0, 0, 23 );

            }


    } else {
        if ( m_position == MultiTabBar::Right ) {
            paint->setPen( colorGroup().shadow() );
            paint->drawLine( 0, height() - 1, 23, height() - 1 );
            paint->drawLine( 0, height() - 2, 23, height() - 2 );
            paint->drawLine( 23, 0, 23, height() - 1 );
            paint->drawLine( 22, 0, 22, height() - 1 );
            paint->fillRect( 0, 0, 21, height() - 3, QBrush( colorGroup().light() ) );
            paint->drawPixmap( 10 - pixmap.width() / 2, 10 - pixmap.height() / 2, pixmap );

            if ( m_showActiveTabText ) {
                if ( height() < 25 + 4 ) return ;

                QPixmap tpixmap( height() - 25 - 3, width() - 2 );
                QPainter painter( &tpixmap );

                painter.fillRect( 0, 0, tpixmap.width(), tpixmap.height(), QBrush( colorGroup().light() ) );

                painter.setPen( colorGroup().text() );
                painter.drawText( 0, + width() / 2 + QFontMetrics( QFont() ).height() / 2, m_text );

                paint->rotate( 90 );
                kdDebug() << "tpixmap.width:" << tpixmap.width() << endl;
                paint->drawPixmap( 25, -tpixmap.height() + 1, tpixmap );
            }

        } else
            if ( m_position == MultiTabBar::Top ) {
                paint->fillRect( 0, 0, width() - 1, 23, QBrush( colorGroup().light() ) );
                paint->drawPixmap( 10 - pixmap.width() / 2, 10 - pixmap.height() / 2, pixmap );
                if ( m_showActiveTabText ) {
                    paint->setPen( colorGroup().text() );
                    paint->drawText( 25, height() / 2 + QFontMetrics( QFont() ).height() / 2, m_text );
                }
            } else
                if ( m_position == MultiTabBar::Bottom ) {
                    paint->setPen( colorGroup().shadow() );
                    paint->drawLine( 0, 23, width() - 1, 23 );
                    paint->drawLine( 0, 22, width() - 1, 22 );
                    paint->fillRect( 0, 0, width() - 1, 21, QBrush( colorGroup().light() ) );
                    paint->drawPixmap( 10 - pixmap.width() / 2, 10 - pixmap.height() / 2, pixmap );
                    if ( m_showActiveTabText ) {
                        paint->setPen( colorGroup().text() );
                        paint->drawText( 25, height() / 2 + QFontMetrics( QFont() ).height() / 2, m_text );
                    }

                } else {


                    paint->setPen( colorGroup().shadow() );
                    paint->drawLine( 0, height() - 1, 23, height() - 1 );
                    paint->drawLine( 0, height() - 2, 23, height() - 2 );
                    paint->fillRect( 0, 0, 23, height() - 3, QBrush( colorGroup().light() ) );
                    paint->drawPixmap( 10 - pixmap.width() / 2, 10 - pixmap.height() / 2, pixmap );
                    if ( m_showActiveTabText ) {

                        if ( height() < 25 + 4 ) return ;

                        QPixmap tpixmap( height() - 25 - 3, width() - 2 );
                        QPainter painter( &tpixmap );

                        painter.fillRect( 0, 0, tpixmap.width(), tpixmap.height(), QBrush( colorGroup().light() ) );

                        painter.setPen( colorGroup().text() );
                        painter.drawText( tpixmap.width() - QFontMetrics( QFont() ).width( m_text ), + width() / 2 + QFontMetrics( QFont() ).height() / 2, m_text );

                        paint->rotate( -90 );
                        kdDebug() << "tpixmap.width:" << tpixmap.width() << endl;

                        paint->drawPixmap( -24 - tpixmap.width(), 2, tpixmap );

                    }

                }

    }
}

void MultiTabBarTab::drawButtonAmarok( QPainter *paint )
{
    QColor fillColor, textColor;
    if ( isOn() ) {
        fillColor = blendColors( colorGroup().highlight(), colorGroup().background(), static_cast<int>( m_animCount * 3.5 ) );
        textColor = blendColors( colorGroup().highlightedText(), colorGroup().text(), static_cast<int>( m_animCount * 4.5 ) );
    } else if ( isEnabled() ) {
        fillColor = blendColors( colorGroup().background(), colorGroup().highlight(), static_cast<int>( m_animCount * 3.5 ) );
        textColor = blendColors( colorGroup().text(), colorGroup().highlightedText(), static_cast<int>( m_animCount * 4.5 ) );
    } else {
        fillColor = colorGroup().background();
        textColor = colorGroup().text();
    }

#ifndef QT_NO_ICONSET
    if ( iconSet() && !iconSet() ->isNull() )
    {
        QPixmap icon = iconSet()->pixmap( QIconSet::Small, QIconSet::Normal );

        // Apply icon effect when widget disabled. Should really be cached, but *shrug*.
        if( !isEnabled() )
            icon = kapp->iconLoader()->iconEffect()->apply( icon, KIcon::Small, KIcon::DisabledState );

        if( m_position == MultiTabBar::Left || m_position == MultiTabBar::Right ) {
            QPixmap pixmap( height(), width() );
            pixmap.fill( fillColor );
            QPainter painter( &pixmap );

            // Draw the frame
            painter.setPen( colorGroup().mid() );
            /*if ( m_id != bar->visibleTabCount() - 1 )*/ painter.drawLine( 0, 0, 0, pixmap.height() - 1 );
            painter.drawLine( 0, pixmap.height() - 1, pixmap.width() - 1, pixmap.height() - 1 );

            // Draw the text
            QFont font;
            painter.setFont( font );
            QString text = KStringHandler::rPixelSqueeze( m_text, QFontMetrics( font ), pixmap.width() - icon.width() - 3 );
            text.replace( "...", ".." );
            const int textX = pixmap.width() / 2 - QFontMetrics( font ).width( text ) / 2;
            painter.setPen( textColor );
            const QRect rect( textX + icon.width() / 2 + 2, 0, pixmap.width(), pixmap.height() );
            painter.drawText( rect, Qt::AlignLeft | Qt::AlignVCenter, text );

            // Draw the icon
            painter.drawPixmap( textX - icon.width() / 2 - 2, pixmap.height() / 2 - icon.height() / 2, icon );

            // Paint to widget
            paint->rotate( -90 );
            paint->drawPixmap( 1 - pixmap.width(), 0, pixmap );

        } else { // Horizontal

            QPixmap pixmap( width(), height() );
            pixmap.fill( fillColor );
            QPainter painter( &pixmap );

            // Draw the frame
            painter.setPen( colorGroup().mid() );
            /*if ( m_id != bar->visibleTabCount() - 1 )*/ painter.drawLine( 0, 0, 0, pixmap.height() - 1 );
            painter.drawLine( 0, pixmap.height() - 1, pixmap.width() - 1, pixmap.height() - 1 );

            // Draw the text
            QFont font;
            painter.setFont( font );
            QString text = KStringHandler::rPixelSqueeze( m_text, QFontMetrics( font ), pixmap.width() - icon.width() - 3 );
            text.replace( "...", ".." );
            const int textX = pixmap.width() / 2 - QFontMetrics( font ).width( text ) / 2;
            painter.setPen( textColor );
            const QRect rect( textX + icon.width() / 2 + 2, 0, pixmap.width(), pixmap.height() );
            painter.drawText( rect, Qt::AlignLeft | Qt::AlignVCenter, text );

            // Draw the icon
            painter.drawPixmap( textX - icon.width() / 2 - 2, pixmap.height() / 2 - icon.height() / 2, icon );

            // Paint to widget
            paint->drawPixmap( 0, 0, pixmap );
        }
    }
#endif
}


QColor MultiTabBarTab::blendColors( const QColor& color1, const QColor& color2, int percent )
{
    const float factor1 = ( 100 - ( float ) percent ) / 100;
    const float factor2 = ( float ) percent / 100;

    const int r = static_cast<int>( color1.red() * factor1 + color2.red() * factor2 );
    const int g = static_cast<int>( color1.green() * factor1 + color2.green() * factor2 );
    const int b = static_cast<int>( color1.blue() * factor1 + color2.blue() * factor2 );

    QColor result;
    result.setRgb( r, g, b );

    return result;
}




MultiTabBar::MultiTabBar( MultiTabBarMode bm, QWidget *parent, const char *name ) : QWidget( parent, name )
{
    m_buttons.setAutoDelete( false );
    if ( bm == Vertical ) {
        m_l = new QVBoxLayout( this );
        setSizePolicy( QSizePolicy::Fixed, QSizePolicy::Expanding, true );
        //		setFixedWidth(24);
    } else {
        m_l = new QHBoxLayout( this );
        setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Fixed, true );
        //		setFixedHeight(24);
    }
    m_l->setMargin( 0 );
    m_l->setAutoAdd( false );

    m_internal = new MultiTabBarInternal( this, bm );
    setPosition( ( bm == MultiTabBar::Vertical ) ? MultiTabBar::Right : MultiTabBar::Bottom );
    setStyle( VSNET );
    //	setStyle(KDEV3);
    //setStyle(KONQSBC);
    m_l->insertWidget( 0, m_internal );
    m_l->insertWidget( 0, m_btnTabSep = new QFrame( this ) );
    m_btnTabSep->setFixedHeight( 4 );
    m_btnTabSep->setFrameStyle( QFrame::Panel | QFrame::Sunken );
    m_btnTabSep->setLineWidth( 2 );
    m_btnTabSep->hide();

    updateGeometry();
}

MultiTabBar::~MultiTabBar()
{}

/*int MultiTabBar::insertButton(QPixmap pic,int id ,const QString&)
{
(new KToolbarButton(pic,id,m_internal))->show();
return 0;
}*/

int MultiTabBar::appendButton( const QPixmap &pic , int id, QPopupMenu *popup, const QString& )
{
    MultiTabBarButton * btn;
    m_buttons.append( btn = new MultiTabBarButton( pic, QString::null,
                            popup, id, this, m_position, m_internal->m_style ) );
    m_l->insertWidget( 0, btn );
    btn->show();
    m_btnTabSep->show();
    return 0;
}

void MultiTabBar::updateSeparator()
{
    bool hideSep = true;
    for ( QPtrListIterator<MultiTabBarButton> it( m_buttons );it.current();++it ) {
        if ( it.current() ->isVisibleTo( this ) ) {
            hideSep = false;
            break;
        }
    }
    if ( hideSep ) m_btnTabSep->hide();
    else m_btnTabSep->show();

}

int MultiTabBar::appendTab( const QPixmap &pic , int id , const QString& text, const QString& identifier )
{
    m_internal->appendTab( pic, id, text, identifier );
    return 0;
}

MultiTabBarButton* MultiTabBar::button( int id ) const
{
    for ( QPtrListIterator<MultiTabBarButton> it( m_buttons );it.current();++it ) {
        if ( it.current() ->id() == id ) return it.current();
    }
    return 0;
}

MultiTabBarTab* MultiTabBar::tab( int id ) const
{
    return m_internal->tab( id );
}



void MultiTabBar::removeButton( int id )
{
    for ( uint pos = 0;pos < m_buttons.count();pos++ ) {
        if ( m_buttons.at( pos ) ->id() == id ) {
            m_buttons.take( pos ) ->deleteLater();
            break;
        }
    }
    if ( m_buttons.count() == 0 ) m_btnTabSep->hide();
}

void MultiTabBar::removeTab( int id )
{
    m_internal->removeTab( id );
}

void MultiTabBar::setTab( int id, bool state )
{
    MultiTabBarTab * ttab = tab( id );
    if ( ttab ) {
        ttab->setState( state );
        if( state && !ttab->visible() )
            m_internal->setTabVisible( id, true );
    }
}

bool MultiTabBar::isTabRaised( int id ) const
{
    MultiTabBarTab * ttab = tab( id );
    if ( ttab ) {
        return ttab->isOn();
    }

    return false;
}


void MultiTabBar::showActiveTabTexts( bool show )
{
    m_internal->showActiveTabTexts( show );
}

uint MultiTabBar::visibleTabCount()
{
    return m_internal->visibleTabCount( );
}

uint MultiTabBar::sizePerTab()
{
    return m_internal->sizePerTab( );
}

void MultiTabBar::setStyle( MultiTabBarStyle style )
{
    m_internal->setStyle( style );
}

void MultiTabBar::setPosition( MultiTabBarPosition pos )
{
    m_position = pos;
    m_internal->setPosition( pos );
    for ( uint i = 0;i < m_buttons.count();i++ )
        m_buttons.at( i ) ->setPosition( pos );
}
void MultiTabBar::fontChange( const QFont& /* oldFont */ )
{
    for ( uint i = 0;i < tabs() ->count();i++ )
        tabs() ->at( i ) ->resize();
    repaint();
}

QPtrList<MultiTabBarTab>* MultiTabBar::tabs() { return m_internal->tabs();}
QPtrList<MultiTabBarButton>* MultiTabBar::buttons() { return & m_buttons;}

void MultiTabBar::showTabSelectionMenu(QPoint pos)
{
    m_internal->showTabSelectionMenu(pos);
}

