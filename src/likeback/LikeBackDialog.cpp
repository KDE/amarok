/****************************************************************************************
 * Copyright (c) 2006 Sebastien Laout <slaout@linux62.org>                              *
 * Copyright (c) 2008,2009 Valerio Pilo <amroth@kmess.org>                              *
 * Copyright (c) 2008,2009 Sjors Gielen <sjors@kmess.org>                               *
 * Copyright (c) 2010 Téo Mrnjavac <teo@kde.org>                                        *
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

#include "LikeBackDialog.h"

#include <KAboutData>
#include <KApplication>
#include <KConfig>
#include <KMessageBox>
#include <KPushButton>
#include <kio/job.h>

#include "core/support/Debug.h"
#include "LikeBack.h"


// Constructor
LikeBackDialog::LikeBackDialog( LikeBack::Button reason, const QString &initialComment,
                                const QString &windowPath, const QString &context, LikeBack *likeBack )
    : KDialog( kapp->activeWindow() )
    , Ui::LikeBackDialog()
    , m_context( context )
    , m_likeBack( likeBack )
    , m_windowPath( windowPath )
{
    // KDialog Options
    setCaption( i18n( "Send a Comment to the Developers" ) );
    setButtons( Ok | Cancel );
    setDefaultButton( Ok );
    setObjectName( "LikeBackFeedBack" );
    showButtonSeparator( true );
    restoreDialogSize( KGlobal::config()->group( "LikeBackDialog" ) );

    // Set up the user interface
    QWidget *mainWidget = new QWidget( this );
    setupUi( mainWidget );
    setMainWidget( mainWidget );

    // Group the buttons together to retrieve the checked one quickly
    m_typeGroup_ = new QButtonGroup( this );
    m_typeGroup_->addButton( likeRadio_,    LikeBack::Like    );
    m_typeGroup_->addButton( dislikeRadio_, LikeBack::Dislike );
    m_typeGroup_->addButton( bugRadio_,     LikeBack::Bug     );
    m_typeGroup_->addButton( featureRadio_, LikeBack::Feature );

    // Hide unneeded buttons
    LikeBack::Button buttons = m_likeBack->buttons();
    likeRadio_   ->setVisible( buttons & LikeBack::Like    );
    dislikeRadio_->setVisible( buttons & LikeBack::Dislike );
    bugRadio_    ->setVisible( buttons & LikeBack::Bug     );
    featureRadio_->setVisible( buttons & LikeBack::Feature );

    // If no specific "reason" is provided, choose the first one:
    if( reason == LikeBack::AllButtons || reason == LikeBack::DefaultButtons || reason == LikeBack::AllButBugs )
    {
        if( buttons & LikeBack::Dislike )      reason = LikeBack::Dislike;
        else if( buttons & LikeBack::Bug     ) reason = LikeBack::Bug;
        else if( buttons & LikeBack::Feature ) reason = LikeBack::Feature;
        else                                   reason = LikeBack::Like;
    }
  // Choose which button to check
    switch( reason )
    {
        case LikeBack::Like:    likeRadio_   ->setChecked( true ); break;
        case LikeBack::Dislike: dislikeRadio_->setChecked( true ); break;
        case LikeBack::Bug:     bugRadio_    ->setChecked( true ); break;
        case LikeBack::Feature: featureRadio_->setChecked( true ); break;
        default: break; // Will never arrive here
    }

    // Disable the Ok button if no comment is present
    connect( m_comment, SIGNAL( textChanged() ),
            this,      SLOT  (      verify() ) );

    // If no window path is provided, get the current active window path:
    if( m_windowPath.isEmpty() )
    {
        m_windowPath = LikeBack::activeWindowPath();
    }

    // Specify the initial comment
    m_comment->setPlainText( initialComment );
    m_comment->setFocus();

    // Provide the initial status for email address widgets if available
    emailAddressEdit_->setText( m_likeBack->emailAddress() );
    specifyEmailCheckBox_->setChecked( true );

    // The introduction message is long and will require a new minimum dialog size
    m_informationLabel->setText( introductionText() );
    setMinimumSize( minimumSizeHint() );

    // Initially verify the widgets status
    verify();
}


// Destructor
LikeBackDialog::~LikeBackDialog()
{
    KConfigGroup group = KGlobal::config()->group( "LikeBackDialog" );
    saveDialogSize( group );
}


// Construct the introductory text of the dialog
QString LikeBackDialog::introductionText()
{
    QStringList acceptedLocales;
    KLocale *kLocale = KGlobal::locale();
    QStringList acceptedLocaleCodes = m_likeBack->acceptedLocales();

    // Define a list of languages which the application developers are able to understand
    if( ! acceptedLocaleCodes.isEmpty() )
    {
        foreach( const QString &locale, acceptedLocaleCodes )
        {
            acceptedLocales << kLocale->languageCodeToName( locale );
        }
    }
    else if( ! kLocale->language().startsWith( "en" ) )
    {
        acceptedLocales << kLocale->languageCodeToName( "en" );
    }

    // Put the locales list together in a readable string
    QString languagesMessage;
    if( ! acceptedLocales.isEmpty() )
    {
        // TODO: Replace the URL with a localized one:
        QString translationTool( "http://www.google.com/language_tools?hl=" + kLocale->language() );

        if( acceptedLocales.count() == 1 )
        {
            languagesMessage = i18nc( "Feedback dialog text, message with one accepted language for the comments",
                                      "Please, write it in <b>%1</b> (you may want to use an <a href=\"%3\">online translation tool</a> for this).<br/>",
                                      acceptedLocales.first(),
                                      translationTool );
        }
        else
        {
            languagesMessage = i18nc( "Feedback dialog text, message with list of accepted languages for the comments",
                                      "Please, write it in <b>%1 or %2</b> (you may want to use an <a href=\"%3\">online translation tool</a> for this).<br/>",
                                      QStringList( acceptedLocales.mid( 0, acceptedLocales.count() - 1 ) ).join( ", " ),
                                      acceptedLocales.last(),
                                      translationTool );
        }
    }

    // If both "I Like" and "I Dislike" buttons are shown and one is clicked:
    QString balancingMessage;
    if( m_likeBack->isLikeActive() && m_likeBack->isDislikeActive()
    && ( m_typeGroup_->checkedId() == LikeBack::Like || m_typeGroup_->checkedId() == LikeBack::Dislike ) )
    {
        balancingMessage = i18nc( "Feedback dialog text, message to remind to balance the likes and dislikes",
                                  "To make the comments you send more useful in improving this application, "
                                  "try to send the same amount of positive and negative comments.<br/>" );
    }

    // If feature requests are not allowed:
    QString noFeatureRequestsMessage;
    if( ! m_likeBack->isFeatureActive() )
    {
        noFeatureRequestsMessage = i18nc( "Feedback dialog text, text to disallow feature requests",
                                          "Please, do not ask for new features: this kind of request will be ignored.<br/>" );
    }

    // Blend all previous messages together
    return i18nc( "Feedback dialog text, %1=Application name,%2=message with list of accepted languages for the comment,"
                  "%3=optional text to remind to balance the likes and dislikes,%4=optional text to disallow feature requests.",
                  "<p>You can provide the developers a brief description of your opinions about %1.<br/>"
                  "%2 " // %2: Contains the newline if present
                  "%3%4</p>",
                  m_likeBack->aboutData()->programName(),
                  languagesMessage,
                  balancingMessage,
                  noFeatureRequestsMessage );
}


// Check if the UI should allow the user to send the comment
void LikeBackDialog::verify()
{
    bool hasComment = ( ! m_comment->document()->isEmpty() );
    bool hasType    = ( m_typeGroup_->checkedId() != -1 );

    button( Ok )->setEnabled( hasComment && hasType );
}


// Send the comment to the developers site (reimplemented from KDialog)
void LikeBackDialog::slotButtonClicked( int buttonId )
{
    // If the user has not pressed Ok, do nothing
    if( buttonId != Ok )
    {
        KDialog::slotButtonClicked( buttonId );
        return;
    }

    QString type;
    QString emailAddress;

    // Only send the email if the user wants it to be sent
    if( specifyEmailCheckBox_->isChecked() )
    {
        emailAddress = emailAddressEdit_->text();

        // lame-ass way to check if the e-mail address is valid:
        if( !emailAddress.contains( QRegExp( "^[A-Z0-9._%\\-]+@(?:[A-Z0-9\\-]+\\.)+[A-Z]{2,4}$", Qt::CaseInsensitive ) ) )
        {
            KMessageBox::error( this, i18n( "The email address you have entered is not valid, and cannot be used: '%1'", emailAddress ) );
            return;
        }

        m_likeBack->setEmailAddress( emailAddress, true );
    }

    // Disable the UI while we're sending the request
    m_comment->setEnabled( false );
    button( Ok )->setEnabled( false );

    // Choose the type of feedback
    switch( m_typeGroup_->checkedId() )
    {
        case LikeBack::Like:    type = "Like";    break;
        case LikeBack::Dislike: type = "Dislike"; break;
        case LikeBack::Bug:     type = "Bug";     break;
        case LikeBack::Feature: type = "Feature"; break;
    }

    // Compile the feedback data
    QString data( "protocol=" + QUrl::toPercentEncoding( "1.0" )                              + '&' +
                  "type="     + QUrl::toPercentEncoding( type )                               + '&' +
                  "version="  + QUrl::toPercentEncoding( m_likeBack->aboutData()->version() ) + '&' +
                  "locale="   + QUrl::toPercentEncoding( KGlobal::locale()->language() )      + '&' +
                  "window="   + QUrl::toPercentEncoding( m_windowPath )                       + '&' +
                  "context="  + QUrl::toPercentEncoding( m_context )                          + '&' +
                  "comment="  + QUrl::toPercentEncoding( m_comment->toPlainText() )           + '&' +
                  "email="    + QUrl::toPercentEncoding( emailAddress ) );

    #ifdef DEBUG_LIKEBACK
    debug() << "http://" << m_likeBack->hostName() << ":" << m_likeBack->hostPort() << m_likeBack->remotePath();
    debug() << data;
    #endif

    // Create the HTTP sending object and the actual request
    KUrl url;
    url.setHost( m_likeBack->hostName() );
    url.setPort( m_likeBack->hostPort() );
    url.setPath( m_likeBack->remotePath() );
    KIO::StoredTransferJob *job = KIO::storedHttpPost( data.toUtf8(), url );
    connect( job, SIGNAL(finished(KJob*)),
             this, SLOT(finished(KJob*)) );
    job->addMetaData( "content-type", "application/x-www-form-urlencoded" );
}


// Display confirmation of the sending action
void LikeBackDialog::finished( KJob *j )
{
    KIO::StoredTransferJob *job = static_cast< KIO::StoredTransferJob *>(j);

    #ifdef DEBUG_LIKEBACK
    debug() << "Request has" << (job->error()?"failed":"succeeded");
    #endif

    m_likeBack->disableBar();

    if( job->error() == 0 )
    {
        KMessageBox::information( this,
                                  i18nc( "Dialog box text",
                                         "<p>Your comment has been sent successfully.</p>"
                                         "<p>Thank you for your time.</p>" ),
                                  i18nc( "Dialog box title", "Comment Sent" ) );

        hide();
        m_likeBack->enableBar();
        KDialog::accept();
        return;
    }

    // TODO: Save to file if error (connection not present at the moment)
    KMessageBox::error( this,
                        i18nc( "Dialog box text",
                               "<p>There has been an error while trying to send the comment.</p>"
                               "<p>Please, try again later.</p>"),
                        i18nc( "Dialog box title", "Comment Sending Error" ) );

    m_likeBack->enableBar();

    // Re-enable the UI
    m_comment->setEnabled( true );
    verify();
}

#include "LikeBackDialog.moc"
