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



CoverFetcher::CoverFetcher( QWidget *parent, QString artist, QString album )
    : QObject( parent, "CoverFetcher" )
    , m_artist( artist )
    , m_album( album )
    , m_buffer( 0 )
    , m_bufferIndex( 0 )
    , m_size( 2 )
{
    #define foreach( list ) \
        for( QStringList::ConstIterator it = list.begin(), end = list.end(); it != end; ++it )

    QStringList extensions;
    extensions << i18n("disc") << i18n("disk") << i18n("remaster") << i18n("cd") << i18n("single") << i18n("soundtrack") << i18n("part")
               << "disc" << "disk" << "remaster" << "cd" << "single" << "soundtrack" << "part" << "cds" /*cd single*/;

    //we do several queries, one raw ie, without the following modifications
    //the others have the above strings removed with the following regex, as this can increase hit-rate
    const QString template1 = " ?-? ?[(^{]* ?%1 ?\\d*[)^}\\]]* *$"; //eg album - [disk 1] -> album
    foreach( extensions ) {
        QRegExp regexp( template1.arg( *it ) );
        regexp.setCaseSensitive( false );
        album.remove( regexp );
    }

    //TODO try queries that remove anything in album after a " - " eg Les Mis. - Excerpts

   /**
    * We search for artist - album, and just album, using the exact album text and the
    * manipulated album text.
    */

    //search on our modified term, then the original
    m_queries += m_artist + " - " + album;
    m_queries += m_userQuery = m_artist + " - " + m_album;
    m_queries += album;
    m_queries += m_album;

    //don't do the same searches twice in a row
    if( m_album == album )  {
        m_queries.pop_front();
        m_queries.pop_back();
    }

    /**
     * Finally we do a search for just the artist, just in case as this often
     * turns up a cover, and it might just be the right one! Also it would be
     * the only valid search if m_album.isEmpty()
     */
    m_queries += m_artist;

    QApplication::setOverrideCursor( KCursor::workingCursor() );
}

CoverFetcher::~CoverFetcher()
{
    QApplication::restoreOverrideCursor();

    delete[] m_buffer;
}

void
CoverFetcher::startFetch()
{
    /* Static license Key. Thanks muesli ;-) */
    const QString LICENSE = "D1URM11J3F2CEH";

    // reset all values (if search isn't started as new CoverFetcher)
    delete[] m_buffer;
    m_bufferIndex = 0;
    m_fetchedXML = QString::null;

    QString query;
    if ( !m_queries.isEmpty() ) {
        query = m_queries.front();
        m_queries.pop_front();
    }
    else
        query = m_userQuery;

    QString url = "http://xml.amazon.%1/onca/xml3?t=webservices-20&dev-t=%2&KeywordSearch=%3&mode=music&type=%4&page=1&f=xml";
    url = url.arg( AmarokConfig::amazonLocale(), LICENSE, KURL::encode_string( query ), "heavy" );

    kdDebug() << "[CoverFetcher] " << url << endl;

    KIO::TransferJob* job = KIO::get( url, false, false );

    connect( job, SIGNAL(result( KIO::Job* )), SLOT(finishedXmlFetch( KIO::Job* )) );
    connect( job, SIGNAL(data( KIO::Job*, const QByteArray& )), SLOT(receivedXmlData( KIO::Job*, const QByteArray& )) );
}


//////////////////////////////////////////////////////////////////////////////////////////
// PRIVATE SLOTS
//////////////////////////////////////////////////////////////////////////////////////////

void
CoverFetcher::receivedXmlData( KIO::Job*, const QByteArray& data ) //SLOT
{
    // Append new chunk of string
    m_fetchedXML += QString( data );
}


void
CoverFetcher::finishedXmlFetch( KIO::Job *job ) //SLOT
{
    if ( job->error() ) {
        error( i18n("There was an error communicating with amazon."), job );
        return;
    }

    QDomDocument doc;
    doc.setContent( m_fetchedXML );

    // Fetch url for product info page
    m_amazonURL = doc.documentElement()
        .namedItem( "Details" )
        .attributes()
        .namedItem( "url" ).toAttr().value();

    const QString size = (m_size == 0)
        ? "ImageUrlSmall" : (m_size == 1)
        ? "ImageUrlMedium"
        : "ImageUrlLarge";

    m_imageURL = doc.documentElement()
        .namedItem( "Details" )
        .namedItem( size )
        .firstChild().toText().nodeValue();

    m_buffer = new uchar[BUFFER_SIZE];

    KIO::TransferJob* imageJob = KIO::get( m_imageURL, false, false );
    connect( imageJob, SIGNAL(result( KIO::Job* )), SLOT(finishedImageFetch( KIO::Job* )) );
    connect( imageJob, SIGNAL(data( KIO::Job*, const QByteArray& )), SLOT(receivedImageData( KIO::Job*, const QByteArray& )) );
}


void
CoverFetcher::receivedImageData( KIO::Job *job, const QByteArray& data ) //SLOT
{
    if ( m_bufferIndex + (uint) data.size() >= BUFFER_SIZE ) {
        error( i18n("The Image amazon have sent is too large, please report this error to the amaroK-devel@lists.sf.net.") );
        //TODO delete the KIO job?
        return;
    }

    // Append new chunk of data to buffer
    memcpy( m_buffer + m_bufferIndex, data.data(), data.size() );
    m_bufferIndex += data.size();
}


void
CoverFetcher::finishedImageFetch( KIO::Job *job ) //SLOT
{
    if ( job->error() ) {
        kdDebug() << "[CoverFetcher] Could not fetch image data, KIO errno: " << job->error() << endl;

        if ( !m_queries.isEmpty() )
            startFetch();
        else if ( m_userCanEditQuery )
            showQueryEditor( i18n("amaroK could not find the cover in Amazon's database. You can refine the search below:") );
        else
            error( i18n("Amazon has no covers available for the requested album."), job );

        return;
    }

    m_image.loadFromData( m_buffer, m_bufferIndex );

    if ( m_image.width() <= 1 ) {
        //Amazon has nothing to offer us for the requested image size
        if ( m_size > 0 ) {
            //we need to fetch the image again, but this time requesting a size smaller
            m_size--;
            finishedXmlFetch( job );
        }
        else if( m_userCanEditQuery )
            showQueryEditor( i18n("Amazon has a record for this album, <b>but with no associated images</b>. You can refine the search below:") );
        else
            error( i18n("Amazon has a record for this album, but can offer no cover images.") );
    }
    else if( m_userCanEditQuery )
    {
        class CoverFoundDialog : public KDialog {
        public:
            CoverFoundDialog( QWidget *parent, const QString &caption, const QImage &cover )
                    : KDialog( parent )
            {
                (new QVBoxLayout( this ))->setAutoAdd( true );
                (new QLabel( this ))->setPixmap( cover );
                QHBox* buttons         = new QHBox( this );
                KPushButton* save      = new KPushButton( KStdGuiItem::save(), buttons );
                KPushButton* newsearch = new KPushButton( i18n( "New &Search" ), buttons, "NewSearch" );
                KPushButton* cancel    = new KPushButton( KStdGuiItem::cancel(), buttons );
                save->setDefault( true );

                connect( save,      SIGNAL(clicked()), SLOT(accept()) );
                connect( newsearch, SIGNAL(clicked()), SLOT(accept()) );
                connect( cancel,    SIGNAL(clicked()), SLOT(reject()) );

                setFixedSize( sizeHint() );
                setCaption( caption );
            }

            virtual void accept()
            {
                if ( qstrcmp( sender()->name(), "NewSearch" ) == 0 )
                    done( 1000 );
                else
                    QDialog::accept();
            }
        };

        CoverFoundDialog dialog( (QWidget*)parent(), m_album, m_image );

        switch( dialog.exec() ) {
        case KDialog::Accepted:
            finish();
            break;
        case 1000:
            showQueryEditor();
            break;
        default:
            deleteLater();
            break;
        }
    }
    else
        //image loaded successfully yay!
        finish();
}

void
CoverFetcher::showQueryEditor( QString text ) //SLOT
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
            KPushButton* searchButton = new KPushButton( i18n("Search"), this );

            hbox->addItem( new QSpacerItem( 80, 8, QSizePolicy::Expanding, QSizePolicy::Minimum ) );
            hbox->addWidget( searchButton );
            hbox->addWidget( cancelButton );

            vbox->addWidget( new QLabel( "<qt>" + text, this ) );
            vbox->addWidget( new KLineEdit( keyword, this, "Query" ) );
            vbox->addLayout( hbox );

            searchButton->setDefault( true );

            adjustSize();
            setFixedHeight( height() );

            connect( searchButton, SIGNAL(clicked()), SLOT(accept()) );
            connect( cancelButton, SIGNAL(clicked()), SLOT(reject()) );
        }

        QString query() { return static_cast<KLineEdit*>(child( "Query" ))->text(); }
    };

    //use the user query, not stuff from in here
    //HACK this sucks, improve the api
    m_queries.clear();

    if ( text.isEmpty() )
        text = i18n("Search Amazon's cover database with this query:");

    EditSearchDialog dialog( (QWidget*)parent(), text, m_userQuery );

    switch( dialog.exec() ) {
    case QDialog::Accepted:
        m_userQuery = dialog.query();
        startFetch();
        break;
    default:
        deleteLater();
        break;
    }
}

void
CoverFetcher::finish() //SLOT
{
    emit result( this );

    deleteLater();
}

void
CoverFetcher::error( const QString &message, KIO::Job *job )
{
    kdWarning() << "[CoverFetcher] " << message << " KIO::error(): " << (job ? job->errorText() : "none") << endl;

    m_errorMessage = message;

    emit result( this );

    deleteLater();
}

#include "coverfetcher.moc"
