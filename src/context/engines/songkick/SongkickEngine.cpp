/****************************************************************************************
 * Copyright (c) 2008 Jeff Mitchell <mitchell@kde.org>                                  *
 * Copyright (c) 2007-2008 Leo Franchi <lfranchi@gmail.com>                             *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Pulic License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#include "SongkickEngine.h"

#include "JsonQt/lib/JsonToVariant.h"
#include "JsonQt/lib/ParseException.h"

#include "Amarok.h"
#include "Debug.h"
#include "ContextObserver.h"
#include "ContextView.h"
#include "EngineController.h"

#include <KIO/Job>

#include <QFile>
#include <QLocale>
#include <QUrl>

using namespace Context;

SongkickEngine::SongkickEngine( QObject* parent, const QList<QVariant>& args )
    : DataEngine( parent )
    , ContextObserver( ContextView::self() )
    , m_datesJob( 0 )
    , m_currentTrack( 0 )
    , m_ontour( true )
    , m_dates( true )
{
    Q_UNUSED( args )
    DEBUG_BLOCK

    m_sources << I18N_NOOP( "ontour" ) << I18N_NOOP( "dates" );
}

QStringList SongkickEngine::sources() const
{
    DEBUG_BLOCK
    return m_sources;
}

bool SongkickEngine::sourceRequestEvent( const QString& name )
{
    DEBUG_BLOCK
    debug() << "sourceRequested with name " << name;

    removeAllData( name );
    setData( name, "fetching" );
    update();
    return true;
}

void SongkickEngine::message( const ContextState& state )
{
    DEBUG_BLOCK
    if( state == Current )
        update();
}

void SongkickEngine::metadataChanged( Meta::TrackPtr track )
{
    Q_UNUSED( track )
    DEBUG_BLOCK

    update();
}

void
SongkickEngine::engineNewMetaData( const QHash<qint64, QString> &newMetaData, bool trackChanged )
{
    Q_UNUSED( trackChanged );
    Q_UNUSED( newMetaData );
    DEBUG_BLOCK

    update();
}

void SongkickEngine::update()
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

    QString country = QLocale::system().name().right( 2 ).toLower();
    KUrl ontourUrl( QString( "http://api.songkick.com/api/V2/get_tour_status?key=kJcAUmzi8AoAngzh&id=0&country=%2&range=all&name=%1" ).arg( QUrl::toPercentEncoding( currentTrack->artist()->prettyName() ), country ) );
    debug() << "getting ontour status: " << ontourUrl;
    m_ontourJob = KIO::storedGet( ontourUrl, KIO::NoReload, KIO::HideProgressInfo );
    connect( m_ontourJob, SIGNAL( result( KJob* ) ), this, SLOT( ontourResult( KJob* ) ) );

    KUrl datesUrl( QString( "http://api.songkick.com/api/V2/get_dates_extended?key=kJcAUmzi8AoAngzh&id=0&country=%2&range=all&name=%1" ).arg( QUrl::toPercentEncoding( currentTrack->artist()->prettyName() ), country ) );
    debug() << "getting concert dates: " << datesUrl;
    m_datesJob = KIO::storedGet( datesUrl, KIO::NoReload, KIO::HideProgressInfo );
    connect( m_datesJob, SIGNAL( result( KJob* ) ), this, SLOT( datesResult( KJob* ) ) );
} 

void SongkickEngine::datesResult( KJob* job )
{
    DEBUG_BLOCK
    if( job != m_datesJob )
        return;

    if( !m_datesJob )
        return;
    if( !job->error() == 0 && m_datesJob == job)
    {
        setData( "dates", "error" );
        return;
    }

    KIO::StoredTransferJob* const storedJob = static_cast<KIO::StoredTransferJob*>( job );
    QString data = QString( storedJob->data() );

    QVariantMap dates;
    /*QVariant datesResult = JsonQt::JsonToVariant::parse( data );

    debug() << "got dates: " << dates;
    QMapIterator< QString, QVariant > iter( dates );
    while( iter.hasNext() )
    {
        iter.next();
        setData( "dates", iter.key(), iter.value() );
    }
    */
    setData( "dates", data );
}

void SongkickEngine::ontourResult( KJob* job )
{
    DEBUG_BLOCK
    if( job != m_ontourJob )
        return;

    m_ontour = false;
    if( !m_ontourJob )
        return;
    if( !job->error() == 0 && m_ontourJob == job )
    {
        setData( "ontour", "error" );
        return;
    }

    KIO::StoredTransferJob* const storedJob = static_cast<KIO::StoredTransferJob*>( job );
    QString data = QString( storedJob->data() );
    QVariantMap status;
    setData( "ontour", data );
}

#include "SongkickEngine.moc"
