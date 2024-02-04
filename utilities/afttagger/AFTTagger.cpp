/*
 *  Copyright (c) 2008-2009 Jeff Mitchell <mitchell@kde.org>
 *  QStringToTString and TStringToQString macros Copyright 2002-2008 by Scott Wheeler, wheeler@kde.org, licensed under LGPL 2.1
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include "AFTTagger.h"
#include "SafeFileSaver.h"

//Taglib
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wnon-virtual-dtor"
#include <apetag.h>
#include <fileref.h>
#include <flacfile.h>
#include <id3v2tag.h>
#include <mp4tag.h>
#include <mpegfile.h>
#include <oggfile.h>
#include <oggflacfile.h>
#include <speexfile.h>
#include <tstringlist.h>
#include <tfile.h>
#include <uniquefileidentifierframe.h>
#include <vorbisfile.h>
#include <xiphcomment.h>
#pragma GCC diagnostic pop

#include <QtDebug>
#include <QCryptographicHash>
#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QRandomGenerator>
#include <QString>
#include <QTextStream>

#include <iostream>

//QT5-happy versions
#define Qt5QStringToTString(s) TagLib::String(s.toUtf8().data(), TagLib::String::UTF8)

static int s_currentVersion = 1;

int main( int argc, char *argv[] )
{
    AFTTagger tagger( argc, argv );
    return tagger.exec();
}

AFTTagger::AFTTagger( int &argc, char **argv )
    :QCoreApplication( argc, argv )
    , m_delete( false )
    , m_newid( false )
    , m_quiet( false )
    , m_recurse( false )
    , m_verbose( false )
    , m_fileFolderList()
    , m_time()
    , m_textStream( stderr )
{

    setObjectName( QStringLiteral("amarok_afttagger") );

    readArgs();

    QString terms;
    if( !m_quiet )
    {
        m_textStream << qPrintable( tr( "TERMS OF USE:\n\n"
                "This program has been extensively tested and errs on the side of safety wherever possible.\n\n"
                "With that being said, since this program can modify thousands or hundreds of thousands of files\n"
                "at a time, here is the obligatory warning text:\n\n"
                "This program makes use of multiple libraries not written by the author, and as such neither\n"
                "the author nor the Amarok project can or do take any responsibility for any damage that may\n"
                "occur to your files through the use of this program.\n\n"
                "If you want more information, please see http://community.kde.org/Amarok/Development/AFT\n\n"
                "If you agree to be bound by these terms of use, enter 'y' or 'Y', or anything else to exit:\n" ) );

        m_textStream.flush();
        std::string response;
        std::cin >> response;
        std::cin.get();

        if( response != "y" && response != "Y")
        {
            m_textStream << tr( "INFO: Terms not accepted; exiting..." ) << Qt::endl;
            ::exit( 1 );
        }
    }

    m_time.start();

    foreach( const QString &path, m_fileFolderList )
        processPath( path );

    m_textStream << tr( "INFO: All done, exiting..." ) << Qt::endl;
    ::exit( 0 );
}

void
AFTTagger::processPath( const QString &path )
{
    QFileInfo info( path );
    if( !info.isDir() && !info.isFile() )
    {
        if( m_verbose )
            m_textStream << tr( "INFO: Skipping %1 because it is neither a directory nor file." ).arg( path ) << Qt::endl;
        return;
    }
    if( info.isDir() )
    {
        if( !m_recurse )
        {
            if( m_verbose )
               m_textStream << tr( "INFO: Skipping %1 because it is a directory and recursion is not specified." ).arg( path ) << Qt::endl;
            return;
        }
        else
        {
            if( m_verbose )
                m_textStream << tr( "INFO: Processing directory %1" ).arg( path ) << Qt::endl;
            foreach( const QString &pathEntry, QDir( path ).entryList() )
            {
                if( pathEntry != QLatin1String(".") && pathEntry != QLatin1String("..") )
                    processPath( QDir( path ).canonicalPath() + QLatin1Char('/') +  pathEntry );
            }
        }
    }
    else //isFile()
    {
        QString filePath = info.absoluteFilePath();


#ifdef COMPLEX_TAGLIB_FILENAME
    const wchar_t *encodedName = reinterpret_cast< const wchar_t *>(filePath.utf16());
#else
    QByteArray fileName = QFile::encodeName( filePath );
    const char *encodedName = fileName.constData();
#endif

        TagLib::FileRef fileRef = TagLib::FileRef( encodedName, true, TagLib::AudioProperties::Fast );

        if( fileRef.isNull() )
        {
            if( m_verbose )
                m_textStream << tr( "INFO: file %1 not able to be opened by TagLib" ).arg( filePath ) << Qt::endl;
            return;
        }

        m_textStream << tr( "INFO: Processing file %1" ).arg( filePath ) << Qt::endl;

        SafeFileSaver sfs( filePath );
        sfs.setVerbose( false );
        sfs.setPrefix( QStringLiteral("amarok-afttagger") );
        QString tempFilePath = sfs.prepareToSave();
        if( tempFilePath.isEmpty() )
        {
            m_textStream << tr( "Error: could not create temporary file when processing %1" ).arg( filePath ) << Qt::endl;
            return;
        }

        if( m_verbose )
            m_textStream << tr( "INFO: Temporary file is at %1").arg( tempFilePath ) << Qt::endl;

#ifdef COMPLEX_TAGLIB_FILENAME
    const wchar_t *encodedName = reinterpret_cast< const wchar_t * >(tempFilePath.utf16());
#else
    QByteArray tempFileName = QFile::encodeName( tempFilePath );
    const char *tempEncodedName = tempFileName.constData();
#endif

        bool saveNecessary = false;

        TagLib::FileRef tempFileRef = TagLib::FileRef( tempEncodedName, true, TagLib::AudioProperties::Fast );
        if( TagLib::MPEG::File *file = dynamic_cast<TagLib::MPEG::File *>( tempFileRef.file() ) )
            saveNecessary = handleMPEG( file );
        else if( TagLib::Ogg::File *file = dynamic_cast<TagLib::Ogg::File *>( tempFileRef.file() ) )
            saveNecessary = handleOgg( file );
        else if( TagLib::FLAC::File *file = dynamic_cast<TagLib::FLAC::File *>( tempFileRef.file() ) )
            saveNecessary = handleFLAC( file );
        else if( TagLib::MPC::File *file = dynamic_cast<TagLib::MPC::File *>( tempFileRef.file() ) )
            saveNecessary = handleMPC( file );
        else if( TagLib::MP4::File *file = dynamic_cast<TagLib::MP4::File *>( tempFileRef.file() ) )
            saveNecessary = handleMP4( file );
        else
        {
            if( m_verbose )
                m_textStream << tr( "INFO: File not able to be parsed by TagLib or wrong kind (currently this program only supports MPEG, Ogg MP4, MPC, and FLAC files), cleaning up temp file" ) << Qt::endl;
            if( !sfs.cleanupSave() )
                m_textStream << tr( "WARNING: file at %1 could not be cleaned up; check for strays" ).arg( filePath ) << Qt::endl;
            return;
        }
        if( saveNecessary )
        {
            if( m_verbose )
                m_textStream << tr( "INFO: Safe-saving file" ) << Qt::endl;
            if( !sfs.doSave() )
                m_textStream << tr( "WARNING: file at %1 could not be saved" ).arg( filePath ) << Qt::endl;
        }
        if( m_verbose )
            m_textStream << tr( "INFO: Cleaning up..." ) << Qt::endl;
        if( !sfs.cleanupSave() )
            m_textStream << tr( "WARNING: file at %1 could not be cleaned up; check for strays" ).arg( filePath ) << Qt::endl;
        return;
    }
}

bool
AFTTagger::handleMPEG( TagLib::MPEG::File *file )
{
    if( file->readOnly() )
    {
        m_textStream << tr( "ERROR: File is read-only or could not be opened" ) << Qt::endl;
        return false;
    }

    QString uid;
    bool newUid = false;
    bool nothingfound = true;
    if( m_verbose )
        m_textStream << tr( "INFO: File is a MPEG file, opening..." ) << Qt::endl;
    if ( file->ID3v2Tag( true ) )
    {
        if( file->ID3v2Tag()->frameListMap()["UFID"].isEmpty() )
        {
            if( m_verbose )
                m_textStream << tr( "INFO: No UFID frames found" ) << Qt::endl;

            if( m_delete )
                return false;

            newUid = true;
        }
        else
        {
            if( m_verbose )
                m_textStream << tr( "INFO: Found existing UFID frames, parsing" )  << Qt::endl;
            TagLib::ID3v2::FrameList frameList = file->ID3v2Tag()->frameListMap()["UFID"];
            TagLib::ID3v2::FrameList::Iterator iter;
            if( m_verbose )
                m_textStream << tr( "INFO: Frame list size is %1" ).arg( frameList.size() ) << Qt::endl;
            for( iter = frameList.begin(); iter != frameList.end(); ++iter )
            {
                TagLib::ID3v2::UniqueFileIdentifierFrame* currFrame = dynamic_cast<TagLib::ID3v2::UniqueFileIdentifierFrame*>(*iter);
                if( currFrame )
                {
                    QString owner = TStringToQString( currFrame->owner() ).toUpper();
                    if( owner.startsWith( QLatin1String("AMAROK - REDISCOVER YOUR MUSIC") ) )
                    {
                        nothingfound = false;
                        if( m_verbose )
                            m_textStream << tr( "INFO: Removing old-style ATF identifier" ) << Qt::endl;

                        iter = frameList.erase( iter );
                        file->ID3v2Tag()->removeFrame( currFrame );
                        file->save();
                        if( !m_delete )
                            newUid = true;
                        else
                            return true;
                    }
                    if( owner.startsWith( QLatin1String("AMAROK 2 AFT") ) )
                    {
                        nothingfound = false;
                        if( m_verbose )
                            m_textStream << tr( "INFO: Found an existing AFT identifier: %1" ).arg( TStringToQString( TagLib::String( currFrame->identifier() ) ) ) << Qt::endl;

                        if( m_delete )
                        {
                            iter = frameList.erase( iter );
                            if( m_verbose )
                                m_textStream << tr( "INFO: Removing current AFT frame" ) << Qt::endl;
                            file->ID3v2Tag()->removeFrame( currFrame );
                            file->save();
                            return true;
                        }

                        int version = owner.at( 13 ).digitValue();
                        if( version < s_currentVersion )
                        {
                            if( m_verbose )
                                m_textStream << tr( "INFO: Upgrading AFT identifier from version %1 to version %2" ).arg( version, s_currentVersion ) << Qt::endl;
                            uid = upgradeUID( version, TStringToQString( TagLib::String( currFrame->identifier() ) ) );
                            if( m_verbose )
                                m_textStream << tr( "INFO: Removing current AFT frame" ) << Qt::endl;
                            iter = frameList.erase( iter );
                            file->ID3v2Tag()->removeFrame( currFrame );
                            newUid = true;
                        }
                        else if( version == s_currentVersion && m_newid )
                        {
                            if( m_verbose )
                                m_textStream << tr( "INFO: New IDs specified to be generated, doing so" ) << Qt::endl;
                            iter = frameList.erase( iter );
                            file->ID3v2Tag()->removeFrame( currFrame );
                            newUid = true;
                        }
                        else
                        {
                            if( m_verbose )
                                m_textStream << tr( "INFO: ID is current" ) << Qt::endl;
                        }
                    }
                }
            }
        }
        if( newUid || ( nothingfound && !m_delete ) )
        {
            QString ourId = QString( "Amarok 2 AFTv" + QString::number( s_currentVersion ) + " - amarok.kde.org" );
            if( uid.isEmpty() )
                uid = createCurrentUID( file );
            if( m_verbose )
                m_textStream << tr( "INFO: Adding new frame and saving file with UID: %1" ).arg( uid ) << Qt::endl;
            file->ID3v2Tag()->addFrame( new TagLib::ID3v2::UniqueFileIdentifierFrame(
                Qt5QStringToTString( ourId ), Qt5QStringToTString( uid ).data( TagLib::String::Latin1 ) ) );
            file->save();
            return true;
        }
    }
    return false;
}

bool
AFTTagger::handleOgg( TagLib::Ogg::File *file )
{
    if( file->readOnly() )
    {
        m_textStream << tr( "ERROR: File is read-only or could not be opened" ) << Qt::endl;
        return false;
    }

    TagLib::Ogg::XiphComment *comment = dynamic_cast<TagLib::Ogg::XiphComment *>( file->tag() );

    if( !comment )
        return false;

    if( handleXiphComment( comment, file ) )
    {
        file->save();
        return true;
    }

    return false;
}

bool
AFTTagger::handleFLAC( TagLib::FLAC::File *file )
{
    if( file->readOnly() )
    {
        m_textStream << tr( "ERROR: File is read-only or could not be opened" ) << Qt::endl;
        return false;
    }

    TagLib::Ogg::XiphComment *comment = file->xiphComment( true );
    if( !comment )
        return false;

    if( handleXiphComment( comment, file ) )
    {
        file->save();
        return true;
    }

    return false;
}

bool
AFTTagger::handleXiphComment( TagLib::Ogg::XiphComment *comment, TagLib::File *file )
{
    QString uid;
    bool newUid = false;
    bool nothingfound = true;
    TagLib::StringList toRemove;
    if( m_verbose )
        m_textStream << tr( "INFO: File has a XiphComment, opening..." ) << Qt::endl;

    if( comment->fieldListMap().isEmpty() )
    {
        if( m_verbose )
            m_textStream << tr( "INFO: No fields found in XiphComment" ) << Qt::endl;

        if( m_delete )
            return false;
    }
    else
    {
        if( m_verbose )
            m_textStream << tr( "INFO: Found existing XiphComment frames, parsing" )  << Qt::endl;
        TagLib::Ogg::FieldListMap fieldListMap = comment->fieldListMap();

        if( m_verbose )
            m_textStream << tr( "INFO: fieldListMap size is %1" ).arg( fieldListMap.size() ) << Qt::endl;

        TagLib::Ogg::FieldListMap::Iterator iter;
        for( iter = fieldListMap.begin(); iter != fieldListMap.end(); ++iter )
        {
            TagLib::String key = iter->first;
            QString qkey = TStringToQString( key ).toUpper();
            if( qkey.startsWith( QLatin1String("AMAROK - REDISCOVER YOUR MUSIC") ) )
            {
                nothingfound = false;

                if( m_verbose )
                    m_textStream << tr( "INFO: Removing old-style ATF identifier %1" ).arg( qkey ) << Qt::endl;

                toRemove.append( key );
                if( !m_delete )
                    newUid = true;
            }
            else if( qkey.startsWith( QLatin1String("AMAROK 2 AFT") ) )
            {
                nothingfound = false;

                if( m_verbose )
                    m_textStream << tr( "INFO: Found an existing AFT identifier: %1" ).arg( qkey ) << Qt::endl;

                if( m_delete )
                {
                    toRemove.append( key );
                    if( m_verbose )
                        m_textStream << tr( "INFO: Removing current AFT frame" ) << Qt::endl;
                }
                else
                {
                    int version = qkey.at( 13 ).digitValue();
                    if( m_verbose )
                        m_textStream << tr( "INFO: AFT identifier is version %1" ).arg( version ) << Qt::endl;
                    if( version < s_currentVersion )
                    {
                        if( m_verbose )
                            m_textStream << tr( "INFO: Upgrading AFT identifier from version %1 to version %2" ).arg( version, s_currentVersion ) << Qt::endl;
                        uid = upgradeUID( version, TStringToQString( fieldListMap[key].front() ) );
                        if( m_verbose )
                            m_textStream << tr( "INFO: Removing current AFT frame" ) << Qt::endl;
                        toRemove.append( key );
                        newUid = true;
                    }
                    else if( version == s_currentVersion && m_newid )
                    {
                        if( m_verbose )
                            m_textStream << tr( "INFO: New IDs specified to be generated, doing so" ) << Qt::endl;
                        toRemove.append( key );
                        newUid = true;
                    }
                    else
                    {
                        if( m_verbose )
                            m_textStream << tr( "INFO: ID is current" ) << Qt::endl;
                        return false;
                    }
                }
            }
        }
        for( TagLib::StringList::ConstIterator iter = toRemove.begin(); iter != toRemove.end(); ++iter )
            comment->removeFields( *iter );
    }
    if( newUid || ( nothingfound && !m_delete ) )
    {
        QString ourId = QString( "Amarok 2 AFTv" + QString::number( s_currentVersion ) + " - amarok.kde.org" );
        if( uid.isEmpty() )
            uid = createCurrentUID( file );
        if( m_verbose )
            m_textStream << tr( "INFO: Adding new field and saving file with UID: %1" ).arg( uid ) << Qt::endl;
        comment->addField( Qt5QStringToTString( ourId ), Qt5QStringToTString( uid ) );
        return true;
    }
    else if( toRemove.size() )
        return true;

    return false;
}

bool
AFTTagger::handleMPC( TagLib::MPC::File *file )
{
    if( file->readOnly() )
    {
        m_textStream << tr( "ERROR: File is read-only or could not be opened" ) << Qt::endl;
        return false;
    }

    QString uid;
    bool newUid = false;
    bool nothingfound = true;
    TagLib::StringList toRemove;
    if( m_verbose )
        m_textStream << tr( "INFO: File is a MPC file, opening..." ) << Qt::endl;

    if( file->APETag() )
    {
        const TagLib::APE::ItemListMap &itemsMap = file->APETag()->itemListMap();
        if( itemsMap.isEmpty() )
        {
          m_textStream << tr( "INFO: No fields found in APE tags." ) << Qt::endl;

          if( m_delete )
              return false;
        }

        for( TagLib::APE::ItemListMap::ConstIterator it = itemsMap.begin(); it != itemsMap.end(); ++it )
        {
            TagLib::String key = it->first;
            QString qkey = TStringToQString( key ).toUpper();
            if( qkey.startsWith( QLatin1String("AMAROK - REDISCOVER YOUR MUSIC") ) )
            {
                nothingfound = false;

                if( m_verbose )
                    m_textStream << tr( "INFO: Removing old-style ATF identifier %1" ).arg( qkey ) << Qt::endl;

                toRemove.append( key );
                if( !m_delete )
                    newUid = true;
            }
            else if( qkey.startsWith( QLatin1String("AMAROK 2 AFT") ) )
            {
                nothingfound = false;

                if( m_verbose )
                    m_textStream << tr( "INFO: Found an existing AFT identifier: %1" ).arg( qkey ) << Qt::endl;

                if( m_delete )
                {
                    toRemove.append( key );
                    if( m_verbose )
                        m_textStream << tr( "INFO: Removing current AFT frame" ) << Qt::endl;
                }
                else
                {
                    int version = qkey.at( 13 ).digitValue();
                    if( m_verbose )
                        m_textStream << tr( "INFO: AFT identifier is version %1" ).arg( version ) << Qt::endl;
                    if( version < s_currentVersion )
                    {
                        if( m_verbose )
                            m_textStream << tr( "INFO: Upgrading AFT identifier from version %1 to version %2" ).arg( version, s_currentVersion ) << Qt::endl;
                        uid = upgradeUID( version, TStringToQString( itemsMap[ key ].toString() ) );
                        if( m_verbose )
                            m_textStream << tr( "INFO: Removing current AFT frame" ) << Qt::endl;
                        toRemove.append( key );
                        newUid = true;
                    }
                    else if( version == s_currentVersion && m_newid )
                    {
                        if( m_verbose )
                            m_textStream << tr( "INFO: New IDs specified to be generated, doing so" ) << Qt::endl;
                        toRemove.append( key );
                        newUid = true;
                    }
                    else
                    {
                        if( m_verbose )
                            m_textStream << tr( "INFO: ID is current" ) << Qt::endl;
                        return false;
                    }
                }
            }
        }
        for( TagLib::StringList::ConstIterator it = toRemove.begin(); it != toRemove.end(); ++it )
            file->APETag()->removeItem( *it );
    }

    if( newUid || ( nothingfound && !m_delete ) )
    {
        QString ourId = QString( "Amarok 2 AFTv" + QString::number( s_currentVersion ) + " - amarok.kde.org" );
        if( uid.isEmpty() )
            uid = createCurrentUID( file );
        if( m_verbose )
            m_textStream << tr( "INFO: Adding new field and saving file with UID: %1" ).arg( uid ) << Qt::endl;
        file->APETag()->addValue( Qt5QStringToTString( ourId.toUpper() ), Qt5QStringToTString( uid ) );
        file->save();
        return true;
    }
    else if( toRemove.size() )
    {
        file->save();
        return true;
    }

    return false;
}

bool
AFTTagger::handleMP4( TagLib::MP4::File *file )
{
    if( file->readOnly() )
    {
        m_textStream << tr( "ERROR: File is read-only or could not be opened" ) << Qt::endl;
        return false;
    }

    QString uid;
    bool newUid = false;
    bool nothingfound = true;
    TagLib::StringList toRemove;
    if( m_verbose )
        m_textStream << tr( "INFO: File is a MP4 file, opening..." ) << Qt::endl;

    TagLib::MP4::ItemMap itemsMap = file->tag()->itemMap();
    if( !itemsMap.isEmpty() )
    {
        for( TagLib::MP4::ItemMap::Iterator it = itemsMap.begin(); it != itemsMap.end(); ++it )
        {
            TagLib::String key = it->first;
            const QString qkey = TStringToQString( key ).toUpper();
            if( qkey.contains( QLatin1String("AMAROK - REDISCOVER YOUR MUSIC") ) )
            {
                nothingfound = false;

                if( m_verbose )
                    m_textStream << tr( "INFO: Removing old-style ATF identifier %1" ).arg( key.toCString() ) << Qt::endl;

                toRemove.append( key );
                if( !m_delete )
                    newUid = true;
            }
            else if( qkey.contains( QLatin1String("AMAROK 2 AFT") ) )
            {
                nothingfound = false;

                if( m_verbose )
                    m_textStream << tr( "INFO: Found an existing AFT identifier: %1" ).arg( key.toCString() ) << Qt::endl;

                if( m_delete )
                {
                    toRemove.append( key );
                    if( m_verbose )
                        m_textStream << tr( "INFO: Removing current AFT frame" ) << Qt::endl;
                }
                else
                {
                    int version = qkey.at( qkey.indexOf( QLatin1String("AMAROK 2 AFT") ) + 13 ).digitValue();
                    if( m_verbose )
                        m_textStream << tr( "INFO: AFT identifier is version %1" ).arg( version ) << Qt::endl;
                    if( version < s_currentVersion )
                    {
                        if( m_verbose )
                            m_textStream << tr( "INFO: Upgrading AFT identifier from version %1 to version %2" )
                            .arg( QString::number( version ), QString::number( s_currentVersion ) )
                            << Qt::endl;
                        uid = upgradeUID( version, TStringToQString( itemsMap[ key ].toStringList().toString() ) );
                        if( m_verbose )
                            m_textStream << tr( "INFO: Removing current AFT frame" ) << Qt::endl;
                        toRemove.append( key );
                        newUid = true;
                    }
                    else if( version == s_currentVersion && m_newid )
                    {
                        if( m_verbose )
                            m_textStream << tr( "INFO: New IDs specified to be generated, doing so" ) << Qt::endl;
                        toRemove.append( key );
                        newUid = true;
                    }
                    else
                    {
                        if( m_verbose )
                            m_textStream << tr( "INFO: ID is current" ) << Qt::endl;
                        return false;
                    }
                }
            }
        }
        for( TagLib::StringList::ConstIterator it = toRemove.begin(); it != toRemove.end(); ++it )
            itemsMap.erase( *it );
    }

    if( newUid || ( nothingfound && !m_delete ) )
    {
        QString ourId = QString( "Amarok 2 AFTv" + QString::number( s_currentVersion ) + " - amarok.kde.org" );
        if( uid.isEmpty() )
            uid = createCurrentUID( file );
        if( m_verbose )
            m_textStream << tr( "INFO: Adding new field and saving file with UID: %1" ).arg( uid ) << Qt::endl;
        itemsMap.insert( Qt5QStringToTString( QString( "----:com.apple.iTunes:" + ourId ) ),
                         TagLib::StringList( Qt5QStringToTString( uid ) ) );
        file->save();
        return true;
    }
    else if( toRemove.size() )
    {
        file->save();
        return true;
    }

    return false;
}



QString
AFTTagger::createCurrentUID( TagLib::File *file )
{
    return createV1UID( file );
}

QString
AFTTagger::createV1UID( TagLib::File *file )
{
    QCryptographicHash md5( QCryptographicHash::Md5 );
    QByteArray size;
    md5.addData( size.setNum( (qulonglong)(file->length()) ) );
    md5.addData( QString::number( m_time.elapsed() ).toUtf8() );
    md5.addData( QString::number( QRandomGenerator::global()->generate() ).toUtf8() );
    md5.addData( QString::number( QRandomGenerator::global()->generate() ).toUtf8() );
    md5.addData( QString::number( QRandomGenerator::global()->generate() ).toUtf8() );
    md5.addData( QString::number( QRandomGenerator::global()->generate() ).toUtf8() );
    md5.addData( QString::number( QRandomGenerator::global()->generate() ).toUtf8() );
    md5.addData( QString::number( m_time.elapsed() ).toUtf8() );
    return QString( md5.result().toHex() );
}

QString
AFTTagger::upgradeUID( int version, const QString &currValue )
{
    Q_UNUSED(version)
    return currValue + "abcd";
}

void
AFTTagger::readArgs()
{
    QStringList argslist = arguments();
    if( argslist.size() < 2 )
        displayHelp();
    bool nomore = false;
    int argnum = 0;
    foreach( const QString &arg, argslist )
    {
        ++argnum;
        if( arg.isEmpty() || argnum == 1 )
            continue;
        if( nomore )
        {
            m_fileFolderList.append( arg );
        }
        else if( arg.startsWith( QLatin1String("--") ) )
        {
            QString myarg = QString( arg ).remove( 0, 2 );
            if ( myarg == QLatin1String("recurse") || myarg == QLatin1String("recursively") )
                m_recurse = true;
            else if( myarg == QLatin1String("verbose") )
                m_verbose = true;
            else if( myarg == QLatin1String("quiet") )
                m_quiet = true;
            else if( myarg == QLatin1String("newid") )
                m_newid = true;
            else if( myarg == QLatin1String("delete") )
                m_delete = true;
            else
                displayHelp();
        }
        else if( arg.startsWith( '-' ) )
        {
            QString myarg = QString( arg ).remove( 0, 1 );
            int pos = 0;
            while( pos < myarg.length() )
            {
                if( myarg[pos] == 'd' )
                    m_delete = true;
                else if( myarg[pos] == 'n' )
                    m_newid = true;
                else if( myarg[pos] == 'q' )
                    m_quiet = true;
                else if( myarg[pos] == 'r' )
                    m_recurse = true;
                else if( myarg[pos] == 'v' )
                    m_verbose = true;
                else
                    displayHelp();

                ++pos;
            }
        }
        else
        {
            nomore = true;
            m_fileFolderList.append( arg );
        }
    }
}

void
AFTTagger::displayHelp()
{
    m_textStream << tr( "Amarok AFT Tagger" ) << Qt::endl << Qt::endl;
    m_textStream << tr( "IRC:\nserver: irc.libera.chat / channels: #amarok, #amarok-de, #amarok-es, #amarok-fr\n\nFeedback:\namarok@kde.org" ) << Qt::endl << Qt::endl;
    m_textStream << tr( "Usage: amarok_afttagger [options] +File/Folder(s)" ) << Qt::endl << Qt::endl;
    m_textStream << tr( "User-modifiable Options:" ) << Qt::endl;
    m_textStream << tr( "+File/Folder(s)       : Files or folders to tag" ) << Qt::endl;
    m_textStream << tr( "-h, --help            : This help text" ) << Qt::endl;
    m_textStream << tr( "-r, --recursive       : Process files and folders recursively" ) << Qt::endl;
    m_textStream << tr( "-d, --delete          : Remove AFT tag" ) << Qt::endl;
    m_textStream << tr( "-n  --newid           : Replace any existing ID with a new one" ) << Qt::endl;
    m_textStream << tr( "-v, --verbose         : Verbose output" ) << Qt::endl;
    m_textStream << tr( "-q, --quiet           : Quiet output; Implies that you accept the terms of use" ) << Qt::endl;
    m_textStream.flush();
    ::exit( 0 );
}
