/****************************************************************************************
 * Copyright (c) 2007 Urs Wolfer <uwolfer at kde.org>                                   *
 * Copyright (c) 2008 Friedrich W. H. Kossebau <kossebau@kde.org>                       *
 * Copyright (c) 2009 Téo Mrnjavac <teo.mrnjavac@gmail.com>                             *
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
 * PARTICULAR PURPOSE. See the GNU General Pulic License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#include "ExtendedAboutDialog.h"

#include "Amarok.h"
#include "Debug.h"
#include "libattica-ocsclient/ocsapi.h"
#include "libattica-ocsclient/personjob.h"
#include "OcsPersonItem.h"

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
  : KDialog(parent),
    d(new Private(this))
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

    aboutLayout->addStretch();

    QWidget *aboutWidget = new QWidget(this);
    aboutWidget->setLayout(aboutLayout);

    tabWidget->addTab(aboutWidget, i18n("&About"));


    //Stuff needed by both Authors and Credits pages:
    m_transparentBackgroundPalette.setColor(QPalette::Base, Qt::transparent);
    m_transparentBackgroundPalette.setColor(QPalette::Text, m_transparentBackgroundPalette.color(QPalette::WindowText));
    QPixmap openDesktopPixmap = QPixmap( KStandardDirs::locate( "data", "amarok/images/opendesktop.png" ) );
    QIcon openDesktopIcon = QIcon( openDesktopPixmap );


    //And now, the Authors page:
    m_authorWidget = new QWidget( this );
    QVBoxLayout *authorLayout = new QVBoxLayout( m_authorWidget );
    m_offlineAuthorWidget = new QWidget( m_authorWidget );
    m_ocsAuthorWidget = new OcsPersonListWidget( OcsPersonItem::Author, m_authorWidget );

    m_showOcsAuthorButton = new AnimatedBarWidget( openDesktopIcon,
                                 i18n( "Get data from openDesktop.org to learn more about the team" ),
                                 "process-working", m_authorWidget );
    connect( m_showOcsAuthorButton, SIGNAL( clicked() ), this, SLOT( switchToOcsWidgets() ) );
    authorLayout->addWidget( m_showOcsAuthorButton );

    setupOfflineAuthorWidget(); //populate m_authorWidget and set m_authorPageTitle

    authorLayout->addWidget( m_offlineAuthorWidget );
    authorLayout->addWidget( m_ocsAuthorWidget );
    authorLayout->setMargin( 0 );
    m_authorWidget->setLayout( authorLayout );

    tabWidget->addTab(m_authorWidget, m_authorPageTitle);


    //Finally, the Credits page:
    m_creditWidget = new QWidget( this );
    QVBoxLayout *creditLayout = new QVBoxLayout( m_creditWidget );
    m_offlineCreditWidget = new QWidget( m_creditWidget );
    m_ocsCreditWidget = new OcsPersonListWidget( OcsPersonItem::Contributor, m_creditWidget );

    m_showOcsCreditButton = new AnimatedBarWidget( openDesktopIcon,
                                 i18n( "Get data from openDesktop.org to learn more about contributors" ),
                                 "process-working", m_creditWidget );
    connect( m_showOcsCreditButton, SIGNAL( clicked() ), this, SLOT( switchToOcsWidgets() ) );
    creditLayout->addWidget( m_showOcsCreditButton );

    setupOfflineCreditWidget(); //populate m_creditWidget

    creditLayout->addWidget( m_offlineCreditWidget );
    creditLayout->addWidget( m_ocsCreditWidget );
    creditLayout->setMargin( 0 );
    m_creditWidget->setLayout( creditLayout );

    tabWidget->addTab( m_creditWidget, i18n("&Thanks To"));


    //And the translators:
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
        translatorTextBrowser->setPalette(m_transparentBackgroundPalette);
        translatorTextBrowser->setHtml(translatorPageText);
        tabWidget->addTab(translatorTextBrowser, i18n("T&ranslation"));
    }

    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addWidget(titleWidget);
    mainLayout->addWidget(tabWidget);
    mainLayout->setMargin(0);

    QWidget *mainWidget = new QWidget;
    mainWidget->setLayout(mainLayout);
    setMainWidget(mainWidget);
    setInitialSize( QSize( 600, 460 ) );
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
    //TODO: Ask Solid if the network is available.
    if( sender() == m_showOcsAuthorButton )
    {
        setupOcsAuthorWidget();
        setupOcsCreditWidget();
    }
    else if( sender() == m_showOcsCreditButton )
    {
        setupOcsCreditWidget();
        setupOcsAuthorWidget();
    }
    else
        warning() << "Invalid sender for ExtendedAboutDialog::switchToOcsWidgets()";
}

void
ExtendedAboutDialog::setupOfflineAuthorWidget()
{
    m_ocsAuthorWidget->hide();
    const KAboutData *aboutData = d->aboutData;
    const int authorCount = aboutData->authors().count();

    if (authorCount) {
        QString authorPageText;

        m_authorPageTitle = authorCount == 1 ? i18n("A&uthor") : i18n("A&uthors");

        if (!aboutData->customAuthorTextEnabled() || !aboutData->customAuthorRichText().isEmpty()) {
            if (!aboutData->customAuthorTextEnabled()) {
                if (aboutData->bugAddress().isEmpty() || aboutData->bugAddress() == "submit@bugs.kde.org")
                    authorPageText = i18n("Please use <a href=\"http://bugs.kde.org\">http://bugs.kde.org</a> to report bugs.\n");
                else {
                    if(aboutData->authors().count() == 1 && (aboutData->authors().first().emailAddress() == aboutData->bugAddress())) {
                        authorPageText = i18n("Please report bugs to <a href=\"mailto:%1\">%2</a>.\n",
                                              aboutData->authors().first().emailAddress(),
                                              aboutData->authors().first().emailAddress());
                    }
                    else {
                        authorPageText = i18n("Please report bugs to <a href=\"mailto:%1\">%2</a>.\n",
                                              aboutData->bugAddress(), aboutData->bugAddress());
                    }
                }
            }
            else
                authorPageText = aboutData->customAuthorRichText();
        }

        authorPageText += "<br />";

        const QList<KAboutPerson> lst = aboutData->authors();
        for (int i = 0; i < lst.size(); ++i) {
            authorPageText += QString("<p style=\"margin: 0px;\"><b>%1</b></p>").arg(lst.at(i).name());
            if (!lst.at(i).emailAddress().isEmpty())
                authorPageText += QString("<p style=\"margin: 0px; margin-left: 15px;\"><a href=\"mailto:%1\">%1</a></p>").arg(lst.at(i).emailAddress());
            if (!lst.at(i).webAddress().isEmpty())
                authorPageText += QString("<p style=\"margin: 0px; margin-left: 15px;\"><a href=\"%3\">%3</a></p>").arg(lst.at(i).webAddress());
            if (!lst.at(i).task().isEmpty())
                authorPageText += QString("<p style=\"margin: 0px; margin-left: 15px;\">%4</p>").arg(lst.at(i).task());
            if (i < lst.size() - 1)
                authorPageText += "<p style=\"margin: 0px;\">&nbsp;</p>";
        }

        QVBoxLayout *offlineAuthorWidgetLayout = new QVBoxLayout( m_offlineAuthorWidget );
        offlineAuthorWidgetLayout->setMargin( 0 );
        KTextBrowser *authorTextBrowser = new KTextBrowser( m_offlineAuthorWidget );
        authorTextBrowser->setFrameStyle(QFrame::NoFrame);
        authorTextBrowser->setPalette(m_transparentBackgroundPalette);
        authorTextBrowser->setHtml(authorPageText);

        offlineAuthorWidgetLayout->addWidget( authorTextBrowser );
        m_offlineAuthorWidget->setLayout( offlineAuthorWidgetLayout );
        m_isOfflineAuthorWidget = true;
    }
}

void
ExtendedAboutDialog::setupOcsAuthorWidget()
{
    if( m_isOfflineAuthorWidget )
    {
        m_offlineAuthorWidget->hide();
        m_isOfflineAuthorWidget = false;

        m_authorWidget->layout()->addWidget( m_ocsAuthorWidget );
        connect( m_ocsAuthorWidget, SIGNAL( personAdded( OcsPersonItem::PersonStatus, int ) ),
                 this, SLOT( onPersonAdded( OcsPersonItem::PersonStatus, int ) ) );

        Attica::PersonJob *personJob;
        for( OcsData::OcsPersonList::const_iterator author = m_ocsData.authors()->constBegin();
                author != m_ocsData.authors()->constEnd(); ++author )
        {
            QString userName = (*author).first;
            if( !userName.isEmpty() )
            {
                personJob = Attica::OcsApi::requestPerson( userName );
                connect( personJob, SIGNAL( result( KJob * ) ), this, SLOT( authorJobFinished( KJob * ) ) );
            }
            else
            {
                m_ocsAuthorWidget->addPerson( (*author).second );
            }
        }
        m_ocsAuthorWidget->show();
    }
}

void
ExtendedAboutDialog::setupOfflineCreditWidget()
{
    m_ocsCreditWidget->hide();
    const KAboutData *aboutData = d->aboutData;
    const int creditCount = aboutData->credits().count();

    if (creditCount) {
        QString creditPageText;

        const QList<KAboutPerson> lst = aboutData->credits();
        for (int i = 0; i < lst.size(); ++i) {
            creditPageText += QString("<p style=\"margin: 0px;\">%1</p>").arg(lst.at(i).name());
            if (!lst.at(i).emailAddress().isEmpty())
                creditPageText += QString("<p style=\"margin: 0px; margin-left: 15px;\"><a href=\"mailto:%1\">%1</a></p>").arg(lst.at(i).emailAddress());
            if (!lst.at(i).webAddress().isEmpty())
                creditPageText += QString("<p style=\"margin: 0px; margin-left: 15px;\"><a href=\"%3\">%3</a></p>").arg(lst.at(i).webAddress());
            if (!lst.at(i).task().isEmpty())
                creditPageText += QString("<p style=\"margin: 0px; margin-left: 15px;\">%4</p>").arg(lst.at(i).task());
            if (i < lst.size() - 1)
                creditPageText += "<p style=\"margin: 0px;\">&nbsp;</p>";
        }

        QVBoxLayout *offlineCreditWidgetLayout = new QVBoxLayout( m_offlineCreditWidget );
        offlineCreditWidgetLayout->setMargin( 0 );
        KTextBrowser *creditTextBrowser = new KTextBrowser( m_offlineCreditWidget );
        creditTextBrowser->setFrameStyle(QFrame::NoFrame);
        creditTextBrowser->setPalette(m_transparentBackgroundPalette);
        creditTextBrowser->setHtml(creditPageText);

        offlineCreditWidgetLayout->addWidget( creditTextBrowser );
        m_offlineCreditWidget->setLayout( offlineCreditWidgetLayout );
        m_isOfflineCreditWidget = true;
    }
}

void
ExtendedAboutDialog::setupOcsCreditWidget()
{
    if( m_isOfflineCreditWidget )
    {
        m_offlineCreditWidget->hide();
        m_isOfflineCreditWidget = false;

        m_creditWidget->layout()->addWidget( m_ocsCreditWidget );
        connect( m_ocsCreditWidget, SIGNAL( personAdded( OcsPersonItem::PersonStatus, int ) ),
                 this, SLOT( onPersonAdded( OcsPersonItem::PersonStatus, int ) ) );

        Attica::PersonJob *personJob;
        for( OcsData::OcsPersonList::const_iterator credit = m_ocsData.credits()->constBegin();
                credit != m_ocsData.credits()->constEnd(); ++credit )
        {
            QString userName = (*credit).first;
            if( !userName.isEmpty() )
            {
                if( userName == "%%category%%" )
                {
                    //m_ocsCreditWidget->addPerson( (*credit).second );
                    // ^ this is a category separator string in the old widget
                    //TODO: implement two sections: active and former
                }
                else
                {
                    personJob = Attica::OcsApi::requestPerson( userName );
                    connect( personJob, SIGNAL( result( KJob * ) ), this, SLOT( creditJobFinished( KJob * ) ) );
                }
            }
            else
            {
                m_ocsCreditWidget->addPerson( (*credit).second );
            }
        }
        m_ocsCreditWidget->show();
    }
}

void
ExtendedAboutDialog::authorJobFinished( KJob *job )
{
    Attica::PersonJob *personJob = qobject_cast< Attica::PersonJob * >( job );
    QString userName = personJob->person().id();

    KAboutPerson *person = 0;
    for( OcsData::OcsPersonList::const_iterator it = m_ocsData.authors()->constBegin();
            it != m_ocsData.authors()->constEnd(); ++it )
    {
        if( (*it).first == userName )
        {
            person = new KAboutPerson( (*it).second );
            break;
        }
    }
    if ( person )
    {
        m_ocsAuthorWidget->addPerson( *person, personJob->person() );
    }
}

void
ExtendedAboutDialog::creditJobFinished( KJob *job )
{
    DEBUG_BLOCK
    Attica::PersonJob *personJob = qobject_cast< Attica::PersonJob * >( job );

    debug()<< m_ocsData.credits()->count();
    debug()<< d->aboutData->credits().count();
    QString userName = personJob->person().id();
    KAboutPerson *person = 0;
    for( OcsData::OcsPersonList::const_iterator it = m_ocsData.credits()->constBegin();
            it != m_ocsData.credits()->constEnd(); ++it )
    {
        if( (*it).first == userName )
        {
            //TODO: for some reason this is capped to 12 persons, fix it somehow
            person = new KAboutPerson( (*it).second );
            break;
        }
    }
    if ( person )
    {
        m_ocsCreditWidget->addPerson( *person, personJob->person() );
    }
}

void
ExtendedAboutDialog::onPersonAdded( OcsPersonItem::PersonStatus status, int persons )  //SLOT
{
    DEBUG_BLOCK
    if( status == OcsPersonItem::Author )
    {
        if( persons == d->aboutData->authors().count() )
        {
            //Yay, the OCS authors list has been populated!
            m_showOcsAuthorButton->stop();
            m_showOcsAuthorButton->fold();
        }
    }
    else if( status == OcsPersonItem::Contributor )
    {
        debug()<< "There are" << persons<<"credits loaded";
        debug()<< "     out of" << d->aboutData->credits().count();
        //TODO: implement separator. until then, this must be -1:
        if( persons == d->aboutData->credits().count() -1 )
        {
            //DEBUG:
            debug()<<"Listed contributors";
            debug()<<"Inserted | Name";
            for( OcsData::OcsPersonList::const_iterator it = m_ocsData.credits()->constBegin();
                    it != m_ocsData.credits()->constEnd(); ++it )
            {
                debug() << m_ocsCreditWidget->m_addedNames.contains( (*it).second.name() ) << "    "<< (*it).second.name();
            }
            debug()<< "And for some reason, these have been added at least twice:";
            foreach( QString st, m_ocsCreditWidget->m_twiceAddedNames )
            {
                debug() << st;
            }

            m_showOcsCreditButton->stop();
            m_showOcsCreditButton->fold();
        }
    }
}

#include "ExtendedAboutDialog.moc"
