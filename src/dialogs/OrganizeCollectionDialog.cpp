/****************************************************************************************
 * Copyright (c) 2008 Bonne Eggleston <b.eggleston@gmail.com>                           *
 * Copyright (c) 2008 Téo Mrnjavac <teo@kde.org>                                        *
 * Copyright (c) 2010 Casey Link <unnamedrambler@gmail.com>                             *
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

#include "core/support/Amarok.h"
#include "core/support/Debug.h"
#include "amarokconfig.h"
#include "core-impl/meta/file/File.h"
#include "QStringx.h"
#include "ui_OrganizeCollectionDialogBase.h"
#include "TrackOrganizer.h"
#include <kcolorscheme.h>
#include <KInputDialog>

#include <QApplication>
#include <QDir>
#include <QTimer>

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
    , m_formatListModified( false )
{
    Q_UNUSED( name )

    setCaption( caption );
    setModal( modal );
    setButtons( buttonMask );
    showButtonSeparator( true );
    m_targetFileExtension = targetExtension;

    if ( tracks.size() > 0 )
    {
        m_allTracks = tracks;
    }

    KVBox *mainVBox = new KVBox( this );
    setMainWidget( mainVBox );
    QWidget *mainContainer = new QWidget( mainVBox );

    ui->setupUi( mainContainer );

    m_trackOrganizer = new TrackOrganizer( m_allTracks, this );
    m_filenameLayoutDialog = new FilenameLayoutDialog( mainContainer, 1 );   //", 1" means isOrganizeCollection ==> doesn't show Options frame
//    m_filenameLayoutDialog->hide();
    connect( this, SIGNAL( accepted() ),
             m_filenameLayoutDialog, SLOT( onAccept() ) );
    ui->verticalLayout->insertWidget( 1, m_filenameLayoutDialog );
    ui->ignoreTheCheck->show();

    ui->folderCombo->insertItems( 0, folders );
    if( ui->folderCombo->contains( AmarokConfig::organizeDirectory() ) )
        ui->folderCombo->setCurrentItem( AmarokConfig::organizeDirectory() );
    else
        ui->folderCombo->setCurrentIndex( 0 ); //TODO possible bug: assumes folder list is not empty.

    ui->overwriteCheck->setChecked( AmarokConfig::overwriteFiles() );
    ui->spaceCheck->setChecked( AmarokConfig::replaceSpace() );
    ui->ignoreTheCheck->setChecked( AmarokConfig::ignoreThe() );
    ui->vfatCheck->setChecked( AmarokConfig::vfatCompatible() );
    ui->asciiCheck->setChecked( AmarokConfig::asciiOnly() );
    ui->regexpEdit->setText( AmarokConfig::replacementRegexp() );
    ui->replaceEdit->setText( AmarokConfig::replacementString() );

    ui->previewTableWidget->horizontalHeader()->setResizeMode( QHeaderView::ResizeToContents );
    ui->conflictLabel->setText("");
    QPalette p = ui->conflictLabel->palette();
    KColorScheme::adjustForeground( p, KColorScheme::NegativeText ); // TODO this isn't working, the color is still normal
    ui->conflictLabel->setPalette( p );

    QTimer *updatePreviewTimer = new QTimer( this );
    updatePreviewTimer->setSingleShot( true );
    updatePreviewTimer->setInterval( 2000 );
    connect( updatePreviewTimer, SIGNAL(timeout()), SLOT(slotUpdatePreview()) );
    //schedule first run next iteration of eventloop (will not block UI)
    updatePreviewTimer->start();

    // to show the conflict error
    connect( ui->overwriteCheck, SIGNAL( stateChanged( int ) ), SLOT( slotUpdatePreview() ) );
    connect( ui->ignoreTheCheck, SIGNAL(toggled(bool)), SLOT(slotUpdatePreview()) );
    connect( ui->spaceCheck    , SIGNAL(toggled(bool)), SLOT(slotUpdatePreview()) );
    connect( ui->asciiCheck    , SIGNAL(toggled(bool)), SLOT(slotUpdatePreview()) );
    connect( ui->regexpEdit    , SIGNAL(textChanged(QString)), SLOT(slotUpdatePreview()) );
    connect( ui->replaceEdit    , SIGNAL(textChanged(QString)), SLOT(slotUpdatePreview()) );
    //only start calculating preview after timeout.
    connect( m_filenameLayoutDialog, SIGNAL( schemeChanged() ), updatePreviewTimer,
            SLOT( start() ) );

    connect( this, SIGNAL( finished(int) ), SLOT( slotSaveFormatList() ) );
    connect( this , SIGNAL( accepted() ), SLOT( slotDialogAccepted() ) );
    connect( ui->folderCombo, SIGNAL( currentIndexChanged( const QString & ) ),
             this, SLOT( slotUpdatePreview() ) );
    connect( ui->folderCombo, SIGNAL( currentIndexChanged( const QString & ) ),
             this, SLOT( slotEnableOk( const QString & ) ) );
    connect( ui->addPresetButton, SIGNAL( clicked( bool ) ), this, SLOT( slotAddFormat() ) );
    connect( ui->removePresetButton, SIGNAL( clicked( bool ) ), this, SLOT( slotRemoveFormat() ) );
    connect( ui->updatePresetButton, SIGNAL( clicked( bool ) ), this, SLOT( slotUpdateFormat() ) );

    slotEnableOk( ui->folderCombo->currentText() );

    init();
}

OrganizeCollectionDialog::~OrganizeCollectionDialog()
{
    DEBUG_BLOCK

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
OrganizeCollectionDialog::buildFormatTip() const
{
    //FIXME: This is directly copied from mediadevice/generic/genericmediadeviceconfigdialog.ui.h
    QMap<QString, QString> args;
    args["albumartist"] = i18n( "%1 or %2", QLatin1String("Album Artist, The") , QLatin1String("The Album Artist") );
    args["thealbumartist"] = "The Album Artist";
    args["theartist"] = "The Artist";
    args["artist"] = i18n( "%1 or %2", QLatin1String("Artist, The") , QLatin1String("The Artist") );
    args["initial"] = i18n( "Artist's Initial" );
    args["filetype"] = i18n( "File Extension of Source" );
    args["track"] = i18n( "Track Number" );

    QString tooltip = i18n( "<h3>Custom Format String</h3>" );
    tooltip += i18n( "You can use the following tokens:" );
    tooltip += "<ul>";

    for( QMap<QString, QString>::iterator it = args.begin(); it != args.end(); ++it )
        tooltip += QString( "<li>%1 - %%2%" ).arg( it.value(), it.key() );

    tooltip += "</ul>";
    tooltip += i18n( "If you surround sections of text that contain a token with curly-braces, "
            "that section will be hidden if the token is empty." );

    return tooltip;
}


QString
OrganizeCollectionDialog::buildFormatString() const
{
    if( m_filenameLayoutDialog->getParsableScheme().simplified().isEmpty() )
        return "";
    return "%folder%/" + m_filenameLayoutDialog->getParsableScheme() + ".%filetype%";
}

QString
OrganizeCollectionDialog::commonPrefix( const QStringList &list ) const
{
    QString option = list.first().toLower();
    int length = option.length();
    while( length > 0 )
    {
        bool found = true;
        foreach( QString string, list )
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
OrganizeCollectionDialog::preview( const QString &format )
{
    ui->previewTableWidget->clearContents();
    bool conflict = false;

    QApplication::setOverrideCursor( QCursor( Qt::WaitCursor ) );
    m_trackOrganizer->setFormatString( format );
    m_trackOrganizer->setTargetFileExtension( m_targetFileExtension );
    QMap<Meta::TrackPtr, QString> dests = m_trackOrganizer->getDestinations();
    ui->previewTableWidget->setRowCount( dests.count() );
    QMapIterator<Meta::TrackPtr, QString> it( dests );
    int i = 0;
    while( it.hasNext() )
    {
        it.next();
        Meta::TrackPtr track = it.key();

        QString originalPath = track->prettyUrl();
        QString newPath = it.value();

        QFileInfo info( newPath );
        if( !conflict  && info.exists() )
            conflict = true;

        //new path preview in the 1st column
        QPalette p = ui->previewTableWidget->palette();
        QTableWidgetItem *item = new QTableWidgetItem( newPath );
        if( info.exists() )
            item->setBackgroundColor( p.color( QPalette::Base ) );
        ui->previewTableWidget->setItem( i, 0, item );

        //original in the second column
        item = new QTableWidgetItem( originalPath );
        ui->previewTableWidget->setItem( i, 1, item );
        KColorScheme::adjustBackground(p, KColorScheme::NegativeBackground);
        if( info.exists() )
            item->setBackgroundColor( p.color( QPalette::Base ) );

        ++i;
    }
    QApplication::restoreOverrideCursor();
    if( conflict )
    {
        if( ui->overwriteCheck->isChecked() )
            ui->conflictLabel->setText( i18n( "There is a filename conflict, existing files will be overwritten." ) );
        else
            ui->conflictLabel->setText( i18n( "There is a filename conflict, existing files will not be changed." ) );
    }
    else
        ui->conflictLabel->setText(""); // we clear the text instead of hiding it to retain the layout spacing
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
    populateFormatList();
    slotUpdatePreview();
}

void OrganizeCollectionDialog::populateFormatList()
{
    // items are stored in the config list in the following format:
    // Label#DELIM#format string#DELIM#selected
    // the last item to have the third parameter is the default selected preset
    // the third param isnis optional 
    QStringList presets_raw;
    int selected_index = -1;
    ui->presetCombo->clear();
    presets_raw = AmarokConfig::formatPresets();
    foreach( QString str, presets_raw )
    {
        QStringList items;
        items = str.split( "#DELIM#", QString::SkipEmptyParts );
        if( items.size() < 2 )
            continue;
        ui->presetCombo->addItem( items.at( 0 ), items.at( 1 ) ); // Label, format string
        if( items.size() == 3 )
            selected_index = ui->presetCombo->findData( items.at( 1 ) );
    }
    if( selected_index > 0 )
        ui->presetCombo->setCurrentIndex( selected_index );
    slotFormatPresetSelected( selected_index );
    connect( ui->presetCombo, SIGNAL( currentIndexChanged( int ) ), this, SLOT( slotFormatPresetSelected( int ) ) );
}

void OrganizeCollectionDialog::slotSaveFormatList()
{
    if( !m_formatListModified )
        return;

    QStringList presets;
    int n = ui->presetCombo->count();
    int current_idx = ui->presetCombo->currentIndex();
    for( int i = 0; i < n; ++i )
    {
        QString item;
        if( i == current_idx )
            item = "%1#DELIM#%2#DELIM#selected";
        else
            item = "%1#DELIM#%2";
        QString scheme = ui->presetCombo->itemData( i ).toString();
        QString label = ui->presetCombo->itemText( i );
        item = item.arg( label, scheme );
        presets.append( item );
    }
    AmarokConfig::setFormatPresets( presets );
}


void
OrganizeCollectionDialog::slotUpdatePreview()
{
    QString formatString = buildFormatString();
    m_trackOrganizer->setAsciiOnly( ui->asciiCheck->isChecked() );
    m_trackOrganizer->setFolderPrefix( ui->folderCombo->currentText() );
    m_trackOrganizer->setFormatString( formatString );
    m_trackOrganizer->setIgnoreThe( ui->ignoreTheCheck->isChecked() );
    m_trackOrganizer->setReplaceSpaces( ui->spaceCheck->isChecked() );
    m_trackOrganizer->setReplace( ui->regexpEdit->text(), ui->replaceEdit->text() );
    m_trackOrganizer->setVfatSafe( ui->vfatCheck->isChecked() );

    preview( formatString );

    int index = ui->presetCombo->currentIndex();
    if( index != -1 )
    {
        m_schemeModified = ( m_filenameLayoutDialog->getParsableScheme() !=
                           ui->presetCombo->itemData( index ).toString() );
    }
}

void
OrganizeCollectionDialog::slotDialogAccepted()
{
    AmarokConfig::setOrganizeDirectory( ui->folderCombo->currentText() );
    AmarokConfig::setIgnoreThe( ui->ignoreTheCheck->isChecked() );
    AmarokConfig::setReplaceSpace( ui->spaceCheck->isChecked() );
    AmarokConfig::setVfatCompatible( ui->vfatCheck->isChecked() );
    AmarokConfig::setAsciiOnly( ui->asciiCheck->isChecked() );
    AmarokConfig::setReplacementRegexp( ui->regexpEdit->text() );
    AmarokConfig::setReplacementString( ui->replaceEdit->text() );
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

void OrganizeCollectionDialog::slotFormatPresetSelected( int index )
{
    QString scheme = ui->presetCombo->itemData( index ).toString();
    m_filenameLayoutDialog->setScheme( scheme );
}

void OrganizeCollectionDialog::slotAddFormat()
{
    bool ok = false;
    QString name = KInputDialog::getText( i18n( "New Format Preset" ), i18n( "Preset Name" ), i18n( "New Preset" ),  &ok, this );
    if( !ok )
        return; // user canceled.
    QString format = m_filenameLayoutDialog->getParsableScheme();
    ui->presetCombo->insertItem(0, name, format);
    ui->presetCombo->setCurrentIndex( 0 );
    m_formatListModified = true;
}

void OrganizeCollectionDialog::slotRemoveFormat()
{
    int idx = ui->presetCombo->currentIndex();
    ui->presetCombo->removeItem( idx );
    m_formatListModified = true;
}

void
OrganizeCollectionDialog::slotUpdateFormat()
{
    int idx = ui->presetCombo->currentIndex();
    QString formatString = m_filenameLayoutDialog->getParsableScheme();
    ui->presetCombo->setItemData( idx, formatString );
    ui->updatePresetButton->setEnabled( m_schemeModified = false );
    m_formatListModified = true;
}



#endif  //AMAROK_ORGANIZECOLLECTIONDIALOG_UI_H
