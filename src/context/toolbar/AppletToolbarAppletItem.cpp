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

#include "AppletToolbarAppletItem.h"

#include "Debug.h"

#include <plasma/applet.h>
#include <plasma/widgets/iconwidget.h>

#include <KIcon>

#include <QAction>
#include <QBitmap>
#include <QDrag>
#include <QMimeData>
#include <QStyleOptionGraphicsItem>
#include <QPainter>
#include <QGraphicsSceneMouseEvent>


Context::AppletToolbarAppletItem::AppletToolbarAppletItem( QGraphicsItem* parent, Plasma::Applet* applet )
    : QGraphicsWidget( parent )
    , m_applet( applet )
    , m_label( 0 )
    , m_deleteIcon( 0 )
    , m_labelPadding( 5 )
    , m_configEnabled( false )
{
    m_label = new QGraphicsSimpleTextItem( this );
    if( m_applet )
       m_label->setText( m_applet->name() );
    else
        m_label->setText( "no applet name" );
        
    QAction* delApplet = new QAction( i18n( "Remove Applet" ), this );
    delApplet->setIcon( KIcon( "edit-delete" ) );
    delApplet->setVisible( true );
    delApplet->setEnabled( true );

    connect( delApplet, SIGNAL( triggered() ), this, SLOT( deleteApplet() ) );
    m_deleteIcon = addAction( delApplet, 18 );
    m_deleteIcon->hide();
    
    setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Expanding );
}

Context::AppletToolbarAppletItem::~AppletToolbarAppletItem()
{
    
}

void 
Context::AppletToolbarAppletItem::paint ( QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget )
{
    //DEBUG_BLOCK
    painter->save();
    painter->setRenderHint( QPainter::Antialiasing );
    QColor fillColor( 102, 102, 102, 210 );
    QPainterPath fillPath;
    fillPath.addRoundedRect( boundingRect(), 5, 5 );
    painter->fillPath( fillPath ,fillColor );
    painter->restore();
      
}

void 
Context::AppletToolbarAppletItem::setConfigEnabled( bool config )
{
    if( config && !m_configEnabled ) // switching to config mode
    {
        // center over top-right corner
        m_deleteIcon->setPos( ( boundingRect().width() - (m_deleteIcon->boundingRect().width() ) ) - 1, -1);
     //   m_deleteIcon->show();

    } else
        m_deleteIcon->hide();
    
    m_configEnabled = config;
}

bool 
Context::AppletToolbarAppletItem::configEnabled()
{
    return m_configEnabled;
}

QRectF 
Context::AppletToolbarAppletItem::delIconSceneRect()
{
    return mapToScene( m_deleteIcon->boundingRect() ).boundingRect();
}

void 
Context::AppletToolbarAppletItem::resizeEvent( QGraphicsSceneResizeEvent * event )
{
    if( m_configEnabled )
        m_deleteIcon->setPos( ( boundingRect().width() - (m_deleteIcon->boundingRect().width() ) ) - 1, -1);
    m_label->setPos( ( boundingRect().width() / 2 ) - ( m_label->boundingRect().width() / 2 ),  ( boundingRect().height() / 2 ) - ( m_label->boundingRect().height() / 2 ) );
    
    emit geometryChanged();
}


QVariant 
Context::AppletToolbarAppletItem::itemChange(GraphicsItemChange change, const QVariant &value)
{
    QVariant ret = QGraphicsWidget::itemChange(change, value);
    
    if( change == ItemPositionHasChanged )
        emit geometryChanged();
        
    return ret;
}

QSizeF 
Context::AppletToolbarAppletItem::sizeHint( Qt::SizeHint which, const QSizeF & constraint ) const
{
    if( which == Qt::MinimumSize )
        return QSizeF( m_label->boundingRect().width() + 2 * m_labelPadding, QGraphicsWidget::sizeHint( which, constraint ).height() );
    else
       // return QGraphicsWidget::sizeHint( which, constraint );
        return QSizeF( 10000, 10000 );
}

void 
Context::AppletToolbarAppletItem::mousePressEvent( QGraphicsSceneMouseEvent * event )
{
    emit appletChosen( m_applet );
    event->accept();
}

void 
Context::AppletToolbarAppletItem::deleteApplet()
{
    DEBUG_BLOCK
    m_applet->deleteLater();
}

Plasma::IconWidget*
Context::AppletToolbarAppletItem::addAction( QAction *action, int size )
{
    if ( !action ) {
        debug() << "ERROR!!! PASSED INVALID ACTION";
        return 0;
    }

    Plasma::IconWidget *tool = new Plasma::IconWidget( this );

    tool->setAction( action );
    tool->setText( QString() );
    tool->setToolTip( action->text() );
    tool->setDrawBackground( false );
    tool->setOrientation( Qt::Horizontal );
    QSizeF iconSize = tool->sizeFromIconSize( size );
    tool->setMinimumSize( iconSize );
    tool->setMaximumSize( iconSize );
    tool->resize( iconSize );

    tool->hide();
    tool->setZValue( zValue() + 1000 );

    return tool;
}

#include "AppletToolbarAppletItem.moc"
