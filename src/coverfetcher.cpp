// (c) 2004 Mark Kretschmann <markey@web.de>
// (c) 2004 Stefan Bogner <bochi@online.ms>
// See COPYING file for licensing information.

#include "amarokconfig.h"
#include "coverfetcher.h"
#include "amazonsearch.h"
#include "collectiondb.h"

#include <qdom.h>
#include <qlabel.h>
#include <qcombobox.h>
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
CoverFetcher::getCover( const QString& keyword, const QString& album, QueryMode mode, bool noedit )
{
    /* reset all values (if search isn't started as new CoverFetcher) */
    delete m_buffer;
    m_bufferIndex = 0;
    m_xmlDocument = "";
    
    kdDebug() << k_funcinfo << endl;
    m_keyword = keyword;
    m_album = album;
    m_noedit = noedit;
        
    QString url = QString( "http://xml.amazon.com/onca/xml3?t=webservices-20&dev-t=%1"
                           "&KeywordSearch=%2&mode=music&type=%3&page=1&f=xml" )
                           .arg( m_license )
                           .arg( keyword )
                           .arg( mode == lite ? "lite" : "heavy" );
                           
    kdDebug() << "Using this url: " << url << endl;
                           
    KIO::TransferJob* job = KIO::get( url, false, false );
    connect( job, SIGNAL( result( KIO::Job* ) ),
             this,  SLOT( xmlResult( KIO::Job*  ) ) ); 
    connect( job, SIGNAL( data( KIO::Job*, const QByteArray& ) ),
             this,  SLOT( xmlData( KIO::Job*, const QByteArray& ) ) ); 
}
       

//////////////////////////////////////////////////////////////////////////////////////////
// PRIVATE SLOTS
//////////////////////////////////////////////////////////////////////////////////////////

void 
CoverFetcher::xmlData( KIO::Job*, const QByteArray& data ) //SLOT
{
    // Append new chunk of string
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
    
    if ( AmarokConfig::coverSize() == 0 )
    {
        m_url = doc.documentElement()
                   .namedItem( "Details" )
                   .namedItem( "ImageUrlSmall" )
                   .firstChild().toText().nodeValue();
    }
    else if ( AmarokConfig::coverSize() == 1 )
    {
        m_url = doc.documentElement()
                   .namedItem( "Details" )
                   .namedItem( "ImageUrlMedium" )
                   .firstChild().toText().nodeValue();
    }
    else
    {        
        m_url = doc.documentElement()
                   .namedItem( "Details" )
                   .namedItem( "ImageUrlLarge" )
                   .firstChild().toText().nodeValue();
    }
   
    kdDebug() << "imageUrl: " << m_url << endl;
    m_buffer = new uchar[BUFFER_SIZE];
    
    KIO::TransferJob* imageJob = KIO::get( m_url, false, false );
    connect( imageJob, SIGNAL( result( KIO::Job* ) ),
             this,       SLOT( imageResult( KIO::Job* ) ) ); 
    connect( imageJob, SIGNAL( data( KIO::Job*, const QByteArray& ) ),
             this,       SLOT( imageData( KIO::Job*, const QByteArray& ) ) ); 
}


void 
CoverFetcher::imageData( KIO::Job*, const QByteArray& data ) //SLOT
{
    if ( m_bufferIndex + (uint) data.size() >= BUFFER_SIZE ) {
        KMessageBox::error( 0, i18n( "CoverFetcher buffer overflow. Image is bigger than <i>1</i> byte. Aborting.",
                                     "CoverFetcher buffer overflow. Image is bigger than <i>%n</i> bytes. Aborting.",
                                     BUFFER_SIZE ) );
        deleteLater();
        return;
    }
        
    // Append new chunk of data to buffer
    memcpy( m_buffer + m_bufferIndex, data.data(), data.size() );
    m_bufferIndex += data.size();
}


void 
CoverFetcher::imageResult( KIO::Job* job ) //SLOT
{
    kdDebug() << k_funcinfo << endl;

/* This is for the cover manager. When fetching all remaining covers, only save the cover */
    if ( !m_noedit )
    {
        /* if no cover is found, open the amazon search dialogue, else show the cover viewer. */
        if ( job->error() != 0 ) 
        {

            m_text = "<h3>No cover image found!</h3>"
            "If you would like to search again, you can edit the search string below and press <b>OK</b>.";
            editSearch();
        }
        else
        {
            m_text = "<h3>New Search</h3>Please edit the search string below and press <b>OK</b>.";
            m_pixmap.loadFromData( m_buffer, m_bufferIndex );
    
            QVBox* container = new QVBox( 0, 0, WDestructiveClose );
            /* we show m_album here, since it's always the filename on save */
            container->setCaption( m_album + " - amaroK" );
            connect( this, SIGNAL( destroyed() ), container, SLOT( deleteLater() ) );
    
            QWidget* widget = new QWidget( container );
            widget->setPaletteBackgroundPixmap( m_pixmap );
            widget->setFixedSize( m_pixmap.size() );
    
            QHBox* buttons = new QHBox( container );
            KPushButton* save = new KPushButton( i18n( "Save" ), buttons );
            KPushButton* newsearch = new KPushButton( i18n( "New search" ), buttons );
            KPushButton* cancel = new KPushButton( i18n( "Cancel" ), buttons );
            connect( cancel, SIGNAL( clicked() ), this, SLOT( deleteLater() ) );
            connect( newsearch, SIGNAL( clicked() ), this, SLOT( editSearch() ) );
            connect( save, SIGNAL( clicked() ), this, SLOT( saveCover() ) );
            
            container->adjustSize();
            container->setFixedSize( container->size() );
            container->show();
        }
    }
    else
    {
        m_pixmap.loadFromData( m_buffer, m_bufferIndex );
        saveCover();
    }
}

void 
CoverFetcher::editSearch() //SLOT
{
    AmazonSearch* sdlg = new AmazonSearch();
    sdlg->textLabel->setText( m_text );
    sdlg->sizeCombo->setCurrentItem( AmarokConfig::coverSize() );
    sdlg->searchString->setText( m_keyword );
    sdlg->setModal( true );
            
    if ( sdlg->exec() == QDialog::Accepted ) 
    {    
        m_keyword = sdlg->searchString->text();
        AmarokConfig::setCoverSize( sdlg->sizeCombo->currentItem() );
        getCover( m_keyword, m_album, CoverFetcher::heavy );
        return;
    }
    else
    {
        deleteLater();
        return;
    }
}

void 
CoverFetcher::saveCover() //SLOT
{
    kdDebug() << k_funcinfo << endl;
    
    emit imageReady( m_album, m_pixmap );
    deleteLater();
}


#include "coverfetcher.moc"
