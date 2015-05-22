/****************************************************************************************
 * Copyright (c) 2007 Leo Franchi <lfranchi@kde.org>                                    *
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

#define DEBUG_PREFIX "VerticalToolbarContainment"

#include "VerticalToolbarContainment.h"

#include "ContextView.h"
#include "core/interfaces/Logger.h"
#include "core/support/Components.h"
#include "core/support/Debug.h"
#include "EngineController.h"
#include "PaletteHandler.h"
#include "VerticalAppletLayout.h"

#include <KConfig>

#include <QGraphicsLinearLayout>

Context::VerticalToolbarContainment::VerticalToolbarContainment( QObject *parent, const QVariantList &args )
    : Containment( parent, args )
    , m_view( 0 )
    , m_applets( 0 )
    , m_noAppletText( 0 )
{
    DEBUG_BLOCK
    setContainmentType( CustomContainment );
    setDrawWallpaper( false );
   // setScreen( -1 );
    setImmutability( Plasma::Mutable );

    debug() << "applet containment has corona:" << corona();
    m_applets = new VerticalAppletLayout( this );

    connect( this, SIGNAL(appletRemoved(Plasma::Applet*)), SLOT(appletRemoved(Plasma::Applet*)) );
    connect( this, SIGNAL(appletRemoved(Plasma::Applet*)), SIGNAL(geometryChanged()) );

    connect( m_applets, SIGNAL(appletAdded(Plasma::Applet*,int)), SIGNAL(appletAdded(Plasma::Applet*,int)) );
    connect( m_applets, SIGNAL(appletAdded(Plasma::Applet*,int)), SIGNAL(geometryChanged()) );
    connect( m_applets, SIGNAL(noApplets(bool)), SLOT(showEmptyText(bool)) );

}

Context::VerticalToolbarContainment::~VerticalToolbarContainment()
{}

void
Context::VerticalToolbarContainment::constraintsEvent( Plasma::Constraints constraints )
{
    Q_UNUSED( constraints )
    if( m_noAppletText )
    {
        QRectF masterRect = contentsRect();
        m_noAppletText->setTextWidth( masterRect.width() * .4 );
        QPointF topLeft( ( masterRect.width() / 2 ) - ( m_noAppletText->boundingRect().width() / 2 ), ( masterRect.height() / 2 ) - ( m_noAppletText->boundingRect().height() / 2 ) );
        m_noAppletText->setPos( topLeft );
    }
}

QList<QAction*>
Context::VerticalToolbarContainment::contextualActions()
{
    return QList< QAction* >();
}

void
Context::VerticalToolbarContainment::saveToConfig( KConfigGroup &conf )
{
    m_applets->saveToConfig( conf );
}

void
Context::VerticalToolbarContainment::loadConfig( const KConfigGroup &conf )
{
    DEBUG_BLOCK

    QStringList plugins = conf.readEntry( "plugins", QStringList() );
    debug() << "plugins.size(): " << plugins.size();

    foreach( const QString& plugin, plugins )
    {
        PERF_LOG( qPrintable( QString("Adding applet: %1").arg( plugin ) ) )
        debug() << "Adding applet: " << plugin;
        addApplet( plugin, -1 );
    }

    int showing = conf.readEntry( "firstShowingApplet", 0 );
    m_applets->showAtIndex( showing );
}

void
Context::VerticalToolbarContainment::setView( ContextView* view )
{
    DEBUG_BLOCK

    m_view = view;
    // kick the toolbar with a real corona no w
    emit updatedContainment( this );
}

Context::ContextView*
Context::VerticalToolbarContainment::view()
{
    return m_view;
}

void
Context::VerticalToolbarContainment::updateGeometry()
{
    Context::Containment::updateGeometry();

    /* We used to use _scene_ sceneRect here to update applets and geomtery, but that
     * leaded to infinite loop (across mainloop) - see bug 278897.
     * (m_applets->setGeometry(), refresh() would enlarge _scene_ sceneRect by a few
     * pixels which would trigger updateGeometry() and so on...)
     *
     * We now use _view_ sceneRect to update geometry and do nothing without a view
     */
    if(!view())
        return;

    // mimic ContextView::resizeEvent(), nothing else seems to work, bug 292895
    QRectF rect( view()->pos(), view()->maximumViewportSize() );
    setGeometry( rect );
    m_applets->setGeometry( rect );
    m_applets->refresh();
}

void
Context::VerticalToolbarContainment::addApplet( const QString& pluginName, const int loc ) // SLOT
{
    DEBUG_BLOCK

    if( pluginName == "analyzer" && !EngineController::instance()->supportsAudioDataOutput() )
    {
        Amarok::Components::logger()->longMessage( i18n( "Error: Visualizations are not supported by your current Phonon backend." ),
                                                   Amarok::Logger::Error ) ;

        return;
    }

    Plasma::Applet* applet = Plasma::Containment::addApplet( pluginName );

    Q_ASSERT_X( applet, "addApplet", "FAILED ADDING APPLET TO CONTAINMENT!! NOT FOUND!!" );

    m_applets->addApplet( applet, loc );
    applet->setFlag( QGraphicsItem::ItemIsMovable, false );
}

void
Context::VerticalToolbarContainment::appletRemoved( Plasma::Applet* applet )
{
    m_applets->appletRemoved( applet );
}

void
Context::VerticalToolbarContainment::showApplet( Plasma::Applet* applet )
{
    m_applets->showApplet( applet );
}

void
Context::VerticalToolbarContainment::moveApplet( Plasma::Applet* applet, int a, int b)
{
    m_applets->moveApplet( applet, a, b);
}

void
Context::VerticalToolbarContainment::wheelEvent( QGraphicsSceneWheelEvent* event )
{
    Q_UNUSED( event )
    //eat wheel events, we don't want scrolling
}

void
Context::VerticalToolbarContainment::showEmptyText( bool toShow ) // SLOT
{
    if( toShow )
    {
        if( !m_noAppletText )
        {
            m_noAppletText = new QGraphicsTextItem( this );
            m_noAppletText->setHtml( QString( "<html>  <style type=\"text/css\"> body { background-color: %1; } </style> \
                                              <body> <p align=\"center\"> %2 </</p></body></html>" )
                                     .arg( The::paletteHandler()->highlightColor().name() )
                                     .arg( i18n( "Please add some applets from the toolbar at the bottom of the context view." ) ) );
        }
        m_noAppletText->show();
    }
    else if( m_noAppletText )
    {
        m_noAppletText->hide();
    }
    updateConstraints();
    update();
}

