/****************************************************************************************
 * Copyright (c) 2008 Bonne Eggleston <b.eggleston@gmail.com>                           *
 * Copyright (c) 2008 Teo Mrnjavac <teo.mrnjavac@gmail.com>                             *
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

#include "Amarok.h"
#include "Debug.h"
#include "amarokconfig.h"
#include "file/File.h"
#include "MountPointManager.h"
#include "QStringx.h"
#include "ui_OrganizeCollectionDialogBase.h"

#include <QDir>

OrganizeCollectionDialog::OrganizeCollectionDialog( const Meta::TrackList &tracks, const QStringList &folders, QWidget *parent,  const char *name, bool modal,
                                                    const QString &caption, QFlags<KDialog::ButtonCode> buttonMask )
    : KDialog( parent )
    , ui( new Ui::OrganizeCollectionDialogBase )
    , m_detailed( true )
{
    Q_UNUSED( name )

    setCaption( caption );
    setModal( modal );
    setButtons( buttonMask );
    showButtonSeparator( true );
    m_previewTrack = 0;

    if ( tracks.size() > 0 )
    {
        m_previewTrack = tracks[0];
        m_allTracks = tracks;
    }

    KVBox *mainVBox = new KVBox( this );
    setMainWidget( mainVBox );
    QWidget *mainContainer = new QWidget( mainVBox );

    ui->setupUi( mainContainer );

    m_filenameLayoutDialog = new FilenameLayoutDialog( mainContainer, 1 );   //", 1" means isOrganizeCollection ==> doesn't show Options frame
    m_filenameLayoutDialog->hide();
    connect( this, SIGNAL( accepted() ),
             m_filenameLayoutDialog, SLOT( onAccept() ) );
    ui->verticalLayout->insertWidget( 3, m_filenameLayoutDialog );
    ui->ignoreTheCheck->show();

    ui->folderCombo->insertItems( 0, folders );
    ui->folderCombo->setCurrentIndex( AmarokConfig::organizeDirectory() );
    ui->overwriteCheck->setChecked( AmarokConfig::overwriteFiles() );
    ui->filetypeCheck->setChecked( AmarokConfig::groupByFiletype() );
    ui->initialCheck->setChecked( AmarokConfig::groupArtists() );
    ui->spaceCheck->setChecked( AmarokConfig::replaceSpace() );
    ui->ignoreTheCheck->setChecked( AmarokConfig::ignoreThe() );
    ui->vfatCheck->setChecked( AmarokConfig::vfatCompatible() );
    ui->asciiCheck->setChecked( AmarokConfig::asciiOnly() );
    ui->customschemeCheck->setChecked( AmarokConfig::useCustomScheme() );
    ui->regexpEdit->setText( AmarokConfig::replacementRegexp() );
    ui->replaceEdit->setText( AmarokConfig::replacementString() );

    connect( this, SIGNAL( updatePreview( QString ) ), ui->previewText, SLOT( setText( QString ) ) );

    connect( ui->filetypeCheck , SIGNAL(toggled(bool)), SLOT(slotUpdatePreview()) );
    connect( ui->initialCheck  , SIGNAL(toggled(bool)), SLOT(slotUpdatePreview()) );
    connect( ui->ignoreTheCheck, SIGNAL(toggled(bool)), SLOT(slotUpdatePreview()) );
    connect( ui->spaceCheck    , SIGNAL(toggled(bool)), SLOT(slotUpdatePreview()) );
    connect( ui->asciiCheck    , SIGNAL(toggled(bool)), SLOT(slotUpdatePreview()) );
    connect( ui->customschemeCheck, SIGNAL(toggled(bool)), SLOT(slotUpdatePreview()) );
    connect( ui->regexpEdit    , SIGNAL(textChanged(QString)), SLOT(slotUpdatePreview()) );
    connect( ui->replaceEdit    , SIGNAL(textChanged(QString)), SLOT(slotUpdatePreview()) );
    connect( m_filenameLayoutDialog, SIGNAL( schemeChanged() ), this, SLOT( slotUpdatePreview() ) );
    connect( ui->customschemeCheck, SIGNAL( toggled( bool ) ), this, SLOT( toggleCustomScheme( bool ) ) );

    connect( this , SIGNAL( accepted() ), SLOT( slotDialogAccepted() ) );
    connect( ui->folderCombo, SIGNAL( currentIndexChanged( const QString & ) ),
             this, SLOT( slotUpdatePreview() ) );
    connect( ui->folderCombo, SIGNAL( currentIndexChanged( const QString & ) ),
             this, SLOT( slotEnableOk( const QString & ) ) );

    toggleCustomScheme( ui->customschemeCheck->isChecked() );
    slotEnableOk( ui->folderCombo->currentText() );

    init();
}

OrganizeCollectionDialog::~OrganizeCollectionDialog()
{
    DEBUG_BLOCK

    AmarokConfig::setOrganizeDirectory( ui->folderCombo->currentIndex() );
    delete ui;
}

QMap<Meta::TrackPtr, QString>
OrganizeCollectionDialog::getDestinations()
{
    QString format = buildFormatString();
    QMap<Meta::TrackPtr, QString> destinations;
    foreach( const Meta::TrackPtr &track, m_allTracks )
    {
        if( track )
            destinations.insert( track, buildDestination( format, track ) );
    }
    return destinations;
}

bool
OrganizeCollectionDialog::overwriteDestinations() const
{
    return ui->overwriteCheck->isChecked();
}

QString
OrganizeCollectionDialog::buildDestination( const QString &format, const Meta::TrackPtr &track ) const
{
    //TODO: handle if track==NULL to avoid bug 169684
    //This could maybe happen with an empty collection, when the TrackList is empty and then m_previewTrack is null.
    //FIXME: 169684

    bool isCompilation = track->album() && track->album()->isCompilation();

    QMap<QString, QString> args;
    QString artist = track->artist()->name();
    QString albumartist;
    if( isCompilation )
        albumartist = i18n( "Various Artists" );
    else
    {
        if( track->album() && track->album()->albumArtist() )
            albumartist = track->album()->albumArtist()->name();
        else
            albumartist = artist;
    }
    args["theartist"] = cleanPath( artist );
    args["thealbumartist"] = cleanPath( albumartist );

    if( ui->ignoreTheCheck->isChecked() && artist.startsWith( "The " ) )
        Amarok::manipulateThe( artist, true );

    artist = cleanPath( artist );

    if( ui->ignoreTheCheck->isChecked() && albumartist.startsWith( "The " ) )
        Amarok::manipulateThe( albumartist, true );

    albumartist = cleanPath( albumartist );

    //these additional columns from MetaBundle were used before but haven't
    //been ported yet. Do they need to be?
    //Bpm,Directory,Bitrate,SampleRate,Mood
    args["folder"] = ui->folderCombo->currentText();
    args["title"] = cleanPath( track->prettyName() );
    args["composer"] = track->composer() ? cleanPath( track->composer()->prettyName() ) : QString();
    args["year"] = track->year() ? cleanPath( track->year()->prettyName() ) : QString();
    args["album"] = track->album() ? cleanPath( track->album()->prettyName() ) : QString();

    if( track->discNumber() )
        args["discnumber"] = QString::number( track->discNumber() );

    args["genre"] = track->genre() ? cleanPath( track->genre()->prettyName() ) : QString();
    args["comment"] = cleanPath( track->comment() );
    args["artist"] = artist;
    args["albumartist"] = albumartist;
    args["initial"] = albumartist.mid( 0, 1 ).toUpper();    //artists starting with The are already handled above
    args["filetype"] = track->type();
    args["rating"] = track->rating();
    args["filesize"] = track->filesize();
    args["length"] = track->length() / 1000;

    if ( track->trackNumber() )
    {
        QString trackNum = QString("%1").arg( track->trackNumber(), 2, 10, QChar('0') );
        args["track"] = trackNum;
    }

    Amarok::QStringx formatx( format );
    QString result = formatx.namedOptArgs( args );
    if( !result.startsWith( '/' ) )
        result.prepend( "/" );

   return result.replace( QRegExp( "/\\.*" ), "/" );
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
        tooltip += QString( "<li>%1 - %2" ).arg( it.value(), '%' + it.key() );

    tooltip += "</ul>";
    tooltip += i18n( "If you surround sections of text that contain a token with curly-braces, "
            "that section will be hidden if the token is empty." );

    return tooltip;
}


QString
OrganizeCollectionDialog::buildFormatString() const
{
    if( ui->customschemeCheck->isChecked() )
        return "%folder/" + m_filenameLayoutDialog->getParsableScheme() + ".%filetype";
    QString format = "%folder/";
    if( ui->filetypeCheck->isChecked() )
        format += "%filetype/";
    if( ui->initialCheck->isChecked() )
        format += "%initial/";

    format += "%albumartist/";
    if( ui->spaceCheck->isChecked() )   //replace spaces with underscores
    {
        format += "%album{_(Disc_%discnumber)}/";
        format += "{%track_-_}%title.%filetype";
    }
    else
    {
        format += "%album{ (Disc %discnumber)}/";
        format += "{%track - }%title.%filetype";
    }


    format = QDir::fromNativeSeparators( format );      //fromNativeSeparators handles \\ under windows
    return format;
}



void
OrganizeCollectionDialog::preview( const QString &format )
{
    if( m_previewTrack )
        emit updatePreview( buildDestination( format, m_previewTrack ) );
}


QString
OrganizeCollectionDialog::cleanPath( const QString &component ) const
{
    QString result = component;

    if( ui->asciiCheck->isChecked() )
    {
        result = Amarok::cleanPath( result );
        result = Amarok::asciiPath( result );
    }

    if( !ui->regexpEdit->text().isEmpty() )
        result.replace( QRegExp( ui->regexpEdit->text() ), ui->replaceEdit->text() );

    result.simplified();
    if( ui->spaceCheck->isChecked() )
        result.replace( QRegExp( "\\s" ), "_" );
    debug()<<"I'm about to do Amarok::vfatPath( result ), this is result: "<<result;
    if( ui->vfatCheck->isChecked() )
        result = Amarok::vfatPath( result );

    result.replace( '/', '-' );

    return result;
}


void
OrganizeCollectionDialog::update( int dummy )   //why the dummy?
{
    Q_UNUSED( dummy );

    if( m_previewTrack )
    {
        if( ui->customschemeCheck->isChecked() )
            emit updatePreview( buildDestination( "%folder/" + m_filenameLayoutDialog->getParsableScheme(), m_previewTrack ) );
        else
            emit updatePreview( buildDestination( buildFormatString(), m_previewTrack ) );
    }
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
    preview( buildFormatString() );
}

void
OrganizeCollectionDialog::slotDialogAccepted()
{
    AmarokConfig::setOrganizeDirectory( ui->folderCombo->currentIndex() );
    AmarokConfig::setGroupByFiletype( ui->filetypeCheck->isChecked() );
    AmarokConfig::setGroupArtists( ui->initialCheck->isChecked() );
    AmarokConfig::setIgnoreThe( ui->ignoreTheCheck->isChecked() );
    AmarokConfig::setReplaceSpace( ui->spaceCheck->isChecked() );
    AmarokConfig::setVfatCompatible( ui->vfatCheck->isChecked() );
    AmarokConfig::setAsciiOnly( ui->asciiCheck->isChecked() );
    AmarokConfig::setUseCustomScheme( ui->customschemeCheck->isChecked() );
    AmarokConfig::setReplacementRegexp( ui->regexpEdit->text() );
    AmarokConfig::setReplacementString( ui->replaceEdit->text() );
}

//Hides and shows the right elements in the interface in the right order to keep the layout sane
void
OrganizeCollectionDialog::toggleCustomScheme( bool state )  //SLOT
{
    if( state )
    {
        ui->initialCheck->hide();
        ui->filetypeCheck->hide();
        m_filenameLayoutDialog->setVisible( true );
    }
    else
    {
        m_filenameLayoutDialog->hide();
        ui->initialCheck->show();
        ui->filetypeCheck->show();
    }
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

/* Code to port
 *
bool
CollectionView::isOrganizingFiles() const
{
    return m_organizeURLs.count() > 0;
}

void CollectionView::cancelOrganizingFiles()
{
    // Set the indicator
    m_organizingFileCancelled = true;

    // Cancel the current underlying CollectionDB::instance()->moveFile operation
    CollectionDB::instance()->cancelMovingFileJob();
}
void
CollectionView::organizeFiles( const KURL::List &urls, const QString &caption, bool copy )  //SLOT
{
    if( m_organizingFileCancelled )
    {
        QString shortMsg = i18n( "Cannot start organize operation until jobs are aborted." );
        The::statusBar()->shortMessage( shortMsg, StatusBar::Sorry );
        return;
    }

    if( m_organizeURLs.count() )
    {
        if( copy != m_organizeCopyMode )
        {
            QString shortMsg = i18n( "Cannot start organize operation of different kind while another is in progress." );
            The::statusBar()->shortMessage( shortMsg, StatusBar::Sorry );
            return;
        }
        else
        {
            m_organizeURLs += Amarok::recursiveUrlExpand( urls );
            The::statusBar()->incrementProgressTotalSteps( this, urls.count() );
            return;
        }
    }

    QStringList folders = MountPointManager::instance()->collectionFolders();
    if( folders.isEmpty() )
    {
        QString longMsg = i18n( "You need to configure at least one folder for your collection for organizing your files." );
        The::statusBar()->longMessage( longMsg, StatusBar::Sorry );
        return;
    }


    KURL::List previewURLs = Amarok::recursiveUrlExpand( urls.first(), 1 );
    if( previewURLs.count() )
    {
        dialog.setPreviewBundle( MetaBundle( previewURLs.first() ) );
        dialog.update( 0 );
    }

    base.setInitialSize( QSize( 450, 350 ) );

    if( base.exec() == KDialogBase::Accepted )
    {
        AmarokConfig::setOrganizeDirectory( dialog.folderCombo->currentIndex() );
        AmarokConfig::setOverwriteFiles( dialog.overwriteCheck->isChecked() );
        AmarokConfig::setGroupByFiletype( dialog.filetypeCheck->isChecked() );
        AmarokConfig::setGroupArtists( dialog.initialCheck->isChecked() );
        AmarokConfig::setIgnoreThe( dialog.ignoreTheCheck->isChecked() );
        AmarokConfig::setReplaceSpace( dialog.spaceCheck->isChecked() );
        AmarokConfig::setVfatCompatible( dialog.vfatCheck->isChecked() );
        AmarokConfig::setAsciiOnly( dialog.asciiCheck->isChecked() );
        AmarokConfig::setUseCustomScheme( dialog.customschemeCheck->isChecked() );
        AmarokConfig::setReplacementRegexp( dialog.regexpEdit->text() );
        AmarokConfig::setReplacementString( dialog.replaceEdit->text() );
        KURL::List skipped;

        m_organizeURLs = Amarok::recursiveUrlExpand( urls );
        m_organizeCopyMode = copy;
        CollectionDB::instance()->createTables( true ); // create temp tables
        The::statusBar()->newProgressOperation( this )
            .setDescription( caption )
            .setAbortSlot( this, SLOT( cancelOrganizingFiles() ) )
            .setTotalSteps( m_organizeURLs.count() );

        while( !m_organizeURLs.empty() && !m_organizingFileCancelled )
        {
            KURL &src = m_organizeURLs.first();

            if( !CollectionDB::instance()->organizeFile( src, dialog, copy ) )
            {
                skipped += src;
            }

            m_organizeURLs.pop_front();
            The::statusBar()->incrementProgress( this );

            if( m_organizingFileCancelled ) m_organizeURLs.clear();
        }

        CollectionDB::instance()->sanitizeCompilations(); //queryBuilder doesn't handle unknownCompilations
        CollectionDB::instance()->copyTempTables(); // copy temp table contents to permanent tables
        CollectionDB::instance()->dropTables( true ); // and drop them

        // and now do an incremental scan since this was disabled while organizing files
        QTimer::singleShot( 0, CollectionDB::instance(), SLOT( scanMonitor() ) );

        if( !m_organizingFileCancelled && skipped.count() > 0 )
        {
            QString longMsg = i18n( "The following file could not be organized: ",
                    "The following %1 files could not be organized: ", skipped.count() );
            bool first = true;
            for( KURL::List::iterator it = skipped.begin();
                    it != skipped.end();
                    it++ )
            {
                if( !first )
                    longMsg += i18n( ", " );
                else
                    first = false;
                longMsg += (*it).path();
            }
            longMsg += i18n( "." );

            QString shortMsg = i18n( "Sorry, one file could not be organized.",
                    "Sorry, %1 files could not be organized.", skipped.count() );
            The::statusBar()->shortLongMessage( shortMsg, longMsg, StatusBar::Sorry );
        }
        else if ( m_organizingFileCancelled )
        {
            The::statusBar()->shortMessage( i18n( "Aborting jobs..." ) );
            m_organizingFileCancelled = false;
        }

        m_dirty = true;
        QTimer::singleShot( 0, CollectionView::instance(), SLOT( renderView() ) );
        The::statusBar()->endProgressOperation( this );
    }
}
 */

#endif  //AMAROK_ORGANIZECOLLECTIONDIALOG_UI_H
