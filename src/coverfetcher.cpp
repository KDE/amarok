// (c) 2004 Mark Kretschmann <markey@web.de>
// See COPYING file for licensing information.

#include "coverfetcher.h"
#include "amazonsearch.h"
#include "collectiondb.h"

#include <qdom.h>
#include <qvbox.h>

#include <kconfig.h>
#include <kdebug.h>
#include <kio/job.h>
#include <kio/jobclasses.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kpushbutton.h>   
#include <kinputdialog.h>  
#include <klineedit.h> 


CoverFetcher::CoverFetcher( const QString& license, QObject* parent)
    : QObject( parent, "CoverFetcher" )
    , m_license( license )
    , m_buffer( 0 )
    , m_bufferIndex( 0 )
{
    kdDebug() << k_funcinfo << endl;
}


CoverFetcher::~CoverFetcher()
{
    delete[] m_buffer;
}


//////////////////////////////////////////////////////////////////////////////////////////
// PUBLIC
//////////////////////////////////////////////////////////////////////////////////////////

void
CoverFetcher::getCover( const QString& keyword, const QString& album, QueryMode mode )
{
    /* reset all values (if search isn't started as new CoverFetcher) */
    delete m_buffer;
    m_bufferIndex = 0;
    m_xmlDocument = "";
    
    kdDebug() << k_funcinfo << endl;
    m_keyword = keyword;
    m_album = album;
        
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
    m_buffer = new uchar[BUFFER_SIZE];
    
    KIO::TransferJob* imageJob = KIO::get( imageUrl, false, false );
    connect( imageJob, SIGNAL( result( KIO::Job* ) ),
             this,       SLOT( imageResult( KIO::Job* ) ) ); 
    connect( imageJob, SIGNAL( data( KIO::Job*, const QByteArray& ) ),
             this,       SLOT( imageData( KIO::Job*, const QByteArray& ) ) ); 
}


void 
CoverFetcher::imageData( KIO::Job*, const QByteArray& data ) //SLOT
{
    if ( m_bufferIndex + (uint) data.size() >= BUFFER_SIZE ) {
        KMessageBox::error( 0, i18n( "CoverFetcher buffer overflow. Image is bigger than <i>%1</i> bytes. Aborting." )
                               .arg( BUFFER_SIZE ) );
        deleteLater();
        return;
    }
        
    //append new chunk of data to buffer
    memcpy( m_buffer + m_bufferIndex, data.data(), data.size() );
    m_bufferIndex += data.size();
}


void 
CoverFetcher::imageResult( KIO::Job* job ) //SLOT
{
    kdDebug() << k_funcinfo << endl;

    /* if no cover is found, open the amazon search dialogue */
    if ( job->error() != 0 ) {
        
        AmazonSearch* sdlg = new AmazonSearch();
        sdlg->searchString->setText( m_keyword );
        sdlg->setModal( true );

        /* if the "OK" button is pressed, search again using the search string provided in AmazonSearch's lineedit */
        if ( sdlg->exec() == QDialog::Accepted ) 
        {    
            m_keyword = sdlg->searchString->text();
            getCover( m_keyword, m_album, CoverFetcher::heavy );
            return;
        }
        else
        {
            deleteLater();
            return;
        }
    }
    m_pixmap.loadFromData( m_buffer, m_bufferIndex );
    
    QVBox* container = new QVBox( 0, 0, WDestructiveClose );
    container->setCaption( m_keyword + " - amaroK" );
    connect( this, SIGNAL( destroyed() ), container, SLOT( deleteLater() ) );
    
    QWidget* widget = new QWidget( container );
    widget->setPaletteBackgroundPixmap( m_pixmap );
    widget->setFixedSize( m_pixmap.size() );
    
    QHBox* buttons = new QHBox( container );
    KPushButton* save = new KPushButton( i18n( "Save" ), buttons );
    KPushButton* cancel = new KPushButton( i18n( "Cancel" ), buttons );
    connect( cancel, SIGNAL( clicked() ), this, SLOT( deleteLater() ) );
    connect( save, SIGNAL( clicked() ), this, SLOT( saveCover() ) );
            
    container->adjustSize();
    container->setFixedSize( container->size() );
    container->show();
}


void 
CoverFetcher::saveCover() //SLOT
{
    kdDebug() << k_funcinfo << endl;
    
    emit imageReady( m_album, m_pixmap );
    deleteLater();
}


#include "coverfetcher.moc"
