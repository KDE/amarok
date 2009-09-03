/****************************************************************************************
 * Copyright (c) 2007-2009 Leo Franchi <lfranchi@gmail.com>                             *
 * Copyright (c) 2009 Riccardo Iaconelli <riccardo@kde.org>                             *
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

#include "Applet.h"
#include "Containment.h"
#include "Debug.h"

#include "PaletteHandler.h"
#include <Plasma/Animator>
#include <Plasma/FrameSvg>
#include <Plasma/IconWidget>
#include <Plasma/Theme>

#include <QGraphicsLayout>
#include <QGraphicsScene>
#include <QFontMetrics>
#include <QPainter>
#include "ContextView.h"

namespace Context
{

} // Context namespace

Context::Applet::Applet( QObject * parent, const QVariantList& args )
    : Plasma::Applet( parent, args )
    , m_collapsed( false )
    , m_animationId( 0 )
    , m_transient( 0 )
    , m_standardPadding( 6.0 )
    , m_textBackground( 0 )
{
    connect ( Plasma::Animator::self(), SIGNAL(customAnimationFinished ( int ) ), this, SLOT( animateEnd( int ) ) );
    setBackgroundHints(NoBackground);
}

Context::Applet::~Applet( )
{
    if ( m_animationId != 0)
        Plasma::Animator::self()->stopCustomAnimation( m_animationId );
}


QFont
Context::Applet::shrinkTextSizeToFit( const QString& text, const QRectF& bounds )
{
    Q_UNUSED( text );

    int size = 13; // start here, shrink if needed
    QFont font( QString(), size, QFont::Light );
    font.setStyleHint( QFont::SansSerif );
    font.setStyleStrategy( QFont::PreferAntialias );

    QFontMetrics fm( font );
    while( fm.height() > bounds.height() + 4 )
    {
        if( size < 0 )
        {
            size = 5;
            break;
        }
        size--;
        fm = QFontMetrics( QFont( QString(), size ) );
    }

    // for aesthetics, we make it one smaller
    size--;

    QFont returnFont( QString(), size, QFont::Light );
    font.setStyleHint( QFont::SansSerif );
    font.setStyleStrategy( QFont::PreferAntialias );

    return QFont( returnFont );
}

QString
Context::Applet::truncateTextToFit( QString text, const QFont& font, const QRectF& bounds )
{
    QFontMetrics fm( font );
    return fm.elidedText ( text, Qt::ElideRight, (int)bounds.width() );
}

void
Context::Applet::drawRoundedRectAroundText( QPainter* p, QGraphicsSimpleTextItem* t )
{
    p->save();
    p->setRenderHint( QPainter::Antialiasing );

    if ( !m_textBackground ) {
        m_textBackground = new Plasma::FrameSvg();
        m_textBackground->setImagePath( "widgets/text-background" );
        m_textBackground->setEnabledBorders( Plasma::FrameSvg::AllBorders );
    }

    // Paint in integer coordinates, align to grid
    QRectF rect = t->boundingRect();
    QPointF pos = t->pos();
    rect.setX( qRound( rect.x() ) );
    rect.setY( qRound( rect.y() ) );
    rect.setHeight( qRound( rect.height() ) );
    rect.setWidth( qRound( rect.width() ) );
    rect.moveTopLeft( t->pos() );
    pos.setX( qRound( pos.x() ) );
    pos.setY( qRound( pos.y() ) );
    rect.moveTopLeft( pos );
    rect.adjust( -5, -5, 5, 5 );
    m_textBackground->resize( rect.size() );
    m_textBackground->paintFrame( p, rect.topLeft() );
    p->restore();
}

void
Context::Applet::addGradientToAppletBackground( QPainter* p )
{
        // tint the whole applet
    // draw non-gradient backround. going for elegance and style
    p->save();
    QPainterPath path;
    path.addRoundedRect( boundingRect().adjusted( 0, 2, -2, -2 ), 4, 4 );
    //p->fillPath( path, gradient );
    QColor highlight = PaletteHandler::highlightColor( 0.4, 1.05 );
    highlight.setAlphaF( highlight.alphaF() * 0.5 );
    p->fillPath( path, highlight );
    p->restore();

    p->save();
    p->translate( 0.5, 0.5 );
    QColor col = PaletteHandler::highlightColor( 0.3, 0.5 );
    col.setAlphaF( col.alphaF() * 0.7 );
    p->setPen( col );
    p->drawRoundedRect( boundingRect().adjusted( 0, 2, -2, -2 ), 4, 4 );
    p->restore();
}

qreal
Context::Applet::standardPadding()
{
    return  m_standardPadding;
}

void
Context::Applet::destroy()
{
    if ( Plasma::Applet::immutability() != Plasma::Mutable || m_transient ) {
        return; //don't double delete
    }
    m_transient = true;
    cleanUpAndDelete();
}

void
Context::Applet::cleanUpAndDelete()
{
    QGraphicsWidget *parent = dynamic_cast<QGraphicsWidget *>( parentItem() );
    //it probably won't matter, but right now if there are applethandles, *they* are the parent.
    //not the containment.

    //is the applet in a containment and is the containment have a layout? if yes, we remove the applet in the layout
    if ( parent && parent->layout() )
    {
        QGraphicsLayout *l = parent->layout();
        for ( int i = 0; i < l->count(); ++i )
        {
            if ( this == l->itemAt( i ) )
            {
                l->removeAt( i );
                break;
            }
        }
    }

    if ( Plasma::Applet::configScheme() ) {
        Plasma::Applet::configScheme()->setDefaults();
    }
    Plasma::Applet::scene()->removeItem( this );
    Plasma::Applet::deleteLater();
}

QSizeF
Context::Applet::sizeHint( Qt::SizeHint which, const QSizeF & constraint ) const
{
    return QSizeF( QGraphicsWidget::sizeHint( which, constraint ).width(), m_heightCurrent );
}

void
Context::Applet::resize( qreal wid, qreal hei)
{
    m_heightCollapseOff = hei;
    m_heightCurrent = hei;
    QGraphicsWidget::resize( wid, hei );
}

Plasma::IconWidget*
Context::Applet::addAction( QAction *action, const int size )
{
    if( !action )
        return 0;

    Plasma::IconWidget *tool = new Plasma::IconWidget( this );
    tool->setAction( action );
    tool->setText( QString() );
    tool->setToolTip( action->text() );
    tool->setDrawBackground( false );
    tool->setOrientation( Qt::Horizontal );
    const QSizeF iconSize = tool->sizeFromIconSize( size );
    tool->setMinimumSize( iconSize );
    tool->setMaximumSize( iconSize );
    tool->resize( iconSize );
    tool->setZValue( zValue() + 1 );

    return tool;
}

void
Context::Applet::setCollapseOn()
{
    if ( size().height() == m_heightCollapseOn )
        return;
    debug() << "collapsing applet to..." << m_heightCollapseOn;
    if( m_heightCollapseOff == -1 )
        m_animFromHeight = size().height();
    else
        m_animFromHeight = m_heightCollapseOff;

    if ( m_animationId != 0 ) // warning we are moving right now
    {
        // stop the anim
        Plasma::Animator::self()->stopCustomAnimation( m_animationId );
        m_animationId = 0;
    }
    m_collapsed = false;
    m_animationId = Plasma::Animator::self()->customAnimation(20, 1000, Plasma::Animator::EaseInCurve, this, "animateOn" );
}

void
Context::Applet::setCollapseOff()
{
    DEBUG_BLOCK
    debug() << "height:" << size().height() << "target:" << m_heightCollapseOff << "m_collapsed:" << m_collapsed;
    if ( size().height() == m_heightCollapseOff )
        return;

    if( m_heightCollapseOff == -1) // if it's self-expanding, don't animate as we don't know where we're going. also, if we're shrinking
    {                                                                       // stop that and expand regardless
        // stop the anim
        if( m_animationId != 0 )
        {
            Plasma::Animator::self()->stopCustomAnimation( m_animationId );
            m_animationId = 0;
        }
        m_heightCurrent =  m_heightCollapseOff;
        emit sizeHintChanged(Qt::PreferredSize);
        updateGeometry();
        m_collapsed = false;
        return;
    }

    if ( m_animationId != 0 ) // warning we are moving right now
    {
        // stop the anim
        Plasma::Animator::self()->stopCustomAnimation( m_animationId );
        m_animationId = 0;
    }
    m_collapsed = true ;
    m_animationId = Plasma::Animator::self()->customAnimation(20, 1000, Plasma::Animator::EaseInCurve, this, "animateOff" );
}

void
Context::Applet::setCollapseHeight( int h )
{
    m_heightCollapseOn = h;
}

void
Context::Applet::animateOn( qreal anim )
{
    m_heightCurrent = m_animFromHeight - ( m_animFromHeight - m_heightCollapseOn ) * anim ;
    emit sizeHintChanged(Qt::PreferredSize);
}

void
Context::Applet::animateOff( qreal anim )
{
    m_heightCurrent =  m_heightCollapseOn + ( m_heightCollapseOff - m_heightCollapseOn ) * anim ;
    emit sizeHintChanged(Qt::PreferredSize);
}

void
Context::Applet::animateEnd( int id )
{
    if( id == m_animationId )
    {
        if( !m_collapsed )
        {
            Plasma::Applet::resize( size().width(), m_heightCollapseOn );
            m_collapsed = true;
        }
        else
        {
            Plasma::Applet::resize( size().width(), m_heightCollapseOff );
            m_collapsed = false;
        }
        updateGeometry();
        emit sizeHintChanged(Qt::PreferredSize);
        m_animationId = 0;
    }
}



#include "Applet.moc"
