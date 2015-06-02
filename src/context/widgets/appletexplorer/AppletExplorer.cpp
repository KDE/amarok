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

#include <QIcon>
#include <Plasma/Applet>
#include <Plasma/ScrollWidget>

#include <QGraphicsLinearLayout>
#include <QGraphicsScene>
#include <QGraphicsSceneWheelEvent>
#include <QGraphicsProxyWidget>
#include <QLabel>
#include <QStyleOptionGraphicsItem>
#include <QSignalMapper>

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
    // m_scrollWidget->setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
    m_scrollWidget->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Minimum );
    QGraphicsLinearLayout *scrollLayout = new QGraphicsLinearLayout( scrollView );
    m_scrollWidget->setMinimumHeight( 0 );

    foreach( AppletIconWidget *widget, listAppletWidgets() )
    {
        scrollLayout->addItem( widget );
        scrollLayout->setAlignment( widget, Qt::AlignCenter );
        widget->setMinimumSize( widget->sizeFromIconSize( 48 ) );
        widget->setMaximumSize( widget->sizeFromIconSize( 48 ) );
        connect( widget, SIGNAL(clicked()), iconTriggerMapper, SLOT(map()) );
        iconTriggerMapper->setMapping( widget, widget->pluginName() );
    }
    connect( iconTriggerMapper, SIGNAL(mapped(QString)), SLOT(addApplet(QString)) );

    Plasma::IconWidget *appletIcon = new Plasma::IconWidget( this );
    appletIcon->setIcon( QIcon::fromTheme( "preferences-plugin" ) );
    const QSizeF iconSize = appletIcon->sizeFromIconSize( 22 );
    appletIcon->setMinimumSize( iconSize );
    appletIcon->setMaximumSize( iconSize );

    Plasma::IconWidget *hideIcon = new Plasma::IconWidget( this );
    hideIcon->setIcon( QIcon::fromTheme( "window-close" ) );
    hideIcon->setToolTip( i18n( "Hide menu" ) );
    hideIcon->setMinimumSize( iconSize );
    hideIcon->setMaximumSize( iconSize );
    connect( hideIcon, SIGNAL(clicked()), this, SLOT(hideMenu()) );

    Plasma::IconWidget *forwardIcon = new Plasma::IconWidget( this );
    forwardIcon->setIcon( QIcon::fromTheme( "go-next" ) );
    forwardIcon->setMinimumSize( iconSize );
    forwardIcon->setMaximumSize( iconSize );
    connect( forwardIcon, SIGNAL(clicked()), this, SLOT(scrollRight()) );

    Plasma::IconWidget *backIcon = new Plasma::IconWidget( this );
    backIcon->setIcon( QIcon::fromTheme( "go-previous" ) );
    backIcon->setMinimumSize( iconSize );
    backIcon->setMaximumSize( iconSize );
    connect( backIcon, SIGNAL(clicked()), this, SLOT(scrollLeft()) );

    QLabel *titleLabel = new QLabel( i18n("<strong>Applet Explorer</strong>") );
    titleLabel->setAttribute( Qt::WA_NoSystemBackground );
    titleLabel->setWordWrap( false );
    QGraphicsProxyWidget *titleWidget = new QGraphicsProxyWidget( this );
    titleWidget->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Preferred );
    titleWidget->setWidget( titleLabel );

    QGraphicsLinearLayout *headerLayout = new QGraphicsLinearLayout;
    headerLayout->addItem( appletIcon );
    headerLayout->addItem( titleWidget );
    headerLayout->addItem( backIcon );
    headerLayout->addItem( forwardIcon );
    headerLayout->addItem( hideIcon );
    headerLayout->setAlignment( appletIcon, Qt::AlignLeft | Qt::AlignTop );
    headerLayout->setAlignment( titleWidget, Qt::AlignLeft | Qt::AlignTop );
    headerLayout->setAlignment( backIcon, Qt::AlignRight | Qt::AlignTop );
    headerLayout->setAlignment( forwardIcon, Qt::AlignRight | Qt::AlignTop );
    headerLayout->setAlignment( hideIcon, Qt::AlignRight | Qt::AlignTop );
    headerLayout->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Maximum );
    
    layout->addItem( headerLayout );
    layout->addItem( m_scrollWidget );
    layout->setAlignment( headerLayout, Qt::AlignTop );
    layout->setAlignment( m_scrollWidget, Qt::AlignCenter );
}

QSizeF
AppletExplorer::sizeHint( Qt::SizeHint which, const QSizeF &constraint ) const
{
    QSizeF sz = QGraphicsWidget::sizeHint( which, constraint );
    return QSizeF( sz.width(), sz.height() + 2 );
}

void
AppletExplorer::paint( QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget )
{
    Q_UNUSED( option )
    Q_UNUSED( widget )

    painter->save();
    painter->setRenderHint( QPainter::Antialiasing );
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
    painter->setRenderHint( QPainter::Antialiasing );
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

void
AppletExplorer::scrollLeft()
{
    QGraphicsSceneWheelEvent event( QEvent::GraphicsSceneWheel );
    event.setDelta( 480 );
    scene()->sendEvent( m_scrollWidget, &event );
}

void
AppletExplorer::scrollRight()
{
    QGraphicsSceneWheelEvent event( QEvent::GraphicsSceneWheel );
    event.setDelta( -480 );
    scene()->sendEvent( m_scrollWidget, &event );
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

