
#ifndef AMAROK_ORGANIZECOLLECTIONDIALOG_UI_H
#define AMAROK_ORGANIZECOLLECTIONDIALOG_UI_H

#include "amarok.h"
#include "OrganizeCollectionDialog.h"
#include "ui_OrganizeCollectionDialogBase.h"
#include "CollectionTreeItemModel.h"
#include "qstringx.h"
#include "atomicstring.h"
#include "file/File.h"

OrganizeCollectionDialog::OrganizeCollectionDialog( QWidget *parent,  const char *name, bool modal,
        const QString &caption,
        QFlags<KDialog::ButtonCode> buttonMask
        )
    : KDialog( parent )
      , ui( new Ui::OrganizeCollectionDialogBase() ),
      detailed(true)

{
    Q_UNUSED( name )
        setCaption( caption );
    setModal( modal );
    setButtons( buttonMask );
    showButtonSeparator( true );
   // m_previewTrack = new MetaFile::Track; FIXME: Find a way to get a default track

    init();
}

OrganizeCollectionDialog::~OrganizeCollectionDialog()
{
    delete ui;
}


QString OrganizeCollectionDialog::buildDestination( const QString &format, const Meta::TrackPtr track ) const
{
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
    emit detailsClicked();
}


void OrganizeCollectionDialog::init()
{
    detailed = true;

    ui->formatHelp->setText( QString( "<a href='whatsthis:%1'>%2</a>" ).
            arg( Amarok::escapeHTMLAttr( buildFormatTip() ), i18n( "(Help)" ) ) );
}

#endif
