/****************************************************************************************
 * Copyright (c) 2007 Urs Wolfer <uwolfer at kde.org>                                   *
 * Copyright (c) 2008 Friedrich W. H. Kossebau <kossebau@kde.org>                       *
 * Copyright (c) 2009 Téo Mrnjavac <teo@kde.org>                                        *
 *                                                                                      *
 * Parts of this class have been take from the KAboutApplication class, which was       *
 * Copyright (c) 2000 Waldo Bastian (bastian@kde.org) and Espen Sand (espen@kde.org)    *
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

#include "ExtendedAboutDialog.h"

#include "core/support/Amarok.h"
#include "core/support/Debug.h"
#include "OcsPersonItem.h"
#include "libattica-ocsclient/providerinitjob.h"

#include <QLabel>
#include <QLayout>
#include <QPushButton>
#include <QScrollBar>
#include <QTabWidget>

#include <kapplication.h>
#include <kglobal.h>
#include <kglobalsettings.h>
#include <kiconloader.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kstandarddirs.h>
#include <ktextbrowser.h>
#include <ktitlewidget.h>
#include <solid/networking.h>


class ExtendedAboutDialog::Private
{
public:
    Private(ExtendedAboutDialog *parent)
        : q(parent),
          aboutData(0)
    {}

    void _k_showLicense( const QString &number );

    ExtendedAboutDialog *q;

    const KAboutData *aboutData;
};

ExtendedAboutDialog::ExtendedAboutDialog(const KAboutData *aboutData, const OcsData *ocsData, QWidget *parent)
  : KDialog(parent)
  , d(new Private(this))
{
    DEBUG_BLOCK
    if (aboutData == 0)
        aboutData = KGlobal::mainComponent().aboutData();

    d->aboutData = aboutData;

    if (!aboutData) {
        QLabel *errorLabel = new QLabel(i18n("<qt>No information available.<br />"
                                             "The supplied KAboutData object does not exist.</qt>"), this);

        errorLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
        setMainWidget(errorLabel);
        return;
    }
    if( !ocsData )
    {
        QLabel *errorLabel = new QLabel(i18n("<qt>No information available.<br />"
                                             "The supplied OcsData object does not exist.</qt>"), this);

        errorLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
        setMainWidget(errorLabel);
        return;
    }
    m_ocsData = *ocsData;

    setPlainCaption(i18n("About %1", aboutData->programName()));
    setButtons(KDialog::Close);
    setDefaultButton(KDialog::Close);
    setModal(false);


    //Set up the title widget...
    KTitleWidget *titleWidget = new KTitleWidget(this);

    QIcon windowIcon;
    if (!aboutData->programIconName().isEmpty()) {
        windowIcon = KIcon(aboutData->programIconName());
    } else {
        windowIcon = qApp->windowIcon();
    }
    titleWidget->setPixmap(windowIcon.pixmap(64, 64), KTitleWidget::ImageLeft);
    if (aboutData->programLogo().canConvert<QPixmap>())
        titleWidget->setPixmap(aboutData->programLogo().value<QPixmap>(), KTitleWidget::ImageLeft);
    else if (aboutData->programLogo().canConvert<QImage>())
        titleWidget->setPixmap(QPixmap::fromImage(aboutData->programLogo().value<QImage>()), KTitleWidget::ImageLeft);

    titleWidget->setText(i18n("<html><font size=\"5\">%1</font><br /><b>Version %2</b><br />Using KDE %3</html>",
                         aboutData->programName(), aboutData->version(), QString(KDE_VERSION_STRING)));


    //Now let's add the tab bar...
    QTabWidget *tabWidget = new QTabWidget;
    tabWidget->setUsesScrollButtons(false);


    //Set up the first page...
    QString aboutPageText = aboutData->shortDescription() + '\n';

    if (!aboutData->otherText().isEmpty())
        aboutPageText += '\n' + aboutData->otherText() + '\n';

    if (!aboutData->copyrightStatement().isEmpty())
        aboutPageText += '\n' + aboutData->copyrightStatement() + '\n';

    if (!aboutData->homepage().isEmpty())
        aboutPageText += '\n' + QString("<a href=\"%1\">%1</a>").arg(aboutData->homepage()) + '\n';
    aboutPageText = aboutPageText.trimmed();

    QLabel *aboutLabel = new QLabel;
    aboutLabel->setWordWrap(true);
    aboutLabel->setOpenExternalLinks(true);
    aboutLabel->setText(aboutPageText.replace('\n', "<br />"));
    aboutLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);

    QVBoxLayout *aboutLayout = new QVBoxLayout;
    aboutLayout->addStretch();
    aboutLayout->addWidget(aboutLabel);

    const int licenseCount = aboutData->licenses().count();
    debug()<< "About to show license stuff";
    debug()<< "License count is"<<licenseCount;
    for (int i = 0; i < licenseCount; ++i) {
        const KAboutLicense &license = aboutData->licenses().at(i);

        QLabel *showLicenseLabel = new QLabel;
        showLicenseLabel->setText(QString("<a href=\"%1\">%2</a>").arg(QString::number(i),
                                                                       i18n("License: %1",
                                                                            license.name(KAboutData::FullName))));
        showLicenseLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
        connect(showLicenseLabel, SIGNAL(linkActivated(QString)), this, SLOT(_k_showLicense(QString)));

        aboutLayout->addWidget(showLicenseLabel);
    }
    debug()<<"License widget added";

    aboutLayout->addStretch();

    QWidget *aboutWidget = new QWidget(this);
    aboutWidget->setLayout(aboutLayout);

    tabWidget->addTab(aboutWidget, i18n("&About"));


    //Stuff needed by both Authors and Credits pages:
    QPixmap openDesktopPixmap = QPixmap( KStandardDirs::locate( "data", "amarok/images/opendesktop-22.png" ) );
    QIcon openDesktopIcon = QIcon( openDesktopPixmap );


    //And now, the Authors page:
    const int authorCount = d->aboutData->authors().count();

    if (authorCount)
    {
        m_authorWidget = new QWidget( this );
        QVBoxLayout *authorLayout = new QVBoxLayout( m_authorWidget );

        m_showOcsAuthorButton = new AnimatedBarWidget( openDesktopIcon,
                                     i18n( "Get data from openDesktop.org to learn more about the team" ),
                                     "process-working", m_authorWidget );
        connect( m_showOcsAuthorButton, SIGNAL( clicked() ), this, SLOT( switchToOcsWidgets() ) );
        authorLayout->addWidget( m_showOcsAuthorButton );

        if (!aboutData->customAuthorTextEnabled() || !aboutData->customAuthorRichText().isEmpty())
        {
            QLabel *bugsLabel = new QLabel( m_authorWidget );
            bugsLabel->setContentsMargins( 4, 2, 0, 4 );
            if (!aboutData->customAuthorTextEnabled())
            {
                if (aboutData->bugAddress().isEmpty() || aboutData->bugAddress() == "submit@bugs.kde.org")
                    bugsLabel->setText( i18n("Please use <a href=\"http://bugs.kde.org\">http://bugs.kde.org</a> to report bugs.\n") );
                else
                {
                    if(aboutData->authors().count() == 1 && (aboutData->authors().first().emailAddress() == aboutData->bugAddress()))
                    {
                        bugsLabel->setText( i18n("Please report bugs to <a href=\"mailto:%1\">%2</a>.\n",
                                              aboutData->authors().first().emailAddress(),
                                              aboutData->authors().first().emailAddress()));
                    }
                    else
                    {
                        bugsLabel->setText( i18n("Please report bugs to <a href=\"mailto:%1\">%2</a>.\n",
                                              aboutData->bugAddress(), aboutData->bugAddress()));
                    }
                }
            }
            else
                bugsLabel->setText( aboutData->customAuthorRichText() );
            authorLayout->addWidget( bugsLabel );
        }

        m_authorListWidget = new OcsPersonListWidget( d->aboutData->authors(), m_ocsData.authors(), OcsPersonItem::Author, m_authorWidget );
        connect( m_authorListWidget, SIGNAL( switchedToOcs() ), m_showOcsAuthorButton, SLOT( stop() ) );
        connect( m_authorListWidget, SIGNAL( switchedToOcs() ), m_showOcsAuthorButton, SLOT( fold() ) );

        authorLayout->addWidget( m_authorListWidget );
        authorLayout->setMargin( 0 );
        authorLayout->setSpacing( 2 );
        m_authorWidget->setLayout( authorLayout );

        m_authorPageTitle = ( authorCount == 1 ) ? i18n("A&uthor") : i18n("A&uthors");
        tabWidget->addTab(m_authorWidget, m_authorPageTitle);
        m_isOfflineAuthorWidget = true; //is this still used?
    }

    //Finally, the Credits page:
    const int creditCount = aboutData->credits().count();

    if (creditCount)
    {
        m_creditWidget = new QWidget( this );
        QVBoxLayout *creditLayout = new QVBoxLayout( m_creditWidget );

        m_showOcsCreditButton = new AnimatedBarWidget( openDesktopIcon,
                                     i18n( "Get data from openDesktop.org to learn more about contributors" ),
                                     "process-working", m_creditWidget );
        connect( m_showOcsCreditButton, SIGNAL( clicked() ), this, SLOT( switchToOcsWidgets() ) );
        creditLayout->addWidget( m_showOcsCreditButton );

        m_creditListWidget = new OcsPersonListWidget( d->aboutData->credits(), m_ocsData.credits(), OcsPersonItem::Contributor, m_creditWidget );
        connect( m_creditListWidget, SIGNAL( switchedToOcs() ), m_showOcsCreditButton, SLOT( stop() ) );
        connect( m_creditListWidget, SIGNAL( switchedToOcs() ), m_showOcsCreditButton, SLOT( fold() ) );

        creditLayout->addWidget( m_creditListWidget );
        creditLayout->setMargin( 0 );
        creditLayout->setSpacing( 2 );
        m_creditWidget->setLayout( creditLayout );

        tabWidget->addTab( m_creditWidget, i18n("&Thanks To"));
        m_isOfflineCreditWidget = true; //is this still used?
    }

    //And the translators:
    QPalette transparentBackgroundPalette;
    transparentBackgroundPalette.setColor( QPalette::Base, Qt::transparent );
    transparentBackgroundPalette.setColor( QPalette::Text, transparentBackgroundPalette.color( QPalette::WindowText ) );


    const QList<KAboutPerson> translatorList = aboutData->translators();

    if(translatorList.count() > 0) {
        QString translatorPageText;

        QList<KAboutPerson>::ConstIterator it;
        for(it = translatorList.begin(); it != translatorList.end(); ++it) {
            translatorPageText += QString("<p style=\"margin: 0px;\">%1</p>").arg((*it).name());
            if (!(*it).emailAddress().isEmpty())
                translatorPageText += QString("<p style=\"margin: 0px; margin-left: 15px;\"><a href=\"mailto:%1\">%1</a></p>").arg((*it).emailAddress());
            translatorPageText += "<p style=\"margin: 0px;\">&nbsp;</p>";
        }

        translatorPageText += KAboutData::aboutTranslationTeam();

        KTextBrowser *translatorTextBrowser = new KTextBrowser;
        translatorTextBrowser->setFrameStyle(QFrame::NoFrame);
        translatorTextBrowser->setPalette(transparentBackgroundPalette);
        translatorTextBrowser->setHtml(translatorPageText);
        tabWidget->addTab(translatorTextBrowser, i18n("T&ranslation"));
    }

    //Jam everything together in a layout:
    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addWidget(titleWidget);
    mainLayout->addWidget(tabWidget);
    mainLayout->setMargin(0);

    QWidget *mainWidget = new QWidget;
    mainWidget->setLayout(mainLayout);
    setMainWidget(mainWidget);
    setInitialSize( QSize( 480, 460 ) );
}

ExtendedAboutDialog::~ExtendedAboutDialog()
{
    delete d;
}

void ExtendedAboutDialog::Private::_k_showLicense( const QString &number )
{
    KDialog *dialog = new KDialog(q);

    dialog->setCaption(i18n("License Agreement"));
    dialog->setButtons(KDialog::Close);
    dialog->setDefaultButton(KDialog::Close);

    const QFont font = KGlobalSettings::fixedFont();
    QFontMetrics metrics(font);

    const QString licenseText = aboutData->licenses().at(number.toInt()).text();
    KTextBrowser *licenseBrowser = new KTextBrowser;
    licenseBrowser->setFont(font);
    licenseBrowser->setLineWrapMode(QTextEdit::NoWrap);
    licenseBrowser->setText(licenseText);

    dialog->setMainWidget(licenseBrowser);

    // try to set up the dialog such that the full width of the
    // document is visible without horizontal scroll-bars being required
    const qreal idealWidth = licenseBrowser->document()->idealWidth() + (2 * dialog->marginHint())
        + licenseBrowser->verticalScrollBar()->width() * 2;

    // try to allow enough height for a reasonable number of lines to be shown
    const int idealHeight = metrics.height() * 30;

    dialog->setInitialSize(dialog->sizeHint().expandedTo(QSize((int)idealWidth,idealHeight)));
    dialog->show();
}

void
ExtendedAboutDialog::switchToOcsWidgets()
{    
    if( !( Solid::Networking::status() == Solid::Networking::Connected ||
           Solid::Networking::status() == Solid::Networking::Unknown ) )
    {
        KMessageBox::error( this, i18n( "Internet connection not available" ), i18n( "Network error" ) );
        return;
    }

    m_showOcsAuthorButton->animate();
    m_showOcsCreditButton->animate();
    AmarokAttica::ProviderInitJob *providerJob = AmarokAttica::Provider::byId( m_ocsData.providerId() );
    connect( providerJob, SIGNAL( result( KJob * ) ), this, SLOT( onProviderFetched( KJob * ) ) );
}

void
ExtendedAboutDialog::onProviderFetched( KJob *job )
{
    AmarokAttica::ProviderInitJob *providerJob = qobject_cast< AmarokAttica::ProviderInitJob * >( job );
    if( providerJob->error() == 0 )
    {
        debug()<<"Successfully fetched OCS provider"<< providerJob->provider().name();
        debug()<<"About to request OCS data";
        m_authorListWidget->switchToOcs( providerJob->provider() );
        m_creditListWidget->switchToOcs( providerJob->provider() );
    }
    else
        warning() << "OCS provider fetch failed";
}

#include "ExtendedAboutDialog.moc"
