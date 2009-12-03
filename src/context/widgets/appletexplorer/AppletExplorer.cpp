/****************************************************************************************
 * Copyright (c) 2009 William Viana Soares <vianasw@gmail.com>                          *
 *                       Significant parts of this code is inspired                     *
 *                       and/or copied from KDE Plasma sources, available               *
 *                       at kdebase/workspace/libs/plasmagenericshell                   *
 *                                                                                      *
 ****************************************************************************************/
 
/****************************************************************************************
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


#include "AppletExplorer.h"

#include "AppletIcon.h"
#include "Debug.h"
#include "PaletteHandler.h"

#include <plasma/containment.h>
#include <plasma/widgets/pushbutton.h>
#include <plasma/widgets/label.h>

#include <KIcon>

#include <QAction>
#include <QStyleOptionGraphicsItem>
#include <QSizePolicy>

#define HEIGHT 140
#define ICON_SIZE 16


namespace Context
{
    
AppletExplorer::AppletExplorer( QGraphicsItem *parent )
    : QGraphicsWidget( parent )
    , m_containment( 0 )
    , m_mainLayout( 0 )
{
    init();
}

AppletExplorer::~AppletExplorer()
{}

void
AppletExplorer::addApplet( AppletItem *appletItem )
{
    if( appletItem && !appletItem->pluginName().isEmpty() && containment() )
        emit( addAppletToContainment( appletItem->pluginName() ) );
}

void
AppletExplorer::hideMenu()
{
    hide();
}

void
AppletExplorer::init()
{
    m_mainLayout = new QGraphicsLinearLayout( Qt::Vertical );

    m_appletsListWidget = new AppletsListWidget();

    m_appletsListWidget->setPreferredSize( -1, -1 );

    connect( m_appletsListWidget, SIGNAL( appletClicked( AppletItem * ) ), SLOT( addApplet( AppletItem * ) ) );

    m_appletsListWidget->setModel( &m_model );

    m_hideIcon = new Plasma::IconWidget( this );
    
    m_hideIcon->setIcon( KIcon( "window-close" ) );
    m_hideIcon->setToolTip( i18n( "Hide menu" ) );

    connect( m_hideIcon, SIGNAL( clicked() ), this, SLOT( hideMenu() ) );
    m_hideIcon->setMinimumSize( m_hideIcon->sizeFromIconSize( ICON_SIZE ) );
    m_hideIcon->setMaximumSize( m_hideIcon->sizeFromIconSize( ICON_SIZE ) );
    
    m_mainLayout->addItem( m_hideIcon );
    m_mainLayout->addItem( m_appletsListWidget );
    m_mainLayout->setAlignment( m_hideIcon, Qt::AlignLeft );
    m_mainLayout->setAlignment( m_appletsListWidget, Qt::AlignTop | Qt::AlignHCenter );
    
    setMaximumHeight( HEIGHT );
    setMinimumHeight( HEIGHT );

    setLayout( m_mainLayout );
}

void
AppletExplorer::paint( QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget )
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
AppletExplorer::setContainment( Containment *containment )
{
    m_containment = containment;
}

Containment *
AppletExplorer::containment() const
{
    return m_containment;
}

void
AppletExplorer::resizeEvent( QGraphicsSceneResizeEvent *event )
{
    //FIXME This method is never actually called

    DEBUG_BLOCK

    m_mainLayout->setGeometry( QRectF( QPointF( 0, 0 ), event->newSize() ) );
}

}//namespace Context

#include "AppletExplorer.moc"
