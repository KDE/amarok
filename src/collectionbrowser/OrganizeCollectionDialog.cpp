
#ifndef AMAROK_ORGANIZECOLLECTIONDIALOG_UI_H
#define AMAROK_ORGANIZECOLLECTIONDIALOG_UI_H

#include "amarok.h"
#include "amarokconfig.h"
#include "OrganizeCollectionDialog.h"
#include "ui_OrganizeCollectionDialogBase.h"
#include "CollectionTreeItemModel.h"
#include "collection/CollectionManager.h"
#include "collection/BlockingQuery.h"
#include "qstringx.h"
#include "atomicstring.h"
#include "file/File.h"

#include <KVBox>

OrganizeCollectionDialog::OrganizeCollectionDialog(QueryMaker *qm, QWidget *parent,  const char *name, bool modal,
        const QString &caption,
        QFlags<KDialog::ButtonCode> buttonMask
        )
    : KDialog( parent )
      ,ui(new Ui::OrganizeCollectionDialogBase),
      detailed(true)

{
    Q_UNUSED( name )

      setCaption( caption );
    setModal( modal );
    setButtons( buttonMask );
    showButtonSeparator( true );
    /*
    m_previewTrack = 0;
    if ( tracks.size() > 0)
    {
        m_previewTrack = tracks[0];
    }

    Collection *coll = CollectionManager::instance()->primaryCollection();
    QueryMaker *qm = coll->queryMaker()->startTrackQuery()->limitMaxResultSize(1);
    BlockingQuery bq( qm );
    bq.startQuery();
    Meta::TrackList tracks = bq.tracks( coll->collectionId() );
    if ( tracks.size() > 0)
    {
        m_previewTrack = tracks[0];
    }
    */


    KVBox *vbox = new KVBox( this );
    setMainWidget( vbox );
    QWidget *widget;
    widget = new QWidget(vbox);

    ui->setupUi(widget);
    //init();

    

    //ui->folderCombo->insertStringList( folders, 0 );
    //ui->folderCombo->setCurrentItem( AmarokConfig::organizeDirectory() );
    ui->overwriteCheck->setChecked( AmarokConfig::overwriteFiles() );
    ui->filetypeCheck->setChecked( AmarokConfig::groupByFiletype() );
    ui->initialCheck->setChecked( AmarokConfig::groupArtists() );
    ui->spaceCheck->setChecked( AmarokConfig::replaceSpace() );
    ui->coverCheck->setChecked( AmarokConfig::coverIcons() );
    ui->ignoreTheCheck->setChecked( AmarokConfig::ignoreThe() );
    ui->vfatCheck->setChecked( AmarokConfig::vfatCompatible() );
    ui->asciiCheck->setChecked( AmarokConfig::asciiOnly() );
    ui->customschemeCheck->setChecked( AmarokConfig::useCustomScheme() );
    ui->formatEdit->setText( AmarokConfig::customScheme() );
    ui->regexpEdit->setText( AmarokConfig::replacementRegexp() );
    ui->replaceEdit->setText( AmarokConfig::replacementString() );
    connect( this, SIGNAL(buttonClicked(KDialog::ButtonCode)), this, SLOT(slotButtonClicked(KDialog::ButtonCode)));
    if( ui->customschemeCheck->isChecked())
        setDetailsWidgetVisible(true);
    else
        slotDetails();
    init();
    show();
}

OrganizeCollectionDialog::~OrganizeCollectionDialog()
{
    delete ui;
}


QString OrganizeCollectionDialog::buildDestination( const QString &format, const Meta::TrackPtr track ) const
{
    return "abc";
    //FIXME: This code is directly copied from mediadevice/generic/genericmediadevice.cpp (with some modificatoins)

    bool isCompilation = track->album()->isCompilation();
    QMap<QString, QString> args;
    QString artist = track->artist()->name();
    QString albumartist = artist;
    if( isCompilation )
        albumartist = i18n( "Various Artists" );
    args["theartist"] = cleanPath( artist );
    args["thealbumartist"] = cleanPath( albumartist );
    if(!ui->ignoreTheCheck->isChecked() && artist.startsWith( "The " ) )
        Amarok::manipulateThe( artist, true );
    artist = cleanPath( artist );
    if(!ui->ignoreTheCheck->isChecked() && albumartist.startsWith( "The " ) )
        Amarok::manipulateThe( albumartist, true );

    albumartist = cleanPath( albumartist );

    //these additional columns from MetaBundle were used before but haven't
    //been ported yet. Do they need to be?
    //Bpm,Directory,Bitrate,SampleRate,Mood
    args["title"] = cleanPath( track->prettyName() );
    args["composer"] = cleanPath( track->composer()->prettyName() );
    args["year"] = cleanPath( track->year()->prettyName() );
    args["album"] = cleanPath( track->album()->prettyName() );
    args["discnumber"] = QString::number( track->discNumber() );
    args["genre"] = cleanPath( track->genre()->prettyName() );
    args["comment"] = cleanPath( track->comment() );
    args["artist"] = artist;
    args["albumartist"] = albumartist;
    args["initial"] = albumartist.mid( 0, 1 ).toUpper();
    args["filetype"] = track->type();
    args["rating"] = track->rating();
    args["filesize"] = track->filesize();
    args["length"] = track->length();
    QString trackNum;
    if ( track->trackNumber() )
        trackNum = QString("%1").arg( track->trackNumber(), 2, 10, QChar('0') );
    args["track"] = trackNum;

    Amarok::QStringx formatx( format );
    QString result = formatx.namedOptArgs( args );
    if( !result.startsWith( '/' ) )
        result.prepend( "/" );

   return result.replace( QRegExp( "/\\.*" ), "/" );
}

QString OrganizeCollectionDialog::buildFormatTip() const
{
    //FIXME: This is directly copied from mediadevice/generic/genericmediadeviceconfigdialog.ui.h
    QMap<QString, QString> args;
    args["albumartist"] = i18n( "%1 or %2" ).arg( "Album Artist, The" , "The Album Artist" );
    args["thealbumartist"] = "The Album Artist";
    args["theartist"] = "The Artist";
    args["artist"] = i18n( "%1 or %2" ).arg( "Artist, The" , "The Artist" );
    args["initial"] = i18n( "Artist's Initial" );
    args["filetype"] = i18n( "File Extension of Source" );
    args["track"] = i18n( "Track Number" );

    QString tooltip = i18n( "<h3>Custom Format String</h3>" );
    tooltip += i18n( "You can use the following tokens:" );
    tooltip += "<ul>";
    for( QMap<QString, QString>::iterator it = args.begin();
            it != args.end();
            ++it )
    {
        tooltip += QString( "<li>%1 - %2" ).arg( it.value(), "%" + it.key() );
    }
    tooltip += "</ul>";

    tooltip += i18n( "If you surround sections of text that contain a token with curly-braces, "
            "that section will be hidden if the token is empty." );

    return tooltip;
}


QString OrganizeCollectionDialog::buildFormatString() const
{
    QString format = "%folder/";
    if( ui->filetypeCheck->isChecked() )
        format += "%filetype/";
    if( ui->initialCheck->isChecked() )
        format += "%initial/";

    format += "%albumartist/";
    if( ui->spaceCheck->isChecked() )
    {
        format += "%album{_(Disc_%discnumber)}/";
        format += "{%track_-_}%title.%filetype";
    }
    else
    {
        format += "%album{ (Disc %discnumber)}/";
        format += "{%track - }%title.%filetype";
    }

    if( ui->customschemeCheck->isChecked() )
        format = ui->formatEdit->text();

    return format;
}


void OrganizeCollectionDialog::setPreviewTrack( const Meta::TrackPtr track )
{
   m_previewTrack = track;
}


void OrganizeCollectionDialog::preview( const QString &format )
{
   emit updatePreview( buildDestination( format, m_previewTrack ) );
}


QString OrganizeCollectionDialog::cleanPath( const QString &component ) const
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
    if( ui->vfatCheck->isChecked() )
        result = Amarok::vfatPath( result );

    result.replace( "/", "-" );

    return result;
}


void OrganizeCollectionDialog::update( int dummy )
{
    Q_UNUSED( dummy );

    QString oldFormat = ui->formatEdit->text();
    if( !ui->customschemeCheck->isChecked() )
        ui->formatEdit->setText( buildFormatString() );

    if( ui->customschemeCheck->isChecked() || oldFormat == ui->formatEdit->text() )
        emit updatePreview( buildDestination( ui->formatEdit->text(), m_previewTrack ) );
}


void OrganizeCollectionDialog::update( const QString & dummy )
{
    Q_UNUSED( dummy );

    update( 0 );
}

void OrganizeCollectionDialog::slotButtonClicked(KDialog::ButtonCode button)
{
    if(button == Details)
        slotDetails();
}

void OrganizeCollectionDialog::slotDetails()
{
    detailed = !detailed;

    if( detailed )
    {
        ui->ignoreTheCheck->show();
        ui->customschemeCheck->show();
        ui->replacementGroup->show();
        ui->formatLabel->show();
        ui->formatEdit->show();
        ui->formatHelp->show();
    }
    else
    {
        ui->ignoreTheCheck->hide();
        ui->customschemeCheck->hide();
        ui->replacementGroup->hide();
        ui->formatLabel->hide();
        ui->formatEdit->hide();
        ui->formatHelp->hide();
    }

    if( dynamic_cast<QWidget *>(parent()) ) {
        static_cast<QWidget *>(parent())->adjustSize();
        static_cast<QWidget *>(parent())->updateGeometry();
    }
    //adjustSize();
    //updateGeometry();
}


         


void OrganizeCollectionDialog::init()
{
    ui->formatHelp->setText( QString( "<a href='whatsthis:%1'>%2</a>" ).
            arg( Amarok::escapeHTMLAttr( buildFormatTip() ), i18n( "(Help)" ) ) );
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
        Amarok::StatusBar::instance()->shortMessage( shortMsg, KDE::StatusBar::Sorry );
        return;
    }

    if( m_organizeURLs.count() )
    {
        if( copy != m_organizeCopyMode )
        {
            QString shortMsg = i18n( "Cannot start organize operation of different kind while another is in progress." );
            Amarok::StatusBar::instance()->shortMessage( shortMsg, KDE::StatusBar::Sorry );
            return;
        }
        else
        {
            m_organizeURLs += Amarok::recursiveUrlExpand( urls );
            Amarok::StatusBar::instance()->incrementProgressTotalSteps( this, urls.count() );
            return;
        }
    }

    QStringList folders = MountPointManager::instance()->collectionFolders();
    if( folders.isEmpty() )
    {
        QString longMsg = i18n( "You need to configure at least one folder for your collection for organizing your files." );
        Amarok::StatusBar::instance()->longMessage( longMsg, KDE::StatusBar::Sorry );
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
        AmarokConfig::setOrganizeDirectory( dialog.folderCombo->currentItem() );
        AmarokConfig::setOverwriteFiles( dialog.overwriteCheck->isChecked() );
        AmarokConfig::setGroupByFiletype( dialog.filetypeCheck->isChecked() );
        AmarokConfig::setGroupArtists( dialog.initialCheck->isChecked() );
        AmarokConfig::setIgnoreThe( dialog.ignoreTheCheck->isChecked() );
        AmarokConfig::setReplaceSpace( dialog.spaceCheck->isChecked() );
        AmarokConfig::setCoverIcons( dialog.coverCheck->isChecked() );
        AmarokConfig::setVfatCompatible( dialog.vfatCheck->isChecked() );
        AmarokConfig::setAsciiOnly( dialog.asciiCheck->isChecked() );
        AmarokConfig::setUseCustomScheme( dialog.customschemeCheck->isChecked() );
        AmarokConfig::setCustomScheme( dialog.formatEdit->text() );
        AmarokConfig::setReplacementRegexp( dialog.regexpEdit->text() );
        AmarokConfig::setReplacementString( dialog.replaceEdit->text() );
        KURL::List skipped;

        m_organizeURLs = Amarok::recursiveUrlExpand( urls );
        m_organizeCopyMode = copy;
        CollectionDB::instance()->createTables( true ); // create temp tables
        Amarok::StatusBar::instance()->newProgressOperation( this )
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
            Amarok::StatusBar::instance()->incrementProgress( this );

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
                    "The following %n files could not be organized: ", skipped.count() );
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
                    "Sorry, %n files could not be organized.", skipped.count() );
            Amarok::StatusBar::instance()->shortLongMessage( shortMsg, longMsg, KDE::StatusBar::Sorry );
        }
        else if ( m_organizingFileCancelled )
        {
            Amarok::StatusBar::instance()->shortMessage( i18n( "Aborting jobs..." ) );
            m_organizingFileCancelled = false;
        }

        m_dirty = true;
        QTimer::singleShot( 0, CollectionView::instance(), SLOT( renderView() ) );
        Amarok::StatusBar::instance()->endProgressOperation( this );
    }
}
 */

#endif
