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

#include "Amarok.h"
#include "App.h"
#include "AppletItemOverlay.h"
#include "AppletToolbarAddItem.h"
#include "AppletToolbarAppletItem.h"
#include "AppletToolbarConfigItem.h"
#include "Containment.h"
#include "Debug.h"
#include "PaletteHandler.h"

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
    , m_addItem( 0 )
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
    
    m_addItem = new AppletToolbarAddItem( this, m_cont, true );
    connect( cont, SIGNAL( updatedContainment( Containment* ) ), m_addItem, SLOT( updatedContainment( Containment* ) ) );
    connect( m_addItem, SIGNAL( addApplet( const QString&, AppletToolbarAddItem* ) ), this, SLOT( addApplet( const QString&, AppletToolbarAddItem* ) ) );
    
    m_appletLayout->addItem( m_addItem );
    m_appletLayout->setAlignment( m_addItem, Qt::AlignRight );
    m_appletLayout->setContentsMargins( 3, 3, 3, 3 );
    m_appletLayout->setSpacing( 4 );

}

Context::AppletToolbar::~AppletToolbar()
{
}

void
Context::AppletToolbar::paint( QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget )
{
    Q_UNUSED( option )
    Q_UNUSED( widget )

    painter->setRenderHint( QPainter::Antialiasing );
    painter->save();

    QColor col = PaletteHandler::highlightColor();
    qreal radius = 6;

    QPainterPath outline;
    outline.addRoundedRect( boundingRect(), radius, radius );
    painter->fillPath( outline, QBrush( col ) );

    painter->restore();
}

void
Context::AppletToolbar::resizeEvent( QGraphicsSceneResizeEvent * event )
{
    debug() << "setting layout to" << QRectF( QPointF( 0, 0 ), event->newSize() );
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
Context::AppletToolbar::appletLayout()
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
        AppletToolbarAppletItem* app = dynamic_cast< AppletToolbarAppletItem* >( m_appletLayout->itemAt( i ) );
        if( app && app->applet() == applet )
        {
            m_appletLayout->removeItem( app );
            app->deleteLater();
        }
    }
    // if all applets are removed, re-add the add item
    if( m_appletLayout->count() == 2 && m_configMode )
    {
        toggleConfigMode();
        m_appletLayout->removeItem( m_configItem );
        delete m_configItem;
        m_configItem = 0;
        m_appletLayout->insertItem( 0, m_addItem );
        m_addItem->show();
    }
}

QSizeF
Context::AppletToolbar::sizeHint( Qt::SizeHint which, const QSizeF & constraint ) const
{
    Q_UNUSED( which )
    return QSizeF( constraint.width(), 30 );
}


void
Context::AppletToolbar::mousePressEvent( QGraphicsSceneMouseEvent *event )
{
    Q_UNUSED( event )
}

// user clicked on one of the add applet buttons, figure out which one he selected and tell the containment to
// actually add the applet. appletAdded is called by the containment when it has been created.
void 
Context::AppletToolbar::addApplet( const QString& pluginName, Context::AppletToolbarAddItem* item ) // SLOT
{
    DEBUG_BLOCK
    
    int loc = -1; // -1  means at end
    
    if( m_configMode )
    {
        for( int i = 0; i < m_appletLayout->count(); i++ )
        {
            if( item == m_appletLayout->itemAt( i ) )
                loc = i;
        }
        if( loc == -1 )
        {
            warning() << "HELP GOT ADD REQUEST FROM NON-EXISTENT LOCATION";
            return;
        }
    }
    
    debug() << "ADDING APPLET AT LOC:" << loc;
    emit addAppletToContainment( pluginName, loc );
}

// called when the containment is done successfully adding the applet, updates the toolbar
void 
Context::AppletToolbar::appletAdded( Plasma::Applet* applet, int loc ) // SLOT
{
    DEBUG_BLOCK
    
    debug() << "inserting applet icon in position" << loc;
    if( !m_configItem )
    {
        m_configItem = new AppletToolbarConfigItem( this );
        connect( m_configItem, SIGNAL( triggered() ), this, SLOT( toggleConfigMode() ) );
        m_appletLayout->addItem( m_configItem );
        m_appletLayout->setAlignment( m_configItem, Qt::AlignRight );
    }
  
    if( m_configMode )
    {
        // loc doesn't take into account additional + icons, also we need to add 1 more + icon
        Context::AppletToolbarAppletItem* item = new Context::AppletToolbarAppletItem( this, applet );
        item->setConfigEnabled( true );
        connect( item, SIGNAL( appletChosen( Plasma::Applet* ) ), this, SIGNAL( showApplet( Plasma::Applet* ) ) );
        
        // add the item
        m_appletLayout->insertItem( loc, item );
    }
    else
    {
        Context::AppletToolbarAppletItem* item = new Context::AppletToolbarAppletItem( this, applet );
        connect( item, SIGNAL( appletChosen( Plasma::Applet* ) ), this, SIGNAL( showApplet( Plasma::Applet* ) ) );
        m_appletLayout->insertItem( loc, item );
        // since we have an applet, we remove the add applet button
        // would be better to check if it is in there (otherwise we try to erase it on each add)
        // but there is no QGraphicsLinearLayout->contains() or ->indexOf()
        m_addItem->hide();
        m_appletLayout->removeItem( m_addItem );
    }
    // notifications for others who need to know when the layout is done adding the applet
    emit appletAddedToToolbar( applet, loc );
}

void 
Context::AppletToolbar::toggleConfigMode() // SLOT
{
    DEBUG_BLOCK
    if( !m_configMode )
    {
        // place add icons in all possible places that the user can add an icon

        m_configMode = true;

        int loc = -1;                        // location of the configure icon
        int count = m_appletLayout->count(); // save now so we don't check count after adding :)
        for( int i = 0; i < count; i++ ) // tell each applet we are configuring
        {
            QGraphicsLayoutItem *item = m_appletLayout->itemAt( i );

            Context::AppletToolbarAppletItem* appletItem = dynamic_cast< Context::AppletToolbarAppletItem* >( item );
            if( appletItem )
                appletItem->setConfigEnabled( true );

            Context::AppletToolbarConfigItem* configItem = dynamic_cast< Context::AppletToolbarConfigItem* >( item );
            if( configItem )
                loc = i;
        }

        if( loc >= 0 )
            newAddItem( loc );
    }
    else
    {
        for( int i = 0; i < m_appletLayout->count(); i++ ) // tell each applet we are done configuring
        {
            Context::AppletToolbarAppletItem* appletItem = dynamic_cast< Context::AppletToolbarAppletItem* >( m_appletLayout->itemAt( i ) );
            if( appletItem )
                appletItem->setConfigEnabled( false );
        }
        
        // remove all the config stuff
        foreach( AppletToolbarAddItem* item, m_configAddIcons )
        {
            m_appletLayout->removeItem( item );
            item->deleteLater();
        }
        m_configAddIcons.clear();
        
        m_configMode = false;
    }
    emit configModeToggled();
}

void
Context::AppletToolbar::refreshAddIcons() // SLOT
{
    foreach( AppletToolbarAddItem* item, m_configAddIcons )
    {
        m_appletLayout->removeItem( item );
        item->deleteLater();
    }
    m_configAddIcons.clear();

    int loc = -1;
    for( int i = 0; i < m_appletLayout->count(); ++i )
    {
        QGraphicsLayoutItem *item = m_appletLayout->itemAt( i );
        Context::AppletToolbarConfigItem* configItem = dynamic_cast< Context::AppletToolbarConfigItem* >( item );
        if( configItem )
            loc = i;
    }

    if( loc >=0 )
        newAddItem( loc );
}

void 
Context::AppletToolbar::newAddItem( int loc )
{
    DEBUG_BLOCK
    Context::AppletToolbarAddItem* additem = new Context::AppletToolbarAddItem( this, m_cont, false );
    connect( additem, SIGNAL( addApplet( const QString&, AppletToolbarAddItem* ) ), this, SLOT( addApplet( const QString&, AppletToolbarAddItem* ) ) );
    connect( additem, SIGNAL( installApplets() ), this, SIGNAL( installApplets() ) );
    m_appletLayout->insertItem( loc, additem );
    m_configAddIcons << additem;
}

#include "AppletToolbar.moc"
