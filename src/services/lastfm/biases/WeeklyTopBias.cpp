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
    // TODO implement
    return new Dynamic::WeeklyTopBias( weight );
}

// CLASS WeeklyTopBias

Dynamic::WeeklyTopBias::WeeklyTopBias( double weight )
    : Dynamic::CustomBiasEntry( weight )
    , m_qm( 0 )
    , m_layout( 0 )
    , m_fromEdit( 0 )
    , m_toEdit( 0 )
    , m_rangeJob( 0 )
    , m_dataJob( 0 )
{
    getPossibleRange();
}

Dynamic::WeeklyTopBias::~WeeklyTopBias()
{
    //delete m_qm;
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

    QLabel * label = new QLabel( i18n( "Play Top Tracks From" ), frame );
    label->setWordWrap( true );
    label->setAlignment( Qt::AlignCenter );
    m_layout->addWidget( label, Qt::AlignCenter );

    m_fromEdit = new QDateTimeEdit( QDate::currentDate().addDays( -7 ) );
    m_fromEdit->setMinimumDate( QDate::currentDate().addYears( -10 ) ); // 10 years ago is minimum for now
    m_fromEdit->setMaximumDate( QDate::currentDate() );
    m_fromEdit->setCalendarPopup( true );
    m_layout->addWidget( m_fromEdit );

    connect( m_fromEdit, SIGNAL( dateTimeChanged( const QDateTime& ) ), this, SLOT( fromDateChanged( const QDateTime& ) ) );
    
    QLabel * to = new QLabel( i18nc( "From one date to another, this text is in between", "to" ), parent );
    
    to->setAlignment( Qt::AlignCenter );
    m_layout->addWidget( to, Qt::AlignCenter );

    m_toEdit =new QDateTimeEdit( QDate::currentDate().addDays( -7 ) );
    m_toEdit->setMinimumDate( QDate::currentDate().addYears( -10 ) ); // 10 years ago is minimum for now
    m_toEdit->setMaximumDate( QDate::currentDate() );
    m_toEdit->setCalendarPopup( true );
    m_layout->addWidget( m_toEdit );

    connect( m_toEdit, SIGNAL( dateTimeChanged( const QDateTime& ) ), this, SLOT( toDateChanged( const QDateTime& ) ) );
    
    return frame;
}

bool
Dynamic::WeeklyTopBias::trackSatisfies( const Meta::TrackPtr track )
{
    return false;
}

double
Dynamic::WeeklyTopBias::numTracksThatSatisfy( const Meta::TrackList& tracks )
{
    return 0;
}

QDomElement
Dynamic::WeeklyTopBias::xml( QDomDocument doc ) const
{  
    return QDomElement();
}

bool
Dynamic::WeeklyTopBias::hasCollectionFilterCapability()
{
    return false;
}

Dynamic::CollectionFilterCapability*
Dynamic::WeeklyTopBias::collectionFilterCapability()
{
    return 0;
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

    debug() << "got earliest date:"  << QDateTime::fromTime_t( m_earliestDate ).toString();
    m_fromEdit->setMinimumDate( QDateTime::fromTime_t( m_earliestDate ).date() );

    m_rangeJob->deleteLater();
}

void
Dynamic::WeeklyTopBias::dataJobFinished()
{
    DEBUG_BLOCK

    if( !m_dataJob )
        return;

    try
    {
        lastfm::XmlQuery lfm( m_dataJob->readAll() );

        debug() << "got response:" << lfm;
        // now do the query
        m_collection = CollectionManager::instance()->primaryCollection();
        if( !m_collection )
            return;
        m_qm = m_collection->queryMaker();

        if( !m_qm ) // maybe this is during startup and we don't have a QM for some reason yet
            return;

        debug() << "setting up query";
        m_qm->beginOr();

        int count = 0;
        foreach( lastfm::XmlQuery e, lfm.children( "track" ) )
        {
            m_qm->beginAnd();
            m_qm->addFilter( Meta::valTitle, e[ "name" ].text(), true, true );
            m_qm->addFilter( Meta::valArtist, e[ "artist" ].text(), true, true );
            m_qm->endAndOr();
            if( count > 3 )
                break; // just top 20
            count++;
            
        }
        m_qm->endAndOr();

        debug() << "done with query";

        m_qm->setQueryType( QueryMaker::Custom );
        m_qm->addReturnValue( Meta::valUniqueId );
        m_qm->orderByRandom(); // as to not affect the amortized time
        
        connect( m_qm, SIGNAL( newResultReady( QString, QStringList ) ),
                 SLOT( updateReady( QString, QStringList ) ), Qt::DirectConnection );
                 
        m_qm->run();
        
    } catch( lastfm::ws::ParseError& e )
    {
        debug() << "failed in parsing top tracks!";
    }

    m_dataJob->deleteLater();
}

void
Dynamic::WeeklyTopBias::updateReady( QString collectionId, QStringList uids )
{
    DEBUG_BLOCK
    Q_UNUSED( collectionId );

    int protocolLength = ( QString( m_collection->uidUrlProtocol() ) + "://" ).length();

    debug() << "setting cache of top UIDs for selected date: to:" << uids;
    m_trackList.clear();
    m_trackList.reserve( uids.size() );
    
    QByteArray uid;
    foreach( const QString &uidString, uids )
    {
        uid = QByteArray::fromHex( uidString.mid(protocolLength).toAscii() );
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

    QMap< QString, QString > params;
    params[ "method" ] = "user.getWeeklyTrackChart";
    params[ "user" ] = lastfm::ws::Username;
    params[ "from" ] = QString::number( m_toDate );
    params[ "to" ] = QString::number( m_toDate );

    m_dataJob = lastfm::ws::get( params );

    connect( m_dataJob, SIGNAL( finished() ), this, SLOT( dataJobFinished() ) );
    
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
