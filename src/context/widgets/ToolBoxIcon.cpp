/****************************************************************************************
 * Copyright (c) 2008 William Viana Soares <vianasw@gmail.com>                          *
 * Copyright (c) 2009 Leo Franchi <lfranchi@kde.org>                                    *
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

#include "ToolBoxIcon.h"

#include "Debug.h"
#include "PaletteHandler.h"

#include <plasma/animator.h>
#include <plasma/paintutils.h>

#include <KColorScheme>

#include <QFont>

#define PADDING 15


static const float TOOLBOX_OPACITY = 0.4;

ToolBoxIcon::ToolBoxIcon( QGraphicsItem *parent )
    : Plasma::IconWidget( parent )
    , m_hovering( 0 )
    , m_animOpacity( TOOLBOX_OPACITY )
    , m_animHighlightId( 0 )
{
    m_text = new QGraphicsSimpleTextItem( this );
    m_text->setCursor( Qt::ArrowCursor ); // Don't show the carot, the text isn't editable.

    QFont font;
    font.setBold( false );
    font.setPointSize( font.pointSize() - 2 );
    font.setStyleStrategy( QFont::PreferAntialias );
    
    m_text->setFont( font );
    m_text->show();
}

ToolBoxIcon::~ToolBoxIcon()
{}

void
ToolBoxIcon::mousePressEvent( QGraphicsSceneMouseEvent *event )
{
    if( event->button() != Qt::LeftButton )
    {
        Plasma::IconWidget::mousePressEvent( event );
        return;
    }
    
    if( data( 0 ) != QVariant() )
    {
        DEBUG_LINE_INFO
        debug() << data( 0 ).toString();
        emit appletChosen( data( 0 ).toString() );
    }
    else
    {
        Plasma::IconWidget::mousePressEvent( event );
    }
}

void
ToolBoxIcon::mousePressed( bool pressed )
{
    DEBUG_BLOCK
    
    if( pressed && data( 0 ) != QVariant() )
    {
        debug() << data( 0 ).toString();
        emit appletChosen( data( 0 ).toString() );
    }
}

void
ToolBoxIcon::paint( QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget )
{
    painter->setRenderHint( QPainter::Antialiasing );
    if( Plasma::IconWidget::drawBackground() )
    {
        if( m_text->text().isEmpty() )
            m_text->setText( text() );
        
        const QFontMetricsF fm( m_text->font() );
        m_text->setPos( PADDING, size().height() / 2 - fm.boundingRect( m_text->text() ).height() / 2 );
        painter->save();
        
       // QColor color = KColorScheme( QPalette::Active, KColorScheme::Window,
       //                        Plasma::Theme::defaultTheme()->colorScheme() ).background().color();
        painter->setOpacity( m_animOpacity );

       QLinearGradient gradient( boundingRect().topLeft(), boundingRect().bottomLeft() );
       QColor highlight = PaletteHandler::highlightColor();
       highlight.setAlpha( 160 );
       gradient.setColorAt( 0, highlight.darker( 140 ) );
       highlight.setAlpha( 220 );
       gradient.setColorAt( 1, highlight.darker( 180 ) );
       QPainterPath path;
       path.addRoundedRect( boundingRect(), 3, 3 );
       painter->fillPath( path, gradient );
       painter->restore();

       // draw border
       painter->save();
       painter->translate(0.5, 0.5);
       painter->setPen( PaletteHandler::highlightColor().darker( 150 ) );
       painter->drawRoundedRect( boundingRect(), 3, 3 );
       painter->restore();
    }
    else
        Plasma::IconWidget::paint( painter, option, widget );
}

QRectF
ToolBoxIcon::boundingRect() const
{
    return QRectF( QPointF( 0, 0 ), size() );
}

void
ToolBoxIcon::hoverEnterEvent( QGraphicsSceneHoverEvent *event )
{
    if( m_animHighlightId )
        Plasma::Animator::self()->stopCustomAnimation( m_animHighlightId );
    
    m_hovering = true;
    m_animHighlightId = Plasma::Animator::self()->customAnimation( 10, 240, Plasma::Animator::EaseInCurve, this, "animateHighlight" );
    m_defaultTextBrush = m_text->brush();
    m_text->setBrush( The::paletteHandler()->palette().highlightedText() );
    Plasma::IconWidget::hoverEnterEvent( event );
}

void
ToolBoxIcon::hoverLeaveEvent( QGraphicsSceneHoverEvent *event )
{
    if( m_animHighlightId )
        Plasma::Animator::self()->stopCustomAnimation( m_animHighlightId );
    
    m_hovering = false;
    m_animHighlightId = Plasma::Animator::self()->customAnimation( 10, 240, Plasma::Animator::EaseOutCurve, this, "animateHighlight" );
    m_text->setBrush( m_defaultTextBrush );
    Plasma::IconWidget::hoverLeaveEvent( event );
}

void
ToolBoxIcon::animateHighlight( qreal progress )
{
    if( m_hovering )
        m_animOpacity = TOOLBOX_OPACITY + ( ( 1.0 - TOOLBOX_OPACITY ) * progress );
    else
        m_animOpacity = 1.0 - ( ( 1.0 - TOOLBOX_OPACITY ) * progress );

    if( progress >= 1.0 )
        m_animHighlightId = 0;

    update();
}


QPainterPath
ToolBoxIcon::shape() const
{
    if( Plasma::IconWidget::drawBackground() )
    {
        QSize shapeSize( size().width() - 2, size().height() - 2 );
        return Plasma::PaintUtils::roundedRectangle( QRectF( QPointF( 0.0, 0.0 ), shapeSize ), 10.0 );
    }

    return Plasma::IconWidget::shape();
}

void
ToolBoxIcon::setText( const QString &text )
{
    m_text->setText( text );
    update();
}

QString
ToolBoxIcon::text() const
{
    return m_text->text();
}

void
ToolBoxIcon::setBrush( const QBrush& b )
{
    m_text->setBrush( b );
}

#include "ToolBoxIcon.moc"

