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
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#include "ToolbarView.h"

#include "Containment.h"
#include "core/support/Debug.h"
#include "PaletteHandler.h"
#include "toolbar/AppletItemOverlay.h"
#include "toolbar/AppletToolbarAppletItem.h"
#include "toolbar/AppletToolbar.h"

#include <kfiledialog.h>
#include <kstandarddirs.h>
#include <plasma/applet.h>
#include <plasma/containment.h>
#include <plasma/packagestructure.h>
#include <plasma/theme.h>

#include <QApplication>
#include <QDBusInterface>
#include <QGraphicsLinearLayout>
#include <QGraphicsScene>
#include <QPalette>
#include <QSizePolicy>
#include <QWidget>
#include <QTimer>

#define TOOLBAR_X_OFFSET 2000
#define TOOLBAR_SCENE_PADDING 2

Context::ToolbarView::ToolbarView( Plasma::Containment* containment, QGraphicsScene* scene, QWidget* parent )
    : QGraphicsView( scene, parent )
    , m_height( 36 )
    , m_cont( containment )
{
    setObjectName( "ContextToolbarView" );

    setFixedHeight( m_height );
    setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Fixed );
    setAutoFillBackground( true );
    setContentsMargins( 0, 0, 0, 0 );

    setFrameStyle( QFrame::NoFrame );
    applyStyleSheet();

    connect( The::paletteHandler(), SIGNAL(newPalette(QPalette)), SLOT(applyStyleSheet()) );

    //Padding required to prevent view scrolling, probably caused by the 1px ridge
    setSceneRect( TOOLBAR_X_OFFSET, 0, size().width()-TOOLBAR_SCENE_PADDING,
                  size().height()-TOOLBAR_SCENE_PADDING );

    setInteractive( true );
    setAcceptDrops( true );
    setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
    setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOff );

    // now we create the toolbar
    m_toolbar = new AppletToolbar(0);
    scene->addItem(m_toolbar.data());
    m_toolbar.data()->setContainment( qobject_cast<Context::Containment *>(containment) );
    m_toolbar.data()->setZValue( m_toolbar.data()->zValue() + 1000 );
    m_toolbar.data()->setPos( TOOLBAR_X_OFFSET, 0 );

   connect( m_toolbar.data(), SIGNAL(configModeToggled()), SLOT(toggleConfigMode()) );
   connect( m_toolbar.data(), SIGNAL(hideAppletExplorer()), SIGNAL(hideAppletExplorer()) );
   connect( m_toolbar.data(), SIGNAL(showAppletExplorer()), SIGNAL(showAppletExplorer()) );

   Context::Containment* cont = dynamic_cast< Context::Containment* >( containment );
   if( cont )
   {
       connect( cont, SIGNAL(appletAdded(Plasma::Applet*,int)), m_toolbar.data(), SLOT(appletAdded(Plasma::Applet*,int)) );
       connect( m_toolbar.data(), SIGNAL(appletAddedToToolbar(Plasma::Applet*,int)), this, SLOT(appletAdded(Plasma::Applet*,int)) );
       connect( cont, SIGNAL(appletRemoved(Plasma::Applet*)), this, SLOT(appletRemoved(Plasma::Applet*)) );
       connect( m_toolbar.data(), SIGNAL(showApplet(Plasma::Applet*)), cont, SLOT(showApplet(Plasma::Applet*)) );
       connect( m_toolbar.data(), SIGNAL(moveApplet(Plasma::Applet*,int,int)), cont, SLOT(moveApplet(Plasma::Applet*,int,int)) );
   }

}

Context::ToolbarView::~ToolbarView()
{
    delete m_toolbar.data();
}

void
Context::ToolbarView::resizeEvent( QResizeEvent *event )
{
    Q_UNUSED( event )

    setSceneRect( TOOLBAR_X_OFFSET, 0, size().width()-TOOLBAR_SCENE_PADDING,
                  size().height()-TOOLBAR_SCENE_PADDING );

    if( m_toolbar )
        m_toolbar.data()->setGeometry( sceneRect() );
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
Context::ToolbarView::applyStyleSheet() // SLOT
{
    DEBUG_BLOCK

    const QPalette palette = QApplication::palette();

    setStyleSheet( QString( "QFrame#ContextToolbarView { border: 1px ridge %1; " \
                            "background-color: %2; color: %3; border-radius: 3px; }" \
                            "QLabel { color: %3; }" )
                           .arg( palette.color( QPalette::Active, QPalette::Window ).name() )
                           .arg( The::paletteHandler()->highlightColor().name() )
                           .arg( palette.color( QPalette::Active, QPalette::HighlightedText ).name() )
                 );
}

void
Context::ToolbarView::toggleConfigMode()
{
    DEBUG_BLOCK
    if( m_toolbar.data()->configEnabled() ) // set up config stuff
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

        for( int i = 0; i < m_toolbar.data()->appletLayout()->count(); i++ )
        {
            debug() << "creating a move overlay";
            Context::AppletToolbarAppletItem* item = dynamic_cast< Context::AppletToolbarAppletItem* >( m_toolbar.data()->appletLayout()->itemAt( i ) );
            if( item )
            {
                Context::AppletItemOverlay *moveOverlay = new Context::AppletItemOverlay( item, m_toolbar.data()->appletLayout(), this );
                connect( moveOverlay, SIGNAL(moveApplet(Plasma::Applet*,int,int)), m_cont, SLOT(moveApplet(Plasma::Applet*,int,int)) );
                connect( moveOverlay, SIGNAL(moveApplet(Plasma::Applet*,int,int)), this, SLOT(refreshOverlays()) );
                connect( moveOverlay, SIGNAL(deleteApplet(Plasma::Applet*)), this, SLOT(appletRemoved(Plasma::Applet*)) );
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
    m_toolbar.data()->appletRemoved( applet );
    applet->deleteLater();
}

void
Context::ToolbarView::appletAdded( Plasma::Applet* applet, int loc )
{
    DEBUG_BLOCK
    Q_UNUSED( applet )
    Q_UNUSED( loc )

    if( m_toolbar.data()->configEnabled() )
        recreateOverlays();
}


void
Context::ToolbarView::refreshOverlays()
{
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
    for( int i = 0; i < m_toolbar.data()->appletLayout()->count(); i++ )
    {
        debug() << "creating a move overlay";
        Context::AppletToolbarAppletItem* item = dynamic_cast< Context::AppletToolbarAppletItem* >( m_toolbar.data()->appletLayout()->itemAt( i ) );
        if( item )
        {
            Context::AppletItemOverlay *moveOverlay = new Context::AppletItemOverlay( item, m_toolbar.data()->appletLayout(), this );
            connect( moveOverlay, SIGNAL(moveApplet(Plasma::Applet*,int,int)), m_cont, SLOT(moveApplet(Plasma::Applet*,int,int)) );
            connect( moveOverlay, SIGNAL(moveApplet(Plasma::Applet*,int,int)), this, SLOT(refreshOverlays()) );
            connect( moveOverlay, SIGNAL(deleteApplet(Plasma::Applet*)), this, SLOT(appletRemoved(Plasma::Applet*)) );
            moveOverlay->setPalette( p );
            moveOverlay->show();
            moveOverlay->raise();
            m_moveOverlays << moveOverlay;
            debug() << moveOverlay << moveOverlay->geometry();
        }

    }
}

void
Context::ToolbarView::refreshSycoca()
{
    QDBusInterface dbus("org.kde.kded", "/kbuildsycoca", "org.kde.kbuildsycoca");
    dbus.call(QDBus::Block, "recreate");

    recreateOverlays();
}

