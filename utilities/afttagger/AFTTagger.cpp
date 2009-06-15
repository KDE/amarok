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
#include <fileref.h>
#include <id3v2tag.h>
#include <mpegfile.h>
#include <tfile.h>
#include <uniquefileidentifierframe.h>

#include <QtDebug>
#include <QCryptographicHash>
#include <QDir>
#include <QFileInfo>
#include <QString>
#include <QTextStream>

#include <iostream>

//QT4-happy versions
#undef QStringToTString
#define QStringToTString(s) TagLib::String(s.toUtf8().data(), TagLib::String::UTF8)
#undef TStringToQString
#define TStringToQString(s) QString::fromUtf8(s.toCString(true))

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
                "This program makes use of multiple libraries not written by the author, and as such neither the\n"
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
            qDebug() << "INFO: Terms not accepted; exiting...";
            ::exit( 1 );
        }
    }
    
    srandom( (unsigned)time( 0 ) );
    m_time.start();

    for(int i = 0; i < m_fileFolderList.count(); i++) // Counting start at 0!
        processPath( m_fileFolderList.at( i ) );
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

        QString ourId = QString( "Amarok 2 AFTv" + QString::number( s_currentVersion ) + " - amarok.kde.org" );
        
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
                qDebug() << "INFO: file " << filePath << " not able to be opened by TagLib";
            return;
        }

        qDebug() << "INFO: Processing file " << filePath;

        SafeFileSaver sfs( filePath );
        sfs.setVerbose( false );
        sfs.setPrefix( "amarok-afttagger" );
        QString tempFilePath = sfs.prepareToSave();
        if( tempFilePath.isEmpty() )
        {
            m_textStream << qPrintable( tr( "Error: could not create temporary file when processing %1" ).arg( filePath ) ) << endl;
            return;
        }

        QString uid = createCurrentUID( path );
        bool newUid = false;
#ifdef COMPLEX_TAGLIB_FILENAME
    const wchar_t *encodedName = reinterpret_cast< const wchar_t * >(tempFilePath.utf16());
#else
    QByteArray tempFileName = QFile::encodeName( tempFilePath );
    const char *tempEncodedName = tempFileName.constData();
#endif
        TagLib::FileRef tempFileRef = TagLib::FileRef( tempEncodedName, true, TagLib::AudioProperties::Fast );
        if ( TagLib::MPEG::File *file = dynamic_cast<TagLib::MPEG::File *>( tempFileRef.file() ) )
        {
            if( m_verbose )
                qDebug() << "INFO: File is a MPEG file, opening...";
            if ( file->ID3v2Tag( true ) )
            {
                if( file->ID3v2Tag()->frameListMap()["UFID"].isEmpty() )
                {
                    if( m_verbose )
                        qDebug() << "INFO: No UFID frames found";
                    if( m_delete )
                    {
                        sfs.cleanupSave();
                        return;
                    }
                    newUid = true;
                }
                else
                {
                    if( m_verbose )
                        qDebug() << "INFO: Found existing UFID frames, parsing";
                    TagLib::ID3v2::FrameList frameList = file->ID3v2Tag()->frameListMap()["UFID"];
                    TagLib::ID3v2::FrameList::Iterator iter;
                    if( m_verbose )
                        qDebug() << "INFO: Frame list size is " << frameList.size();
                    for( iter = frameList.begin(); iter != frameList.end(); ++iter )
                    {
                        TagLib::ID3v2::UniqueFileIdentifierFrame* currFrame = dynamic_cast<TagLib::ID3v2::UniqueFileIdentifierFrame*>(*iter);
                        if( currFrame )
                        {
                            QString owner = TStringToQString( currFrame->owner() );
                            if( owner.startsWith( "AMAROK - REDISCOVER YOUR MUSIC" ) )
                            {
                                if( m_verbose )
                                    qDebug() << "INFO: Removing old-style ATF identifier";

                                iter = frameList.erase( iter );
                                file->ID3v2Tag()->removeFrame( currFrame );
                                file->save();
                                if( !m_delete )
                                    newUid = true;
                                else
                                    continue;
                            }
                            if( owner.startsWith( "Amarok 2 AFT" ) )
                            {
                                if( m_verbose )
                                    qDebug() << "INFO: Found an existing AFT identifier";

                                if( m_delete )
                                {
                                    iter = frameList.erase( iter );
                                    if( m_verbose )
                                        qDebug() << "INFO: Removing current AFT frame";
                                    file->ID3v2Tag()->removeFrame( currFrame );
                                    file->save();
                                    continue;
                                }

                                int version = owner.at( 13 ).digitValue();
                                if( version < s_currentVersion )
                                {
                                    if( m_verbose )
                                        qDebug() << "INFO: Upgrading AFT identifier from version " << version << " to version " << s_currentVersion;
                                    uid = upgradeUID( version, TStringToQString( TagLib::String( currFrame->identifier() ) ) );
                                    if( m_verbose )
                                        qDebug() << "INFO: Removing current AFT frame";
                                    iter = frameList.erase( iter );
                                    file->ID3v2Tag()->removeFrame( currFrame );
                                    newUid = true;
                                }
                                else if( version == s_currentVersion && m_newid )
                                {
                                    if( m_verbose )
                                        qDebug() << "INFO: new IDs specified to be generated, doing so";
                                    iter = frameList.erase( iter );
                                    file->ID3v2Tag()->removeFrame( currFrame );
                                    newUid = true;
                                }
                                else
                                {
                                    if( m_verbose )
                                        qDebug() << "INFO: ID is current";
                                }
                            }
                        }
                    }
                }
                if( newUid )
                {
                    if( m_verbose )
                        qDebug() << "INFO: Adding new frame and saving file";
                    file->ID3v2Tag()->addFrame( new TagLib::ID3v2::UniqueFileIdentifierFrame(
                        QStringToTString( ourId ), QStringToTString( uid ).data( TagLib::String::Latin1 ) ) );
                    file->save();
                }
            }
        }
        else
        {
            if( m_verbose )
            qDebug() << "INFO: File not able to be parsed by TagLib or wrong kind (currently this program only supports MPEG files), cleaning up temp file";
            if( !sfs.cleanupSave() )
                qWarning() << "WARNING: file at " << filePath << " could not be cleaned up; check for strays";
            return;
        }
        if( m_newid || m_delete )
        {
            if( m_verbose )
                qDebug() << "INFO: Safe-saving file";
            if( !sfs.doSave() )
                qWarning() << "WARNING: file at " << filePath << " could not be saved";
        }
        if( m_verbose )
            qDebug() << "INFO: Cleaning up...";
        if( !sfs.cleanupSave() )
            qWarning() << "WARNING: file at " << filePath << " could not be cleaned up; check for strays";
        return;
    }
}

QString
AFTTagger::createCurrentUID( const QString &path )
{
    return createV1UID( path );
}

QString
AFTTagger::createV1UID( const QString &path )
{
    QCryptographicHash md5( QCryptographicHash::Md5 );
    QFile qfile( path );
    QByteArray size;
    md5.addData( size.setNum( qfile.size() ) );
    md5.addData( QString::number( m_time.elapsed() ).toAscii() );
    md5.addData( QString::number( random() ).toAscii() );
    md5.addData( QString::number( random() ).toAscii() );
    md5.addData( QString::number( random() ).toAscii() );
    md5.addData( QString::number( random() ).toAscii() );
    md5.addData( QString::number( random() ).toAscii() );
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
