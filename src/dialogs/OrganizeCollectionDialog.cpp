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

#ifndef AMAROK_ORGANIZECOLLECTIONDIALOG_UI_H
#define AMAROK_ORGANIZECOLLECTIONDIALOG_UI_H

#include "OrganizeCollectionDialog.h"
#include "../widgets/TokenPool.h"

#include "amarokconfig.h"

#include "core/support/Amarok.h"
#include "core/support/Debug.h"
#include "core-impl/meta/file/File.h"

#include "ui_OrganizeCollectionDialogBase.h"
#include "TrackOrganizer.h"

#include <kcolorscheme.h>
#include <KInputDialog>

#include <QApplication>
#include <QDesktopWidget>
#include <QDir>
#include <QTimer>

// -------------- OrganizeCollectionOptionWidget ------------
OrganizeCollectionOptionWidget::OrganizeCollectionOptionWidget( QWidget *parent )
    : QGroupBox( parent )
{
    setupUi( this );

    connect( spaceCheck, SIGNAL(toggled(bool)), SIGNAL(optionsChanged()) );
    connect( ignoreTheCheck, SIGNAL(toggled(bool)), SIGNAL(optionsChanged()) );
    connect( vfatCheck, SIGNAL(toggled(bool)), SIGNAL(optionsChanged()) );
    connect( asciiCheck, SIGNAL(toggled(bool)), SIGNAL(optionsChanged()) );
    connect( regexpEdit, SIGNAL(editingFinished()), SIGNAL(optionsChanged()) );
    connect( replaceEdit, SIGNAL(editingFinished()), SIGNAL(optionsChanged()) );
}

// ------------------------- OrganizeCollectionWidget -------------------

OrganizeCollectionWidget::OrganizeCollectionWidget( QWidget *parent )
    : FilenameLayoutWidget( parent )
{
    m_configCategory = "OrganizeCollectionDialog";

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
    const QRect screenRect = QApplication::desktop()->screenGeometry();
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
    args["albumartist"] = i18n( "%1 or %2", QLatin1String("Album Artist, The") , QLatin1String("The Album Artist") );
    args["thealbumartist"] = "The Album Artist";
    args["theartist"] = "The Artist";
    args["artist"] = i18n( "%1 or %2", QLatin1String("Artist, The") , QLatin1String("The Artist") );
    args["initial"] = i18n( "Artist's Initial" );
    args["filetype"] = i18n( "File Extension of Source" );
    args["track"] = i18n( "Track Number" );

    QString tooltip = i18n( "You can use the following tokens:" );
    tooltip += "<ul>";

    for( QMap<QString, QString>::iterator it = args.begin(); it != args.end(); ++it )
        tooltip += QString( "<li>%1 - %%2%" ).arg( it.value(), it.key() );

    tooltip += "</ul>";
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
                                                    QFlags<KDialog::ButtonCode> buttonMask )
    : KDialog( parent )
    , ui( new Ui::OrganizeCollectionDialogBase )
    , m_detailed( true )
    , m_schemeModified( false )
    , m_conflict( false )
{
    Q_UNUSED( name )

    setCaption( caption );
    setModal( modal );
    setButtons( buttonMask );
    showButtonSeparator( true );
    m_targetFileExtension = targetExtension;

    if( tracks.size() > 0 )
        m_allTracks = tracks;

    KVBox *mainVBox = new KVBox( this );
    setMainWidget( mainVBox );
    QWidget *mainContainer = new QWidget( mainVBox );

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

    // save some space if the screensize is too small (BR: 283361)
    const QRect screenRect = QApplication::desktop()->screenGeometry();
    if( screenRect.height() < 800 )
        ui->previewTableWidget->hide();

    ui->previewTableWidget->horizontalHeader()->setResizeMode( QHeaderView::ResizeToContents );
    ui->conflictLabel->setText("");
    QPalette p = ui->conflictLabel->palette();
    KColorScheme::adjustForeground( p, KColorScheme::NegativeText ); // TODO this isn't working, the color is still normal
    ui->conflictLabel->setPalette( p );

    // to show the conflict error
    connect( ui->overwriteCheck, SIGNAL(stateChanged(int)),
             SLOT(slotUpdatePreview()) );
    connect( ui->folderCombo, SIGNAL(currentIndexChanged(QString)),
             SLOT(slotUpdatePreview()) );
    connect( ui->organizeCollectionWidget, SIGNAL(schemeChanged()),
             SLOT(slotUpdatePreview()) );
    connect( ui->optionsWidget, SIGNAL(optionsChanged()),
             SLOT(slotUpdatePreview()));

    connect( this, SIGNAL(finished(int)), ui->organizeCollectionWidget, SLOT(slotSaveFormatList()) );
    connect( this, SIGNAL(accepted()), ui->organizeCollectionWidget, SLOT(onAccept()) );
    connect( this, SIGNAL(accepted()), SLOT(slotDialogAccepted()) );
    connect( ui->folderCombo, SIGNAL(currentIndexChanged(QString)),
             SLOT(slotEnableOk(QString)) );

    slotEnableOk( ui->folderCombo->currentText() );

    init();
}

OrganizeCollectionDialog::~OrganizeCollectionDialog()
{
    AmarokConfig::setOrganizeDirectory( ui->folderCombo->currentText() );
    delete ui;
}

QMap<Meta::TrackPtr, QString>
OrganizeCollectionDialog::getDestinations()
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
        return "";
    return "%collectionroot%/" + ui->organizeCollectionWidget->getParsableScheme() + ".%filetype%";
}

QString
OrganizeCollectionDialog::commonPrefix( const QStringList &list ) const
{
    QString option = list.first().toLower();
    int length = option.length();
    while( length > 0 )
    {
        bool found = true;
        foreach( const QString &string, list )
        {
            if( string.left(length).toLower() != option )
            {
                found = false;
                break;
            }
        }
        if( found )
            break;
        --length;
        option = option.left( length );
    }
    return option;

}

void
OrganizeCollectionDialog::update( int dummy )   //why the dummy?
{
    Q_UNUSED( dummy );
}


void
OrganizeCollectionDialog::update( const QString & dummy )
{
    Q_UNUSED( dummy );

    update( 0 );
}

void
OrganizeCollectionDialog::init()
{
    slotUpdatePreview();
}

void
OrganizeCollectionDialog::slotUpdatePreview()
{
    DEBUG_BLOCK;

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

    //empty the table, not only it's contents
    ui->previewTableWidget->setRowCount( 0 );
    m_trackOrganizer->resetTrackOffset();
    m_conflict = false;

    setCursor( Qt::BusyCursor );

    previewNextBatch();
}

void
OrganizeCollectionDialog::previewNextBatch() //private slot
{
    const int batchSize = 10;

    QMap<Meta::TrackPtr, QString> dests = m_trackOrganizer->getDestinations( batchSize );
    QMapIterator<Meta::TrackPtr, QString> it( dests );
    while( it.hasNext() )
    {
        it.next();
        Meta::TrackPtr track = it.key();

        QString originalPath = track->prettyUrl();
        QString newPath = it.value();

        int newRow = ui->previewTableWidget->rowCount();
        ui->previewTableWidget->insertRow( newRow );

        //new path preview in the 1st column
        QPalette p = ui->previewTableWidget->palette();
        QTableWidgetItem *item = new QTableWidgetItem( newPath );
        KColorScheme::adjustBackground( p, KColorScheme::NegativeBackground );
        if( QFileInfo( newPath ).exists() )
        {
            item->setBackgroundColor( p.color( QPalette::Base ) );
            m_conflict = true;
        }
        ui->previewTableWidget->setItem( newRow, 0, item );

        //original in the second column
        item = new QTableWidgetItem( originalPath );
        ui->previewTableWidget->setItem( newRow, 1, item );
    }

    if( m_conflict )
    {
        if( ui->overwriteCheck->isChecked() )
            ui->conflictLabel->setText( i18n( "There is a filename conflict, existing files will be overwritten." ) );
        else
            ui->conflictLabel->setText( i18n( "There is a filename conflict, existing files will not be changed." ) );
    }
    else
        ui->conflictLabel->setText(""); // we clear the text instead of hiding it to retain the layout spacing

    //non-blocking way of updating the preview table.
    if( dests.count() == batchSize )
        QTimer::singleShot( 10, this, SLOT(previewNextBatch()) );
    else
        unsetCursor(); // finished
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

    ui->organizeCollectionWidget->onAccept();
}

//The Ok button should be disabled when there's no collection root selected, and when there is no .%filetype in format string
void
OrganizeCollectionDialog::slotEnableOk( const QString & currentCollectionRoot )
{
    if( currentCollectionRoot == 0 )
        enableButtonOk( false );
    else
        enableButtonOk( true );
}

#endif  //AMAROK_ORGANIZECOLLECTIONDIALOG_UI_H
