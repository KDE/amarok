/*
  (c) 2006 Roel Meeuws <r.j.meeuws+amarok@gmail.com>

 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/


/****************************************************************************
** ui.h extension file, included from the uic-generated form implementation.
**
** If you wish to add, delete or rename functions or slots use
** Qt Designer which will update this file, preserving your code. Create an
** init() function in place of a constructor, and a destroy() function in
** place of a destructor.
*****************************************************************************/


/// Configuration Dialog Extension slots

void
GenericMediaDeviceConfigDialog::addSupportedButtonClicked( int id )
{
    QPopupMenu *unsupported = m_addSupportedButton->popup();
    QComboBox *convert     = m_convertComboBox;
    QListBox  *supported   = m_supportedListBox;

    QString text = unsupported->text( id );
    if( text.startsWith( "&" ) )
        supported->insertItem( text.right( text.length() - 1 ) );
    else
        supported->insertItem( text );

    QString temp = convert->currentText();
    convert->insertItem( unsupported->text( id ) );

    unsupported->removeItem( id );

    supported->sort();
    convert->listBox()->sort();

    convert->setCurrentText( temp );
}


void
GenericMediaDeviceConfigDialog::removeSupportedButtonClicked()
{
    QStringList unsupported;

    QComboBox *convert     = m_convertComboBox;
    QListBox  *supported   = m_supportedListBox;

    for( uint i = 0; i < m_addSupportedButton->popup()->count(); i++ )
    {
        int id = m_addSupportedButton->popup()->idAt( i );
        unsupported.append( m_addSupportedButton->popup()->text( id ) );
    }

    for( uint i = 0; i < supported->count() ; /* nothing */)
    {
        QListBoxItem *item = supported->item( i );

        if( item->isSelected() )
        {
            QString temp;

            unsupported.append( item->text() );

            temp = convert->currentText();

            convert->setCurrentText( item->text() );
            convert->removeItem( convert->currentItem() );

            if( temp == item->text() )
                convert->setCurrentItem( 0 );
            else
                convert->setCurrentText( temp );

            item = 0;
            supported->removeItem( i );

            continue;
        }

        i++;
    }

    // at least support mp3 format.
    if( supported->count() <= 0 )
    {
        supported->insertItem( "mp3" );
        convert->insertItem( "mp3" );
        convert->setCurrentItem( 0 );
        unsupported.remove( "mp3" );
    }

    unsupported.sort();
    m_addSupportedButton->popup()->clear();
    for( QStringList::Iterator it = unsupported.begin(); it != unsupported.end(); it++ )
    {
        m_addSupportedButton->popup()->insertItem( *it );
    }
}


void
GenericMediaDeviceConfigDialog::supportedListBoxDoubleClicked( QListBoxItem* item )
{
    m_convertComboBox->setCurrentText( item->text() );
}




void
GenericMediaDeviceConfigDialog::updateConfigDialogLists( const QStringList & supportedFileTypes )
{
    QStringList allTypes;
    allTypes << "mp3" << "ogg" << "wma" << "mp4" << "aac" << "m4a" << "ac3";
    allTypes << "wav" << "flac" << "asf" << "asx" << "mpg" << "mp4v" << "mpeg";
    allTypes << "aa" << "3gp" << "mp2" << "ape" << "mpc";

    QStringList unsupported;
    QComboBox *convert      = m_convertComboBox;
    QListBox  *supported    = m_supportedListBox;

    for( QStringList::Iterator it = allTypes.begin(); it != allTypes.end(); it++ )
    {
        if( supportedFileTypes.contains( *it ) )
        {
            supported->insertItem( *it );
            convert->insertItem( *it );
        }
        else
        {
            unsupported.append( *it );
        }
    }

    supported->sort();
    unsupported.sort();

    m_addSupportedButton->popup()->clear();

    for( QStringList::Iterator it = unsupported.begin(); it != unsupported.end(); it++ )
    {
        m_addSupportedButton->popup()->insertItem( *it );
    }
    convert->listBox()->sort();
    convert->setCurrentText( supportedFileTypes.first() );
}

QString
GenericMediaDeviceConfigDialog::buildDestination( const QString &format, const MetaBundle &mb ) const
{
    bool isCompilation = mb.compilation() > 0;
    QMap<QString, QString> args;
    QString artist = mb.artist();
    QString albumartist = artist;
    if( isCompilation )
        albumartist = i18n( "Various Artists" );
    args["theartist"] = cleanPath( artist );
    args["thealbumartist"] = cleanPath( albumartist );
    if( m_ignoreTheCheck->isChecked() && artist.startsWith( "The " ) )
        CollectionView::instance()->manipulateThe( artist, true );
    artist = cleanPath( artist );
    if( m_ignoreTheCheck->isChecked() && albumartist.startsWith( "The " ) )
        CollectionView::instance()->manipulateThe( albumartist, true );

    albumartist = cleanPath( albumartist );
    for( int i = 0; i < MetaBundle::NUM_COLUMNS; i++ )
    {
        if( i == MetaBundle::Score || i == MetaBundle::PlayCount || i == MetaBundle::LastPlayed )
            continue;
        args[mb.exactColumnName( i ).lower()] = cleanPath( mb.prettyText( i ) );
    }
    args["artist"] = artist;
    args["albumartist"] = albumartist;
    args["initial"] = albumartist.mid( 0, 1 ).upper();
    args["filetype"] = mb.url().path().section( ".", -1 ).lower();
    QString track;
    if ( mb.track() )
        track.sprintf( "%02d", mb.track() );
    args["track"] = track;

    Amarok::QStringx formatx( format );
    QString result = m_device->mountPoint().append( formatx.namedOptArgs( args ) );
    QString tail = result.mid( m_device->mountPoint().length() );
    if( !tail.startsWith( "/" ) )
        tail.prepend( "/" );

   return m_device->mountPoint() + tail.replace( QRegExp( "/\\.*" ), "/" );
}

QString GenericMediaDeviceConfigDialog::cleanPath( const QString &component ) const
{
    QString result = Amarok::cleanPath( component );

    if( m_asciiCheck->isChecked() )
        result = Amarok::asciiPath( result );

    result.simplifyWhiteSpace();
    if( m_spaceCheck->isChecked() )
        result.replace( QRegExp( "\\s" ), "_" );
    if( m_device->m_actuallyVfat || m_vfatCheck->isChecked() )
        result = Amarok::vfatPath( result );

    result.replace( "/", "-" );

    return result;
}


void
GenericMediaDeviceConfigDialog::updatePreviewLabel()
{
    m_previewLabel->setText( buildDestination( m_songLocationBox->text(), *m_previewBundle ) );
}

void
GenericMediaDeviceConfigDialog::updatePreviewLabel( const QString& format)
{
    m_previewLabel->setText( buildDestination( format , *m_previewBundle ) );
}

void
GenericMediaDeviceConfigDialog::setDevice( GenericMediaDevice* device )
{
    m_device = device;
    m_songLocationBox->setText( m_device->m_songLocation );
    m_podcastLocationBox->setText( m_device->m_podcastLocation );

    updatePreviewLabel( m_device->m_songLocation );

    updateConfigDialogLists( m_device->m_supportedFileTypes );
    m_asciiCheck->setChecked( m_device->m_asciiTextOnly );
    m_vfatCheck->setChecked( m_device->m_vfatTextOnly );
    m_spaceCheck->setChecked( m_device->m_spacesToUnderscores );
    m_ignoreTheCheck->setChecked( m_device->m_ignoreThePrefix );
}

QString
GenericMediaDeviceConfigDialog::buildFormatTip() const
{
    QMap<QString, QString> args;
    for( int i = 0; i < MetaBundle::NUM_COLUMNS; i++ )
    {
        if( i == MetaBundle::Score || i == MetaBundle::PlayCount || i == MetaBundle::LastPlayed )
            continue;
        args[MetaBundle::exactColumnName( i ).lower()] = MetaBundle::prettyColumnName( i );
    }
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
        tooltip += QString( "<li>%1 - %2" ).arg( it.data(), "%" + it.key() );
    }
    tooltip += "</ul>";

    tooltip += i18n( "If you surround sections of text that contain a token with curly-braces, "
            "that section will be hidden if the token is empty." );

    return tooltip;
}

void
GenericMediaDeviceConfigDialog::init()
{
    m_previewBundle = new MetaBundle();
    m_previewBundle->setAlbum( AtomicString( "Some Album" ) );
    m_previewBundle->setArtist( AtomicString( "The One Artist" ) );
    m_previewBundle->setBitrate( 128 );
    m_previewBundle->setComment( AtomicString( "Some Comment" ) );
    m_previewBundle->setCompilation( 0 );
    m_previewBundle->setComposer( AtomicString( "The One Composer" ) );
    m_previewBundle->setDiscNumber( 1 );
    m_previewBundle->setFileType( 2 );
    m_previewBundle->setFilesize( 1003264 );
    m_previewBundle->setGenre( AtomicString( "Some Genre" ) );
    m_previewBundle->setLength( 193 );
    m_previewBundle->setPlayCount( 2 );
    m_previewBundle->setRating( 3 );
    m_previewBundle->setSampleRate( 44100 );
    m_previewBundle->setScore( 3.f );
    m_previewBundle->setTitle( AtomicString( "Some Title" ) );
    m_previewBundle->setTrack( 7 );
    m_previewBundle->setUrl( "/some%20directory/some%20file.mp3" );
    m_previewBundle->setYear( 2006 );

    m_formatHelp->setText( QString( "<a href='whatsthis:%1'>%2</a>" ).
            arg( Amarok::escapeHTMLAttr( buildFormatTip() ), i18n( "(Help)" ) ) );

    m_unsupportedMenu = new QPopupMenu( m_addSupportedButton, "unsupported" );

    m_addSupportedButton->setPopup( m_unsupportedMenu );

    connect( m_unsupportedMenu, SIGNAL( activated( int ) ), this, SLOT( addSupportedButtonClicked( int ) ) );
}


void GenericMediaDeviceConfigDialog::destroy()
{
    if( m_unsupportedMenu != NULL )
        delete m_unsupportedMenu;

    m_unsupportedMenu = NULL;
}
