// (c) 2004 Mark Kretschmann <markey@web.de>
// See COPYING file for licensing information.


#include "coverfetcher.h"

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
CoverFetcher::getCover( const QString& keyword )
{
    kdDebug() << k_funcinfo << endl;
    
    QString url = QString( "http://xml.amazon.com/onca/xml3?t=webservices-20&dev-t=%1"
                           "&KeywordSearch=%2&mode=music&type=lite&page=1&f=xml" )
                           .arg( m_license )
                           .arg( keyword );
    
    KIO::TransferJob* job = KIO::get( url );

    connect( job, SIGNAL( result( KIO::Job* ) ),
             this,  SLOT( slotResult( KIO::Job* ) ) ); 
    connect( job, SIGNAL( data( KIO::Job*, const QByteArray& ) ),
             this,  SLOT( slotData( KIO::Job*, const QByteArray& ) ) ); 
}
       

//////////////////////////////////////////////////////////////////////////////////////////
// PRIVATE SLOTS
//////////////////////////////////////////////////////////////////////////////////////////

void 
CoverFetcher::slotData( KIO::Job*, const QByteArray& data ) //SLOT
{
    m_resultStr += QString( data );
}


void 
CoverFetcher::slotResult( KIO::Job* job ) //SLOT
{
    kdDebug() << k_funcinfo << endl;

    if ( !job->error() == 0 ) {
        kdWarning() << "KIO error! errno: " << job->error() << endl;
        return;
    }
    
    kdDebug() << m_resultStr << endl;
}


#include "coverfetcher.moc"
