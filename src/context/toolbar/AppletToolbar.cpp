/****************************************************************************************
 * Copyright (c) 2008 Leo Franchi <lfranchi@kde.org>                                    *
 * Copyright (c) 2009 Mark Kretschmann <kretschmann@kde.org>                            *
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

#include "AppletToolbar.h"

#include "App.h"
#include "PaletteHandler.h"
#include "context/toolbar/AppletToolbarAddItem.h"
#include "context/toolbar/AppletToolbarAppletItem.h"
#include "context/toolbar/AppletToolbarConfigItem.h"
#include "context/Containment.h"
#include "core/support/Amarok.h"
#include "core/support/Debug.h"

#include <QGraphicsScene>
#include <QPainter>
#include <QPalette>
#include <QStyleOptionGraphicsItem>
#include <QGraphicsSceneResizeEvent>
#include <QGraphicsLinearLayout>
#include <QSizePolicy>

Context::AppletToolbar::AppletToolbar( QGraphicsItem* parent )
    : QGraphicsWidget( parent )
    , m_configMode( false )
    , m_appletLayout( 0 )
    , m_cont( 0 )
    , m_configItem( 0 )
{    
    Context::Containment* cont = dynamic_cast<Context::Containment*>( parent );
    if( cont )
    {    
        m_cont = cont;
        debug() << "applettoolbar created with a real containment";
    }
        
    setAcceptDrops( true );

    m_appletLayout = new QGraphicsLinearLayout( Qt::Horizontal, this );
    
    m_appletLayout->setContentsMargins( 3, 3, 3, 3 );
    m_appletLayout->setSpacing( 4 );

    m_configItem = new AppletToolbarConfigItem( this );
    connect( m_configItem, SIGNAL(triggered()), this, SLOT(toggleConfigMode()) );
    m_appletLayout->addItem( m_configItem );
    m_appletLayout->setAlignment( m_configItem, Qt::AlignRight );
    m_appletLayout->setSizePolicy( QSizePolicy::MinimumExpanding, QSizePolicy::Fixed );
}

Context::AppletToolbar::~AppletToolbar()
{
}

void

Context::AppletToolbar::setContainment( Containment * containment )
{
    m_cont = containment;
}

Context::Containment *
Context::AppletToolbar::containment() const
{
    return m_cont;
}

void
Context::AppletToolbar::resizeEvent( QGraphicsSceneResizeEvent *event )
{
    m_appletLayout->setGeometry( QRectF( QPointF( 0, 0 ), event->newSize() ) );
}

QSizePolicy 
Context::AppletToolbar::sizePolicy () const
{
    return QSizePolicy( QSizePolicy::Expanding,  QSizePolicy::Fixed );
}

bool
Context::AppletToolbar::configEnabled() const
{
    return m_configMode;
}

QGraphicsLinearLayout* 
Context::AppletToolbar::appletLayout() const
{
    return m_appletLayout;
}


// this takes care of the cleanup after the applet has been removed from the containment itself
void 
Context::AppletToolbar::appletRemoved( Plasma::Applet* applet )
{
    DEBUG_BLOCK
    for( int i = 0; i < m_appletLayout->count(); i++ )
    {
        AppletToolbarAppletItem* app = static_cast<AppletToolbarAppletItem*>( m_appletLayout->itemAt(i) );
        if( app && app->applet() == applet )
        {
            m_appletLayout->removeItem( app );
            app->deleteLater();
        }
    }
}

QSizeF
Context::AppletToolbar::sizeHint( Qt::SizeHint which, const QSizeF &constraint ) const
{
    Q_UNUSED( which )
    return QSizeF( constraint.width(), constraint.height() );
}


void
Context::AppletToolbar::mousePressEvent( QGraphicsSceneMouseEvent *event )
{
    Q_UNUSED( event )
}

// called when the containment is done successfully adding the applet, updates the toolbar
void 
Context::AppletToolbar::appletAdded( Plasma::Applet* applet, int loc ) // SLOT
{
    DEBUG_BLOCK

    debug() << "inserting applet icon in position" << loc;
    Context::AppletToolbarAppletItem* item = new Context::AppletToolbarAppletItem( this, applet );
    item->setConfigEnabled( m_configMode );
    connect( item, SIGNAL(appletChosen(Plasma::Applet*)),
             this, SIGNAL(showApplet(Plasma::Applet*)) );

    // add the item
    m_appletLayout->insertItem( loc, item );

    // notifications for others who need to know when the layout is done adding the applet
    emit appletAddedToToolbar( applet, loc );
}

void 
Context::AppletToolbar::toggleConfigMode() // SLOT
{
    DEBUG_BLOCK
    if( !m_configMode )
    {
        m_configMode = true;
        emit showAppletExplorer();
    }
    else
    {
        for( int i = 0; i < m_appletLayout->count(); i++ ) // tell each applet we are done configuring
        {
            Context::AppletToolbarAppletItem* appletItem = dynamic_cast< Context::AppletToolbarAppletItem* >( m_appletLayout->itemAt( i ) );
            if( appletItem )
                appletItem->setConfigEnabled( false );
        }

        m_configMode = false;

        emit hideAppletExplorer();
    }
    emit configModeToggled();
}

