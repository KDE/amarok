// (c) 2004 Mark Kretschmann <markey@web.de>
// See COPYING file for licensing information.

#include "coverfetcher.h"

#include <qdom.h>
#include <qpixmap.h>

#include <kdebug.h>
#include <kio/job.h>
#include <kio/jobclasses.h>


CoverFetcher::CoverFetcher( QObject* parent )
    : QObject( parent, "CoverFetcher" )
{
    kdDebug() << k_funcinfo << endl;
}


CoverFetcher::~CoverFetcher()
{}


//////////////////////////////////////////////////////////////////////////////////////////
// PUBLIC
//////////////////////////////////////////////////////////////////////////////////////////

void
CoverFetcher::getCover( const QString& keyword, QueryMode mode )
{
    kdDebug() << k_funcinfo << endl;
    
    //reset stuff
    m_xmlDocument = QString();
    m_image = QByteArray();
    
    QString url = QString( "http://xml.amazon.com/onca/xml3?t=webservices-20&dev-t=%1"
                           "&KeywordSearch=%2&mode=music&type=%3&page=1&f=xml" )
                           .arg( m_license )
                           .arg( keyword )
                           .arg( mode == lite ? "lite" : "heavy" );
                           
    kdDebug() << "Using this url: " << url << endl;
                           
    KIO::TransferJob* job = KIO::get( url, false, false );
    connect( job, SIGNAL( result( KIO::Job* ) ),
             this,  SLOT( xmlResult( KIO::Job* ) ) ); 
    connect( job, SIGNAL( data( KIO::Job*, const QByteArray& ) ),
             this,  SLOT( xmlData( KIO::Job*, const QByteArray& ) ) ); 
}
       

//////////////////////////////////////////////////////////////////////////////////////////
// PRIVATE SLOTS
//////////////////////////////////////////////////////////////////////////////////////////

void 
CoverFetcher::xmlData( KIO::Job*, const QByteArray& data ) //SLOT
{
    m_xmlDocument += QString( data );
}


void 
CoverFetcher::xmlResult( KIO::Job* job ) //SLOT
{
    kdDebug() << k_funcinfo << endl;

    if ( !job->error() == 0 ) {
        kdWarning() << "KIO error! errno: " << job->error() << endl;
        return;
    }
    kdDebug() << m_xmlDocument << endl;

    QDomDocument doc;
    doc.setContent( m_xmlDocument );
    
    QString imageUrl = doc.documentElement()
                          .namedItem( "Details" )
                          .namedItem( "ImageUrlMedium" )
                          .firstChild().toText().nodeValue();
    
    kdDebug() << "imageUrl: " << imageUrl << endl;

    KIO::TransferJob* imageJob = KIO::get( imageUrl, false, false );
    connect( imageJob, SIGNAL( result( KIO::Job* ) ),
             this,       SLOT( imageResult( KIO::Job* ) ) ); 
    connect( imageJob, SIGNAL( data( KIO::Job*, const QByteArray& ) ),
             this,       SLOT( imageData( KIO::Job*, const QByteArray& ) ) ); 
}


void 
CoverFetcher::imageData( KIO::Job*, const QByteArray& data ) //SLOT
{
    int oldSize = m_image.size();
    m_image.resize( m_image.size() + data.size() );
    
    //append new data to array
    for ( uint i = 0; i < data.size(); i++ )
        m_image[ oldSize + i ] = data[ i ];
}


void 
CoverFetcher::imageResult( KIO::Job* job ) //SLOT
{
    kdDebug() << k_funcinfo << endl;

    if ( !job->error() == 0 ) {
        kdWarning() << "KIO error! errno: " << job->error() << endl;
        return;
    }

    emit imageReady( QPixmap( m_image ) );
}


#include "coverfetcher.moc"
