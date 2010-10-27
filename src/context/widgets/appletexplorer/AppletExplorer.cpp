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

#define DEBUG_PREFIX "AppletExplorer"

#include "AppletExplorer.h"

#include "AppletIcon.h"
#include "core/support/Debug.h"
#include "PaletteHandler.h"

#include <KIcon>
#include <Plasma/Applet>
#include <Plasma/ScrollWidget>

#include <QGraphicsLinearLayout>
#include <QStyleOptionGraphicsItem>
#include <QSignalMapper>

#define HEIGHT 120

namespace Context
{
    
AppletExplorer::AppletExplorer( QGraphicsItem *parent )
    : QGraphicsWidget( parent )
    , m_containment( 0 )
    , m_scrollWidget( 0 )
{
    init();
}

AppletExplorer::~AppletExplorer()
{}

void
AppletExplorer::addApplet( const QString &name )
{
    DEBUG_BLOCK
    if( !name.isEmpty() && containment() )
        emit addAppletToContainment( name, -1 ); //always add the applet at the end
}

void
AppletExplorer::hideMenu()
{
    hide();
    emit appletExplorerHid();
}

void
AppletExplorer::init()
{
    QGraphicsLinearLayout *layout = new QGraphicsLinearLayout( Qt::Vertical, this );
    QSignalMapper *iconTriggerMapper = new QSignalMapper( this );
    QGraphicsWidget *scrollView = new QGraphicsWidget( this );
    m_scrollWidget = new Plasma::ScrollWidget( this );
    m_scrollWidget->setWidget( scrollView );
    m_scrollWidget->setHorizontalScrollBarPolicy( Qt::ScrollBarAsNeeded );
    QGraphicsLinearLayout *scrollLayout = new QGraphicsLinearLayout( scrollView );

    foreach( AppletIconWidget *widget, listAppletWidgets() )
    {
        scrollLayout->addItem( widget );
        widget->setMinimumSize( widget->sizeFromIconSize( 48 ) );
        widget->setMaximumSize( widget->sizeFromIconSize( 48 ) );
        connect( widget, SIGNAL(clicked()), iconTriggerMapper, SLOT(map()) );
        iconTriggerMapper->setMapping( widget, widget->pluginName() );
    }

    connect( iconTriggerMapper, SIGNAL(mapped(QString)), SLOT(addApplet(QString)) );

    m_hideIcon = new Plasma::IconWidget( this );
    
    m_hideIcon->setIcon( KIcon( "window-close" ) );
    m_hideIcon->setToolTip( i18n( "Hide menu" ) );

    connect( m_hideIcon, SIGNAL( clicked() ), this, SLOT( hideMenu() ) );
    m_hideIcon->setMinimumSize( m_hideIcon->sizeFromIconSize( 16 ) );
    m_hideIcon->setMaximumSize( m_hideIcon->sizeFromIconSize( 16 ) );
    
    layout->addItem( m_hideIcon );
    layout->addItem( m_scrollWidget );
    layout->setAlignment( m_hideIcon, Qt::AlignRight );
    
    setMaximumHeight( HEIGHT );
    setMinimumHeight( HEIGHT );
}

void
AppletExplorer::paint( QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget )
{
    Q_UNUSED( option )
    Q_UNUSED( widget )

    painter->setRenderHint( QPainter::Antialiasing );
    painter->save();
    
    painter->setOpacity( 0.9 );
    
    QLinearGradient gradient( boundingRect().topLeft().x(), boundingRect().topLeft().y(),
                              boundingRect().bottomLeft().x(), boundingRect().bottomLeft().y() / 1.8 + 3 );
                              
    QColor highlight = PaletteHandler::highlightColor();
    gradient.setSpread( QGradient::RepeatSpread );
    gradient.setColorAt( 0, highlight.lighter( 100 ) );
    gradient.setColorAt( 1, highlight.lighter( 140 ) ); 
    QPainterPath path;
    path.addRoundedRect( boundingRect(), 6, 6 );
    painter->fillPath( path, gradient );
    painter->restore();

    // draw border
    painter->save();
    painter->translate( 0.5, 0.5 );
    QPen pen( PaletteHandler::highlightColor().lighter( 140 ) );
    pen.setWidth( 3 );
    painter->setPen( pen );
    painter->drawRoundedRect( boundingRect(), 6, 6 );
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

QList<AppletIconWidget*>
AppletExplorer::listAppletWidgets()
{
    QList<AppletIconWidget*> widgets;
    foreach( const KPluginInfo &info, Plasma::Applet::listAppletInfo( QString(), "amarok" ) )
    {
        if( info.property( "NoDisplay" ).toBool() || info.category() == i18n( "Containments" ) )
            continue;

        widgets << new AppletIconWidget( info, this );
    }
    return widgets;
}

} //namespace Context

#include "AppletExplorer.moc"
