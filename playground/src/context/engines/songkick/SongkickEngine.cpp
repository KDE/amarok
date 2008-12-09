/***************************************************************************
 * copyright            : (C) 2008 Jeff Mitchell <mitchell@kde.org>        *
 *                        (C) 2007-2008 Leo Franchi <lfranchi@gmail.com>   *
 **************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "SongkickEngine.h"

#include "JsonQt/JsonToVariant.h"
#include "JsonQt/ParseException.h"

#include "Amarok.h"
#include "Debug.h"
#include "ContextObserver.h"
#include "ContextView.h"
#include "EngineController.h"

#include <KIO/Job>

#include <QLocale>
#include <QFile>

using namespace Context;

SongkickEngine::SongkickEngine( QObject* parent, const QList<QVariant>& args )
    : DataEngine( parent )
    , ContextObserver( ContextView::self() )
    , m_datesJob( 0 )
    , m_currentTrack( 0 )
    , m_ontour( false )
    , m_dates( true )
{
    Q_UNUSED( args )
    DEBUG_BLOCK

    m_sources << I18N_NOOP( "ontour" ) << I18N_NOOP( "dates" );

}

SongkickEngine::~SongkickEngine()
{
    DEBUG_BLOCK
}

QStringList SongkickEngine::sources() const
{
    DEBUG_BLOCK
    return m_sources;
}

bool SongkickEngine::sourceRequested( const QString& name )
{
    DEBUG_BLOCK
    debug() << "sourceRequested with name " << name;
    if( name == I18N_NOOP( "ontour" ) )
    {
        m_ontour = true;
    } else if( name == I18N_NOOP( "dates" ) )
    {
        m_dates = true;
    } else
    {
        debug() << "data source not found!";
        return false;
    }

    setData( name, QVariant());
    updateData();
    return true;
}

void SongkickEngine::message( const ContextState& state )
{
    DEBUG_BLOCK
    if( state == Current )
        updateData();

}

void SongkickEngine::metadataChanged( Meta::TrackPtr track )
{
    Q_UNUSED( track )
    DEBUG_BLOCK

    updateData();
}

// takes care of fetching events: from network //if needed, otherwise from cache
void SongkickEngine::updateData()
{
    DEBUG_BLOCK

    unsubscribeFrom( m_currentTrack );
    Meta::TrackPtr currentTrack = The::engineController()->currentTrack();
    m_currentTrack = currentTrack;
    subscribeTo( currentTrack );

    if ( !currentTrack )
    {
        debug() << "No current track!";
        return;
    }
    else if ( !currentTrack->artist() )
    {
        debug() << "No artist found!";
        return;
    }

    setData( "artist", currentTrack->artist()->prettyName() );

    if( m_dates )
    {
        debug() << "getting concert dates";

        // do dates
        /*
        QString cached = getCached( QString( Amarok::saveLocation() + "songkick.dates/dates.cache" ) );
        if( cached.isEmpty() ) // not cached, lets fetch it
        {
            debug() << "got no cached dates";
        */

            QString country = QLocale::system().name().right( 2 ).toLower();
            KUrl url( QString( "http://api.songkick.com/api/V1/get_dates_extended?id=0&key=kJcAUmzi8AoAngzh&name=%1&country=%2&range=all" ).arg( QUrl::toPercentEncoding( currentTrack->artist()->prettyName() ), country ) );
            m_datesJob = KIO::storedGet( url, KIO::NoReload, KIO::HideProgressInfo );
            connect( m_datesJob, SIGNAL( result( KJob* ) ), this, SLOT( datesResult( KJob* ) ) );
        /*
        }
        else // load from cache
        {
            debug() << "got cached dates";
            QVariantMap events = m_cacheMap( m_artist );
            QMapIterator< QString, QVariant > iter( events );
            while( iter.hasNext() )
            {
                iter.next();
                setData( "userevents", iter.key(), iter.value() );
            }
        }
        */
    }
}

void SongkickEngine::datesResult( KJob* job )
{
    DEBUG_BLOCK
    if( !m_datesJob ) return;
    if( !job->error() == 0  && m_datesJob == job)
    {
        setData( "dates", "error" );
        return;
    }
    if( job != m_datesJob ) return;

    KIO::StoredTransferJob* const storedJob = static_cast<KIO::StoredTransferJob*>( job );
    QString data = QString( storedJob->data() );

/*    // cache data for later attempts
    QFile cache( QString( Amarok::saveLocation() + "songkick.events/events.rss" ) );
    if ( cache.open( QFile::WriteOnly | QFile::Truncate ) )
    {
        QTextStream out( &cache );
        out << data;
    }
*/

    QVariantMap dates;
    /*try{ dates = JsonQt::JsonToVariant::parse( data ); }
    catch( JsonQt::ParseException e )
    {
        debug() << "Parse exception (stopping parsing): " << e.what();
    }
    debug() << "got dates: " << dates;
    QMapIterator< QString, QVariant > iter( dates );
    while( iter.hasNext() )
    {
        iter.next();
        setData( "dates", iter.key(), iter.value() );
    }*/
    setData( "dates", data );
}

QString SongkickEngine::getCached( QString path )
{
    QFile cache( path );
    QString contents;
    if( cache.open( QFile::ReadOnly ) )
    {
        QTextStream cachestream( &cache );
        contents = cachestream.readAll();
    }
    return contents;
}

#include "SongkickEngine.moc"
