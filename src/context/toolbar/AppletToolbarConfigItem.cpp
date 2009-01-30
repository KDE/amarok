/**************************************************************************
* copyright            : (C) 2008 Leo Franchi <lfranchi@kde.org  >        *
**************************************************************************/

/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#include "AppletToolbarConfigItem.h"

#include "plasma/widgets/iconwidget.h"

#include <KIcon>

#include <QAction>
#include <QPainter>
#include <QSizePolicy>
#include <QStyleOptionGraphicsItem>

Context::AppletToolbarConfigItem::AppletToolbarConfigItem( QGraphicsItem* parent )
    : QGraphicsWidget( parent )
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
    
}

Context::AppletToolbarConfigItem::~AppletToolbarConfigItem()
{}

void 
Context::AppletToolbarConfigItem::paint ( QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget )
{
    painter->save();
    painter->setRenderHint( QPainter::Antialiasing );
    QColor fillColor( 88, 88, 88, 225 );
    QPainterPath fillPath;
    fillPath.addRoundedRect( boundingRect(), 5, 5 );
    painter->fillPath( fillPath ,fillColor );
    painter->restore();
}
    
QSizePolicy 
Context::AppletToolbarConfigItem::sizePolicy () const
{
    return QSizePolicy( QSizePolicy::Fixed, QSizePolicy::Fixed );
}

      
void 
Context::AppletToolbarConfigItem::resizeEvent( QGraphicsSceneResizeEvent * event )
{
    // center horizontally and vertically
    m_icon->setPos( ( boundingRect().width() / 2 ) - ( m_icon->boundingRect().width() / 2 ) , ( boundingRect().height() / 2 ) - ( m_icon->size().height() / 2 ) );
}

QSizeF 
Context::AppletToolbarConfigItem::sizeHint( Qt::SizeHint which, const QSizeF & constraint ) const
{
    return QSizeF( m_icon->size().width() + 2 * m_iconPadding , QGraphicsWidget::sizeHint( which, constraint ).height() );
}

void
Context::AppletToolbarConfigItem::mousePressEvent( QGraphicsSceneMouseEvent * event )
{
    emit triggered();
}

#include "AppletToolbarConfigItem.moc"
