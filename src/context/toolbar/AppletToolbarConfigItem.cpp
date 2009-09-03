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

#include "AppletToolbarConfigItem.h"

#include "App.h"
#include "PaletteHandler.h"

#include <plasma/widgets/iconwidget.h>

#include <KIcon>

#include <QAction>
#include <QPainter>
#include <QSizePolicy>
#include <QStyleOptionGraphicsItem>

Context::AppletToolbarConfigItem::AppletToolbarConfigItem( QGraphicsItem* parent )
    : AppletToolbarBase( parent )
    , m_iconPadding( 2 )
    , m_icon( 0 )
{
    QAction* listAdd = new QAction( i18n( "Configure Applets..." ), this );
    listAdd->setIcon( KIcon( "configure" ) );
    listAdd->setVisible( true );
    listAdd->setEnabled( true );
    
    connect( listAdd, SIGNAL( triggered() ), this, SIGNAL( triggered() ) );
    
    m_icon = new Plasma::IconWidget( this );

    m_icon->setAction( listAdd );
    m_icon->setText( QString() );
    m_icon->setToolTip( listAdd->text() );
    m_icon->setDrawBackground( false );
    m_icon->setOrientation( Qt::Horizontal );
    QSizeF iconSize = m_icon->sizeFromIconSize( 22 );
    m_icon->setMinimumSize( iconSize );
    m_icon->setMaximumSize( iconSize );
    m_icon->resize( iconSize );
    m_icon->setZValue( zValue() + 1 );

    setSizePolicy( QSizePolicy::Fixed, QSizePolicy::Preferred );
    
}

Context::AppletToolbarConfigItem::~AppletToolbarConfigItem()
{}
   
void 
Context::AppletToolbarConfigItem::resizeEvent( QGraphicsSceneResizeEvent * event )
{
    Q_UNUSED( event )
    // center horizontally and vertically
    m_icon->setPos( ( boundingRect().width() / 2 ) - ( m_icon->boundingRect().width() / 2 ) , ( boundingRect().height() / 2 ) - ( m_icon->size().height() / 2 ) );
}

QSizeF 
Context::AppletToolbarConfigItem::sizeHint( Qt::SizeHint which, const QSizeF & constraint ) const
{
    return QSizeF( m_icon->size().width() + 2 * m_iconPadding , QGraphicsWidget::sizeHint( which, constraint ).height() );
}

void
Context::AppletToolbarConfigItem::mousePressEvent( QGraphicsSceneMouseEvent *event )
{
    Q_UNUSED( event )
    emit triggered();
}

#include "AppletToolbarConfigItem.moc"
