#include <kdialog.h>
#include <qstringx.h>
#include <collectiondb.h>
#include <collectionbrowser.h>



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
    if( ignoreTheCheck->isChecked() && artist.startsWith( "the ", false ) )
        CollectionView::instance()->manipulateThe( artist, true );
    artist = cleanPath( artist );
    args["artist"] = artist;
    for( int i = 0; i < MetaBundle::NUM_COLUMNS; i++ )
    {
        args[mb.exactColumnName( i ).lower()] = cleanPath( mb.prettyText( i ) );
    }
    args["folder"] = folderCombo->currentText();
    args["initial"] = artist.mid( 0, 1 ).upper();
    args["filetype"] = mb.url().path().section( ".", -1 ).lower();
    QString track;
    track.sprintf( "%02d", mb.track() );
    args["track"] = track;

    amaroK::QStringx formatx( format );
    return formatx.namedArgs( args );
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
        // german umlauts
        result.replace( "ä", "ae" ).replace( "Ä", "Ae" );
        result.replace( "ö", "oe" ).replace( "Ö", "Oe" );
        result.replace( "ü", "ue" ).replace( "Ü", "Ue" );
        result.replace( "ß", "ss" );
        // french accented characters
        result.replace( QRegExp("[áàâ]"), "a" ).replace( QRegExp("[ÁÀÂ]"), "A" );
        result.replace( "ç", "c" ).replace( "Ç", "C" );
        result.replace( QRegExp("[éèêë]"), "e" ).replace( QRegExp("[ÉÈÊË]"), "E" );
        result.replace( QRegExp("[íìîï]"), "i" ).replace( QRegExp("[ÍÌÎÏ]"), "I" );
        result.replace( QRegExp("[óòô]"), "o" ).replace( QRegExp("[ÓÒÔ]"), "O" );
        result.replace( QRegExp("[úùû]"), "u" ).replace( QRegExp("[Û]"), "U" );
        // add here
        result.replace( "ñ", "n" );

        for( uint i = 0; i < result.length(); i++ )
        {
            uchar c = result.ref( i );
            if( c > 0x7f || c == 0 )
            {
                c = '_';
                result.ref( i ) = c;
            }
        }
    }

    result.simplifyWhiteSpace();
    if( spaceCheck->isChecked() )
        result.replace( QRegExp( "\\s" ), "_" );
    if( vfatCheck->isChecked() )
        result.replace( "?", "_" ).replace( "\\", "_" ).replace( "*", "_" ).replace( ":", "_" );

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
