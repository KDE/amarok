#include "amarok.h"
#include "collectionbrowser.h"
#include "collectiondb.h"
#include "qstringx.h"


QString OrganizeCollectionDialog::buildDestination( const QString &format, const MetaBundle &mb ) const
{
    bool isCompilation = false;
    if( !mb.album().isEmpty() )
    {
        const int albumId = CollectionDB::instance()->albumID( mb.album() );
        isCompilation = CollectionDB::instance()->albumIsCompilation( QString::number(albumId) );
    }

    QMap<QString, QString> args;
    QString artist = mb.artist();
    QString albumartist = artist;
    if( isCompilation )
        albumartist = i18n( "Various Artists" );
    args["theartist"] = cleanPath( artist );
    args["thealbumartist"] = cleanPath( albumartist );
    if( ignoreTheCheck->isChecked() && artist.startsWith( "The " ) )
        CollectionView::instance()->manipulateThe( artist, true );
    artist = cleanPath( artist );
    if( ignoreTheCheck->isChecked() && albumartist.startsWith( "The " ) )
        CollectionView::instance()->manipulateThe( albumartist, true );
    albumartist = cleanPath( albumartist );
    for( int i = 0; i < MetaBundle::NUM_COLUMNS; i++ )
    {
        if( i == MetaBundle::Score || i == MetaBundle::PlayCount
            || i == MetaBundle::LastPlayed || i == MetaBundle::Mood )
            continue;
        args[mb.exactColumnName( i ).lower()] = cleanPath( mb.prettyText( i ) );
    }
    args["artist"] = artist;
    args["albumartist"] = albumartist;
    args["folder"] = folderCombo->currentText();
    args["initial"] = albumartist.mid( 0, 1 ).upper();
    args["filetype"] = mb.url().path().section( ".", -1 ).lower();
    QString track;
    if ( mb.track() )
        track.sprintf( "%02d", mb.track() );
    args["track"] = track;

    Amarok::QStringx formatx( format );
    QString result = formatx.namedOptArgs( args );
    if( result.startsWith( folderCombo->currentText() ) )
    {
        QString tail = result.mid( folderCombo->currentText().length() );
        if( !tail.startsWith( "/" ) )
            tail.prepend( "/" );
        return folderCombo->currentText() + tail.replace( QRegExp( "/\\.*" ), "/" );
    }
    return result.replace( QRegExp( "/\\.*" ), "/" );
}

QString OrganizeCollectionDialog::buildFormatTip() const
{
    QMap<QString, QString> args;
    for( int i = 0; i < MetaBundle::NUM_COLUMNS; i++ )
    {
        if( i == MetaBundle::Score || i == MetaBundle::PlayCount
            || i == MetaBundle::LastPlayed || i == MetaBundle::Mood )
            continue;
        args[MetaBundle::exactColumnName( i ).lower()] = MetaBundle::prettyColumnName( i );
    }
    args["albumartist"] = i18n( "%1 or %2" ).arg( i18n("This feature only works with \"The\", so either don't translate it at all, or only translate artist and album", "Album Artist, The") , i18n("The Album Artist") );
    args["thealbumartist"] = i18n( "The Album Artist" );
    args["theartist"] = i18n( "The Artist" );
    args["artist"] = i18n( "%1 or %2" ).arg( i18n( "This feature only works with \"The\", so either don't translate it at all, or only translate Artist", "Artist, The") , i18n( "The Artist") );
    args["folder"] = i18n( "Collection Base Folder" );
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
        tooltip += QString( "<li>%1 - %2" ).arg( it.data(), "%" + it.key() );
    }
    tooltip += "</ul>";

    tooltip += i18n( "If you surround sections of text that contain a token with curly-braces, "
            "that section will be hidden if the token is empty." );

    return tooltip;
}


QString OrganizeCollectionDialog::buildFormatString() const
{
    QString format = "%folder/";
    if( filetypeCheck->isChecked() )
        format += "%filetype/";
    if( initialCheck->isChecked() )
        format += "%initial/";

    format += "%albumartist/";
    if( spaceCheck->isChecked() )
    {
        format += "%album{_(Disc_%discnumber)}/";
        format += "{%track_-_}%title.%filetype";
    }
    else
    {
        format += "%album{ (Disc %discnumber)}/";
        format += "{%track - }%title.%filetype";
    }

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


QString OrganizeCollectionDialog::cleanPath( const QString &component ) const
{
    QString result = component;

    if( asciiCheck->isChecked() )
    {
        result = Amarok::cleanPath( result );
        result = Amarok::asciiPath( result );
    }

    if( !regexpEdit->text().isEmpty() )
        result.replace( QRegExp( regexpEdit->text() ), replaceEdit->text() );

    result.simplifyWhiteSpace();
    if( spaceCheck->isChecked() )
        result.replace( QRegExp( "\\s" ), "_" );
    if( vfatCheck->isChecked() )
        result = Amarok::vfatPath( result );

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
        formatHelp->show();
    }
    else
    {
        ignoreTheCheck->hide();
        customschemeCheck->hide();
        replacementGroup->hide();
        formatLabel->hide();
        formatEdit->hide();
        formatHelp->hide();
    }

    if( dynamic_cast<QWidget *>(parent()) ) {
        static_cast<QWidget *>(parent())->adjustSize();
        static_cast<QWidget *>(parent())->updateGeometry();
    }
}


void OrganizeCollectionDialog::init()
{
    detailed = true;

    formatHelp->setText( QString( "<a href='whatsthis:%1'>%2</a>" ).
            arg( Amarok::escapeHTMLAttr( buildFormatTip() ), i18n( "(Help)" ) ) );
}
