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

#include "AppletsList.h"
#include "Debug.h"

#include <KIcon>

#include <cmath>

#define ICON_SIZE 70
#define ARROW_SIZE 15

namespace Context
{

AppletsListWidget::AppletsListWidget( QGraphicsItem *parent )
    : QGraphicsWidget( parent )
{
    init();
}

AppletsListWidget::~AppletsListWidget()
{}


void
AppletsListWidget::init()
{    
    m_mainLayout = new QGraphicsLinearLayout( Qt::Horizontal );

    m_leftArrow = new Plasma::IconWidget();
    m_rightArrow = new Plasma::IconWidget();

    m_leftArrow->setIcon( KIcon( "go-previous" ) );
    m_rightArrow->setIcon( KIcon( "go-next" ) );
    
    m_leftArrow->setDrawBackground( false );
    m_rightArrow->setDrawBackground( false );

    m_leftArrow->setMinimumSize( m_leftArrow->sizeFromIconSize( ARROW_SIZE ) );
    m_leftArrow->setMaximumSize( m_leftArrow->sizeFromIconSize( ARROW_SIZE ) );
    m_rightArrow->setMinimumSize( m_rightArrow->sizeFromIconSize( ARROW_SIZE ) );
    m_rightArrow->setMaximumSize( m_rightArrow->sizeFromIconSize( ARROW_SIZE ) );
    
    m_appletsListWidget = new QGraphicsWidget();
    m_appletsListWindow = new QGraphicsWidget();
    
    m_appletsListLayout = new QGraphicsLinearLayout( Qt::Horizontal );
    m_appletsListWidget->setLayout( m_appletsListLayout );
    

    m_appletsListWindow->setFlag( QGraphicsItem::ItemClipsChildrenToShape, true );
    m_appletsListWidget->setParentItem( m_appletsListWindow );

    m_mainLayout->addItem( m_leftArrow );
    m_mainLayout->addItem( m_appletsListWindow );
    m_mainLayout->addItem( m_rightArrow );

    m_mainLayout->setAlignment( m_leftArrow, Qt::AlignVCenter | Qt::AlignHCenter );
    m_mainLayout->setAlignment( m_rightArrow, Qt::AlignVCenter | Qt::AlignHCenter );
    m_mainLayout->setAlignment( m_appletsListWindow, Qt::AlignVCenter | Qt::AlignHCenter );
    setLayout( m_mainLayout );
    
}

void
AppletsListWidget::appletIconDoubleClicked( AppletIconWidget *applet )
{
    emit( appletDoubleClicked( applet->appletItem() ) );
}

AppletIconWidget *
AppletsListWidget::createAppletIcon( AppletItem *appletItem )
{
    AppletIconWidget *applet = new AppletIconWidget( appletItem );
    applet->setMinimumSize( applet->sizeFromIconSize( ICON_SIZE ) );
    applet->setMaximumSize( applet->sizeFromIconSize( ICON_SIZE ) );
    
    connect( applet, SIGNAL( doubleClicked( AppletIconWidget * ) ), this, SLOT( appletIconDoubleClicked( AppletIconWidget * ) ) );
    return applet;
}

void
AppletsListWidget::insertAppletIcon( AppletIconWidget *appletIcon )
{
    appletIcon->setVisible( true );
    m_appletsListLayout->addItem( appletIcon );
    m_appletsListLayout->setAlignment( appletIcon, Qt::AlignHCenter );
    m_appletsListLayout->setAlignment( appletIcon, Qt::AlignVCenter );
}

int
AppletsListWidget::maximumVisibleAppletsOnList()
{
    DEBUG_BLOCK
    qreal listTotalSize = m_appletsListLayout->preferredSize().width();
    qreal iconAverageSize = listTotalSize / m_model->rowCount() + m_appletsListLayout->spacing();
    qreal visibleRectSize = boundingRect().width();
    debug() << "model row count: " << m_model->rowCount();
    debug() << "icon average size: " << iconAverageSize;
    debug() << "visible rect size: " << visibleRectSize;
    debug() << "listTotalSize: " << listTotalSize;
    return floor( visibleRectSize / iconAverageSize );
}


void
AppletsListWidget::resizeEvent( QGraphicsSceneResizeEvent *event )
{
    DEBUG_BLOCK
    Q_UNUSED( event );

    updateGeometry();
    debug() << "visible icons:" << maximumVisibleAppletsOnList();
}


void
AppletsListWidget::setModel( QStandardItemModel *model )
{
    m_model = model;
    AppletItem *appletItem;
    m_applets = new QHash< QString, AppletIconWidget * >();
    m_model->sort( 0 );
    
    for( int i = 0; i < m_model->rowCount(); i++ )
    {
        appletItem = ( AppletItem * )m_model->item( i );
        m_applets->insert( appletItem->pluginName(), createAppletIcon( appletItem ) );
    }
    updateList();

}


void
AppletsListWidget::updateList()
{
    DEBUG_BLOCK
    AppletItem *appletItem;
    AppletIconWidget *appletIcon;
    m_appletsListLayout = new QGraphicsLinearLayout( Qt::Horizontal );
 
    m_appletsListWidget->setLayout( NULL );
    m_appletsListLayout = new QGraphicsLinearLayout( Qt::Horizontal );

    for( int i = 0; i < m_model->rowCount(); i++ )
    {
        appletItem = ( AppletItem * )m_model->item( i );
        if( appletItem != 0 )
        {
            appletIcon = m_applets->value( appletItem->pluginName() );
            insertAppletIcon( appletIcon );
        }
    }
    m_appletsListWidget->setLayout( m_appletsListLayout );
    m_appletsListLayout->setSpacing( 10 );
    debug() << "visible icons:" << maximumVisibleAppletsOnList();
}


}

#include "AppletsList.moc"