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
#include "meta/meta.h"
#include "meta/MetaUtility.h"

#include <QVariant>

using namespace Context;

CurrentEngine::CurrentEngine( QObject* parent, const QList<QVariant>& args )
    : DataEngine( parent )
    , ContextObserver( ContextView::self() )
    , m_requested( true )
{
    Q_UNUSED( args )
    DEBUG_BLOCK
    m_sources = QStringList();
    m_sources << "current";
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
    m_requested = true;
    return true;
}

void CurrentEngine::message( const ContextState& state )
{
    DEBUG_BLOCK
    if( state == Current && m_requested )
        update();
}

void CurrentEngine::metadataChanged( Meta::Album* album )
{
    setData( "albumart", album->image( coverWidth() ) );
}

void CurrentEngine::update()
{
    DEBUG_BLOCK
    Meta::TrackPtr track = EngineController::instance()->currentTrack();
    uint length = EngineController::instance()->trackLength();
    
    QVariantMap trackInfo = Meta::Field::mapFromTrack( track.data() );

    int width = coverWidth();
    track->album()->subscribe( this );
    clearData( "current" );
    setData( "current", "albumart",  QVariant( track->album()->image( width ) ) );
    
    setData( "current", "current", trackInfo );
    
}

#include "CurrentEngine.moc"
