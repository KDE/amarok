/*
 *  Copyright (c) 2008 Jeff Mitchell <mitchell@kde.org>
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

#include "SafeFileSaver.h"

#include "taglib/fileref.h"
#include "taglib/id3v2tag.h"
#include "taglib/mpegfile.h"
#include "taglib/tfile.h"
#include "taglib/uniquefileidentifierframe.h"

#include <KAboutData>
#include <KCmdLineArgs>
#include <KMD5>
#include <KRandom>

#include <QtDebug>
#include <QDir>
#include <QFileInfo>
#include <QString>

#include <iostream>

//QT4-happy versions
#undef QStringToTString
#define QStringToTString(s) TagLib::String(s.toUtf8().data(), TagLib::String::UTF8)
#undef TStringToQString
#define TStringToQString(s) QString::fromUtf8(s.toCString(true))

static bool recurse = false;
static bool verbose = false;
static bool quiet = false;
static bool newid = false;
static bool removeid = false;
static int currentVersion = 1;

void processPath( const QString &path );
QString createCurrentUID();
QString createV1UID();
QString upgradeUID( int version, QString currValue );

int main( int argc, char *argv[] )
{
    KAboutData aboutData( "amarok_afttagger", 0,
    ki18n( "Amarok AFT Tagger" ), "1.0",
    ki18n( "AFT Tagging helper application for Amarok" ), KAboutData::License_GPL,
    ki18n( "(C) 2008, Jeff Mitchell" ),
    ki18n( "IRC:\nirc.freenode.net - #amarok, #amarok.de, #amarok.es, #amarok.fr\n\nFeedback:\namarok@kde.org\n\n(Build Date: " __DATE__ ")" ),
             ( "http://amarok.kde.org" ) );

    aboutData.addAuthor( ki18n("Jeff Mitchell"),
            ki18n( "Developer (jefferai)" ), "mitchell@kde.org" );
    KCmdLineArgs::reset();
    KCmdLineArgs::init( argc, argv, &aboutData );


    KCmdLineOptions options;
    options.add("+file(s)", ki18n( "Files/Directories to tag" ) );
    options.add("d").add("delete", ki18n( "Delete AFT IDs from the files") );
    options.add("n").add("newid", ki18n( "Generate a new ID even if an up-to-date one exists" ) );
    options.add("q").add("quiet", ki18n( "No prompts -- Indicates you agree to the terms of use." ) );
    options.add("r").add("recurse", ki18n( "Recursively process files and directories" ) );
    options.add("V").add("verbose", ki18n( "More verbose output" ) );
    KCmdLineArgs::addCmdLineOptions( options );
    KCmdLineArgs* const args = KCmdLineArgs::parsedArgs();

    if( args->isSet("recurse") )
        recurse = true;
    if( args->isSet("quiet") )
        quiet = true;
    if( args->isSet("verbose") )
        verbose = true;
    if( args->isSet("newid") )
        newid = true;
    if( args->isSet("delete") )
        removeid = true;

    QString terms;
    if( !quiet )
    {
        terms =  i18n( "TERMS OF USE:\n\n" 
                "This program has been extensively tested and errs on the side of safety wherever possible.\n\n"
                "With that being said, since this program can modify thousands or hundreds of thousands of files\n"
                "at a time, here is the obligatory warning text:\n\n"
                "This program makes use of multiple libraries not written by the author, and as such neither the\n"
                "the author nor the Amarok project can or do take any responsibility for any damage that may\n"
                "occur to your files through the use of this program.\n\n"
                "If you want more information, please see http://amarok.kde.org/wiki/AFT\n\n"
                "If you agree to be bound by these terms of use, enter 'y' or 'Y', or anything else to exit:\n" );

        std::cout << terms.toUtf8().data();
        std::string response;
        std::cin >> response;
        cin.get();

        if( response != "y" && response != "Y" && QString::fromUtf8(response.c_str()) != i18nc("translate this according to what you translated in TERMS OF USE sentence", "y") && QString::fromUtf8(response.c_str()) != i18nc("translate this according to what you translated in TERMS OF USE sentence", "Y"))
        {
            qDebug() << "INFO: Terms not accepted; exiting...";
            return 1;
        }
    }

    if( args->count() == 0 )
    {
        qDebug() << "WARNING: No files or directories input; exiting...";
        return 2;
    }

    for(int i = 0; i < args->count(); i++) // Counting start at 0!
        processPath( args->arg(i) );

    return 0;
}

void processPath( const QString &path )
{
    QFileInfo info( path );
    if( !info.isDir() && !info.isFile() )
    {
        if( verbose )
            qDebug() << "INFO: Skipping " << path << " because it is neither a directory nor file.";
        return;
    }
    if( info.isDir() )
    {
        if( !recurse )
        {
            if( verbose )
                qDebug() << "INFO: Skipping " << path << " because it is a directory and recursion is not specified.";
            return;
        }
        else
        {
            if( verbose )
                qDebug() << "INFO: Processing directory " << path;
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

        QString ourId = QString( "Amarok 2 AFTv" + QString::number( currentVersion ) + " - amarok.kde.org" );
        
#ifdef COMPLEX_TAGLIB_FILENAME
    const wchar_t *encodedName = reinterpret_cast< const wchar_t *>(filePath.utf16());
#else
    QByteArray fileName = QFile::encodeName( filePath );
    const char *encodedName = fileName.constData();
#endif
    
        TagLib::FileRef fileRef = TagLib::FileRef( encodedName, true, TagLib::AudioProperties::Fast );
        
        if( fileRef.isNull() )
        {
            if( verbose )
                qDebug() << "INFO: file " << filePath << " not able to be opened by TagLib";
            return;
        }

        qDebug() << "INFO: Processing file " << filePath;

        SafeFileSaver sfs( filePath );
        sfs.setVerbose( false );
        sfs.setPrefix( "amarok-afttagger" );
        QString tempFilePath = sfs.prepareToSave();

        QString uid = createCurrentUID();
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
            if( verbose )
                qDebug() << "INFO: File is a MPEG file, opening...";
            if ( file->ID3v2Tag( true ) )
            {
                if( file->ID3v2Tag()->frameListMap()["UFID"].isEmpty() )
                {
                    if( verbose )
                        qDebug() << "INFO: No UFID frames found";
                    if( removeid )
                    {
                        sfs.cleanupSave();
                        return;
                    }
                    newUid = true;
                }
                else
                {
                    if( verbose )
                        qDebug() << "INFO: Found existing UFID frames, parsing";
                    TagLib::ID3v2::FrameList frameList = file->ID3v2Tag()->frameListMap()["UFID"];
                    TagLib::ID3v2::FrameList::Iterator iter;
                    if( verbose )
                        qDebug() << "INFO: Frame list size is " << frameList.size();
                    for( iter = frameList.begin(); iter != frameList.end(); ++iter )
                    {
                        TagLib::ID3v2::UniqueFileIdentifierFrame* currFrame = dynamic_cast<TagLib::ID3v2::UniqueFileIdentifierFrame*>(*iter);
                        if( currFrame )
                        {
                            QString owner = TStringToQString( currFrame->owner() );
                            if( owner.startsWith( "AMAROK - REDISCOVER YOUR MUSIC" ) )
                            {
                                if( verbose )
                                    qDebug() << "INFO: Removing old-style ATF identifier";

                                iter = frameList.erase( iter );
                                file->ID3v2Tag()->removeFrame( currFrame );
                                file->save();
                                if( !removeid )
                                    newUid = true;
                                else
                                    continue;
                            }
                            if( owner.startsWith( "Amarok 2 AFT" ) )
                            {
                                if( verbose )
                                    qDebug() << "INFO: Found an existing AFT identifier";

                                if( removeid )
                                {
                                    iter = frameList.erase( iter );
                                    if( verbose )
                                        qDebug() << "INFO: Removing current AFT frame";
                                    file->ID3v2Tag()->removeFrame( currFrame );
                                    file->save();
                                    continue;
                                }

                                int version = owner.at( 13 ).digitValue();
                                if( version < currentVersion )
                                {
                                    if( verbose )
                                        qDebug() << "INFO: Upgrading AFT identifier from version " << version << " to version " << currentVersion;
                                    uid = upgradeUID( version, TStringToQString( TagLib::String( currFrame->identifier() ) ) );
                                    if( verbose )
                                        qDebug() << "INFO: Removing current AFT frame";
                                    iter = frameList.erase( iter );
                                    file->ID3v2Tag()->removeFrame( currFrame );
                                    newUid = true;
                                }
                                else if( version == currentVersion && newid )
                                {
                                    if( verbose )
                                        qDebug() << "INFO: new IDs specified to be generated, doing so";
                                    iter = frameList.erase( iter );
                                    file->ID3v2Tag()->removeFrame( currFrame );
                                    newUid = true;
                                }
                                else
                                {
                                    if( verbose )
                                        qDebug() << "INFO: ID is current";
                                }
                            }
                        }
                    }
                }
                if( newUid )
                {
                    if( verbose )
                        qDebug() << "INFO: Adding new frame and saving file";
                    file->ID3v2Tag()->addFrame( new TagLib::ID3v2::UniqueFileIdentifierFrame(
                        QStringToTString( ourId ), QStringToTString( uid ).data( TagLib::String::Latin1 ) ) );
                    file->save();
                }
            }
        }
        else
        {
            if( verbose )
            qDebug() << "INFO: File not able to be parsed by TagLib or wrong kind (currently this program only supports MPEG files), cleaning up temp file";
            if( !sfs.cleanupSave() )
                qWarning() << "WARNING: file at " << filePath << " could not be cleaned up; check for strays";
            return;
        }
        if( newUid || removeid )
        {
            if( verbose )
                qDebug() << "INFO: Safe-saving file";
            if( !sfs.doSave() )
                qWarning() << "WARNING: file at " << filePath << " could not be saved";
        }
        if( verbose )
            qDebug() << "INFO: Cleaning up...";
        if( !sfs.cleanupSave() )
            qWarning() << "WARNING: file at " << filePath << " could not be cleaned up; check for strays";
        return;
    }
}

QString createCurrentUID()
{
    return createV1UID();
}

QString createV1UID()
{
    QString randomString = KRandom::randomString( 32 );
    KMD5 md5sum( randomString.toAscii() );
    return QString( md5sum.hexDigest() );
}

QString upgradeUID( int version, QString currValue )
{
    Q_UNUSED(version)
    return currValue + "abcd";
}

