/*
 *  Copyright (c) 2008-2009 Jeff Mitchell <mitchell@kde.org>
 *  Qt4QStringToTString and TStringToQString macros Copyright 2002-2008 by Scott Wheeler, wheeler@kde.org, licensed under LGPL 2.1
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
#include <fileref.h>
#include <flacfile.h>
#include <id3v2tag.h>
#include <mpegfile.h>
#include <oggfile.h>
#include <oggflacfile.h>
#include <speexfile.h>
#include <tstringlist.h>
#include <tfile.h>
#include <uniquefileidentifierframe.h>
#include <vorbisfile.h>
#include <xiphcomment.h>

#include <QtDebug>
#include <QCryptographicHash>
#include <QDir>
#include <QFileInfo>
#include <QString>
#include <QTextStream>

#include <iostream>

//QT4-happy versions
#define Qt4QStringToTString(s) TagLib::String(s.toUtf8().data(), TagLib::String::UTF8)

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
    
    setObjectName( "amarok_afttagger" );
    
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
                "If you want more information, please see http://amarok.kde.org/wiki/AFT\n\n"
                "If you agree to be bound by these terms of use, enter 'y' or 'Y', or anything else to exit:\n" ) );

        m_textStream.flush();
        std::string response;
        std::cin >> response;
        cin.get();

        if( response != "y" && response != "Y")
        {
            m_textStream << qPrintable( tr( "INFO: Terms not accepted; exiting..." ) ) << endl;
            ::exit( 1 );
        }
    }
    
    qsrand(QDateTime::currentDateTime().toTime_t());
    m_time.start();

    foreach( const QString &path, m_fileFolderList )
        processPath( path );

    m_textStream << qPrintable( tr( "INFO: All done, exiting..." ) ) << endl;
    ::exit( 0 );
}

void
AFTTagger::processPath( const QString &path )
{
    QFileInfo info( path );
    if( !info.isDir() && !info.isFile() )
    {
        if( m_verbose )
            m_textStream << qPrintable( tr( "INFO: Skipping %1 because it is neither a directory nor file." ).arg( path ) ) << endl;
        return;
    }
    if( info.isDir() )
    {
        if( !m_recurse )
        {
            if( m_verbose )
               m_textStream << qPrintable( tr( "INFO: Skipping %1 because it is a directory and recursion is not specified." ).arg( path ) ) << endl;
            return;
        }
        else
        {
            if( m_verbose )
                m_textStream << qPrintable( tr( "INFO: Processing directory %1" ).arg( path ) ) << endl;
            foreach( const QString &pathEntry, QDir( path ).entryList() )
            {
                if( pathEntry != "." && pathEntry != ".." )
                    processPath( QDir( path ).canonicalPath() + '/' +  pathEntry );
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
                m_textStream << qPrintable( tr( "INFO: file %1 not able to be opened by TagLib" ).arg( filePath ) ) << endl;
            return;
        }

        m_textStream << qPrintable( tr( "INFO: Processing file %1" ).arg( filePath ) ) << endl;

        SafeFileSaver sfs( filePath );
        sfs.setVerbose( false );
        sfs.setPrefix( "amarok-afttagger" );
        QString tempFilePath = sfs.prepareToSave();
        if( tempFilePath.isEmpty() )
        {
            m_textStream << qPrintable( tr( "Error: could not create temporary file when processing %1" ).arg( filePath ) ) << endl;
            return;
        }

        if( m_verbose )
            m_textStream << qPrintable( tr( "INFO: Temporary file is at %1").arg( tempFilePath ) ) << endl;

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
        else
        {
            if( m_verbose )
                m_textStream << qPrintable( tr( "INFO: File not able to be parsed by TagLib or wrong kind (currently this program only supports MPEG, Ogg, and FLAC files), cleaning up temp file" ) ) << endl;
            if( !sfs.cleanupSave() )
                m_textStream << qPrintable( tr( "WARNING: file at %1 could not be cleaned up; check for strays" ).arg( filePath ) ) << endl;
            return;
        }
        if( saveNecessary )
        {
            if( m_verbose )
                m_textStream << qPrintable( tr( "INFO: Safe-saving file" ) ) << endl;
            if( !sfs.doSave() )
                m_textStream << qPrintable( tr( "WARNING: file at %1 could not be saved" ).arg( filePath ) ) << endl;
        }
        if( m_verbose )
            m_textStream << qPrintable( tr( "INFO: Cleaning up..." ) ) << endl;
        if( !sfs.cleanupSave() )
            m_textStream << qPrintable( tr( "WARNING: file at %1 could not be cleaned up; check for strays" ).arg( filePath ) ) << endl;
        return;
    }
}

bool
AFTTagger::handleMPEG( TagLib::MPEG::File *file )
{
    if( file->readOnly() )
    {
        m_textStream << qPrintable( tr( "ERROR: File is read-only or could not be opened" ) ) << endl;
        return false;
    }
    
    QString uid;
    bool newUid = false;
    bool nothingfound = true;
    if( m_verbose )
        m_textStream << qPrintable( tr( "INFO: File is a MPEG file, opening..." ) ) << endl;
    if ( file->ID3v2Tag( true ) )
    {
        if( file->ID3v2Tag()->frameListMap()["UFID"].isEmpty() )
        {
            if( m_verbose )
                m_textStream << qPrintable( tr( "INFO: No UFID frames found" ) ) << endl;

            if( m_delete )
                return false;

            newUid = true;
        }
        else
        {
            if( m_verbose )
                m_textStream << qPrintable( tr( "INFO: Found existing UFID frames, parsing" ) )  << endl;
            TagLib::ID3v2::FrameList frameList = file->ID3v2Tag()->frameListMap()["UFID"];
            TagLib::ID3v2::FrameList::Iterator iter;
            if( m_verbose )
                m_textStream << qPrintable( tr( "INFO: Frame list size is %1" ).arg( frameList.size() ) ) << endl;
            for( iter = frameList.begin(); iter != frameList.end(); ++iter )
            {
                TagLib::ID3v2::UniqueFileIdentifierFrame* currFrame = dynamic_cast<TagLib::ID3v2::UniqueFileIdentifierFrame*>(*iter);
                if( currFrame )
                {
                    QString owner = TStringToQString( currFrame->owner() ).toUpper();
                    if( owner.startsWith( "AMAROK - REDISCOVER YOUR MUSIC" ) )
                    {
                        nothingfound = false;
                        if( m_verbose )
                            m_textStream << qPrintable( tr( "INFO: Removing old-style ATF identifier" ) ) << endl;

                        iter = frameList.erase( iter );
                        file->ID3v2Tag()->removeFrame( currFrame );
                        file->save();
                        if( !m_delete )
                            newUid = true;
                        else
                            return true;
                    }
                    if( owner.startsWith( "AMAROK 2 AFT" ) )
                    {
                        nothingfound = false;
                        if( m_verbose )
                            m_textStream << qPrintable( tr( "INFO: Found an existing AFT identifier: %1" ).arg( TStringToQString( TagLib::String( currFrame->identifier() ) ) ) ) << endl;

                        if( m_delete )
                        {
                            iter = frameList.erase( iter );
                            if( m_verbose )
                                m_textStream << qPrintable( tr( "INFO: Removing current AFT frame" ) ) << endl;
                            file->ID3v2Tag()->removeFrame( currFrame );
                            file->save();
                            return true;
                        }

                        int version = owner.at( 13 ).digitValue();
                        if( version < s_currentVersion )
                        {
                            if( m_verbose )
                                m_textStream << qPrintable( tr( "INFO: Upgrading AFT identifier from version %1 to version %2" ).arg( version, s_currentVersion ) ) << endl;
                            uid = upgradeUID( version, TStringToQString( TagLib::String( currFrame->identifier() ) ) );
                            if( m_verbose )
                                m_textStream << qPrintable( tr( "INFO: Removing current AFT frame" ) ) << endl;
                            iter = frameList.erase( iter );
                            file->ID3v2Tag()->removeFrame( currFrame );
                            newUid = true;
                        }
                        else if( version == s_currentVersion && m_newid )
                        {
                            if( m_verbose )
                                m_textStream << qPrintable( tr( "INFO: New IDs specified to be generated, doing so" ) ) << endl;
                            iter = frameList.erase( iter );
                            file->ID3v2Tag()->removeFrame( currFrame );
                            newUid = true;
                        }
                        else
                        {
                            if( m_verbose )
                                m_textStream << qPrintable( tr( "INFO: ID is current" ) ) << endl;
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
                m_textStream << qPrintable( tr( "INFO: Adding new frame and saving file with UID: %1" ).arg( uid ) ) << endl;
            file->ID3v2Tag()->addFrame( new TagLib::ID3v2::UniqueFileIdentifierFrame(
                Qt4QStringToTString( ourId ), Qt4QStringToTString( uid ).data( TagLib::String::Latin1 ) ) );
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
        m_textStream << qPrintable( tr( "ERROR: File is read-only or could not be opened" ) ) << endl;
        return false;
    }
    
    TagLib::Ogg::XiphComment *comment = 0;
    if( dynamic_cast<TagLib::Ogg::FLAC::File*>(file) )
        comment = ( dynamic_cast<TagLib::Ogg::FLAC::File*>(file) )->tag();
    else if( dynamic_cast<TagLib::Ogg::Speex::File*>(file) )
        comment = ( dynamic_cast<TagLib::Ogg::Speex::File*>(file) )->tag();
    else if( dynamic_cast<TagLib::Ogg::Vorbis::File*>(file) )
        comment = ( dynamic_cast<TagLib::Ogg::Vorbis::File*>(file) )->tag();

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
        m_textStream << qPrintable( tr( "ERROR: File is read-only or could not be opened" ) ) << endl;
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
        m_textStream << qPrintable( tr( "INFO: File has a XiphComment, opening..." ) ) << endl;

    if( comment->fieldListMap().isEmpty() )
    {
        if( m_verbose )
            m_textStream << qPrintable( tr( "INFO: No fields found in XiphComment" ) ) << endl;

        if( m_delete )
            return false;
    }
    else
    {
        if( m_verbose )
            m_textStream << qPrintable( tr( "INFO: Found existing XiphComment frames, parsing" ) )  << endl;
        TagLib::Ogg::FieldListMap fieldListMap = comment->fieldListMap();
        
        if( m_verbose )
            m_textStream << qPrintable( tr( "INFO: fieldListMap size is %1" ).arg( fieldListMap.size() ) ) << endl;
        
        TagLib::Ogg::FieldListMap::Iterator iter;
        for( iter = fieldListMap.begin(); iter != fieldListMap.end(); ++iter )
        {
            TagLib::String key = iter->first;
            QString qkey = TStringToQString( key ).toUpper();
            if( qkey.startsWith( "AMAROK - REDISCOVER YOUR MUSIC" ) )
            {
                nothingfound = false;
                
                if( m_verbose )
                    m_textStream << qPrintable( tr( "INFO: Removing old-style ATF identifier %1" ).arg( qkey ) ) << endl;

                toRemove.append( key );
                if( !m_delete )
                    newUid = true;
            }
            else if( qkey.startsWith( "AMAROK 2 AFT" ) )
            {
                nothingfound = false;
                
                if( m_verbose )
                    m_textStream << qPrintable( tr( "INFO: Found an existing AFT identifier: %1" ).arg( qkey ) ) << endl;

                if( m_delete )
                {
                    toRemove.append( key );
                    if( m_verbose )
                        m_textStream << qPrintable( tr( "INFO: Removing current AFT frame" ) ) << endl;
                }
                else
                {
                    int version = qkey.at( 13 ).digitValue();
                    if( m_verbose )
                        m_textStream << qPrintable( tr( "INFO: AFT identifier is version %1" ).arg( version ) ) << endl;
                    if( version < s_currentVersion )
                    {
                        if( m_verbose )
                            m_textStream << qPrintable( tr( "INFO: Upgrading AFT identifier from version %1 to version %2" ).arg( version, s_currentVersion ) ) << endl;
                        uid = upgradeUID( version, TStringToQString( fieldListMap[key].front() ) );
                        if( m_verbose )
                            m_textStream << qPrintable( tr( "INFO: Removing current AFT frame" ) ) << endl;
                        toRemove.append( key );
                        newUid = true;
                    }
                    else if( version == s_currentVersion && m_newid )
                    {
                        if( m_verbose )
                            m_textStream << qPrintable( tr( "INFO: New IDs specified to be generated, doing so" ) ) << endl;
                        toRemove.append( key );
                        newUid = true;
                    }
                    else
                    {
                        if( m_verbose )
                            m_textStream << qPrintable( tr( "INFO: ID is current" ) ) << endl;
                        return false;
                    }
                }
            }
        }        
        for( TagLib::StringList::ConstIterator iter = toRemove.begin(); iter != toRemove.end(); ++iter )
            comment->removeField( *iter );
    }
    if( newUid || ( nothingfound && !m_delete ) )
    {
        QString ourId = QString( "Amarok 2 AFTv" + QString::number( s_currentVersion ) + " - amarok.kde.org" );
        if( uid.isEmpty() )
            uid = createCurrentUID( file );
        if( m_verbose )
            m_textStream << qPrintable( tr( "INFO: Adding new field and saving file with UID: %1" ).arg( uid ) ) << endl;
        comment->addField( Qt4QStringToTString( ourId ), Qt4QStringToTString( uid ) );
        return true;
    }
    else if( toRemove.size() )
        return true;
    
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
    md5.addData( QString::number( m_time.elapsed() ).toAscii() );
    md5.addData( QString::number( qrand() ).toAscii() );
    md5.addData( QString::number( qrand() ).toAscii() );
    md5.addData( QString::number( qrand() ).toAscii() );
    md5.addData( QString::number( qrand() ).toAscii() );
    md5.addData( QString::number( qrand() ).toAscii() );
    md5.addData( QString::number( m_time.elapsed() ).toAscii() );
    return QString( md5.result().toHex() );
}

QString
AFTTagger::upgradeUID( int version, QString currValue )
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
    foreach( QString arg, argslist )
    {
        ++argnum;
        if( arg.isEmpty() || argnum == 1 )
            continue;
        if( nomore )
        {
            m_fileFolderList.append( arg );
        }
        else if( arg.startsWith( "--" ) )
        {
            QString myarg = arg.remove( 0, 2 );
            if ( myarg == "recurse" || myarg == "recursively" )
                m_recurse = true;
            else if( myarg == "verbose" )
                m_verbose = true;
            else if( myarg == "quiet" )
                m_quiet = true;
            else if( myarg == "newid" )
                m_newid = true;
            else if( myarg == "delete" )
                m_delete = true;
            else
                displayHelp();
        }
        else if( arg.startsWith( "-" ) )
        {
            QString myarg = arg.remove( 0, 1 );
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
    m_textStream << qPrintable( tr( "Amarok AFT Tagger" ) ) << endl << endl;
    m_textStream << qPrintable( tr( "IRC:\nserver: irc.freenode.net / channels: #amarok, #amarok.de, #amarok.es, #amarok.fr\n\nFeedback:\namarok@kde.org" ) ) << endl << endl;
    m_textStream << qPrintable( tr( "Usage: amarok_afttagger [options] +File/Folder(s)" ) ) << endl << endl;
    m_textStream << qPrintable( tr( "User-modifiable Options:" ) ) << endl;
    m_textStream << qPrintable( tr( "+File/Folder(s)       : Files or folders to tag" ) ) << endl;
    m_textStream << qPrintable( tr( "-h, --help            : This help text" ) ) << endl;
    m_textStream << qPrintable( tr( "-r, --recursive       : Process files and folders recursively" ) ) << endl;
    m_textStream << qPrintable( tr( "-d, --delete          : Remove AFT tag" ) ) << endl;
    m_textStream << qPrintable( tr( "-n  --newid           : Replace any existing ID with a new one" ) ) << endl;
    m_textStream << qPrintable( tr( "-v, --verbose         : Verbose output" ) ) << endl;
    m_textStream << qPrintable( tr( "-q, --quiet           : Quiet output; Implies that you accept the terms of use" ) ) << endl;
    m_textStream.flush();
    ::exit( 0 );
}
