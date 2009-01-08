/***************************************************************************
* copyright            : (C) 2008 Leo Franchi <lfranchi@kde.org>         *
****************************************************************************/

/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#include "AppletToolbarAddItem.h"

#include "widgets/ToolboxMenu.h"
#include "Debug.h"

#include <KIcon>

#include <QAction>
#include <QGraphicsSceneResizeEvent>
#include <QGraphicsSimpleTextItem>
#include <QSizeF>
#include <QStyleOptionGraphicsItem>
#include <QSizeF>
#include <QPainter>

Context::AppletToolbarAddItem::AppletToolbarAddItem( QGraphicsItem* parent, Context::Containment* cont, bool maximizeHorizontally )
    : QGraphicsWidget( parent )
    , m_iconPadding( 5 )
    , m_maximizeHorizontally( maximizeHorizontally )
    , m_icon( 0 )
    , m_label( 0 )
    , m_addMenu( 0 )
{
    QAction* listAdd = new QAction( i18n( "Add Widgets..." ), this );
    listAdd->setIcon( KIcon( "list-add" ) );
    listAdd->setVisible( true );
    listAdd->setEnabled( true );
    
    connect( listAdd, SIGNAL( triggered() ), this, SLOT( iconClicked() ) );
    
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
    
    m_label = new QGraphicsSimpleTextItem( "Add Applet", this );
    m_label->hide();
    
    m_addMenu = new AmarokToolBoxMenu( this, false );
    m_addMenu->setContainment( cont );
}

Context::AppletToolbarAddItem::~AppletToolbarAddItem()
{
    
}

void 
Context::AppletToolbarAddItem::paint ( QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget )
{
  //  DEBUG_BLOCK
    
    painter->save();
    QColor fillColor( 88, 88, 88, 225 );
    QPainterPath fillPath;
    fillPath.addRoundedRect( boundingRect(), 5, 5 );
    painter->fillPath( fillPath ,fillColor );
    painter->restore();
}


QSizePolicy 
Context::AppletToolbarAddItem::sizePolicy () const
{
    if( m_maximizeHorizontally )
        return QSizePolicy( QSizePolicy::Expanding, QSizePolicy::Expanding );
    else
        return QSizePolicy( QSizePolicy::Fixed, QSizePolicy::Fixed );
}


void 
Context::AppletToolbarAddItem::setMaximized( bool max )
{
    debug() << "add icon no longer maximizing";
    m_maximizeHorizontally = max;
}

void 
Context::AppletToolbarAddItem::updatedContainment( Containment* cont )
{
    m_addMenu->setContainment( cont );
}

void 
Context::AppletToolbarAddItem::iconClicked() // SLOT
{
    showAddAppletsMenu( m_icon->pos() );
}

void 
Context::AppletToolbarAddItem::resizeEvent( QGraphicsSceneResizeEvent * event )
{
    if( m_label->boundingRect().width() < ( boundingRect().width() - 2*m_icon->boundingRect().width() ) ) // do we have size to show it?
    {
        m_icon->setPos( 0, ( boundingRect().height() / 2 ) - ( m_icon->size().height() / 2 ) );
        m_label->setPos( ( boundingRect().width() / 2 ) - ( m_label->boundingRect().width() / 2 ),  ( boundingRect().height() / 2 ) - ( m_label->boundingRect().height() / 2 ) );
        m_label->show();
    } else 
    {        
        m_icon->setPos( ( boundingRect().width() / 2 ) - ( m_icon->boundingRect().width() / 2 ) , ( boundingRect().height() / 2 ) - ( m_icon->size().height() / 2 ) );
        m_label->hide();
    }
}


QSizeF
Context::AppletToolbarAddItem::sizeHint( Qt::SizeHint which, const QSizeF & constraint ) const
{
    if( m_maximizeHorizontally )
        return QGraphicsWidget::sizeHint( which, constraint );
    else
        return QSizeF( m_icon->size().width() /* + 2 * m_iconPadding */, QGraphicsWidget::sizeHint( which, constraint ).height() );
    
}

void 
Context::AppletToolbarAddItem::mousePressEvent( QGraphicsSceneMouseEvent * event )
{    
    showAddAppletsMenu( event->pos() );
    event->accept();
}

void
Context::AppletToolbarAddItem::showAddAppletsMenu( QPointF pos )
{
    DEBUG_BLOCK
    if( m_addMenu->showing() )
    {   // hide again on double-click
        m_addMenu->hide();
        return;
    }
    
    qreal xpos = pos.x();
    const qreal ypos = 0 - m_addMenu->boundingRect().height();
    
    debug() << "checking if it will overflow:"  << xpos + m_addMenu->boundingRect().width() << " > " << sceneBoundingRect().width() ;
    if( xpos + m_addMenu->boundingRect().width() > sceneBoundingRect().width() )
        xpos = sceneBoundingRect().width() - m_addMenu->boundingRect().width();

    m_addMenu->setPos( xpos, ypos );
    m_addMenu->show();
}

#include "AppletToolbarAddItem.moc"
