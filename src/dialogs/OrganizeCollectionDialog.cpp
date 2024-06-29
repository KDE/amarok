/****************************************************************************************
 * Copyright (c) 2008 Bonne Eggleston <b.eggleston@gmail.com>                           *
 * Copyright (c) 2008 Téo Mrnjavac <teo@kde.org>                                        *
 * Copyright (c) 2010 Casey Link <unnamedrambler@gmail.com>                             *
 * Copyright (c) 2012 Ralf Engels <ralf-engels@gmx.de>                                  *
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

#define DEBUG_PREFIX "OrganizeCollectionDialog"

#include "OrganizeCollectionDialog.h"

#include "amarokconfig.h"
#include "core/support/Amarok.h"
#include "core/support/Debug.h"
#include "core-impl/meta/file/File.h"
#include "dialogs/TrackOrganizer.h"
#include "widgets/TokenPool.h"
#include "ui_OrganizeCollectionDialogBase.h"

#include <QApplication>
#include <QDir>
#include <QPushButton>
#include <QScreen>
#include <QTimer>

#include <KColorScheme>
#include <KWindowConfig>

// -------------- OrganizeCollectionOptionWidget ------------
OrganizeCollectionOptionWidget::OrganizeCollectionOptionWidget( QWidget *parent )
    : QGroupBox( parent )
{
    setupUi( this );

    connect( spaceCheck, &QCheckBox::toggled, this, &OrganizeCollectionOptionWidget::optionsChanged );
    connect( ignoreTheCheck, &QCheckBox::toggled, this, &OrganizeCollectionOptionWidget::optionsChanged );
    connect( vfatCheck, &QCheckBox::toggled, this, &OrganizeCollectionOptionWidget::optionsChanged );
    connect( asciiCheck, &QCheckBox::toggled, this, &OrganizeCollectionOptionWidget::optionsChanged );
    connect( regexpEdit, &QLineEdit::editingFinished, this, &OrganizeCollectionOptionWidget::optionsChanged );
    connect( replaceEdit, &QLineEdit::editingFinished, this, &OrganizeCollectionOptionWidget::optionsChanged );
}

// ------------------------- OrganizeCollectionWidget -------------------

OrganizeCollectionWidget::OrganizeCollectionWidget( QWidget *parent )
    : FilenameLayoutWidget( parent )
{
    m_configCategory = QStringLiteral("OrganizeCollectionDialog");

    // TODO: also supported by TrackOrganizer:
    // folder theartist thealbumartist rating filesize length
    m_tokenPool->addToken( createToken( Title ) );
    m_tokenPool->addToken( createToken( Artist ) );
    m_tokenPool->addToken( createToken( AlbumArtist ) );
    m_tokenPool->addToken( createToken( Album ) );
    m_tokenPool->addToken( createToken( Genre ) );
    m_tokenPool->addToken( createToken( Composer ) );
    m_tokenPool->addToken( createToken( Comment ) );
    m_tokenPool->addToken( createToken( Year ) );
    m_tokenPool->addToken( createToken( TrackNumber ) );
    m_tokenPool->addToken( createToken( DiscNumber ) );

    m_tokenPool->addToken( createToken( Folder ) );
    m_tokenPool->addToken( createToken( FileType ) );
    m_tokenPool->addToken( createToken( Initial ) );

    m_tokenPool->addToken( createToken( Slash ) );
    m_tokenPool->addToken( createToken( Underscore ) );
    m_tokenPool->addToken( createToken( Dash ) );
    m_tokenPool->addToken( createToken( Dot ) );
    m_tokenPool->addToken( createToken( Space ) );

    // show some non-editable tags before and after
    // but only if screen size is large enough (BR: 283361)
    const QRect screenRect = QApplication::primaryScreen()->geometry();
    if( screenRect.width() >= 1024 )
    {
        m_schemaLineLayout->insertWidget( 0,
                                          createStaticToken( CollectionRoot ), 0 );
        m_schemaLineLayout->insertWidget( 1,
                                          createStaticToken( Slash ), 0 );

        m_schemaLineLayout->insertWidget( m_schemaLineLayout->count(),
                                          createStaticToken( Dot ) );
        m_schemaLineLayout->insertWidget( m_schemaLineLayout->count(),
                                          createStaticToken( FileType ) );
    }

    m_syntaxLabel->setText( buildFormatTip() );

    populateConfiguration();
}


QString
OrganizeCollectionWidget::buildFormatTip() const
{
    QMap<QString, QString> args;
    args[QStringLiteral("albumartist")] = i18n( "%1 or %2", QLatin1String("Album Artist, The") , QLatin1String("The Album Artist") );
    args[QStringLiteral("thealbumartist")] = i18n( "The Album Artist" );
    args[QStringLiteral("theartist")] = i18n( "The Artist" );
    args[QStringLiteral("artist")] = i18n( "%1 or %2", QLatin1String("Artist, The") , QLatin1String("The Artist") );
    args[QStringLiteral("initial")] = i18n( "Artist's Initial" );
    args[QStringLiteral("filetype")] = i18n( "File Extension of Source" );
    args[QStringLiteral("track")] = i18n( "Track Number" );

    QString tooltip = i18n( "You can use the following tokens:" );
    tooltip += QStringLiteral("<ul>");

    for( QMap<QString, QString>::iterator it = args.begin(), total = args.end(); it != total; ++it )
        tooltip += QStringLiteral( "<li>%1 - %%2%" ).arg( it.value(), it.key() );

    tooltip += QStringLiteral("</ul>");
    tooltip += i18n( "If you surround sections of text that contain a token with curly-braces, "
            "that section will be hidden if the token is empty." );

    return tooltip;
}


OrganizeCollectionDialog::OrganizeCollectionDialog( const Meta::TrackList &tracks,
                                                    const QStringList &folders,
                                                    const QString &targetExtension,
                                                    QWidget *parent,
                                                    const char *name,
                                                    bool modal,
                                                    const QString &caption,
                                                    QFlags<QDialogButtonBox::StandardButton> buttonMask )
    : QDialog( parent )
    , ui( new Ui::OrganizeCollectionDialogBase )
    , m_conflict( false )
{
    Q_UNUSED( name )

    setWindowTitle( caption );
    setModal( modal );
    m_targetFileExtension = targetExtension;

    if( tracks.size() > 0 )
        m_allTracks = tracks;

    QWidget *mainContainer = new QWidget( this );
    QDialogButtonBox* buttonBox = new QDialogButtonBox( buttonMask, this );
    connect(buttonBox, &QDialogButtonBox::accepted, this, &OrganizeCollectionDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &OrganizeCollectionDialog::reject);

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget( mainContainer );
    mainLayout->addWidget( buttonBox );

    ui->setupUi( mainContainer );

    m_trackOrganizer = new TrackOrganizer( m_allTracks, this );

    ui->folderCombo->insertItems( 0, folders );
    if( ui->folderCombo->contains( AmarokConfig::organizeDirectory() ) )
        ui->folderCombo->setCurrentItem( AmarokConfig::organizeDirectory() );
    else
        ui->folderCombo->setCurrentIndex( 0 ); //TODO possible bug: assumes folder list is not empty.

    ui->overwriteCheck->setChecked( AmarokConfig::overwriteFiles() );

    ui->optionsWidget->setReplaceSpaces( AmarokConfig::replaceSpace() );
    ui->optionsWidget->setPostfixThe( AmarokConfig::ignoreThe() );
    ui->optionsWidget->setVfatCompatible( AmarokConfig::vfatCompatible() );
    ui->optionsWidget->setAsciiOnly( AmarokConfig::asciiOnly() );
    ui->optionsWidget->setRegexpText( AmarokConfig::replacementRegexp() );
    ui->optionsWidget->setReplaceText( AmarokConfig::replacementString() );

    ui->previewTableWidget->horizontalHeader()->setSectionResizeMode( QHeaderView::ResizeToContents );
    ui->conflictLabel->setText(QStringLiteral(""));
    QPalette p = ui->conflictLabel->palette();
    KColorScheme::adjustForeground( p, KColorScheme::NegativeText ); // TODO this isn't working, the color is still normal
    ui->conflictLabel->setPalette( p );
    ui->previewTableWidget->sortItems( 0, Qt::AscendingOrder );

    // only show the options when the Options button is checked
    connect( ui->optionsButton, &QAbstractButton::toggled, ui->organizeCollectionWidget, &OrganizeCollectionWidget::setVisible );
    connect( ui->optionsButton, &QAbstractButton::toggled, ui->optionsWidget, &OrganizeCollectionOptionWidget::setVisible );
    ui->organizeCollectionWidget->hide();
    ui->optionsWidget->hide();

    connect( ui->folderCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
             this, &OrganizeCollectionDialog::slotUpdatePreview );
    connect( ui->organizeCollectionWidget, &OrganizeCollectionWidget::schemeChanged, this, &OrganizeCollectionDialog::slotUpdatePreview );
    connect( ui->optionsWidget, &OrganizeCollectionOptionWidget::optionsChanged, this, &OrganizeCollectionDialog::slotUpdatePreview);
    // to show the conflict error
    connect( ui->overwriteCheck, &QCheckBox::stateChanged, this, &OrganizeCollectionDialog::slotOverwriteModeChanged );

    connect( this, &OrganizeCollectionDialog::accepted, ui->organizeCollectionWidget, &OrganizeCollectionWidget::onAccept );
    connect( this, &OrganizeCollectionDialog::accepted, this, &OrganizeCollectionDialog::slotDialogAccepted );
    connect( ui->folderCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
             this, &OrganizeCollectionDialog::slotEnableOk );

    slotEnableOk( ui->folderCombo->currentIndex() );
    KWindowConfig::restoreWindowSize( windowHandle(), Amarok::config( QStringLiteral("OrganizeCollectionDialog") ) );

    QTimer::singleShot( 0, this, &OrganizeCollectionDialog::slotUpdatePreview );
}

OrganizeCollectionDialog::~OrganizeCollectionDialog()
{
    KConfigGroup group = Amarok::config( QStringLiteral("OrganizeCollectionDialog") );
    group.writeEntry( "geometry", saveGeometry() );

    AmarokConfig::setOrganizeDirectory( ui->folderCombo->currentText() );
    delete ui;
}

QMap<Meta::TrackPtr, QString>
OrganizeCollectionDialog::getDestinations() const
{
    return m_trackOrganizer->getDestinations();
}

bool
OrganizeCollectionDialog::overwriteDestinations() const
{
    return ui->overwriteCheck->isChecked();
}


QString
OrganizeCollectionDialog::buildFormatString() const
{
    if( ui->organizeCollectionWidget->getParsableScheme().simplified().isEmpty() )
        return QStringLiteral("");
    return QStringLiteral("%collectionroot%/") + ui->organizeCollectionWidget->getParsableScheme() + QStringLiteral(".%filetype%");
}

void
OrganizeCollectionDialog::slotUpdatePreview()
{
    QString formatString = buildFormatString();

    m_trackOrganizer->setAsciiOnly( ui->optionsWidget->asciiOnly() );
    m_trackOrganizer->setFolderPrefix( ui->folderCombo->currentText() );
    m_trackOrganizer->setFormatString( formatString );
    m_trackOrganizer->setTargetFileExtension( m_targetFileExtension );
    m_trackOrganizer->setPostfixThe( ui->optionsWidget->postfixThe() );
    m_trackOrganizer->setReplaceSpaces( ui->optionsWidget->replaceSpaces() );
    m_trackOrganizer->setReplace( ui->optionsWidget->regexpText(),
                                  ui->optionsWidget->replaceText() );
    m_trackOrganizer->setVfatSafe( ui->optionsWidget->vfatCompatible() );

    // empty the table, not only its contents
    ui->previewTableWidget->clearContents();
    ui->previewTableWidget->setRowCount( 0 );
    ui->previewTableWidget->setSortingEnabled( false ); // interferes with inserting
    m_trackOrganizer->resetTrackOffset();
    m_conflict = false;
    setCursor( Qt::BusyCursor );

    // be nice do the UI, try not to block for too long
    QTimer::singleShot( 0, this, &OrganizeCollectionDialog::processPreviewPaths );
}

void
OrganizeCollectionDialog::processPreviewPaths()
{
    QStringList originals;
    QStringList previews;
    QStringList commonOriginalPrefix; // common initial directories
    QStringList commonPreviewPrefix; // common initial directories

    QMap<Meta::TrackPtr, QString> destinations = m_trackOrganizer->getDestinations();
    for( auto it = destinations.constBegin(); it != destinations.constEnd(); ++it )
    {
        originals << it.key()->prettyUrl();
        previews << it.value();

        QStringList originalPrefix = originals.last().split( QLatin1Char('/') );
        originalPrefix.removeLast(); // we never include file name in the common prefix
        QStringList previewPrefix = previews.last().split( QLatin1Char('/') );
        previewPrefix.removeLast();

        if( it == destinations.constBegin() )
        {
            commonOriginalPrefix = originalPrefix;
            commonPreviewPrefix = previewPrefix;
        } else {
            int commonLength = 0;
            while( commonOriginalPrefix.size() > commonLength &&
                   originalPrefix.size() > commonLength &&
                   commonOriginalPrefix[ commonLength ] == originalPrefix[ commonLength ] )
            {
                commonLength++;
            }
            commonOriginalPrefix = commonOriginalPrefix.mid( 0, commonLength );

            commonLength = 0;
            while( commonPreviewPrefix.size() > commonLength &&
                   previewPrefix.size() > commonLength &&
                   commonPreviewPrefix[ commonLength ] == previewPrefix[ commonLength ] )
            {
                commonLength++;
            }
            commonPreviewPrefix = commonPreviewPrefix.mid( 0, commonLength );
        }
    }

    QString originalPrefix = commonOriginalPrefix.isEmpty() ? QString() : commonOriginalPrefix.join( QLatin1Char('/') ) + QLatin1Char('/');
    m_previewPrefix = commonPreviewPrefix.isEmpty() ? QString() : commonPreviewPrefix.join( QLatin1Char('/') ) + QLatin1Char('/');
    ui->previewTableWidget->horizontalHeaderItem( 1 )->setText( i18n( "Original: %1", originalPrefix ) );
    ui->previewTableWidget->horizontalHeaderItem( 0 )->setText( i18n( "Preview: %1", m_previewPrefix ) );

    m_originals.clear();
    m_originals.reserve( originals.size() );
    m_previews.clear();
    m_previews.reserve( previews.size() );
    for( int i = 0; i < qMin( originals.size(), previews.size() ); i++ )
    {
        m_originals << originals.at( i ).mid( originalPrefix.length() );
        m_previews << previews.at( i ).mid( m_previewPrefix.length() );
    }

    QTimer::singleShot( 0, this, &OrganizeCollectionDialog::previewNextBatch );
}

void
OrganizeCollectionDialog::previewNextBatch()
{
    const int batchSize = 100;

    QPalette negativePalette = ui->previewTableWidget->palette();
    KColorScheme::adjustBackground( negativePalette, KColorScheme::NegativeBackground );

    int processed = 0;
    while( !m_originals.isEmpty() && !m_previews.isEmpty() )
    {
        QString originalPath = m_originals.takeFirst();
        QString newPath = m_previews.takeFirst();

        int newRow = ui->previewTableWidget->rowCount();
        ui->previewTableWidget->insertRow( newRow );

        // new path preview in the 1st column
        QTableWidgetItem *item = new QTableWidgetItem( newPath );
        if( QFileInfo( m_previewPrefix + newPath ).exists() )
        {
            item->setBackground( negativePalette.base() );
            m_conflict = true;
        }
        ui->previewTableWidget->setItem( newRow, 0, item );

        //original in the second column
        item = new QTableWidgetItem( originalPath );
        ui->previewTableWidget->setItem( newRow, 1, item );

        processed++;
        if( processed >= batchSize )
        {
            // yield some room to the other events in the main loop
            QTimer::singleShot( 0, this, &OrganizeCollectionDialog::previewNextBatch );
            return;
        }
    }

    // finished
    unsetCursor();
    ui->previewTableWidget->setSortingEnabled( true );
    slotOverwriteModeChanged(); // in fact, m_conflict may have changed
}

void
OrganizeCollectionDialog::slotOverwriteModeChanged()
{
    if( m_conflict )
    {
        if( ui->overwriteCheck->isChecked() )
            ui->conflictLabel->setText( i18n( "There is a filename conflict, existing files will be overwritten." ) );
        else
            ui->conflictLabel->setText( i18n( "There is a filename conflict, existing files will not be changed." ) );
    }
    else
        ui->conflictLabel->setText(QStringLiteral("")); // we clear the text instead of hiding it to retain the layout spacing
}

void
OrganizeCollectionDialog::slotDialogAccepted()
{
    AmarokConfig::setOrganizeDirectory( ui->folderCombo->currentText() );

    AmarokConfig::setIgnoreThe( ui->optionsWidget->postfixThe() );
    AmarokConfig::setReplaceSpace( ui->optionsWidget->replaceSpaces() );
    AmarokConfig::setVfatCompatible( ui->optionsWidget->vfatCompatible() );
    AmarokConfig::setAsciiOnly( ui->optionsWidget->asciiOnly() );
    AmarokConfig::setReplacementRegexp( ui->optionsWidget->regexpText() );
    AmarokConfig::setReplacementString( ui->optionsWidget->replaceText() );
}

//The Ok button should be disabled when there's no collection root selected, and when there is no .%filetype in format string
void
OrganizeCollectionDialog::slotEnableOk( int currentCollectionRootIndex )
{
    QString currentCollectionRoot = ui->folderCombo->itemText( currentCollectionRootIndex );
    auto okButton = findChild<QDialogButtonBox*>()->button( QDialogButtonBox::Ok );
    if( okButton )
        okButton->setEnabled( !currentCollectionRoot.isEmpty() );
}
