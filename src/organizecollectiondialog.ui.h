#include "amarok.h"
#include "collectionbrowser.h"
#include "collectiondb.h"
#include "qstringx.h"


QString OrganizeCollectionDialog::buildDestination( const QString &format, const MetaBundle &mb )
{
    bool isCompilation = false;
    if( !mb.album().isEmpty() )
    {
        const int albumId = CollectionDB::instance()->albumID( mb.album() );
        isCompilation = CollectionDB::instance()->albumIsCompilation( QString::number(albumId) );
    }

    QMap<QString, QString> args;
    QString artist = mb.artist();
    if( isCompilation )
        artist = i18n( "Various Artists" );
    args["theartist"] = cleanPath( artist );
    if( ignoreTheCheck->isChecked() && artist.startsWith( "The " ) )
        CollectionView::instance()->manipulateThe( artist, true );
    artist = cleanPath( artist );
    for( int i = 0; i < MetaBundle::NUM_COLUMNS; i++ )
    {
        args[mb.exactColumnName( i ).lower()] = cleanPath( mb.prettyText( i ) );
    }
    args["artist"] = artist;
    args["folder"] = folderCombo->currentText();
    args["initial"] = artist.mid( 0, 1 ).upper();
    args["filetype"] = mb.url().path().section( ".", -1 ).lower();
    QString track;
    track.sprintf( "%02d", mb.track() );
    args["track"] = track;

    amaroK::QStringx formatx( format );
    QString result = formatx.namedArgs( args );
    if( result.startsWith( folderCombo->currentText() ) )
    {
        QString tail = result.mid( folderCombo->currentText().length() );
        if( !tail.startsWith( "/" ) )
            tail.prepend( "/" );
        return folderCombo->currentText() + tail.replace( QRegExp( "/\\.*" ), "/" );
    }
    return result.replace( QRegExp( "/\\.*" ), "/" );
}


QString OrganizeCollectionDialog::buildFormatString()
{
    QString format = "%folder/";
    if( filetypeCheck->isChecked() )
        format += "%filetype/";
    if( initialCheck->isChecked() )
        format += "%initial/";

    format += "%artist/";
    format += "%album/";
    if( spaceCheck->isChecked() )
        format += "%track_-_%title.%filetype";
    else
        format += "%track - %title.%filetype";

    if( customschemeCheck->isChecked() )
        format = formatEdit->text();

    return format;
}


void OrganizeCollectionDialog::setPreviewBundle( const MetaBundle &bundle )
{
   previewBundle = bundle;
}


void OrganizeCollectionDialog::preview( const QString &format )
{
   emit updatePreview( buildDestination( format, previewBundle ) );
}


QString OrganizeCollectionDialog::cleanPath( const QString &component )
{
    QString result = component;

    if( asciiCheck->isChecked() )
    {
        result = amaroK::cleanPath(result, true /* replaces weird stuff by '_' */);
    }

    if( !regexpEdit->text().isEmpty() )
        result.replace( QRegExp( regexpEdit->text() ), replaceEdit->text() );

    result.simplifyWhiteSpace();
    if( spaceCheck->isChecked() )
        result.replace( QRegExp( "\\s" ), "_" );
    if( vfatCheck->isChecked() )
        result.replace( "?", "" ).replace( "\\", "_" ).replace( "*", "_" ).replace( ":", "_" ).replace("\"","");

    result.replace( "/", "-" );

    return result;
}


void OrganizeCollectionDialog::update( int dummy )
{
    Q_UNUSED( dummy );

    QString oldFormat = formatEdit->text();
    if( !customschemeCheck->isChecked() )
        formatEdit->setText( buildFormatString() );

    if( customschemeCheck->isChecked() || oldFormat==formatEdit->text() )
        emit updatePreview( buildDestination( formatEdit->text(), previewBundle ) );
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
        ignoreTheCheck->show();
        customschemeCheck->show();
        replacementGroup->show();
        formatLabel->show();
        formatEdit->show();
    }
    else
    {
        ignoreTheCheck->hide();
        customschemeCheck->hide();
        replacementGroup->hide();
        formatLabel->hide();
        formatEdit->hide();
    }

    if( dynamic_cast<QWidget *>(parent()) ) {
        static_cast<QWidget *>(parent())->adjustSize();
        static_cast<QWidget *>(parent())->updateGeometry();
    }
}


void OrganizeCollectionDialog::init()
{
    detailed = true;
}
