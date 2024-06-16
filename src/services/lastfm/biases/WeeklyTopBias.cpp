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

#include "core/meta/Meta.h"
#include "core/support/Amarok.h"
#include "core/support/Debug.h"
#include "core-impl/collections/support/CollectionManager.h"

#include <KLocalizedString>

#include <QDomDocument>
#include <QDomElement>
#include <QDomNode>
#include <QLabel>
#include <QNetworkReply>
#include <QTimeEdit>
#include <QVBoxLayout>
#include <QXmlStreamReader>

#include <XmlQuery.h>

QString
Dynamic::WeeklyTopBiasFactory::i18nName() const
{ return i18nc("Name of the \"WeeklyTop\" bias", "Last.fm weekly top artist"); }

QString
Dynamic::WeeklyTopBiasFactory::name() const
{ return Dynamic::WeeklyTopBias::sName(); }

QString
Dynamic::WeeklyTopBiasFactory::i18nDescription() const
{ return i18nc("Description of the \"WeeklyTop\" bias",
                   "The \"WeeklyTop\" bias adds tracks that are in the weekly top chart of Last.fm."); }

Dynamic::BiasPtr
Dynamic::WeeklyTopBiasFactory::createBias()
{ return Dynamic::BiasPtr( new Dynamic::WeeklyTopBias() ); }


// ----- WeeklyTopBias --------


Dynamic::WeeklyTopBias::WeeklyTopBias()
    : SimpleMatchBias()
    , m_weeklyTimesJob( )
{
    m_range.from = QDateTime::currentDateTime();
    m_range.to = QDateTime::currentDateTime();
    loadFromFile();
}

Dynamic::WeeklyTopBias::~WeeklyTopBias()
{ }


void
Dynamic::WeeklyTopBias::fromXml( QXmlStreamReader *reader )
{
    loadFromFile();

    while (!reader->atEnd()) {
        reader->readNext();

        if( reader->isStartElement() )
        {
            QStringRef name = reader->name();
            if( name == "from" )
                m_range.from = QDateTime::fromSecsSinceEpoch( reader->readElementText(QXmlStreamReader::SkipChildElements).toLong() );
            else if( name == "to" )
                m_range.to = QDateTime::fromSecsSinceEpoch( reader->readElementText(QXmlStreamReader::SkipChildElements).toLong() );
            else
            {
                debug()<<"Unexpected xml start element"<<name<<"in input";
                reader->skipCurrentElement();
            }
        }
        else if( reader->isEndElement() )
        {
            break;
        }
    }
}

void
Dynamic::WeeklyTopBias::toXml( QXmlStreamWriter *writer ) const
{
    writer->writeTextElement( "from", QString::number( m_range.from.toSecsSinceEpoch() ) );
    writer->writeTextElement( "to",   QString::number( m_range.to.toSecsSinceEpoch() ) );
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

QString
Dynamic::WeeklyTopBias::toString() const
{
    return i18nc("WeeklyTopBias bias representation",
                 "Tracks from the Last.fm top lists from %1 to %2", m_range.from.toString(), m_range.to.toString() );
}

QWidget*
Dynamic::WeeklyTopBias::widget( QWidget* parent )
{
    QWidget *widget = new QWidget( parent );
    QVBoxLayout *layout = new QVBoxLayout( widget );

    QLabel *label = new QLabel( i18nc( "in WeeklyTopBias. Label for the date widget", "from:" ) );
    QDateTimeEdit *fromEdit = new QDateTimeEdit( QDate::currentDate().addDays( -7 ) );
    fromEdit->setMinimumDate( QDateTime::fromSecsSinceEpoch( 1111320001 ).date() ); // That's the first week in last fm
    fromEdit->setMaximumDate( QDate::currentDate() );
    fromEdit->setCalendarPopup( true );
    if( m_range.from.isValid() )
        fromEdit->setDateTime( m_range.from );

    connect( fromEdit, &QDateTimeEdit::dateTimeChanged, this, &WeeklyTopBias::fromDateChanged );
    label->setBuddy( fromEdit );
    layout->addWidget( label );
    layout->addWidget( fromEdit );

    label = new QLabel( i18nc( "in WeeklyTopBias. Label for the date widget", "to:" ) );
    QDateTimeEdit *toEdit = new QDateTimeEdit( QDate::currentDate().addDays( -7 ) );
    toEdit->setMinimumDate( QDateTime::fromSecsSinceEpoch( 1111320001 ).date() ); // That's the first week in last fm
    toEdit->setMaximumDate( QDate::currentDate() );
    toEdit->setCalendarPopup( true );
    if( m_range.to.isValid() )
        toEdit->setDateTime( m_range.to );

    connect( toEdit, &QDateTimeEdit::dateTimeChanged, this, &WeeklyTopBias::toDateChanged );
    label->setBuddy( toEdit );
    layout->addWidget( label );
    layout->addWidget( toEdit );

    return widget;
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

    uint fromTime = m_range.from.toSecsSinceEpoch();
    uint toTime   = m_range.to.toSecsSinceEpoch();
    uint lastWeekTime = 0;
    for( uint weekTime : m_weeklyFromTimes )
    {
        if( weekTime > fromTime && weekTime < toTime && lastWeekTime )
        {
            if( m_weeklyArtistMap.contains( lastWeekTime ) )
            {
                artists.append( m_weeklyArtistMap.value( lastWeekTime ) );
                // debug() << "found already-saved data for week:" << lastWeekTime << m_weeklyArtistMap.value( lastWeekTime );
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

    uint fromTime = m_range.from.toSecsSinceEpoch();
    uint toTime   = m_range.to.toSecsSinceEpoch();
    uint lastWeekTime = 0;
    for( uint weekTime : m_weeklyFromTimes )
    {
        if( weekTime > fromTime && weekTime < toTime && lastWeekTime )
        {
            if( m_weeklyArtistMap.contains( lastWeekTime ) )
            {
                artists.append( m_weeklyArtistMap.value( lastWeekTime ) );
                // debug() << "found already-saved data for week:" << lastWeekTime << m_weeklyArtistMap.value( lastWeekTime );
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
    for( const QString &artist : artists )
    {
        // debug() << "adding artist to query:" << artist;
        m_qm->addFilter( Meta::valArtist, artist, true, true );
    }
    m_qm->endAndOr();

    m_qm->setQueryType( Collections::QueryMaker::Custom );
    m_qm->addReturnValue( Meta::valUniqueId );

    connect( m_qm.data(), &Collections::QueryMaker::newResultReady,
             this, &WeeklyTopBias::updateReady );
    connect( m_qm.data(), &Collections::QueryMaker::queryDone,
             this, &WeeklyTopBias::updateFinished );

    // - run the query
    m_qm->run();
}

void
Dynamic::WeeklyTopBias::newWeeklyTimesQuery()
{
    DEBUG_BLOCK

    QMap< QString, QString > params;
    params[ "method" ] = "user.getWeeklyChartList" ;
    params[ "user" ] = lastfm::ws::Username;

    m_weeklyTimesJob = lastfm::ws::get( params );

    connect( m_weeklyTimesJob, &QNetworkReply::finished,
             this, &WeeklyTopBias::weeklyTimesQueryFinished );
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

    uint fromTime = m_range.from.toSecsSinceEpoch();
    uint toTime   = m_range.to.toSecsSinceEpoch();
    uint lastWeekTime = 0;
    for( uint weekTime : m_weeklyFromTimes )
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
                connect( reply, &QNetworkReply::finished,
                         this, &WeeklyTopBias::weeklyArtistQueryFinished );

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


    lastfm::XmlQuery lfm;
    if( lfm.parse( reply->readAll() ) )
    {
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
    }
    else
    {
        debug() << "failed to parse weekly artist chart.";
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

        // debug() << "weeklyTimesResult"<<i<<":"<<m_weeklyFromTimes.last()<<"to"<<m_weeklyToTimes.last();
        m_weeklyFromTimes.append( n.attributes().namedItem( "from" ).nodeValue().toUInt() );
        m_weeklyToTimes.append( n.attributes().namedItem( "to" ).nodeValue().toUInt() );
    }

    m_weeklyTimesJob->deleteLater();

    newQuery(); // try again to get the tracks
}


void
Dynamic::WeeklyTopBias::fromDateChanged( const QDateTime& d ) // SLOT
{
    if( d > m_range.to )
        return;

    m_range.from = d;
    invalidate();
    Q_EMIT changed( BiasPtr( this ) );
}


void
Dynamic::WeeklyTopBias::toDateChanged( const QDateTime& d ) // SLOT
{
    if( d < m_range.from )
        return;

    m_range.to = d;
    invalidate();
    Q_EMIT changed( BiasPtr( this ) );
}


void
Dynamic::WeeklyTopBias::loadFromFile()
{
    QFile file( Amarok::saveLocation() + "dynamic_lastfm_topweeklyartists.xml" );
    file.open( QIODevice::ReadOnly | QIODevice::Text );
    QTextStream in( &file );
    while( !in.atEnd() )
    {
        QString line = in.readLine();
        m_weeklyArtistMap.insert( line.split( '#' )[ 0 ].toUInt(), line.split( '#' )[ 1 ].split( '^' )  );
    }
    file.close();
}


void
Dynamic::WeeklyTopBias::saveDataToFile() const
{
    QFile file( Amarok::saveLocation() + "dynamic_lastfm_topweeklyartists.xml" );
    file.open( QIODevice::Truncate | QIODevice::WriteOnly | QIODevice::Text );
    QTextStream out( &file );
    for( uint key : m_weeklyArtistMap.keys() )
    {
        out << key << "#" << m_weeklyArtistMap[ key ].join( "^" ) << Qt::endl;
    }
    file.close();

}


