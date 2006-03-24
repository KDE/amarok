#include <kdialog.h>
#include <qstringx.h>
#include <collectiondb.h>
#include <collectionbrowser.h>
#include "debug.h"



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
        // german umlauts
        result.replace( QChar(0x00e4), "ae" ).replace( QChar(0x00c4), "Ae" );
        result.replace( QChar(0x00f6), "oe" ).replace( QChar(0x00d6), "Oe" );
        result.replace( QChar(0x00dc), "ue" ).replace( QChar(0x00fc), "Ue" );
        result.replace( QChar(0x00df), "ss" );

        // some strange accents
        result.replace( QChar(0x00e7), "c" ).replace( QChar(0x00c7), "C" );
        result.replace( QChar(0x00fd), "y" ).replace( QChar(0x00dd), "Y" );
        result.replace( QChar(0x00f1), "n" ).replace( QChar(0x00d1), "N" );

        // accented vowels
        QChar a[] = { 'a', 0xe0,0xe1,0xe2,0xe3,0xe5, 0 };
        QChar A[] = { 'A', 0xc0,0xc1,0xc2,0xc3,0xc5, 0 };
        QChar E[] = { 'e', 0xe8,0xe9,0xea,0xeb, 0 };
        QChar e[] = { 'E', 0xc8,0xc9,0xca,0xcb, 0 };
        QChar i[] = { 'i', 0xec,0xed,0xee,0xef, 0 };
        QChar I[] = { 'I', 0xcc,0xcd,0xce,0xcf, 0 };
        QChar o[] = { 'o', 0xf2,0xf3,0xf4,0xf5,0xf8, 0 };
        QChar O[] = { 'O', 0xd2,0xd3,0xd4,0xd5,0xd8, 0 };
        QChar u[] = { 'u', 0xf9,0xfa,0xfb, 0 };
        QChar U[] = { 'U', 0xd9,0xda,0xdb, 0 };
        QChar nul[] = { 0 };
        QChar *replacements[] = { a, A, e, E, i, I, o, O, u, U, nul };

        for( uint i = 0; i < result.length(); i++ )
        {
            QChar c = result.ref( i );
            for( uint n = 0; replacements[n][0] != QChar(0); n++ )
            {
                for( uint k=0; replacements[n][k] != QChar(0); k++ )
                {
                    if( replacements[n][k] == c )
                    {
                        c = replacements[n][0];
                    }
                }
            }
            if( c > QChar(0x7f) || c == QChar(0) )
            {
                c = '_';
            }
            result.ref( i ) = c;
        }
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
