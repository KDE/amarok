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

#define DEBUG_PREFIX "Context::Applet"

#include "Applet.h"

#include "amarokconfig.h"
#include "Containment.h"
#include "PaletteHandler.h"
#include "context/ContextView.h"
#include "core/support/Debug.h"
#include "context/widgets/AppletHeader.h"

#include <KMessageBox>
#include <KServiceTypeTrader>
#include <Plasma/IconWidget>
#include <Plasma/Theme>

#include <QGraphicsLayout>
#include <QGraphicsScene>
#include <QFontMetrics>
#include <QMetaMethod>
#include <QPainter>
#include <QPropertyAnimation>

namespace Context
{

} // Context namespace

Context::Applet::Applet( QObject * parent, const QVariantList& args )
    : Plasma::Applet( parent, args )
    , m_canAnimate( !KServiceTypeTrader::self()->query("Plasma/Animator", QString()).isEmpty() )
    , m_heightCollapseOff( 0 )
    , m_header( 0 )
    , m_transient( 0 )
    , m_standardPadding( 6.0 )
{
    setBackgroundHints( NoBackground );
}

Context::Applet::~Applet()
{
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
Context::Applet::truncateTextToFit( const QString &text, const QFont& font, const QRectF& bounds )
{
    QFontMetrics fm( font );
    return fm.elidedText ( text, Qt::ElideRight, (int)bounds.width() );
}

void
Context::Applet::paintInterface( QPainter *p,
                                 const QStyleOptionGraphicsItem *option,
                                 const QRect &contentsRect )
{
    Plasma::Applet::paintInterface( p, option, contentsRect );
    addGradientToAppletBackground( p );
}

void
Context::Applet::addGradientToAppletBackground( QPainter* p )
{
    // tint the whole applet
    // draw non-gradient backround. going for elegance and style
    const QRectF roundRect = boundingRect().adjusted( 0, 1, -1, -1 );

    p->save();
    p->setRenderHint( QPainter::Antialiasing );
    QPainterPath path;
    path.addRoundedRect( roundRect, 4, 4 );
    QColor highlight = PaletteHandler::highlightColor( 0.4, 1.05 );
    highlight.setAlphaF( highlight.alphaF() * 0.5 );
    p->fillPath( path, highlight );
    p->restore();

    p->save();
    p->setRenderHint( QPainter::Antialiasing );
    p->translate( 0.5, 0.5 );
    QColor col = PaletteHandler::highlightColor( 0.3, 0.5 );
    col.setAlphaF( col.alphaF() * 0.7 );
    p->setPen( col );
    p->drawRoundedRect( roundRect, 4, 4 );
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

Plasma::IconWidget*
Context::Applet::addAction( QGraphicsItem *parent, QAction *action, const int size )
{
    if( !action )
        return 0;

    Plasma::IconWidget *tool = new Plasma::IconWidget( parent );
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
Context::Applet::enableHeader( bool enable )
{
    if( m_header )
    {
        delete m_header;
        m_header = 0;
    }

    if( enable )
    {
        m_header = new AppletHeader( this );
        m_header->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Fixed );
    }
}

Plasma::IconWidget *
Context::Applet::addLeftHeaderAction( QAction *action )
{
    if( !m_header )
        return 0;

    Plasma::IconWidget *icon = addAction( 0, action );
    m_header->addIcon( icon, Qt::AlignLeft );
    return icon;
}

Plasma::IconWidget *
Context::Applet::addRightHeaderAction( QAction *action )
{
    if( !m_header )
        return 0;

    Plasma::IconWidget *icon = addAction( 0, action );
    m_header->addIcon( icon, Qt::AlignRight );
    return icon;
}

QString
Context::Applet::headerText() const
{
    if( !m_header )
        return QString();
    return m_header->titleText();
}

void
Context::Applet::setHeaderText( const QString &text )
{
    if( !m_header )
        return;
    m_header->setTitleText( text );
}

bool
Context::Applet::isAnimating() const
{
    QSharedPointer<QPropertyAnimation> anim = m_animation.toStrongRef();
    if( anim )
        return (anim->state() == QAbstractAnimation::Running);
    return false;
}

bool
Context::Applet::isCollapsed() const
{
    return m_heightCollapseOn == preferredHeight();
}

void
Context::Applet::setCollapseOn()
{
    collapse( true );
}

void
Context::Applet::setCollapseOff()
{
    collapse( false );
}

void
Context::Applet::collapse( bool on )
{
    qreal finalHeight = ( on ) ? m_heightCollapseOn : m_heightCollapseOff;
    const qreal maxHeight = containment()->size().height();
    if( (finalHeight > maxHeight) || (finalHeight < 0) )
        finalHeight = maxHeight;

    prepareGeometryChange();
    // warning: view() currently can return pointer to ToolbarView, not the ContextView
    ContextView *v = ContextView::self(); // may return null
    // Plasma::Applet::view() might return 0, if the widget is not yet constructed, etc.
    // \sa https://bugs.kde.org/show_bug.cgi?id=258741. If view is not available
    // yet, regardless of the animation setting the preferred size is set
    // straight away.
    if( !v || !AmarokConfig::animateAppletCollapse() )
    {
        setPreferredHeight( finalHeight );
        emit sizeHintChanged( Qt::PreferredSize );
        updateGeometry();
        return;
    }

    if( finalHeight == size().height() )
        return;

    // debug() << pluginName() << (on ? "collapsing to" : "uncollapsing to") << finalHeight;
    QPropertyAnimation *pan = m_animation.data();
    if( !pan )
        pan = new QPropertyAnimation( this, "preferredSize" );
    if( pan->state() == QAbstractAnimation::Running )
        pan->stop();
    pan->setDuration( 600 );
    pan->setEasingCurve( QEasingCurve::InQuad );
    pan->setStartValue( size() );
    pan->setEndValue( QSizeF(size().width(), finalHeight) );
    connect( pan, SIGNAL(finished()), SLOT(collapseAnimationFinished()) );
    m_animation = pan;
    pan->setDirection( QAbstractAnimation::Forward );

    v->addCollapseAnimation( pan );
}

int
Context::Applet::collapseHeight() const
{
    return m_heightCollapseOn;
}

int
Context::Applet::collapseOffHeight() const
{
    return m_heightCollapseOff;
}

void
Context::Applet::collapseAnimationFinished()
{
    emit sizeHintChanged( Qt::PreferredSize );
    updateConstraints();
    update();
}

void
Context::Applet::setCollapseHeight( int h )
{
    m_heightCollapseOn = h;
}

void
Context::Applet::setCollapseOffHeight( int h )
{
    m_heightCollapseOff = h;
}

bool Context::Applet::canAnimate()
{
    return m_canAnimate;
}

void
Context::Applet::showWarning( const QString &message, const char *slot )
{
    int ret = KMessageBox::warningYesNo( 0, message );
    Plasma::MessageButton button = (ret == KMessageBox::Yes) ? Plasma::ButtonYes : Plasma::ButtonNo;
    QByteArray sig = QMetaObject::normalizedSignature( slot );
    sig.remove( 0, 1 ); // remove method type
    QMetaMethod me = metaObject()->method( metaObject()->indexOfSlot(sig) );
    QGenericArgument arg1 = Q_ARG( const Plasma::MessageButton, button );
    if( !me.invoke( this, arg1 ) )
        warning() << "invoking failed:" << sig;
}

