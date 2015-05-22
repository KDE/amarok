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

#define DEBUG_PREFIX "VerticalAppletLayout"

#include "VerticalAppletLayout.h"

#include "Applet.h"
#include "Containment.h"
#include "core/support/Debug.h"

#include <Plasma/Applet>
#include <KConfig>

#include <QPainter>
#include <QStyleOptionGraphicsItem>
#include <QGraphicsSceneResizeEvent>
#include <QGraphicsLinearLayout>
#include <QGraphicsScene>
#include <QGraphicsView>

Context::VerticalAppletLayout::VerticalAppletLayout( QGraphicsItem* parent )
    : QGraphicsWidget( parent )
    , m_showingIndex( -1 )
    , m_layout( new QGraphicsLinearLayout(Qt::Vertical, this) )
    , m_dummyWidget( new QGraphicsWidget( this ) )
{
    m_layout->setContentsMargins( 0, 2, 0, 2 );
    m_layout->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Expanding );
    m_layout->setSpacing( 0 );

    // This dummy widget is added at the end of the layout to eat up the
    // remaining space and keep the graphicslayout at the right size. Otherwise,
    // if the last applet has a sizehint that is smaller than ours, the layout
    // will assume that size, causing applets to be constrained when switching
    // to another applet.
    m_dummyWidget->setMinimumHeight( 0.0 );
    m_dummyWidget->setPreferredHeight( 0.0 );
    m_dummyWidget->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::MinimumExpanding );
    setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Expanding );
}

Context::VerticalAppletLayout::~VerticalAppletLayout()
{
    DEBUG_BLOCK
    qDeleteAll( m_appletList );
}

void
Context::VerticalAppletLayout::resizeEvent( QGraphicsSceneResizeEvent * event )
{
    if( testAttribute( Qt::WA_PendingResizeEvent ) )
        return; // lets not do this more than necessary, shall we?
    QGraphicsWidget::resizeEvent( event );
}

void
Context::VerticalAppletLayout::addApplet( Plasma::Applet* applet, int location )
{
    DEBUG_BLOCK
    debug() << "layout told to add applet" << applet->pluginName() << "at" << location;
    if( m_appletList.isEmpty() )
        emit noApplets( false );

    applet->show();

    if( location < 0 ) // being told to add at end
    {
        m_appletList << applet;
        m_layout->addItem( applet );
        location = m_appletList.size() - 1; // so the signal has the correct location
    }
    else
    {
        m_appletList.insert( location, applet );
        m_layout->insertItem( location, applet );
    }

    debug() << "emitting addApplet with location" << location;
    emit appletAdded( applet, location );

    // every time the geometry change, we will call showapplet ;)
    connect( applet, SIGNAL(sizeHintChanged(Qt::SizeHint)), SLOT(refresh()) );
}

void
Context::VerticalAppletLayout::saveToConfig( KConfigGroup &conf )
{
    DEBUG_BLOCK
    QStringList plugins;

    for( int i = 0; i < m_appletList.size(); i++ )
    {
        Plasma::Applet *applet = m_appletList.at(i);
        if( applet != 0 )
        {
            debug() << "saving applet" << applet->pluginName();
            plugins << applet->pluginName();
        }
        conf.writeEntry( "plugins", plugins );
    }
    conf.writeEntry( "firstShowingApplet", m_showingIndex );
}

void
Context::VerticalAppletLayout::refresh()
{
    showAtIndex( m_showingIndex );
}

void
Context::VerticalAppletLayout::showApplet( Plasma::Applet* applet ) // SLOT
{
    debug() << "showing applet" << applet->pluginName();
    showAtIndex( m_appletList.indexOf( applet ) );
}

void
Context::VerticalAppletLayout::moveApplet( Plasma::Applet* applet, int oldLoc, int newLoc)
{
    DEBUG_BLOCK
    // if oldLoc is -1 we search for the applet to get the real location
    if( oldLoc == -1 )
        oldLoc = m_appletList.indexOf( applet );
    if( oldLoc == -1 )
        debug() << "COULDN'T FIND APPLET IN LIST!";

    // debug() << "moving applet in layout from" << oldLoc << "to" << newLoc;

    if( oldLoc < 0 || oldLoc > m_appletList.size() - 1 || newLoc < 0 || newLoc > m_appletList.size() || oldLoc == newLoc )
        return;
    m_appletList.insert( newLoc, m_appletList.takeAt( oldLoc ) );
    QGraphicsLayoutItem *item = m_layout->itemAt( oldLoc );
    m_layout->removeAt( oldLoc );
    m_layout->insertItem( newLoc, item );
    showApplet( applet );
}

void
Context::VerticalAppletLayout::appletRemoved( Plasma::Applet* app )
{
    DEBUG_BLOCK
    int removedIndex = m_appletList.indexOf( app );
    debug() << "removing applet at index:" << removedIndex;
    m_appletList.removeAll( app );
    if( m_showingIndex > removedIndex )
        m_showingIndex--;
    m_layout->removeItem( app );

    debug() << "got " << m_appletList.size() << " applets left";
    if( m_appletList.size() == 0 )
        emit noApplets( true );
    refresh();
}

void
Context::VerticalAppletLayout::showAtIndex( int index )
{
    if( (index < 0) || (index > m_appletList.size() - 1) )
        return;
    if( m_appletList.isEmpty() || !m_appletList.value( index ) )
        return;

    setGeometry( scene()->sceneRect() );
    m_layout->removeItem( m_dummyWidget );

    // remove and hide all applets prior to index
    QList<Plasma::Applet*> toRemove;
    for( int i = 0, count = m_layout->count(); i < count; ++i )
    {
        if( QGraphicsLayoutItem *item = m_layout->itemAt( i ) )
        {
            Plasma::Applet *applet = static_cast<Plasma::Applet*>( item );
            if( m_appletList.indexOf( applet ) < index )
                toRemove << applet;
        }
    }

    foreach( Plasma::Applet *applet, toRemove )
    {
        m_layout->removeItem( applet );
        applet->hide();
    }

    // iterate through the applets and add ones that we can fit using the size
    // hints provided by the applets
    qreal height = 0.0;
    int currentIndex = m_appletList.size();
    for( int count = currentIndex, i = index; i < count; ++i )
    {
        Plasma::Applet *item  = m_appletList.at( i );
        const qreal remainingH = size().height() - height;
        const qreal preferredH = item->effectiveSizeHint( Qt::PreferredSize ).height();
        const qreal minimumH   = item->effectiveSizeHint( Qt::MinimumSize ).height();
        const qreal maximumH   = item->effectiveSizeHint( Qt::MaximumSize ).height();

        Context::Applet *applet = qobject_cast<Context::Applet*>( item );
        if( applet ) {
            const bool wantSpace = (applet->collapseOffHeight() < 0) && (maximumH > remainingH);

            if( (preferredH > remainingH) || (wantSpace && !applet->isCollapsed() ) )
            {
                bool show = ( minimumH <= remainingH );
                currentIndex = i;
                applet->setVisible( show );
                if( show )
                {
                    m_layout->addItem( applet );
                    if( wantSpace  )
                    {
                        applet->resize( size().width(), remainingH );
                        m_layout->setStretchFactor( applet, 10000 );
                    }
                    applet->update();
                    ++currentIndex;
                }
                break;
            }
        }

        height += preferredH;
        m_layout->addItem( item );
        item->show();
        item->update();
    }

    // remove and hide all other applets
    for( int i = currentIndex; i < m_appletList.count(); ++i )
    {
        Plasma::Applet *item = m_appletList.at( i );
        m_layout->removeItem( item );
        item->hide();
    }

    m_layout->addItem( m_dummyWidget );
    m_showingIndex = index;
}

