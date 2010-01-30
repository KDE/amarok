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

#include "AppletItemOverlay.h"
#include "Debug.h"
#include "toolbar/AppletToolbarAddItem.h"
#include "toolbar/AppletToolbarAppletItem.h"
#include "ToolbarView.h"

#include <plasma/applet.h>
#include <plasma/theme.h>
#include <plasma/paintutils.h>

#include <KIcon>
#include <KGlobalSettings>

#include <QAction>
#include <QGraphicsLinearLayout>
#include <QGraphicsWidget>
#include <QStyleOptionGraphicsItem>
#include <QTimer>
#include <QToolButton>
#include <QPainter>

// stolen verbatim and shamelessly from workspace/plasma/shells/desktop/panelappletoverlay
class AppletMoveSpacer : public QGraphicsWidget
{
public:
    AppletMoveSpacer( QGraphicsWidget *applet )
        : QGraphicsWidget( applet ),
          m_applet( applet )
    {
    }

protected:
    void paint( QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget * widget = 0 )
    {
        Q_UNUSED( option )
        Q_UNUSED( widget )

        /*
           results in odd painting corruption
        if (collidesWithItem(m_applet, Qt::IntersectsItemBoundingRect)) {
            painter->fillRect(contentsRect(), Qt::transparent);
            return;
        }
        */

        //TODO: make this a pretty gradient?
        painter->setRenderHint( QPainter::Antialiasing );
        QPainterPath p = Plasma::PaintUtils::roundedRectangle( contentsRect().adjusted( 1, 1, -2, -2 ), 4 );
        QColor c = Plasma::Theme::defaultTheme()->color( Plasma::Theme::TextColor );
        c.setAlphaF( 0.3 );

        painter->fillPath( p, c );
    }

private:
    QGraphicsWidget *m_applet;
};


Context::AppletItemOverlay::AppletItemOverlay( Context::AppletToolbarAppletItem *applet, QGraphicsLinearLayout* layout, QWidget *parent )
    : QWidget( parent ),
      m_applet( applet ),
      m_spacer(0),
      m_layout( layout ),
      m_deleteIcon( 0 ),
      m_index( 0 ),
      m_clickDrag( false )
{
    DEBUG_BLOCK

    if( layout )
    {
        m_layout = layout;
        int i = 0;
        for(; i < m_layout->count(); ++i) 
        {
            QGraphicsWidget *w = dynamic_cast< QGraphicsWidget* >( m_layout->itemAt( i ) );
            if( w == m_applet ) 
            {
                m_index = i;
                break;
            }
        }
    } else
        debug() << "GOT APPLET WITH NO LAYOUT! BAD!";

    m_deleteIcon = new QToolButton( this );
    QAction* delApplet = new QAction( i18n( "Remove Applet" ), this );
    delApplet->setIcon( KIcon( "edit-delete" ) );
    delApplet->setVisible( true );
    delApplet->setEnabled( true );
    m_deleteIcon->addAction( delApplet );
    m_deleteIcon->setIcon( KIcon( "edit-delete" ) );
    m_deleteIcon->setMaximumSize( 24, 24 );
    QColor trans;
    trans.setAlpha( 0 );
    QBrush brush( Qt::transparent );
    QPalette pal = m_deleteIcon->palette();
    pal.setBrush( QPalette::Window, brush );
 //   m_deleteIcon->setBackgroundRole( QPalette::Base );
    m_deleteIcon->setPalette( pal );  
    m_deleteIcon->setAutoFillBackground( false );
    m_deleteIcon->setAttribute( Qt::WA_NoSystemBackground );
    //m_deleteIcon->setAttribute( Qt::WA_TranslucentBackground ); //NB: Introduced in Qt 4.5
    
    connect( delApplet, SIGNAL( triggered() ), this, SLOT( deleteApplet() ) );
    connect( m_deleteIcon, SIGNAL( released() ), this, SLOT( deleteApplet() ) );
    
    syncGeometry();

    connect( m_applet, SIGNAL( destroyed(QObject*) ), this, SLOT( deleteLater() ) );
    connect( m_applet, SIGNAL( geometryChanged() ), this, SLOT( delaySyncGeometry() ) );
}

Context::AppletItemOverlay::~AppletItemOverlay()
{
    DEBUG_BLOCK
    if( m_spacer ) 
    {
        m_layout->removeItem( m_spacer );
        m_spacer->deleteLater();
        m_spacer = 0;
    }
    m_applet = 0;
    m_layout = 0;
}

void 
Context::AppletItemOverlay::paintEvent( QPaintEvent *event )
{
    Q_UNUSED( event )
    QStyleOption op;
    op.initFrom( this );

    bool hovered = op.state & QStyle::State_MouseOver;
    bool mover = mouseGrabber() == this;
    if( !hovered || mover ) 
    {
        return;
    }

    QPainter p( this );
    p.save();
    KIcon icon( "transform-move" );
    int iconSize;
    QRect iconRect;

    if( m_applet )
    {   // it's possible m_applet is null if we just opened amarok and it failed to load the applet plugin
        // so the user is seeing a big red X and trying to get rid of item
        iconSize = qMin( qMin( height(), int( m_applet->size().width() ) ), 64 );
        iconRect = QRect( rect().center() - QPoint( iconSize / 2, iconSize / 2 ), QSize( iconSize, iconSize ) );
        p.drawPixmap( iconRect, icon.pixmap( iconSize, iconSize ) );        
    }

    p.restore();
}

void 
Context::AppletItemOverlay::mousePressEvent( QMouseEvent *event )
{
    Q_UNUSED( event )
    DEBUG_BLOCK
    
    //kDebug() << m_clickDrag;
    if( m_clickDrag ) {
        setMouseTracking( false );
        m_clickDrag = false;
        m_origin = QPoint();
        return;
    }
/*
    // check if under the click is the delete icon, if so, delete instead
    Context::ToolbarView* view = dynamic_cast< Context::ToolbarView* >( parent() );
    debug() << "icon scene rectt:" << m_applet->delIconSceneRect() << "event scene rect:" << view->mapToScene( event->globalPos() );
    debug() << "from view:" << view->mapFromScene( m_applet->delIconSceneRect() ).boundingRect() << event->pos();
    if( view && m_applet->delIconSceneRect().contains( view->mapToScene( event->pos() ) ) )
    {
        m_applet->deleteLater();
        return;
    }
*/
    m_clickDrag = false;
    if( !m_spacer ) 
    {
        m_spacer = new AppletMoveSpacer( m_applet );
    } else 
    {
        m_layout->removeItem( m_spacer );
    }

    m_origin = mapToParent( event->pos() );
    m_spacer->setMinimumSize( m_applet->geometry().size() );
    m_spacer->setMaximumSize( m_applet->geometry().size() );
    m_layout->removeItem( m_applet );
    m_layout->insertItem( m_index, m_spacer );
    m_applet->setZValue( m_applet->zValue() + 1 );

    m_offset = geometry().x() - m_origin.x();

    grabMouse();
}

void 
Context::AppletItemOverlay::mouseMoveEvent( QMouseEvent *event )
{
 //   DEBUG_BLOCK
//    Plasma::FormFactor f = m_applet->formFactor();

    // todo add in support for dragging an item out of the toolbar area
    /*
    if ( ((f != Plasma::Horizontal && f != Plasma::Vertical) && rect().intersects(m_applet->rect().toRect())) ||
          ((f == Plasma::Horizontal || f == Plasma::Vertical) && !rect().contains(event->globalPos())) ) {
        Plasma::View *view = Plasma::View::topLevelViewAt(event->globalPos());

        if (!view) {
            return;
        }

        QPointF pos = view->mapFromGlobal(event->globalPos());
        if (view != m_applet->view()) {

            Plasma::Containment *c = view->containment();

            c->addApplet(m_applet, pos);
            syncOrientation();
            syncGeometry();

            if (m_spacer) {
                m_layout->removeItem(m_spacer);
                m_spacer->deleteLater();
                m_spacer = 0;
            }

            QGraphicsLinearLayout *newLayout = dynamic_cast<QGraphicsLinearLayout *>(c->layout());
            if (newLayout && (c->formFactor() == Plasma::Vertical || c->formFactor() == Plasma::Horizontal)) {
                m_layout->removeItem(m_applet);
                m_layout = newLayout;
                setParent(view);
            }
        }
    } */

    if( !m_spacer ) 
    {
        m_spacer = new AppletMoveSpacer( m_applet );
        m_spacer->setMinimumSize( m_applet->geometry().size() );
        m_spacer->setMaximumSize( m_applet->geometry().size() );
        m_layout->removeItem( m_applet );
        m_layout->insertItem( m_index, m_spacer );
    }

    QPoint p = mapToParent( event->pos() );
    QRectF g = m_applet->geometry();

  //  debug() << p << g << "<-- movin'?";
    g.moveLeft( p.x() + m_offset );

    m_applet->setGeometry( g );

    // find location of the AddItem icon
    QRectF addItemGeom;
    for( int i = 0; i < m_layout->count(); ++i )
    {
        QGraphicsLayoutItem *item = m_layout->itemAt( i );

        Context::AppletToolbarAddItem* addItem =
            dynamic_cast< Context::AppletToolbarAddItem* >( item );

        if( addItem )
        {
            addItemGeom = addItem->geometry();
            break;
        }
    }

    // swap items if we pass completely over the next/previous item or cross
    // more than halfway across it, whichever comes first
    // debug() << m_prevGeom << g << m_nextGeom << addItemGeom;
    if( m_prevGeom.isValid() && g.left() <= m_prevGeom.left() )
    {
        swapWithPrevious();
    }
    else if( m_nextGeom.isValid() && g.right() >= m_nextGeom.right()
                                  && g.right() <  addItemGeom.left() )
    {
        swapWithNext();
    }

    // debug() << "=================================";
}

void 
Context::AppletItemOverlay::mouseReleaseEvent( QMouseEvent *event )
{
    Q_UNUSED( event )
 
    DEBUG_BLOCK   
    
    if( !m_spacer ) 
    {
        releaseMouse();
        return;
    }

    if( !m_origin.isNull() ) 
    {
   //     debug() << m_clickDrag << m_origin << mapToParent( event->pos() );
        m_clickDrag = abs( mapToParent( event->pos() ).x() - m_origin.x() ) < KGlobalSettings::dndEventDelay();

        if( m_clickDrag ) 
        {
  //          debug() << "click dragging." << this << mouseGrabber();
            setMouseTracking( true );
            event->setAccepted( false );
            return;
        }
    }

    releaseMouse();
  //  debug() << "m_origin is null, canceling drag and putting applet back";
    m_layout->removeItem( m_spacer );
    m_spacer->deleteLater();
    m_spacer = 0;

    m_layout->insertItem( m_index, m_applet );
    m_applet->setZValue( m_applet->zValue() - 1 );
    // -1 means not specifying where it is from

    emit moveApplet( m_applet->applet(), -1, m_index );
}

void 
Context::AppletItemOverlay::enterEvent( QEvent *event )
{
    Q_UNUSED( event )
    DEBUG_BLOCK
    update();
}

void 
Context::AppletItemOverlay::leaveEvent( QEvent *event )
{
    Q_UNUSED( event )
    DEBUG_BLOCK
    update();
}


Context::AppletToolbarAppletItem*
Context::AppletItemOverlay::applet()
{
    return m_applet;
}


void
Context::AppletItemOverlay::resizeEvent( QResizeEvent* )
{
    m_deleteIcon->setGeometry( QRect( QPoint( ( size().width() - (m_deleteIcon->size().width() ) ) , 0 ), m_deleteIcon->geometry().size() ) );
}

void
Context::AppletItemOverlay::deleteApplet()
{
    emit deleteApplet( dynamic_cast< Plasma::Applet* >( m_applet->applet() ) );
    m_applet = 0;
    deleteLater();
}

void 
Context::AppletItemOverlay::swapWithPrevious()
{
    DEBUG_BLOCK

    m_index -= 1;

    if( m_index > 1 ) 
    {
        QGraphicsLayoutItem* layout = m_layout->itemAt( m_index - 1 );
        m_prevGeom = layout ? layout->geometry() : QRectF();
    }
    else
    {
        m_prevGeom = QRectF();
    }

    QGraphicsLayoutItem* layout = m_layout->itemAt( m_index + 1 );
    m_nextGeom = layout ? layout->geometry() : QRectF();

    m_layout->removeItem( m_spacer );
    m_layout->insertItem( m_index, m_spacer );
}

void 
Context::AppletItemOverlay::swapWithNext()
{
    DEBUG_BLOCK
    m_index += 1;

    if ( m_index < m_layout->count() - 1 ) {
        m_nextGeom = m_layout->itemAt( m_index + 1)->geometry();
    } else 
    {
        m_nextGeom = QRectF();
    }

    m_prevGeom = m_layout->itemAt( m_index - 1 )->geometry();
    m_layout->removeItem( m_spacer );
    m_layout->insertItem( m_index, m_spacer );
}

void 
Context::AppletItemOverlay::delaySyncGeometry()
{
    // we need to do this because it gets called in a round-about-way
    // from our own mouseMoveEvent. if we call syncGeometry directly,
    // we end up with a maze of duplicated and confused mouseMoveEvents
    // of which only half are real (the other half being caused by the
    // immediate call to syncGeometry!)
    QTimer::singleShot( 0, this, SLOT( syncGeometry() ) );
}

void 
Context::AppletItemOverlay::syncGeometry()
{
  //  DEBUG_BLOCK
    setGeometry( m_applet->geometry().toRect() );
  //  debug() << "setting overlay geometry to" << m_applet->geometry().toRect();

    if( m_index > 1 ) 
    {
        if( m_layout->itemAt( m_index - 1 ) )
            m_prevGeom = m_layout->itemAt( m_index - 1 )->geometry();
    } else 
    {
        m_prevGeom = QRectF();
    }

    if( m_index < m_layout->count() - 1  )
    {
        if( m_layout->itemAt( m_index + 1 ) )
            m_nextGeom = m_layout->itemAt( m_index + 1 )->geometry();
    } else 
    {
        m_nextGeom = QRectF();
    }
    //debug() << m_index << m_layout->count() << m_prevGeom << m_nextGeom;
}

#include "AppletItemOverlay.moc"
