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

#include <QLabel>
#include <QLayout>
#include <QPushButton>
#include <QScrollBar>
#include <QTabWidget>

#include <kaboutdata.h>
#include <kapplication.h>
#include <kglobal.h>
#include <kglobalsettings.h>
#include <kiconloader.h>
#include <klocale.h>
#include <ktextbrowser.h>
#include <ktitlewidget.h>

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

ExtendedAboutDialog::ExtendedAboutDialog(const KAboutData *aboutData, QWidget *parent)
  : KDialog(parent),
    d(new Private(this))
{
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

    setPlainCaption(i18n("TEST About %1", aboutData->programName()));
    setButtons(KDialog::Close);
    setDefaultButton(KDialog::Close);
    setModal(false);

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

    QTabWidget *tabWidget = new QTabWidget;
    tabWidget->setUsesScrollButtons(false);

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

    QPalette transparentBackgroundPalette;
    transparentBackgroundPalette.setColor(QPalette::Base, Qt::transparent);
    transparentBackgroundPalette.setColor(QPalette::Text, transparentBackgroundPalette.color(QPalette::WindowText));

    const int authorCount = aboutData->authors().count();
    if (authorCount) {
        QString authorPageText;

        QString authorPageTitle = authorCount == 1 ? i18n("A&uthor") : i18n("A&uthors");

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
            authorPageText += QString("<p style=\"margin: 0px;\">%1</p>").arg(lst.at(i).name());
            if (!lst.at(i).emailAddress().isEmpty())
                authorPageText += QString("<p style=\"margin: 0px; margin-left: 15px;\"><a href=\"mailto:%1\">%1</a></p>").arg(lst.at(i).emailAddress());
            if (!lst.at(i).webAddress().isEmpty())
                authorPageText += QString("<p style=\"margin: 0px; margin-left: 15px;\"><a href=\"%3\">%3</a></p>").arg(lst.at(i).webAddress());
            if (!lst.at(i).task().isEmpty())
                authorPageText += QString("<p style=\"margin: 0px; margin-left: 15px;\">%4</p>").arg(lst.at(i).task());
            if (i < lst.size() - 1)
                authorPageText += "<p style=\"margin: 0px;\">&nbsp;</p>";
        }

        KTextBrowser *authorTextBrowser = new KTextBrowser;
        authorTextBrowser->setFrameStyle(QFrame::NoFrame);
        authorTextBrowser->setPalette(transparentBackgroundPalette);
        authorTextBrowser->setHtml(authorPageText);
        tabWidget->addTab(authorTextBrowser, authorPageTitle);
    }

    const int creditsCount = aboutData->credits().count();
    if (creditsCount) {
        QString creditsPageText;

        const QList<KAboutPerson> lst = aboutData->credits();
        for (int i = 0; i < lst.size(); ++i) {
            creditsPageText += QString("<p style=\"margin: 0px;\">%1</p>").arg(lst.at(i).name());
            if (!lst.at(i).emailAddress().isEmpty())
                creditsPageText += QString("<p style=\"margin: 0px; margin-left: 15px;\"><a href=\"mailto:%1\">%1</a></p>").arg(lst.at(i).emailAddress());
            if (!lst.at(i).webAddress().isEmpty())
                creditsPageText += QString("<p style=\"margin: 0px; margin-left: 15px;\"><a href=\"%3\">%3</a></p>").arg(lst.at(i).webAddress());
            if (!lst.at(i).task().isEmpty())
                creditsPageText += QString("<p style=\"margin: 0px; margin-left: 15px;\">%4</p>").arg(lst.at(i).task());
            if (i < lst.size() - 1)
                creditsPageText += "<p style=\"margin: 0px;\">&nbsp;</p>";
        }

        KTextBrowser *creditsTextBrowser = new KTextBrowser;
        creditsTextBrowser->setFrameStyle(QFrame::NoFrame);
        creditsTextBrowser->setPalette(transparentBackgroundPalette);
        creditsTextBrowser->setHtml(creditsPageText);
        tabWidget->addTab(creditsTextBrowser, i18n("&Thanks To"));
    }

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

    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addWidget(titleWidget);
    mainLayout->addWidget(tabWidget);
    mainLayout->setMargin(0);

    QWidget *mainWidget = new QWidget;
    mainWidget->setLayout(mainLayout);

    setMainWidget(mainWidget);
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

#include "ExtendedAboutDialog.moc"
