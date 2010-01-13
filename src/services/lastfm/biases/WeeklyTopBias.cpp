/****************************************************************************************
 * Copyright (c) 2009 Leo Franchi <lfranchi@kde.org>                                    *
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

#include "Collection.h"
#include "CollectionManager.h"
#include "Debug.h"
#include "Meta.h"
#include "QueryMaker.h"

#include <lastfm/Artist>
#include <lastfm/ws.h>
#include <lastfm/XmlQuery>

#include <QByteArray>
#include <QDate>
#include <QDomDocument>
#include <QFrame>
#include <QLabel>
#include <QLineEdit>
#include <QNetworkReply>
#include <QTimeEdit>
#include <QVBoxLayout>

// class WeeklyTopBiasFactory
#include <QSignalMapper>

Dynamic::WeeklyTopBiasFactory::WeeklyTopBiasFactory()
{

}

Dynamic::WeeklyTopBiasFactory::~WeeklyTopBiasFactory()
{

}


QString
Dynamic::WeeklyTopBiasFactory::name() const
{
    return i18n( "Weekly Top Artists" );
}

QString
Dynamic::WeeklyTopBiasFactory::pluginName() const
{
    return "lastfm_weeklytop";
}

Dynamic::CustomBiasEntry*
Dynamic::WeeklyTopBiasFactory::newCustomBias( double weight )
{
    return new Dynamic::WeeklyTopBias( weight );
}

Dynamic::CustomBiasEntry*
Dynamic::WeeklyTopBiasFactory::newCustomBias( QDomElement e, double weight )
{

    debug() << "weekly top created with:" << e;
    uint from = e.firstChildElement( "from" ).attribute( "value" ).toUInt();
    uint to = e.firstChildElement( "to" ).attribute( "value" ).toUInt();
    return new Dynamic::WeeklyTopBias( weight, from, to );
}

// CLASS WeeklyTopBias

Dynamic::WeeklyTopBias::WeeklyTopBias( double weight, uint from, uint to )
    : Dynamic::CustomBiasEntry( weight )
    , m_qm( 0 )
    , m_layout( 0 )
    , m_fromEdit( 0 )
    , m_toEdit( 0 )
    , m_fromDate( from )
    , m_toDate( to )
    , m_fetching( 0 )
    , m_rangeJob( 0 )
    , m_dataJob( 0 )
{
    QFile file( Amarok::saveLocation() + "dynamic_lastfm_topweeklyartists.xml" );
    file.open( QIODevice::ReadOnly | QIODevice::Text );
    QTextStream in( &file );
    while( !in.atEnd() )
    {
        QString line = in.readLine();
        m_weeklyChartData.insert( line.split( "#" )[ 0 ].toUInt(), line.split( "#" )[ 1 ].split( "^" )  );
    }
    file.close();
   
    getPossibleRange();
    
    if( from != 0 && to != 0 )
        fetchWeeklyData( from, to );

}

Dynamic::WeeklyTopBias::~WeeklyTopBias()
{
    //delete m_qm;
    delete m_fetching;
}

QString
Dynamic::WeeklyTopBias::pluginName() const
{
    return "lastfm_weeklytop";
}

QWidget*
Dynamic::WeeklyTopBias::configWidget( QWidget* parent )
{
    DEBUG_BLOCK

    QFrame * frame = new QFrame( parent );
    m_layout = new QVBoxLayout( frame );

    QLabel * label = new QLabel( i18n( "Play Top Artists From" ), frame );
    label->setWordWrap( true );
    label->setAlignment( Qt::AlignCenter );
    m_layout->addWidget( label, Qt::AlignCenter );

    m_fromEdit = new QDateTimeEdit( QDate::currentDate().addDays( -7 ) );
    m_fromEdit->setMinimumDate( QDate::currentDate().addYears( -10 ) ); // 10 years ago is minimum for now
    m_fromEdit->setMaximumDate( QDate::currentDate() );
    m_fromEdit->setCalendarPopup( true );
    if( m_fromDate > 0 )
        m_fromEdit->setDateTime( QDateTime::fromTime_t( m_fromDate ) );
    
    m_layout->addWidget( m_fromEdit );

    connect( m_fromEdit, SIGNAL( dateTimeChanged( const QDateTime& ) ), this, SLOT( fromDateChanged( const QDateTime& ) ) );

    QLabel * to = new QLabel( i18nc( "From one date to another, this text is in between", "to (will round to nearest week)" ), parent );

    to->setAlignment( Qt::AlignCenter );
    m_layout->addWidget( to, Qt::AlignCenter );

    m_toEdit =new QDateTimeEdit( QDate::currentDate().addDays( -7 ) );
    m_toEdit->setMinimumDate( QDate::currentDate().addYears( -10 ) ); // 10 years ago is minimum for now
    m_toEdit->setMaximumDate( QDate::currentDate() );
    m_toEdit->setCalendarPopup( true );
    if( m_toDate > 0 )
        m_toEdit->setDateTime( QDateTime::fromTime_t( m_toDate ) );
    
    m_layout->addWidget( m_toEdit );

    connect( m_toEdit, SIGNAL( dateTimeChanged( const QDateTime& ) ), this, SLOT( toDateChanged( const QDateTime& ) ) );

    return frame;
}

bool
Dynamic::WeeklyTopBias::trackSatisfies( const Meta::TrackPtr track )
{
    Q_UNUSED( track )
    return false;
}

double
Dynamic::WeeklyTopBias::numTracksThatSatisfy( const Meta::TrackList& tracks )
{
    Q_UNUSED( tracks )
    return 0;
}

QDomElement
Dynamic::WeeklyTopBias::xml( QDomDocument doc ) const
{
    QDomElement e = doc.createElement( "custombias" );
    e.setAttribute( "type", "weeklytop" );

    QDomElement from = doc.createElement( "from" );
    from.setAttribute( "value", QString::number( m_fromDate ) );

    QDomElement to = doc.createElement( "to" );
    to.setAttribute( "value", QString::number( m_toDate ) );

    e.appendChild( from );
    e.appendChild( to );

    return e;
}

bool
Dynamic::WeeklyTopBias::hasCollectionFilterCapability()
{
    return true;
}

Dynamic::CollectionFilterCapability*
Dynamic::WeeklyTopBias::collectionFilterCapability()
{

    debug() << "returning new cfb with weight:" << weight();
    return new Dynamic::WeeklyTopBiasCollectionFilterCapability( this );
}

void
Dynamic::WeeklyTopBias::fromDateChanged( const QDateTime& d ) // SLOT
{
    DEBUG_BLOCK
    m_fromDate = d.toTime_t();

    update();
}

void
Dynamic::WeeklyTopBias::toDateChanged( const QDateTime& d ) // SLOT
{
    DEBUG_BLOCK

    m_toDate = d.toTime_t();

    update();

}

void
Dynamic::WeeklyTopBias::rangeJobFinished() // SLOT
{
    DEBUG_BLOCK
    if( !m_rangeJob )
        return;

    QDomDocument doc;
    if( !doc.setContent( m_rangeJob->readAll() ) )
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

    m_earliestDate = nodes.at( 0 ).attributes().namedItem( "from" ).nodeValue().toUInt();

    for( int i = 0; i < nodes.size(); i++ )
    {
        QDomNode n = nodes.at( i );
        m_weeklyCharts.append( n.attributes().namedItem( "from" ).nodeValue().toUInt() );
        m_weeklyChartsTo.append( n.attributes().namedItem( "to" ).nodeValue().toUInt() );
    }
    
    debug() << "got earliest date:"  << QDateTime::fromTime_t( m_earliestDate ).toString();
    if( m_fromEdit )
        m_fromEdit->setMinimumDate( QDateTime::fromTime_t( m_earliestDate ).date() );

    m_rangeJob->deleteLater();
}

void
Dynamic::WeeklyTopBias::updateReady( QString collectionId, QStringList uids )
{
    DEBUG_BLOCK
    Q_UNUSED( collectionId );

    int protocolLength = ( QString( m_collection->uidUrlProtocol() ) + "://" ).length();

//     debug() << "setting cache of top UIDs for selected date: to:" << uids;
    m_trackList.clear();
    m_trackList.reserve( uids.size() );

    QByteArray uid;
    foreach( const QString &uidString, uids )
    {
        uid = uidString.mid( protocolLength ).toAscii();
        m_trackList.insert( uid );
    }

}


void
Dynamic::WeeklyTopBias::getPossibleRange()
{
    DEBUG_BLOCK

    QMap< QString, QString > params;
    params[ "method" ] = "user.getWeeklyChartList" ;
    params[ "user" ] = lastfm::ws::Username;

    m_rangeJob = lastfm::ws::get( params );

    connect( m_rangeJob, SIGNAL( finished() ), this, SLOT( rangeJobFinished() ) );

}


void
Dynamic::WeeklyTopBias::update()
{
    DEBUG_BLOCK

    debug() << "EMITTING WeeklyTopBias::biasChanged()";
    emit biasChanged();
    
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


void Dynamic::WeeklyTopBias::fetchWeeklyData(uint from, uint to)
{
    DEBUG_BLOCK
    debug() << "getting top artist info from" << QDateTime::fromTime_t( from ) << "to" << QDateTime::fromTime_t( to );
    // find first weekly chart that contains dateTime
    int earliest = 0, last = 0;
    debug() << "number of weekly charts:" << m_weeklyCharts.size();
    for( int i = 0; i < m_weeklyCharts.size(); i++ )
    {
        if( earliest == 0 && m_weeklyCharts[ i ] > from )
        {
            earliest = m_weeklyCharts[ i - 1 ];
//             debug() << "chose early boundary:" << i - 1;
        }
        if( last == 0 && m_weeklyCharts[ i ] > to )
        {
//             debug() << "chose late boundary:" << i - 1;
            last = m_weeklyCharts[ i - 1];
        }
    }
    if( last == 0 )
        last = m_weeklyCharts[ m_weeklyCharts.size() - 1 ];

    m_currentArtistList.clear();
    debug() << "fetching charts with these ranges:" << QDateTime::fromTime_t( earliest ) << QDateTime::fromTime_t( last );


    for( int i = m_weeklyCharts.indexOf( earliest ); i < m_weeklyCharts.indexOf( last ); i++ )
    {
        if( m_weeklyChartData.keys().contains( m_weeklyCharts[ i ] ) )
        {
            m_currentArtistList.append( m_weeklyChartData[ m_weeklyCharts[ i ] ] );
            
        } else
        {
            QMap< QString, QString > params;
            params[ "method" ] = "user.getWeeklyArtistChart";
            params[ "user" ] = lastfm::ws::Username;
            params[ "from" ] = QString::number( m_weeklyCharts[ i ] );
            params[ "to" ] = QString::number( m_weeklyChartsTo[ i ] );

            m_fetchQueue.enqueue( params );
        }

    }

    m_fetching = new QSignalMapper;
    connect( m_fetching, SIGNAL( mapped(QObject*) ), this, SLOT( weeklyFetch( QObject* ) ) );
    connect( this, SIGNAL( doneFetching() ), this, SLOT( updateDB() ) );
    connect( this, SIGNAL( doneFetching() ), this, SLOT( saveDataToFile() ) );
    
    fetchNextWeeks();
}

void Dynamic::WeeklyTopBias::fetchNextWeeks( int num )
{

    // fetch 5 at a time, so as to conform to lastfm api requirements
    for(int i = 0; i < num; i++)
    {
        if( m_fetchQueue.isEmpty() )
        {
            break;
        }
//         debug() << "queueing up weekly fetch job!";
        QNetworkReply* res = lastfm::ws::get( m_fetchQueue.dequeue() );
        connect( res, SIGNAL( finished() ), m_fetching, SLOT( map() ) );
        m_fetching->setMapping( res, res );
    }
    
}

void Dynamic::WeeklyTopBias::weeklyFetch( QObject* reply )
{
    DEBUG_BLOCK
    if( !reply || !dynamic_cast<QNetworkReply*>( reply ) ) {
        debug() << "failed to get qnetwork reply in finished slot:" << reply;
        return;
    }
    QNetworkReply* r = static_cast< QNetworkReply* >( reply );
    
    try
    {
        lastfm::XmlQuery lfm( r->readAll() );

//         debug() << "got response:" << lfm;
        // take just to 5 artists in that week
        QStringList artists;
        for( int i = 0; i < lfm[ "weeklyartistchart" ].children( "artist" ).size(); i++ )
        {
            if( i == 5 )
                break;
            lastfm::XmlQuery artist = lfm[ "weeklyartistchart" ].children( "artist" ).at( i );
            artists.append( artist[ "name" ].text() );
        }
        debug() << "got artists:" << artists << QDomElement( lfm[ "weeklyartistchart" ] ).attribute( "from" );
        m_weeklyChartData.insert( QDomElement( lfm[ "weeklyartistchart" ] ).attribute( "from" ).toUInt(), artists );
        m_currentArtistList.append( artists );
    } catch( lastfm::ws::ParseError& e )
    {
        debug() << "caught exception parsing weekly artist chart.";
    }

    reply->deleteLater();
    if( m_fetchQueue.isEmpty() )
    {
        emit doneFetching();
    } else
    {
        fetchNextWeeks( 1 );
    }
    
}


void Dynamic::WeeklyTopBias::updateDB()
{
    m_collection = CollectionManager::instance()->primaryCollection();
    if( !m_collection )
        return;
    m_qm = m_collection->queryMaker();

    if( !m_qm ) // maybe this is during startup and we don't have a QM for some reason yet
        return;

    debug() << "setting up query";

    m_qm->beginOr();
    foreach( QString artist, m_currentArtistList )
    {
        m_qm->beginOr();
        debug() << "adding artist to query:" << artist;
        m_qm->addFilter( Meta::valArtist, artist, true, true );
        m_qm->endAndOr();
    }
    m_qm->endAndOr();


    m_qm->setQueryType( QueryMaker::Custom );
    m_qm->addReturnValue( Meta::valUniqueId );
    m_qm->orderByRandom(); // as to not affect the amortized time

    connect( m_qm, SIGNAL( newResultReady( QString, QStringList ) ),
             SLOT( updateReady( QString, QStringList ) ), Qt::DirectConnection );

    m_qm->run();
}


void Dynamic::WeeklyTopBias::saveDataToFile()
{
    QFile file( Amarok::saveLocation() + "dynamic_lastfm_topweeklyartists.xml" );
    file.open( QIODevice::Truncate | QIODevice::WriteOnly | QIODevice::Text );
    QTextStream out( &file );
    foreach( uint key, m_weeklyChartData.keys() )
    {
        out << key << "#" << m_weeklyChartData[ key ].join( "^" ) << endl;
    }
    file.close();

}


// Class WeeklyTopBiasCollectionFilterCapability

const QSet< QByteArray >&
Dynamic::WeeklyTopBiasCollectionFilterCapability::propertySet()
{
    return m_bias->m_trackList;
}

double
Dynamic::WeeklyTopBiasCollectionFilterCapability::weight() const
{
    return m_bias->weight();
}


#include "WeeklyTopBias.moc"
