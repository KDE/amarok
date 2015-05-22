/****************************************************************************************
 * Copyright (c) 2008 Leo Franchi <lfranchi@kde.org>                                    *
 * Copyright (c) 2009 Simon Esneault <simon.esneault@gmail.com>                         *
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

#include "AppletToolbarAppletItem.h"

#include "core/support/Amarok.h"
#include "core/support/Debug.h"
#include "PaletteHandler.h"

#include <Plasma/Applet>
#include <Plasma/IconWidget>

#include <KIcon>

#include <QAction>
#include <QPainter>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsTextItem>
#include <QPropertyAnimation>


Context::AppletToolbarAppletItem::AppletToolbarAppletItem( QGraphicsItem* parent, Plasma::Applet* applet )
    : AppletToolbarBase( parent )
    , m_applet( applet )
    , m_label( 0 )
    , m_deleteIcon( 0 )
    , m_configEnabled( false )
{
    m_label = new QGraphicsTextItem( this );

    // Don't propagate opacity changes to the text label, as this reduces readability
    m_label->setFlags( QGraphicsItem::ItemIgnoresParentOpacity );

    if( m_applet )
    {
       m_label->setPlainText( m_applet->name() );
    }
    else
    {
        m_label->setPlainText( i18n("no applet name") );
    }

    setAcceptHoverEvents( true );
    m_label->setAcceptHoverEvents( true );
    QAction* delApplet = new QAction( i18n( "Remove Applet" ), this );
    delApplet->setIcon( KIcon( "edit-delete" ) );
    delApplet->setVisible( true );
    delApplet->setEnabled( true );

    connect( delApplet, SIGNAL(triggered()), this, SLOT(deleteApplet()) );
    m_deleteIcon = addAction( delApplet, 18 );
    m_deleteIcon->hide();

    setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Expanding );

    paletteChanged( palette() );
    connect( The::paletteHandler(), SIGNAL(newPalette(QPalette)), SLOT(paletteChanged(QPalette)) );
}

Context::AppletToolbarAppletItem::~AppletToolbarAppletItem()
{
}

void
Context::AppletToolbarAppletItem::setConfigEnabled( bool config )
{
    if( config && !m_configEnabled ) // switching to config mode
    {
        // center over top-right corner
        m_deleteIcon->setPos( ( boundingRect().width() - (m_deleteIcon->boundingRect().width() ) ) - 1, -1 );
    }
    else
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
Context::AppletToolbarAppletItem::resizeEvent( QGraphicsSceneResizeEvent *event )
{
    Q_UNUSED( event )
    QFontMetrics fm( m_label->font() );
    if( m_configEnabled )
    {
        m_deleteIcon->setPos( ( boundingRect().width() - (m_deleteIcon->boundingRect().width() ) ) - 1, -1 );

        if( fm.width( m_applet->name() ) + m_deleteIcon->boundingRect().width() > boundingRect().width() )
            m_label->setPlainText( fm.elidedText( m_applet->name(), Qt::ElideRight, boundingRect().width() - m_deleteIcon->boundingRect().width() ) );
        else
            m_label->setPlainText( m_applet->name() );

        m_label->setPos( ( ( boundingRect().width() - m_deleteIcon->boundingRect().width() ) - m_label->boundingRect().width() )  / 2,
                         ( boundingRect().height() - m_label->boundingRect().height() ) / 2 );
    }
    else
    {
        if( fm.width( m_applet->name() ) > boundingRect().width() )
            m_label->setPlainText( fm.elidedText( m_applet->name(), Qt::ElideRight, boundingRect().width() ) );
        else
            m_label->setPlainText( m_applet->name() );

        m_label->setPos( ( boundingRect().width()  - m_label->boundingRect().width() )  / 2,
                         ( boundingRect().height() - m_label->boundingRect().height() ) / 2 );

    }

    emit geometryChanged();
}

void
Context::AppletToolbarAppletItem::paletteChanged( const QPalette &palette )
{
    m_label->setDefaultTextColor( palette.text().color() );
}

QVariant
Context::AppletToolbarAppletItem::itemChange( GraphicsItemChange change, const QVariant &value )
{
    QVariant ret = QGraphicsWidget::itemChange( change, value );

    if( change == ItemPositionHasChanged )
        emit geometryChanged();
    return ret;
}

QSizeF
Context::AppletToolbarAppletItem::sizeHint( Qt::SizeHint which, const QSizeF & constraint ) const
{
    Q_UNUSED( constraint )
    if( which == Qt::MinimumSize )
        return QSizeF();
    else
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

void
Context::AppletToolbarAppletItem::hoverEnterEvent( QGraphicsSceneHoverEvent * )
{
    QPropertyAnimation *animation = m_opacityAnimation.data();
    if( !animation )
    {
        animation = new QPropertyAnimation( this, "opacity" );
        animation->setDuration( 250 );
        animation->setStartValue( 0.3 );
        animation->setEndValue( 1.0 );
        m_opacityAnimation = animation;
    }
    else if( animation->state() == QAbstractAnimation::Running )
        animation->stop();

    animation->setEasingCurve( QEasingCurve::OutCubic );
    animation->setDirection( QAbstractAnimation::Backward );
    animation->start( QAbstractAnimation::KeepWhenStopped );
}

void
Context::AppletToolbarAppletItem::hoverLeaveEvent( QGraphicsSceneHoverEvent * )
{
    QPropertyAnimation *animation = m_opacityAnimation.data();
    if( !animation )
    {
        animation = new QPropertyAnimation( this, "opacity" );
        animation->setDuration( 250 );
        animation->setStartValue( 0.3 );
        animation->setEndValue( 1.0 );
        m_opacityAnimation = animation;
    }
    else if( animation->state() == QAbstractAnimation::Running )
        animation->pause();

    animation->setEasingCurve( QEasingCurve::OutCubic );
    animation->setDirection( QAbstractAnimation::Forward );
    animation->start( QAbstractAnimation::DeleteWhenStopped );
}


