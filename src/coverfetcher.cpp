// (C) 2004 Mark Kretschmann <markey@web.de>
// (C) 2004 Stefan Bogner <bochi@online.ms>
// (C) 2004 Max Howell
// See COPYING file for licensing information.

#include "amarok.h"
#include "amarokconfig.h"
#include "coverfetcher.h"
#include "debug.h"
#include "statusbar.h"

#include <qdom.h>
#include <qhbox.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qregexp.h>

#include <kapplication.h>
#include <kcursor.h> //waiting cursor
#include <kdialog.h>
#include <kio/job.h>
#include <kio/jobclasses.h>
#include <klineedit.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kpushbutton.h>
#include <kcombobox.h>
#include <qmessagebox.h>



CoverFetcher::CoverFetcher( QWidget *parent, QString artist, QString album )
        : QObject( parent, "CoverFetcher" )
        , m_artist( artist )
        , m_album( album )
        , m_buffer( 0 )
        , m_bufferIndex( 0 )
        , m_size( 2 )
{
    DEBUG_FUNC_INFO

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
    DEBUG_FUNC_INFO

    QApplication::restoreOverrideCursor();

    delete[] m_buffer;
}

void
CoverFetcher::startFetch()
{
    DEBUG_FUNC_INFO

    // Static license Key. Thanks muesli ;-)
    const char *LICENSE = "D1URM11J3F2CEH";

    // reset all values
    m_coverUrls.clear();
    m_xml = QString::null;
    m_size = 2;

    QString query = m_queries.front();
    m_queries.pop_front();

    // Bug 97901: Import cover from amazon france doesn't work properly
    // (we have to set "mode=music-fr" instead of "mode=music")
    QString musicMode = AmarokConfig::amazonLocale() == "fr" ? "music-fr" : "music";

    QString url;
    url = "http://xml.amazon.com/onca/xml3?t=webservices-20&dev-t=%1&KeywordSearch=%2&mode=%3&type=heavy&locale=%4&page=1&f=xml";
    url = url.arg( LICENSE, KURL::encode_string_no_slash( query ), musicMode, AmarokConfig::amazonLocale() );

    debug() << url << endl;

    KIO::TransferJob* job = KIO::get( url, false, false );

    connect( job, SIGNAL(result( KIO::Job* )), SLOT(finishedXmlFetch( KIO::Job* )) );
    connect( job, SIGNAL(data( KIO::Job*, const QByteArray& )), SLOT(receivedXmlData( KIO::Job*, const QByteArray& )) );

    amaroK::StatusBar::instance()->newProgressOperation( job );
}

void
CoverFetcher::attemptAnotherFetch()
{
    DEBUG_BLOCK

    delete[] m_buffer;
    m_buffer = 0;
    m_bufferIndex = 0;

    if( !m_coverUrls.isEmpty() ) {
        // Amazon suggested some more cover URLs to try before we
        // try a different query

        m_buffer = new uchar[BUFFER_SIZE];

        KIO::TransferJob* job = KIO::get( KURL(m_coverUrls.front()), false, false );
        connect( job, SIGNAL(result( KIO::Job* )), SLOT(finishedImageFetch( KIO::Job* )) );
        connect( job, SIGNAL(data( KIO::Job*, const QByteArray& )), SLOT(receivedImageData( KIO::Job*, const QByteArray& )) );

        amaroK::StatusBar::instance()->newProgressOperation( job );

        m_coverUrls.pop_front();
    }

    else if( !m_xml.isEmpty() && m_size > 0 ) {
        // we need to try smaller sizes, this often is
        // fruitless, but does work out sometimes.
        m_size--;

        finishedXmlFetch( 0 );
    }

    else if( !m_queries.isEmpty() ) {
        // we have some queries left in the pot
        startFetch();
    }

    else if( m_userCanEditQuery ) {
        // we have exhausted all the predetermined queries
        // so lets let the user give it a try
        getUserQuery( i18n("You have seen all the covers Amazon returned using the query below. Perhaps you can refine it:") );
    }
    else
        finishWithError( i18n("No cover found") );
}



//////////////////////////////////////////////////////////////////////////////////////////
// PRIVATE SLOTS
//////////////////////////////////////////////////////////////////////////////////////////

void
CoverFetcher::receivedXmlData( KIO::Job*, const QByteArray& data ) //SLOT
{
    // Append new chunk of string
    m_xml += QString( data );
}

void
CoverFetcher::finishedXmlFetch( KIO::Job *job ) //SLOT
{
    DEBUG_BLOCK

    if( job && job->error() ) {
        finishWithError( i18n("There was an error communicating with Amazon."), job );
        return;
    }

    QDomDocument doc;
    if( !doc.setContent( m_xml ) ) {
        m_errors += i18n("The XML obtained from Amazon is invalid.");
        startFetch();
        return;
    }

    const QDomNode details = doc.documentElement().namedItem( "Details" );

    // the url for the Amazon product info page
    m_amazonURL = details.attributes().namedItem( "url" ).toAttr().value();

    QString size = "ImageUrl";
    switch( m_size ) {
        case 0:  size += "Small";  break;
        case 1:  size += "Medium"; break;
        default: size += "Large";  break;
    }

    debug() << "Fetching size: " << size << endl;

    m_coverUrls.clear();
    for( QDomNode node = details; !node.isNull(); node = node.nextSibling() ) {
        QString url = node.namedItem( size ).firstChild().toText().nodeValue();

        debug() << url << endl;

        if( !url.isEmpty() )
            m_coverUrls += url;
    }

    attemptAnotherFetch();
}


void
CoverFetcher::receivedImageData( KIO::Job*, const QByteArray& data ) //SLOT
{
    if( m_bufferIndex + (uint)data.size() >= BUFFER_SIZE ) {
        m_errors += i18n(
                "The image Amazon has sent is too large. "
                "Please report this error to amaroK-devel@lists.sf.net.");
        attemptAnotherFetch();
    }
    else {
        // Append new chunk of data to buffer
        memcpy( m_buffer + m_bufferIndex, data.data(), data.size() );
        m_bufferIndex += data.size();
    }
}


void
CoverFetcher::finishedImageFetch( KIO::Job *job ) //SLOT
{
    if( job->error() ) {
        debug() << "finishedImageFetch(): KIO::error(): " << job->error() << endl;

        m_errors += i18n("The cover could not be retrieved.");

        attemptAnotherFetch();
        return;
    }

    m_image.loadFromData( m_buffer, m_bufferIndex );

    if( m_image.width() <= 1 ) {
        //Amazon seems to offer images of size 1x1 sometimes
        //Amazon has nothing to offer us for the requested image size
        m_errors += i18n("The cover-data produced an invalid image.");
        attemptAnotherFetch();
    }

    else if( m_userCanEditQuery ) {
        //yay! image found :)
        //lets see if the user wants it

        showCover();
    }
    else
        //image loaded successfully yay!
        finish();
}

void
CoverFetcher::getUserQuery( QString explanation )
{
    class EditSearchDialog : public KDialog
    {
    public:
        EditSearchDialog( QWidget* parent, const QString &text, const QString &keyword )
                : KDialog( parent )
        {
            setCaption( i18n( "Amazon Query Editor" ) );

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

    if( explanation.isEmpty() )
        explanation = i18n("Ask Amazon for covers using this query:");

    EditSearchDialog dialog(
            (QWidget*)parent(),
            explanation,
            m_userQuery );

    switch( dialog.exec() )
    {
    case QDialog::Accepted:
        m_userQuery = dialog.query();
        m_queries = m_userQuery;
        startFetch();
        break;
    default:
        deleteLater();
        break;
    }
}

void
CoverFetcher::showCover()
{
    class CoverFoundDialog : public KDialog
    {
    public:
        CoverFoundDialog( QWidget *parent, const QString &caption, const QImage &cover )
                : KDialog( parent )
        {
            (new QVBoxLayout( this ))->setAutoAdd( true );

            QLabel      *label     = new QLabel( this );
            QHBox       *buttons   = new QHBox( this );
            KPushButton *save      = new KPushButton( KStdGuiItem::save(), buttons );
            KPushButton *newsearch = new KPushButton( i18n( "New &Search" ), buttons, "NewSearch" );
            KPushButton *nextcover = new KPushButton( i18n( "Next &Cover" ), buttons, "NextCover" );
            KPushButton *cancel    = new KPushButton( KStdGuiItem::cancel(), buttons );

            label->setAlignment( Qt::AlignHCenter );
            label->setPixmap( cover );
            save->setDefault( true );
            this->setFixedSize( sizeHint() );
            this->setCaption( caption );

            connect( save,      SIGNAL(clicked()), SLOT(accept()) );
            connect( newsearch, SIGNAL(clicked()), SLOT(accept()) );
            connect( nextcover, SIGNAL(clicked()), SLOT(accept()) );
            connect( cancel,    SIGNAL(clicked()), SLOT(reject()) );
        }

        virtual void accept()
        {
            if( qstrcmp( sender()->name(), "NewSearch" ) == 0 )
                done( 1000 );
            else if( qstrcmp( sender()->name(), "NextCover" ) == 0 )
                done( 1001 );
            else
                KDialog::accept();
        }
    };

    CoverFoundDialog dialog( (QWidget*)parent(), m_album, m_image );

    switch( dialog.exec() )
    {
    case KDialog::Accepted:
        finish();
        break;
    case 1000: //showQueryEditor()
        getUserQuery();
        break;
    case 1001: //nextCover()
        attemptAnotherFetch();
        break;
    default:
        deleteLater();
        break;
    }
}


void
CoverFetcher::finish()
{
    emit result( this );

    deleteLater();
}

void
CoverFetcher::finishWithError( const QString &message, KIO::Job *job )
{
    if( job )
        warning() << message << " KIO::error(): " << job->errorText() << endl;

    m_errors += message;

    emit result( this );

    deleteLater();
}

#include "coverfetcher.moc"
