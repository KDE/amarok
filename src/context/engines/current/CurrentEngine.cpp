/***************************************************************************
 * copyright            : (C) 2007 Leo Franchi <lfranchi@gmail.com>        *
 **************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "CurrentEngine.h"

#include "amarok.h"
#include "debug.h"
#include "ContextObserver.h"
#include "ContextView.h"
#include "enginecontroller.h"
#include "meta/Meta.h"
#include "meta/MetaUtility.h"

#include <QVariant>

using namespace Context;

CurrentEngine::CurrentEngine( QObject* parent, const QList<QVariant>& args )
    : DataEngine( parent )
    , ContextObserver( ContextView::self() )
    , m_requested( true )
    , m_coverWidth( 0 )
{
    Q_UNUSED( args )
    DEBUG_BLOCK
    m_sources = QStringList();
    m_sources << "current";
}

CurrentEngine::~CurrentEngine()
{
    if( m_currentTrack )
    {
        m_currentTrack->unsubscribe( this );
        if( m_currentTrack->album() )
        {
            m_currentTrack->album()->unsubscribe( this );
        }
    }
}

QStringList CurrentEngine::sources() const
{
    return m_sources; // we don't have sources, if connected, it is enabled.
}

bool CurrentEngine::sourceRequested( const QString& name )
{
    Q_UNUSED( name );
/*    m_sources << name;    // we are already enabled if we are alive*/
    clearData( name );
    setData( name, QVariant());
    update();
    m_requested = true;
    return true;
}

void CurrentEngine::message( const ContextState& state )
{
    DEBUG_BLOCK
    if( state == Current && m_requested )
    {
        if( m_currentTrack )
        {
            m_currentTrack->unsubscribe( this );
            if( m_currentTrack->album() )
            {
                m_currentTrack->album()->unsubscribe( this );
            }
        }
        update();
    }
}

void CurrentEngine::metadataChanged( Meta::Album* album )
{
    setData( "albumart", album->image( coverWidth() ) );
}

void
CurrentEngine::metadataChanged( Meta::Track *track )
{
    QVariantMap trackInfo = Meta::Field::mapFromTrack( track );
    setData( "current", "current", trackInfo );
}

void CurrentEngine::update()
{
    DEBUG_BLOCK
    m_currentTrack = EngineController::instance()->currentTrack();
    if( !m_currentTrack )
        return;
    m_currentTrack->subscribe( this );

    QVariantMap trackInfo = Meta::Field::mapFromTrack( m_currentTrack.data() );

    int width = coverWidth();
    m_currentTrack->album()->subscribe( this );
    clearData( "current" );
    setData( "current", "albumart",  QVariant( m_currentTrack->album()->image( width ) ) );
    setData( "current", "current", trackInfo );
}

#include "CurrentEngine.moc"
