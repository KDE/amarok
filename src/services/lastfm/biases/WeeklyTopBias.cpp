/****************************************************************************************
 * Copyright (c) 2009 Leo Franchi <lfranchi@kde.org>                                    *
 * Copyright (c) 2011 Ralf Engels <ralf-engels@gmx.de>                                     *
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

#include "WeeklyTopBias.h"

#include "core/support/Debug.h"

#include "TrackSet.h"
#include "DynamicBiasWidgets.h"

#include <QDomDocument>
#include <QDomNode>
#include <QDomElement>

#include <QXmlStreamReader>
#include <QXmlStreamWriter>

#include "core/meta/Meta.h"
#include "core/collections/Collection.h"
#include "core/collections/QueryMaker.h"
#include "core-impl/collections/support/CollectionManager.h"

#include "lastfm/Artist"
#include "lastfm/ws.h"
#include "lastfm/XmlQuery"

#include <QTimeEdit>
#include <QFormLayout>

#include <QNetworkReply>

#include <QSignalMapper>



QString
Dynamic::WeeklyTopBiasFactory::i18nName() const
{ return i18nc("Name of the \"WeeklyTop\" bias", "LastFM weekly top artist"); }

QString
Dynamic::WeeklyTopBiasFactory::name() const
{ return Dynamic::WeeklyTopBias::sName(); }

QString
Dynamic::WeeklyTopBiasFactory::i18nDescription() const
{ return i18nc("Description of the \"WeeklyTop\" bias",
                   "The \"WeeklyTop\" bias adds tracks that are in the weekly top chart of LastFM."); }

Dynamic::BiasPtr
Dynamic::WeeklyTopBiasFactory::createBias()
{ return Dynamic::BiasPtr( new Dynamic::WeeklyTopBias() ); }

Dynamic::BiasPtr
Dynamic::WeeklyTopBiasFactory:: createBias( QXmlStreamReader *reader )
{ return Dynamic::BiasPtr( new Dynamic::WeeklyTopBias( reader ) ); }



// ----- WeeklyTopBias --------


Dynamic::WeeklyTopBias::WeeklyTopBias()
    : SimpleMatchBias()
    , m_weeklyTimesJob( 0 )
{
    loadFromFile();
}

Dynamic::WeeklyTopBias::WeeklyTopBias( QXmlStreamReader *reader )
    : SimpleMatchBias()
    , m_weeklyTimesJob( 0 )
{
    loadFromFile();

    while (!reader->atEnd()) {
        reader->readNext();

        if( reader->isStartElement() )
        {
            QStringRef name = reader->name();
            if( name == "from" )
                m_range.from = QDateTime::fromTime_t( reader->readElementText(QXmlStreamReader::SkipChildElements).toLong() );
            if( name == "to" )
                m_range.to = QDateTime::fromTime_t( reader->readElementText(QXmlStreamReader::SkipChildElements).toLong() );
            else
            {
                debug()<<"Unexpected xml start element"<<reader->name()<<"in input";
                reader->skipCurrentElement();
            }
        }
        else if( reader->isEndElement() )
        {
            break;
        }
    }
}

Dynamic::WeeklyTopBias::~WeeklyTopBias()
{
}

void
Dynamic::WeeklyTopBias::toXml( QXmlStreamWriter *writer ) const
{
    writer->writeTextElement( "from", QString::number( m_range.from.toTime_t() ) );
    writer->writeTextElement( "to",   QString::number( m_range.to.toTime_t() ) );
}

QString
Dynamic::WeeklyTopBias::sName()
{
    return "lastfm_weeklytop";
}

QString
Dynamic::WeeklyTopBias::name() const
{
    return Dynamic::WeeklyTopBias::sName();
}

QWidget*
Dynamic::WeeklyTopBias::widget( QWidget* parent )
{
    PlaylistBrowserNS::BiasWidget *bw = new PlaylistBrowserNS::BiasWidget( BiasPtr(this), parent );

    QDateTimeEdit *fromEdit = new QDateTimeEdit( QDate::currentDate().addDays( -7 ) );
    fromEdit->setMinimumDate( QDate::currentDate().addYears( -10 ) ); // 10 years ago is minimum for now
    fromEdit->setMaximumDate( QDate::currentDate() );
    fromEdit->setCalendarPopup( true );
    if( m_range.from.isValid() )
        fromEdit->setDateTime( m_range.from );

    bw->formLayout()->addRow( i18n( "from:" ), fromEdit );

    connect( fromEdit, SIGNAL( dateTimeChanged( const QDateTime& ) ),
             this, SLOT( fromDateChanged( const QDateTime& ) ) );


    QDateTimeEdit *toEdit = new QDateTimeEdit( QDate::currentDate().addDays( -7 ) );
    toEdit->setMinimumDate( QDate::currentDate().addYears( -10 ) ); // 10 years ago is minimum for now
    toEdit->setMaximumDate( QDate::currentDate() );
    toEdit->setCalendarPopup( true );
    if( m_range.to.isValid() )
        toEdit->setDateTime( m_range.to );

    bw->formLayout()->addRow( i18n( "to:" ), toEdit );

    connect( toEdit, SIGNAL( dateTimeChanged( const QDateTime& ) ),
             this, SLOT( toDateChanged( const QDateTime& ) ) );


    return bw;
}


bool
Dynamic::WeeklyTopBias::trackMatches( int position,
                                   const Meta::TrackList& playlist,
                                   int contextCount ) const
{
    Q_UNUSED( contextCount );

    if( position < 0 || position >= playlist.count())
        return false;

    // - determine the current artist
    Meta::TrackPtr currentTrack = playlist[position-1];
    Meta::ArtistPtr currentArtist = currentTrack->artist();
    QString currentArtistName = currentArtist ? currentArtist->name() : QString();

    // - collect all the artists
    QStringList artists;
    bool weeksMissing = false;

    uint fromTime = m_range.from.toTime_t();
    uint toTime   = m_range.to.toTime_t();
    uint lastWeekTime = 0;
    foreach( uint weekTime, m_weeklyFromTimes )
    {
        if( weekTime > fromTime && weekTime < toTime && lastWeekTime )
        {
            if( m_weeklyArtistMap.contains( lastWeekTime ) )
            {
                artists.append( m_weeklyArtistMap.value( lastWeekTime ) );
                debug() << "found already-saved data for week:" << lastWeekTime << m_weeklyArtistMap.value( lastWeekTime );
            }
            else
            {
                weeksMissing = true;
            }
        }

       lastWeekTime = weekTime;
    }

    if( weeksMissing )
        warning() << "didn't have a cached suggestions for weeks:" << m_range.from << "to" << m_range.to;

    return artists.contains( currentArtistName );
}

void
Dynamic::WeeklyTopBias::newQuery()
{
    DEBUG_BLOCK;

    // - check if we have week times
    if( m_weeklyFromTimes.isEmpty() )
    {
        newWeeklyTimesQuery();
        return; // not yet ready to do construct a query maker
    }

    // - collect all the artists
    QStringList artists;
    bool weeksMissing = false;

    uint fromTime = m_range.from.toTime_t();
    uint toTime   = m_range.to.toTime_t();
    uint lastWeekTime = 0;
    foreach( uint weekTime, m_weeklyFromTimes )
    {
        if( weekTime > fromTime && weekTime < toTime && lastWeekTime )
        {
            if( m_weeklyArtistMap.contains( lastWeekTime ) )
            {
                artists.append( m_weeklyArtistMap.value( lastWeekTime ) );
                debug() << "found already-saved data for week:" << lastWeekTime << m_weeklyArtistMap.value( lastWeekTime );
            }
            else
            {
                weeksMissing = true;
            }
        }

       lastWeekTime = weekTime;
    }

    if( weeksMissing )
    {
        newWeeklyArtistQuery();
        return; // not yet ready to construct a query maker
    }

    // ok, I need a new query maker
    m_qm.reset( CollectionManager::instance()->queryMaker() );

    // - construct the query
    m_qm->beginOr();
    foreach( const QString &artist, artists )
    {
        debug() << "adding artist to query:" << artist;
        m_qm->addFilter( Meta::valArtist, artist, true, true );
    }
    m_qm->endAndOr();

    m_qm->setQueryType( Collections::QueryMaker::Custom );
    m_qm->addReturnValue( Meta::valUniqueId );

    connect( m_qm.data(), SIGNAL(newResultReady( QString, QStringList )),
             this, SLOT(updateReady( QString, QStringList )) );
    connect( m_qm.data(), SIGNAL(queryDone()),
             this, SLOT(updateFinished()) );

    // - run the query
    m_qm.data()->run();
}

void
Dynamic::WeeklyTopBias::newWeeklyTimesQuery()
{
    DEBUG_BLOCK

    QMap< QString, QString > params;
    params[ "method" ] = "user.getWeeklyChartList" ;
    params[ "user" ] = lastfm::ws::Username;

    m_weeklyTimesJob = lastfm::ws::get( params );

    connect( m_weeklyTimesJob, SIGNAL( finished() ),
             this, SLOT( weeklyTimesQueryFinished() ) );
}


void Dynamic::WeeklyTopBias::newWeeklyArtistQuery()
{
    DEBUG_BLOCK
    debug() << "getting top artist info from" << m_range.from << "to" << m_range.to;

    // - check if we have week times
    if( m_weeklyFromTimes.isEmpty() )
    {
        newWeeklyTimesQuery();
        return; // not yet ready to do construct a query maker
    }

    // fetch 5 at a time, so as to conform to lastfm api requirements
    uint jobCount = m_weeklyArtistJobs.count();
    if( jobCount >= 5 )
        return;

    uint fromTime = m_range.from.toTime_t();
    uint toTime   = m_range.to.toTime_t();
    uint lastWeekTime = 0;
    foreach( uint weekTime, m_weeklyFromTimes )
    {
        if( weekTime > fromTime && weekTime < toTime && lastWeekTime )
        {
            if( m_weeklyArtistMap.contains( lastWeekTime ) )
            {
                // we already have the data
            }
            else if( m_weeklyArtistJobs.contains( lastWeekTime ) )
            {
                // we already fetch the data
            }
            else
            {
                QMap< QString, QString > params;
                params[ "method" ] = "user.getWeeklyArtistChart";
                params[ "user" ] = lastfm::ws::Username;
                params[ "from" ] = QString::number( lastWeekTime );
                params[ "to" ] = QString::number( m_weeklyToTimes[m_weeklyFromTimes.indexOf(lastWeekTime)] );

                QNetworkReply* reply = lastfm::ws::get( params );
                connect( reply, SIGNAL( finished() ),
                         this, SLOT( weeklyArtistQueryFinished() ) );

                m_weeklyArtistJobs.insert( lastWeekTime, reply );

                jobCount++;
                if( jobCount >= 5 )
                    return;
            }
        }

       lastWeekTime = weekTime;
    }
}


void
Dynamic::WeeklyTopBias::weeklyArtistQueryFinished()
{
    DEBUG_BLOCK
    QNetworkReply *reply = qobject_cast<QNetworkReply*>( sender() );

    if( !reply ) {
        warning() << "Failed to get qnetwork reply in finished slot.";
        return;
    }


    try
    {
        lastfm::XmlQuery lfm( reply->readAll() );

        // debug() << "got response:" << lfm;
        QStringList artists;
        for( int i = 0; i < lfm[ "weeklyartistchart" ].children( "artist" ).size(); i++ )
        {
            if( i == 12 ) // only up to 12 artist.
                break;
            lastfm::XmlQuery artist = lfm[ "weeklyartistchart" ].children( "artist" ).at( i );
            artists.append( artist[ "name" ].text() );
        }

        uint week = QDomElement( lfm[ "weeklyartistchart" ] ).attribute( "from" ).toUInt();
        m_weeklyArtistMap.insert( week, artists );
        debug() << "got artists:" << artists << week;

        if( m_weeklyArtistJobs.contains( week) )
        {
            m_weeklyArtistJobs.remove( week );
        }
        else
        {
            warning() << "Got a reply for a week"<<week<<"that was not requested.";
            return;
        }

    } catch( lastfm::ws::ParseError& e )
    {
        debug() << "caught exception parsing weekly artist chart.";
    }

    reply->deleteLater();

    saveDataToFile();
    newQuery(); // try again to get the tracks
}

void
Dynamic::WeeklyTopBias::weeklyTimesQueryFinished() // SLOT
{
    DEBUG_BLOCK
    if( !m_weeklyTimesJob )
        return; // argh. where does this come from

    QDomDocument doc;
    if( !doc.setContent( m_weeklyTimesJob->readAll() ) )
    {
        debug() << "couldn't parse XML from rangeJob!";
        return;
    }

    QDomNodeList nodes = doc.elementsByTagName( "chart" );
    if( nodes.count() == 0 )
    {
        debug() << "USER has no history! can't do this!";
        return;
    }

    for( int i = 0; i < nodes.size(); i++ )
    {
        QDomNode n = nodes.at( i );
        m_weeklyFromTimes.append( n.attributes().namedItem( "from" ).nodeValue().toUInt() );
        m_weeklyToTimes.append( n.attributes().namedItem( "to" ).nodeValue().toUInt() );
    }

    /*
       m_earliestDate = m_weeklyFromTimes.first();

    debug() << "got earliest date:"  << QDateTime::fromTime_t( m_earliestDate ).toString();
    if( m_fromEdit )
        m_fromEdit->setMinimumDate( QDateTime::fromTime_t( m_earliestDate ).date() );
    */

    m_weeklyTimesJob->deleteLater();

    newQuery(); // try again to get the tracks
}

void
Dynamic::WeeklyTopBias::fromDateChanged( const QDateTime& d ) // SLOT
{
    m_range.from = d;
    invalidate();
    emit changed( BiasPtr( this ) );
}

void
Dynamic::WeeklyTopBias::toDateChanged( const QDateTime& d ) // SLOT
{
    m_range.to = d;
    invalidate();
    emit changed( BiasPtr( this ) );
}


/*

void
Dynamic::WeeklyTopBias::update()
{
    DEBUG_BLOCK
    
    debug() << m_fromDate << m_toDate;
    if( m_fromDate >= m_toDate )
    {
        debug() << "Chose date limits that don't make sense! do nothing!";
        return;
    } else if( m_fromDate < m_earliestDate )
    {
        debug() << "User doesn't have history that goes back this far! rounding!";
        m_fromDate = m_earliestDate;
    }

    fetchWeeklyData( m_fromDate, m_toDate );
}
*/


void
Dynamic::WeeklyTopBias::loadFromFile()
{
    QFile file( Amarok::saveLocation() + "dynamic_lastfm_topweeklyartists.xml" );
    file.open( QIODevice::ReadOnly | QIODevice::Text );
    QTextStream in( &file );
    while( !in.atEnd() )
    {
        QString line = in.readLine();
        m_weeklyArtistMap.insert( line.split( "#" )[ 0 ].toUInt(), line.split( "#" )[ 1 ].split( "^" )  );
    }
    file.close();
}


void
Dynamic::WeeklyTopBias::saveDataToFile() const
{
    QFile file( Amarok::saveLocation() + "dynamic_lastfm_topweeklyartists.xml" );
    file.open( QIODevice::Truncate | QIODevice::WriteOnly | QIODevice::Text );
    QTextStream out( &file );
    foreach( uint key, m_weeklyArtistMap.keys() )
    {
        out << key << "#" << m_weeklyArtistMap[ key ].join( "^" ) << endl;
    }
    file.close();

}


#include "WeeklyTopBias.moc"
