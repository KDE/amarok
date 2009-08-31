/****************************************************************************************
 * Copyright (c) 2008 Leo Franchi <lfranchi@kde.org>                                    *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#include "ToolbarView.h"

#include "Containment.h"
#include "Debug.h"
#include "toolbar/AppletItemOverlay.h"
#include "toolbar/AppletToolbar.h"
#include "toolbar/AppletToolbarAppletItem.h"

#include <knewstuff2/engine.h>
#include <kfiledialog.h>
#include <kstandarddirs.h>
#include <plasma/applet.h>
#include <plasma/containment.h>
#include <plasma/packagestructure.h>
#include <plasma/theme.h>

#include <QDBusInterface>
#include <QGraphicsLinearLayout>
#include <QGraphicsScene>
#include <QPalette>
#include <QSizePolicy>
#include <QWidget>
#include <QTimer>

#define TOOLBAR_X_OFFSET 2000

Context::ToolbarView::ToolbarView( Plasma::Containment* containment, QGraphicsScene* scene, QWidget* parent )
    : QGraphicsView( scene, parent )
    , m_height( 30 )
    , m_toolbar( 0 )
    , m_cont( containment )
{
    setSceneRect( TOOLBAR_X_OFFSET, 0, size().width(), m_height );
    QSizePolicy policy( QSizePolicy::Preferred, QSizePolicy::Fixed );
    policy.setHeightForWidth( true );
    setSizePolicy( policy );
    setAutoFillBackground( true );

    setFrameStyle(QFrame::NoFrame);
    //setAutoFillBackground(true);
    //setDragMode(QGraphicsView::RubberBandDrag);
    //setCacheMode(QGraphicsView::CacheBackground);
    setInteractive(true);
    setAcceptDrops(true);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    // now we create the toolbar
    m_toolbar = new AppletToolbar( containment );
    m_toolbar->setZValue( m_toolbar->zValue() + 1000 );
    // scene()->addItem( m_toolbar );
    m_toolbar->setPos( TOOLBAR_X_OFFSET, 0 );

   connect( m_toolbar, SIGNAL( configModeToggled() ), this, SLOT( toggleConfigMode() ) );
   connect( m_toolbar, SIGNAL( installApplets() ), this, SLOT( installApplets() ) );

   Context::Containment* cont = dynamic_cast< Context::Containment* >( containment );
   if( cont )
   {
       connect( cont, SIGNAL( appletAdded( Plasma::Applet*, int) ), m_toolbar, SLOT( appletAdded( Plasma::Applet*, int) ) );
       connect( m_toolbar, SIGNAL( appletAddedToToolbar( Plasma::Applet*, int) ), this, SLOT( appletAdded( Plasma::Applet*, int) ) );
       connect( cont, SIGNAL( appletRemoved( Plasma::Applet* ) ), this, SLOT( appletRemoved( Plasma::Applet* ) ) );
       connect( m_toolbar, SIGNAL( showApplet( Plasma::Applet* ) ), cont, SLOT( showApplet( Plasma::Applet* ) ) );
       connect( m_toolbar, SIGNAL( moveApplet( Plasma::Applet*, int, int ) ), cont, SLOT( moveApplet( Plasma::Applet*, int, int ) ) );
       connect( m_toolbar, SIGNAL( addAppletToContainment( const QString&, int ) ), cont, SLOT( addApplet( const QString&, int ) ) );
   }

   //make background transparent
   QPalette p = palette();
   QColor c = p.color( QPalette::Base );
   c.setAlpha( 0 );
   p.setColor( QPalette::Base, c );
   setPalette( p );

}

Context::ToolbarView::~ToolbarView()
{

}

QSize
Context::ToolbarView::sizeHint() const
{
    return QSize( size().width(), m_height );
}

int
Context::ToolbarView::heightForWidth( int w ) const
{
    Q_UNUSED( w )
    return m_height;
}


void
Context::ToolbarView::resizeEvent( QResizeEvent *event )
{
    Q_UNUSED( event )
    setSceneRect( TOOLBAR_X_OFFSET, 0, size().width(), m_height );
    m_toolbar->setGeometry( sceneRect() );
}

void
Context::ToolbarView::dragEnterEvent( QDragEnterEvent *event )
{
    Q_UNUSED( event )
}

void
Context::ToolbarView::dragMoveEvent( QDragMoveEvent *event )
{
    Q_UNUSED( event )
}

void
Context::ToolbarView::dragLeaveEvent( QDragLeaveEvent *event )
{
    Q_UNUSED( event )
}

void
Context::ToolbarView::toggleConfigMode()
{
    DEBUG_BLOCK
    if( m_toolbar->configEnabled() ) // set up config stuff
    {
        debug() << "got config enabled, creating all the move overlays";
          // now add the overlays that handle the drag-n-dropping
        QColor overlayColor( Plasma::Theme::defaultTheme()->color( Plasma::Theme::BackgroundColor ) );
        QBrush overlayBrush( overlayColor );
        QPalette p( palette() );
        p.setBrush( QPalette::Window, overlayBrush );
      /* for( int i = 0; i < m_toolbar->appletLayout()->count(); i++ )
        {
            debug() << "item" << i << "has geometry:" << m_toolbar->appletLayout()->itemAt( i )->geometry();
            Context::AppletToolbarAddItem* item = dynamic_cast< Context::AppletToolbarAddItem* >( m_toolbar->appletLayout()->itemAt( i ) );
            if( item )
                debug() << "add item has boundingRect:" << item->boundingRect() << "and geom:" << item->geometry() << "and sizehint" << item->effectiveSizeHint( Qt::PreferredSize );
        } */

        for( int i = 0; i < m_toolbar->appletLayout()->count(); i++ )
        {
            debug() << "creating a move overlay";
            Context::AppletToolbarAppletItem* item = dynamic_cast< Context::AppletToolbarAppletItem* >( m_toolbar->appletLayout()->itemAt( i ) );
            if( item )
            {
                Context::AppletItemOverlay *moveOverlay = new Context::AppletItemOverlay( item, m_toolbar->appletLayout(), this );
                connect( moveOverlay, SIGNAL( moveApplet( Plasma::Applet*, int, int ) ), m_cont, SLOT( moveApplet( Plasma::Applet*, int, int ) ) );
                connect( moveOverlay, SIGNAL( moveApplet( Plasma::Applet*, int, int ) ), this, SLOT( refreshOverlays() ) );
                connect( moveOverlay, SIGNAL( deleteApplet( Plasma::Applet* ) ), this, SLOT( appletRemoved( Plasma::Applet* ) ) );
                moveOverlay->setPalette( p );
                moveOverlay->show();
                moveOverlay->raise();
                m_moveOverlays << moveOverlay;
                debug() << moveOverlay << moveOverlay->geometry();
            }

        }
    } else
    {
        debug() << "removing all the move overlays";
        foreach( Context::AppletItemOverlay *moveOverlay, m_moveOverlays )
            moveOverlay->deleteLater();
        m_moveOverlays.clear();
    }

}

void
Context::ToolbarView::appletRemoved( Plasma::Applet* applet )
{
    DEBUG_BLOCK
    foreach( Context::AppletItemOverlay* overlay, m_moveOverlays )
    {
        if( overlay->applet()->applet() == applet )
        {
            m_moveOverlays.removeAll( overlay );
            debug() << "got an overlay to remove";
        }
    }
    m_toolbar->appletRemoved( applet );
    applet->deleteLater();
}

void
Context::ToolbarView::appletAdded( Plasma::Applet* applet, int loc )
{
    DEBUG_BLOCK
    Q_UNUSED( applet )
    Q_UNUSED( loc )

    if( m_toolbar->configEnabled() )
        recreateOverlays();
}


void
Context::ToolbarView::refreshOverlays()
{
    m_toolbar->refreshAddIcons();
}

void
Context::ToolbarView::recreateOverlays()
{
    DEBUG_BLOCK
    foreach( Context::AppletItemOverlay *moveOverlay, m_moveOverlays )
        moveOverlay->deleteLater();

    m_moveOverlays.clear();

    QColor overlayColor( Plasma::Theme::defaultTheme()->color( Plasma::Theme::BackgroundColor ) );
    QBrush overlayBrush( overlayColor );
    QPalette p( palette() );
    p.setBrush( QPalette::Window, overlayBrush );
    for( int i = 0; i < m_toolbar->appletLayout()->count(); i++ )
    {
        debug() << "creating a move overlay";
        Context::AppletToolbarAppletItem* item = dynamic_cast< Context::AppletToolbarAppletItem* >( m_toolbar->appletLayout()->itemAt( i ) );
        if( item )
        {
            Context::AppletItemOverlay *moveOverlay = new Context::AppletItemOverlay( item, m_toolbar->appletLayout(), this );
            connect( moveOverlay, SIGNAL( moveApplet( Plasma::Applet*, int, int ) ), m_cont, SLOT( moveApplet( Plasma::Applet*, int, int ) ) );
            connect( moveOverlay, SIGNAL( moveApplet( Plasma::Applet*, int, int ) ), this, SLOT( refreshOverlays() ) );
            connect( moveOverlay, SIGNAL( deleteApplet( Plasma::Applet* ) ), this, SLOT( appletRemoved( Plasma::Applet* ) ) );
            moveOverlay->setPalette( p );
            moveOverlay->show();
            moveOverlay->raise();
            m_moveOverlays << moveOverlay;
            debug() << moveOverlay << moveOverlay->geometry();
        }

    }
}

void
Context::ToolbarView::installApplets()
{
    DEBUG_BLOCK

    KNS::Engine engine(0);
    if (engine.init("amarokapplets.knsrc")) {
        KNS::Entry::List entries = engine.downloadDialogModal(this);
    }
    
    // give it some time to run, but not too long
    QTimer::singleShot( 0, this, SLOT( refreshSycoca() ) );
}

void
Context::ToolbarView::refreshSycoca()
{

    QDBusInterface dbus("org.kde.kded", "/kbuildsycoca", "org.kde.kbuildsycoca");
    dbus.call(QDBus::Block, "recreate");

    recreateOverlays();

}


#include "ToolbarView.moc"
