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

#include <QApplication>
#include <QDialogButtonBox>
#include <QFontDatabase>
#include <QLabel>
#include <QPushButton>
#include <QScrollBar>
#include <QStandardPaths>
#include <QStyle>
#include <QTabWidget>
#include <QTextBrowser>
#include <QVBoxLayout>

#include <KCoreAddons>
#include <KTitleWidget>

void ExtendedAboutDialog::Private::_k_showLicense( const QString &number )
{
    QDialog *dialog = new QDialog(q);
    QWidget *mainWidget = new QWidget;

    dialog->setWindowTitle(i18n("License Agreement"));
    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Close, q);
    dialog->connect(buttonBox, &QDialogButtonBox::accepted, dialog, &QDialog::accept);
    dialog->connect(buttonBox, &QDialogButtonBox::rejected, dialog, &QDialog::reject);
    buttonBox->button(QDialogButtonBox::Close)->setDefault(true);

    const QFont font = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    QFontMetrics metrics(font);

    const QString licenseText = aboutData->licenses().at(number.toInt()).text();
    QTextBrowser *licenseBrowser = new QTextBrowser;
    licenseBrowser->setFont(font);
    licenseBrowser->setLineWrapMode(QTextEdit::NoWrap);
    licenseBrowser->setText(licenseText);

    QVBoxLayout *mainLayout = new QVBoxLayout;
    dialog->setLayout(mainLayout);
    mainLayout->addWidget(licenseBrowser);
    mainLayout->addWidget(mainWidget);
    mainLayout->addWidget(buttonBox);

    // try to set up the dialog such that the full width of the
    // document is visible without horizontal scroll-bars being required
    const qreal idealWidth = licenseBrowser->document()->idealWidth()
        + QApplication::style()->pixelMetric(QStyle::PM_LayoutLeftMargin)
        + QApplication::style()->pixelMetric(QStyle::PM_LayoutRightMargin)
        + licenseBrowser->verticalScrollBar()->width() * 2;

    // try to allow enough height for a reasonable number of lines to be shown
    const int idealHeight = metrics.height() * 30;

    dialog->resize(dialog->sizeHint().expandedTo(QSize((int)idealWidth,idealHeight)));
    dialog->show();
}

ExtendedAboutDialog::ExtendedAboutDialog(const KAboutData &about, const OcsData *ocsData, QWidget *parent)
  : QDialog(parent)
  , d(new Private(this))
{
    DEBUG_BLOCK
    const KAboutData *aboutData = &about;

    d->aboutData = aboutData;

    if (!aboutData) {
        QLabel *errorLabel = new QLabel(i18n("<qt>No information available.<br />"
                                             "The supplied KAboutData object does not exist.</qt>"), this);

        errorLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
        QVBoxLayout *mainLayout = new QVBoxLayout;
        setLayout(mainLayout);
        mainLayout->addWidget(errorLabel);
        return;
    }
    if( !ocsData )
    {
        QLabel *errorLabel = new QLabel(i18n("<qt>No information available.<br />"
                                             "The supplied OcsData object does not exist.</qt>"), this);

        errorLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
        QVBoxLayout *mainLayout = new QVBoxLayout;
        setLayout(mainLayout);
        mainLayout->addWidget(errorLabel);
        return;
    }
    m_ocsData = *ocsData;

    setWindowTitle(i18n("About %1", aboutData->displayName()));

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Close);
    QWidget *mainWidget = new QWidget(this);
    QVBoxLayout *mainLayout = new QVBoxLayout;
    setLayout(mainLayout);
    mainLayout->addWidget(mainWidget);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &ExtendedAboutDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &ExtendedAboutDialog::reject);
    buttonBox->button(QDialogButtonBox::Close)->setDefault(true);

    setModal(false);


    //Set up the title widget...
    KTitleWidget *titleWidget = new KTitleWidget(this);

    QIcon windowIcon = qApp->windowIcon();
    titleWidget->setIcon(windowIcon, KTitleWidget::ImageLeft);
    if (aboutData->programLogo().canConvert<QIcon>())
        titleWidget->setIcon(aboutData->programLogo().value<QIcon>(), KTitleWidget::ImageLeft);
    else if (aboutData->programLogo().canConvert<QPixmap>())
        titleWidget->setIcon(QIcon(aboutData->programLogo().value<QPixmap>()), KTitleWidget::ImageLeft);
    else if (aboutData->programLogo().canConvert<QImage>())
        titleWidget->setIcon(QIcon(QPixmap::fromImage(aboutData->programLogo().value<QImage>())), KTitleWidget::ImageLeft);

    titleWidget->setText(i18n("<html><font size=\"5\">%1</font><br /><b>Version %2</b><br />Using KDE Frameworks %3</html>",
                         aboutData->displayName(), aboutData->version(), KCoreAddons::versionString()));

    //Now let's add the tab bar...
    QTabWidget *tabWidget = new QTabWidget;
    tabWidget->setUsesScrollButtons(false);


    //Set up the first page...
    QString aboutPageText = aboutData->shortDescription() + QLatin1Char('\n');

    if (!aboutData->otherText().isEmpty())
        aboutPageText += QLatin1Char('\n') + aboutData->otherText() + QLatin1Char('\n');

    if (!aboutData->copyrightStatement().isEmpty())
        aboutPageText += QLatin1Char('\n') + aboutData->copyrightStatement() + QLatin1Char('\n');

    if (!aboutData->homepage().isEmpty())
        aboutPageText += QLatin1Char('\n') + QStringLiteral("<a href=\"%1\">%1</a>").arg(aboutData->homepage()) + QLatin1Char('\n');
    aboutPageText = aboutPageText.trimmed();

    QLabel *aboutLabel = new QLabel;
    aboutLabel->setWordWrap(true);
    aboutLabel->setOpenExternalLinks(true);
    aboutLabel->setText(aboutPageText.replace(QLatin1Char('\n'), QStringLiteral("<br />")));
    aboutLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);

    QVBoxLayout *aboutLayout = new QVBoxLayout;
    aboutLayout->addStretch();
    aboutLayout->addWidget(aboutLabel);

    const int licenseCount = aboutData->licenses().count();
    debug()<< "About to show license stuff";
    debug()<< "License count is"<<licenseCount;
    for (int i = 0; i < licenseCount; ++i) {
        const QString licenseName = aboutData->licenses().at(i).name(KAboutLicense::FullName);

        QLabel *showLicenseLabel = new QLabel;
        showLicenseLabel->setText(QStringLiteral("<a href=\"%1\">%2</a>").arg(QString::number(i),
                                                                       i18n("License: %1",
                                                                            licenseName)));
        showLicenseLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
        connect(showLicenseLabel, &QLabel::linkActivated, this, &ExtendedAboutDialog::showLicense);

        aboutLayout->addWidget(showLicenseLabel);
    }
    debug()<<"License widget added";

    aboutLayout->addStretch();

    QWidget *aboutWidget = new QWidget(this);
    aboutWidget->setLayout(aboutLayout);

    tabWidget->addTab(aboutWidget, i18n("&About"));

    //And now, the Authors page:
    const int authorCount = d->aboutData->authors().count();

    if (authorCount)
    {
        m_authorWidget = new QWidget( this );
        QVBoxLayout *authorLayout = new QVBoxLayout( m_authorWidget.data() );

        if (!aboutData->customAuthorTextEnabled() || !aboutData->customAuthorRichText().isEmpty())
        {
            QLabel *bugsLabel = new QLabel( m_authorWidget.data() );
            bugsLabel->setContentsMargins( 4, 2, 0, 4 );
            if (!aboutData->customAuthorTextEnabled())
            {
                if (aboutData->bugAddress().isEmpty() || aboutData->bugAddress() == QStringLiteral("submit@bugs.kde.org"))
                    bugsLabel->setText( i18n("Please use <a href=\"https://bugs.kde.org\">https://bugs.kde.org</a> to report bugs.\n") );
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

        m_authorListWidget = new OcsPersonListWidget( d->aboutData->authors(), m_ocsData.authors(), OcsPersonItem::Author, m_authorWidget.data() );

        authorLayout->addWidget( m_authorListWidget.data() );
        authorLayout->setContentsMargins( 0, 0, 0, 0 );
        authorLayout->setSpacing( 2 );
        m_authorWidget->setLayout( authorLayout );

        m_authorPageTitle = ( authorCount == 1 ) ? i18n("A&uthor") : i18n("A&uthors");
        tabWidget->addTab(m_authorWidget.data(), m_authorPageTitle);
    }

    //Then the Credits page:
    const int creditCount = aboutData->credits().count();

    if (creditCount)
    {
        m_creditWidget = new QWidget( this );
        QVBoxLayout *creditLayout = new QVBoxLayout( m_creditWidget.data() );

        m_creditListWidget = new OcsPersonListWidget( d->aboutData->credits(), m_ocsData.credits(), OcsPersonItem::Contributor, m_creditWidget.data() );

        creditLayout->addWidget( m_creditListWidget.data() );
        creditLayout->setContentsMargins( 0, 0, 0, 0 );
        creditLayout->setSpacing( 2 );
        m_creditWidget->setLayout( creditLayout );

        tabWidget->addTab( m_creditWidget.data(), i18n("&Contributors"));
    }

    //Finally, the Donors page:
    const int donorCount = ocsData->donors()->count();

    if (donorCount)
    {
        m_donorWidget = new QWidget( this );
        QVBoxLayout *donorLayout = new QVBoxLayout( m_donorWidget.data() );

        QList< KAboutPerson > donors;
        for( QList< QPair< QString, KAboutPerson > >::const_iterator it = m_ocsData.donors()->constBegin();
             it != m_ocsData.donors()->constEnd(); ++it )
        {
            donors << ( *it ).second;
        }
        m_donorListWidget = new OcsPersonListWidget( donors , m_ocsData.donors(), OcsPersonItem::Contributor, m_donorWidget.data() );

        donorLayout->addWidget( m_donorListWidget.data() );
        donorLayout->setContentsMargins( 0, 0, 0, 0 );
        donorLayout->setSpacing( 2 );
        m_donorWidget->setLayout( donorLayout );

        tabWidget->addTab( m_donorWidget.data(), i18n("&Donors"));
    }


    //And the translators:
    QPalette transparentBackgroundPalette;
    transparentBackgroundPalette.setColor( QPalette::Base, Qt::transparent );
    transparentBackgroundPalette.setColor( QPalette::Text, transparentBackgroundPalette.color( QPalette::WindowText ) );


    const QList<KAboutPerson> translatorList = aboutData->translators();

    if(!translatorList.isEmpty()) {
        QString translatorPageText;

        QList<KAboutPerson>::ConstIterator it;
        for(it = translatorList.begin(); it != translatorList.end(); ++it) {
            translatorPageText += QStringLiteral("<p style=\"margin: 0px;\">%1</p>").arg((*it).name());
            if (!(*it).emailAddress().isEmpty())
                translatorPageText += QStringLiteral("<p style=\"margin: 0px; margin-left: 15px;\"><a href=\"mailto:%1\">%1</a></p>").arg((*it).emailAddress());
            translatorPageText += QStringLiteral("<p style=\"margin: 0px;\">&nbsp;</p>");
        }

        translatorPageText += KAboutData::aboutTranslationTeam();

        QTextBrowser *translatorTextBrowser = new QTextBrowser;
        translatorTextBrowser->setFrameStyle(QFrame::NoFrame);
        translatorTextBrowser->setPalette(transparentBackgroundPalette);
        translatorTextBrowser->setHtml(translatorPageText);
        tabWidget->addTab(translatorTextBrowser, i18n("T&ranslation"));
    }

    //Jam everything together in a layout:
    mainLayout->addWidget(titleWidget);
    mainLayout->addWidget(tabWidget);
    mainLayout->setContentsMargins( 0, 0, 0, 0 );

    mainLayout->addWidget(buttonBox);

    resize( QSize( 480, 460 ) );
}

ExtendedAboutDialog::~ExtendedAboutDialog()
{
    delete d;
}

void
ExtendedAboutDialog::showLicense(const QString& number)
{
    d->_k_showLicense(number);
}

