// (c) 2004 Mark Kretschmann <markey@web.de>
// (c) 2004 Stefan Bogner <bochi@online.ms>
// (c) 2004 Max Howell
// See COPYING file for licensing information.

#include "amarokconfig.h"
#include "coverfetcher.h"

#include <qdom.h>
#include <qhbox.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qregexp.h>

#include <kapplication.h>
#include <kcursor.h> //waiting cursor
#include <kdebug.h>
#include <kdialog.h>
#include <kio/job.h>
#include <kio/jobclasses.h>
#include <klineedit.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kpushbutton.h>



CoverFetcher::CoverFetcher( QWidget *parent, const QString& license )
    : QObject( parent, "CoverFetcher" )
    , m_license( license )
    , m_buffer( 0 )
{
    QApplication::setOverrideCursor( KCursor::workingCursor() );
}

CoverFetcher::~CoverFetcher()
{
    QApplication::restoreOverrideCursor();
    delete[] m_buffer;
}

//////////////////////////////////////////////////////////////////////////////////////////
// PUBLIC
//////////////////////////////////////////////////////////////////////////////////////////

void
CoverFetcher::getCover( const QString& artist, const QString& album, const QString& saveas, QueryMode mode, bool noedit, int size, bool albumonly )
{
    kdDebug() << k_funcinfo << endl << artist << ", " << album << ": " << saveas << endl;

    m_artist = artist;
    m_album = album;

    //remove all matches to the album filter.
    const QString replaceMe = " \\([^}]*%1[^}]*\\)";

    QStringList albumExtension;
    albumExtension << i18n( "disc" ) << i18n( "disk" ) << i18n( "remaster" ) << i18n( "cd" ) << i18n( "single" )
                   << "disc" << "disk" << "remaster" << "cd" << "single" << "cds" /*cd single*/;

    //### we could use something like this globally, but how to make it clear "it" is for use?
    #define foreach( list ) \
        for( QStringList::ConstIterator it = list.begin(), end = list.end(); it != end; ++it )

    foreach( albumExtension ) {
        QRegExp re = replaceMe.arg( *it );
        re.setCaseSensitive( false );
        m_album.remove( re );
    }

    m_keyword = (artist == m_album) ? m_album : artist + " - " + m_album;

    m_saveas = saveas;
    m_noedit = noedit;
    m_size   = size;
    m_albumonly = albumonly;

    /* reset all values (if search isn't started as new CoverFetcher) */
    delete m_buffer;
    m_bufferIndex = 0;
    m_xmlDocument = "";

    QString url = "http://xml.amazon.%1/onca/xml3?t=webservices-20&dev-t=%2&KeywordSearch=%3&mode=music&type=%4&page=1&f=xml";
    url = url.arg( AmarokConfig::amazonLocale() ).arg( m_license ).arg( m_keyword ).arg( mode == lite ? "lite" : "heavy" );

    kdDebug() << "Using this url: " << url << endl;

    KIO::TransferJob* job = KIO::get( url, false, false );
    connect( job, SIGNAL(result( KIO::Job* )), SLOT(xmlResult( KIO::Job* )) );
    connect( job, SIGNAL(data( KIO::Job*, const QByteArray& )), SLOT(xmlData( KIO::Job*, const QByteArray& )) );
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
    if ( !job->error() == 0 ) {
        kdWarning() << "[CoverFetcher] Could not fetch XML data, KIO errno: " << job->error() << endl;
        emit error();
        deleteLater();
        return;
    }

    QDomDocument doc;
    doc.setContent( m_xmlDocument );

    // Fetch url for product info page
    m_amazonUrl = doc.documentElement()
        .namedItem( "Details" )
        .attributes()
        .namedItem( "url" ).toAttr().value();

    const QString size = (m_size == 0)
        ? "ImageUrlSmall" : (m_size == 1)
        ? "ImageUrlMedium"
        : "ImageUrlLarge";

    m_imageUrl = doc.documentElement()
        .namedItem( "Details" )
        .namedItem( size )
        .firstChild().toText().nodeValue();

    m_buffer = new uchar[BUFFER_SIZE];

    KIO::TransferJob* imageJob = KIO::get( m_imageUrl, false, false );
    connect( imageJob, SIGNAL( result( KIO::Job* ) ),
             this,       SLOT( imageResult( KIO::Job* ) ) );
    connect( imageJob, SIGNAL( data( KIO::Job*, const QByteArray& ) ),
             this,       SLOT( imageData( KIO::Job*, const QByteArray& ) ) );
}


void
CoverFetcher::imageData( KIO::Job*, const QByteArray& data ) //SLOT
{
    if ( m_bufferIndex + (uint) data.size() >= BUFFER_SIZE ) {
        emit error();
        KMessageBox::error( 0, i18n( "CoverFetcher buffer overflow. Image is larger than <i>%1B</i>. Aborting." ).arg( BUFFER_SIZE ) );
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

    /* Okay, this next thing may appear a bit weird ;) Short explanation:
       If there's no result for the search string (artist - album), search for the album tag only.
       If there's no (large) cover is found, try the next smaller image (medium) etc. If no covers, open editSearch.
       If we fetch all covers (covermanager), do not show any errors/previews, just save (m_noedit).  */

    if ( job->error() )
    {
        kdDebug() << "[CoverFetcher] Could not fetch image data, KIO errno: " << job->error() << endl;


        if ( !m_albumonly )
            getCover( m_album, m_album, m_saveas, CoverFetcher::heavy, m_noedit, 1, true );

        else if ( !m_noedit )
            editSearch( i18n("The cover was not found in the Amazon database. You may have better luck if you refine the search:") );

        else {
            kdDebug() << "[CoverFetcher] Image not found in amazon.com database.\n";
            emit error();
            deleteLater();
            return;
        }
    }
    else
    {
        m_image.loadFromData( m_buffer, m_bufferIndex );

        if ( m_image.width() == 1 )
        {
            if ( m_size ) {
                if ( m_albumonly ) getCover( m_album, m_album, m_saveas, CoverFetcher::heavy, m_noedit, m_size-1, m_albumonly );
                else getCover( m_artist, m_album, m_saveas, CoverFetcher::heavy, m_noedit, m_size-1, m_albumonly );
            }
            else if ( !m_noedit )
                editSearch( i18n("A suitable cover was found, <b>but with no associated images</b>. You may have better luck if you refine the search:") );

            else
            {
                kdDebug() << "[CoverFetcher] Image is invalid." << endl;
                emit error();
                deleteLater();
                return;
            }
        }
        else if ( !m_noedit )
        {
            KDialog dialog( (QWidget*)parent() );

            (new QVBoxLayout( &dialog ))->setAutoAdd( true );

            QLabel* label = new QLabel( &dialog );
            label->setPixmap( m_image );

            QHBox* buttons         = new QHBox( &dialog );
            KPushButton* save      = new KPushButton( KStdGuiItem::save(), buttons );
            KPushButton* newsearch = new KPushButton( i18n( "Search &Again" ), buttons );
            KPushButton* cancel    = new KPushButton( KStdGuiItem::cancel(), buttons );

            save->setDefault( true );

            connect( cancel, SIGNAL(clicked()), SLOT(deleteLater()) );
            connect( newsearch, SIGNAL(clicked()), SLOT(editSearch()) );
            connect( save, SIGNAL(clicked()), SLOT(saveCover()) );

            connect( cancel, SIGNAL(clicked()), &dialog, SLOT(reject()) );
            connect( newsearch, SIGNAL(clicked()), &dialog, SLOT(accept()) );
            connect( save, SIGNAL(clicked()), &dialog, SLOT(accept()) );

            dialog.setFixedSize( dialog.sizeHint() );
            dialog.setCaption( m_album );
            dialog.exec();
        }
        else
        {
            if ( !m_image.loadFromData( m_buffer, m_bufferIndex ) ) {
                kdDebug() << "[CoverFetcher] Image is invalid." << endl;
                emit error();
                deleteLater();
                return;
            }
            saveCover();
        }
    }
}

void
CoverFetcher::editSearch( QString text ) //SLOT
{
    class EditSearchDialog : public KDialog
    {
    public:
        EditSearchDialog( QWidget* parent, const QString &text, const QString &keyword )
            : KDialog( parent )
        {
            setCaption( i18n( "No Cover Found" ) );

            QVBoxLayout *vbox = new QVBoxLayout( this, 8, 8 );
            QHBoxLayout *hbox = new QHBoxLayout( 8 );

            KPushButton* cancelButton = new KPushButton( KStdGuiItem::cancel(), this );
            KPushButton* okButton     = new KPushButton( i18n("Search"), this );

            hbox->addItem( new QSpacerItem( 80, 8, QSizePolicy::Expanding, QSizePolicy::Minimum ) );
            hbox->addWidget( okButton );
            hbox->addWidget( cancelButton );

            vbox->addWidget( new QLabel( "<qt>" + text, this ) );
            vbox->addWidget( new KLineEdit( keyword, this, "Keyword" ) );
            vbox->addLayout( hbox );

            okButton->setDefault( true );

            adjustSize();
            setFixedHeight( height() );

            connect( okButton, SIGNAL(clicked()), SLOT(accept()) );
            connect( cancelButton, SIGNAL(clicked()), SLOT(reject()) );
        }

        QString keyword() { return static_cast<KLineEdit*>(child( "Keyword" ))->text(); }
    };

    if ( text.isEmpty() )
        text = i18n("Search Amazon's cover database with this query:");

    EditSearchDialog dialog( (QWidget*)parent(), text, m_saveas );

    switch( dialog.exec() ) {
    case QDialog::Accepted:
        getCover( dialog.keyword(), dialog.keyword(), m_saveas, CoverFetcher::heavy );
        break;
    default:
        deleteLater();
        break;
    }
}

void
CoverFetcher::saveCover() //SLOT
{
    saveCover( m_image );
}

void
CoverFetcher::saveCover( const QImage& image ) //SLOT
{
    emit imageReady( m_saveas, m_amazonUrl, image );
    deleteLater();
}


#include "coverfetcher.moc"
