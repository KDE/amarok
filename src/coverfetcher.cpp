// (c) 2004 Mark Kretschmann <markey@web.de>
// See COPYING file for licensing information.

#include "coverfetcher.h"

#include <qdom.h>
#include <qpixmap.h>
#include <qvbox.h>

#include <kdebug.h>
#include <kio/job.h>
#include <kio/jobclasses.h>
#include <klocale.h>
#include <kpushbutton.h>   


CoverFetcher::CoverFetcher( const QString& license, QObject* parent)
    : QObject( parent, "CoverFetcher" )
    , m_license( license )
{
    kdDebug() << k_funcinfo << endl;
}


//////////////////////////////////////////////////////////////////////////////////////////
// PUBLIC
//////////////////////////////////////////////////////////////////////////////////////////

void
CoverFetcher::getCover( const QString& keyword, QueryMode mode )
{
    kdDebug() << k_funcinfo << endl;
    
    // Initialisations
    m_xmlDocument = QString();
    m_image = QByteArray();
    m_keyword = keyword;
        
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
        deleteLater();
        return;
    }
    kdDebug() << m_xmlDocument << endl;

    QDomDocument doc;
    doc.setContent( m_xmlDocument );
    
    QString imageUrl = doc.documentElement()
                          .namedItem( "Details" )
                          .namedItem( "ImageUrlLarge" )
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
        deleteLater();
        return;
    }
    QVBox* container = new QVBox( 0, 0, WDestructiveClose );
    container->setCaption( m_keyword + " - amaroK" );
    
    QWidget* widget = new QWidget( container );
    widget->setPaletteBackgroundPixmap( QPixmap( m_image ) );
    widget->setFixedSize( QPixmap( m_image ).size() );
    
    QHBox* buttons = new QHBox( container );
    KPushButton* save = new KPushButton( i18n( "Save" ), buttons );
    KPushButton* cancel = new KPushButton( i18n( "Cancel" ), buttons );
    connect( cancel, SIGNAL( clicked() ), this, SLOT( deleteLater() ) );
    connect( save, SIGNAL( clicked() ), this, SLOT( deleteLater() ) );
    connect( save, SIGNAL( clicked() ), this, SLOT( saveCover() ) );
    connect( this, SIGNAL( destroyed() ), container, SLOT( deleteLater() ) );
            
    container->adjustSize();
    container->setFixedSize( container->size() );
    container->show();
}


void 
CoverFetcher::saveCover() //SLOT
{
    kdDebug() << k_funcinfo << endl;
    
    emit imageReady( m_keyword, QPixmap( m_image ) );
}


#include "coverfetcher.moc"
