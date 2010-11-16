/***************************************************************************
 *   Copyright (C) 2003-2005 Max Howell <max.howell@methylblue.com>        *
 *             (C) 2003-2010 Mark Kretschmann <kretschmann@kde.org>        *
 *             (C) 2005-2007 Alexandre Oliveira <aleprj@gmail.com>         *
 *             (C) 2008 Dan Meltzer <parallelgrapefruit@gmail.com>         *
 *             (C) 2008-2009 Jeff Mitchell <mitchell@kde.org>              *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#include "CollectionScanner.h"
#include "Directory.h"
#include "Track.h"
#include "BatchFile.h"

#include "shared/Version.h"  // for AMAROK_VERSION

#include <QTimer>
#include <QThread>

#include <QString>
#include <QStringList>
#include <QDir>
#include <QFile>
#include <QDateTime>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>
#include <QSharedMemory>
#include <QByteArray>
#include <QTextStream>
#include <QDataStream>
#include <QBuffer>
#include <QDebug>

#include <audiblefiletyperesolver.h>
#include <realmediafiletyperesolver.h>
#include "shared/taglib_filetype_resolvers/asffiletyperesolver.h"
#include "shared/taglib_filetype_resolvers/mp4filetyperesolver.h"
#include "shared/taglib_filetype_resolvers/wavfiletyperesolver.h"

int
main( int argc, char *argv[] )
{
    CollectionScanner::Scanner scanner( argc, argv );
    return scanner.exec();
}


// ------------ the scanning state -----------

CollectionScanner::ScanningState::ScanningState()
        : m_sharedMemory(0)
        , m_lastFilePos(0)
{
}

CollectionScanner::ScanningState::~ScanningState()
{
    delete m_sharedMemory;
}

void
CollectionScanner::ScanningState::setKey( const QString &key )
{
    delete m_sharedMemory;
    m_sharedMemory = new QSharedMemory( key );
    m_sharedMemory->attach();
}

bool
CollectionScanner::ScanningState::isValid() const
{
    return m_sharedMemory && m_sharedMemory->isAttached();
}

QString
CollectionScanner::ScanningState::lastDirectory() const
{ return m_lastDirectory; }

void
CollectionScanner::ScanningState::setLastDirectory( const QString &dir )
{
    if( dir == m_lastDirectory )
        return;

    m_lastDirectory = dir;
    writeFull();
}

QStringList
CollectionScanner::ScanningState::directories() const
{ return m_directories; }

void
CollectionScanner::ScanningState::setDirectories( const QStringList &directories )
{
    if( directories == m_directories )
        return;

    m_directories = directories;
    writeFull();
}

QStringList
CollectionScanner::ScanningState::badFiles() const
{ return m_badFiles; }

void
CollectionScanner::ScanningState::setBadFiles( const QStringList &badFiles )
{
    if( badFiles == m_badFiles )
        return;

    m_badFiles = badFiles;
    writeFull();
}

QString
CollectionScanner::ScanningState::lastFile() const
{ return m_lastFile; }

void
CollectionScanner::ScanningState::setLastFile( const QString &file )
{
    if( file == m_lastFile )
        return;

    m_lastFile = file;

    if( !isValid() )
        return;

    QBuffer buffer;
    QDataStream out(&buffer);

    buffer.open(QBuffer::WriteOnly);

    out << m_lastFile;
    int size = buffer.size();

    if( size + m_lastFilePos < m_sharedMemory->size() )
    {
        char *to = (char*)m_sharedMemory->data();
        const char *from = buffer.data().data();
        memcpy(to + m_lastFilePos, from, size);
    }

    m_sharedMemory->unlock();
}

void
CollectionScanner::ScanningState::readFull()
{
    if( !isValid() )
        return;

    QBuffer buffer;
    QDataStream in(&buffer);

    m_sharedMemory->lock();
    buffer.setData((char*)m_sharedMemory->constData(), m_sharedMemory->size());
    buffer.open(QBuffer::ReadOnly);

    in >> m_lastDirectory;
    in >> m_directories;
    in >> m_badFiles;
    m_lastFilePos = buffer.pos();
    in >> m_lastFile;

    m_sharedMemory->unlock();
}

void
CollectionScanner::ScanningState::writeFull()
{
    if( !isValid() )
        return;

    QBuffer buffer;
    QDataStream out(&buffer);

    m_sharedMemory->lock();
    buffer.open(QBuffer::WriteOnly);

    out << m_lastDirectory;
    out << m_directories;
    out << m_badFiles;
    m_lastFilePos = buffer.pos();
    out << m_lastFile;
    int size = buffer.size();

    if( size < m_sharedMemory->size() )
    {
        char *to = (char*)m_sharedMemory->data();
        const char *from = buffer.data().data();
        memcpy(to, from, size);
    }

    m_sharedMemory->unlock();
}


// ------------ the scanner -----------

CollectionScanner::Scanner::Scanner( int &argc, char **argv )
        : QCoreApplication( argc, argv )
        , m_charset( false )
        , m_newerTime(0)
        , m_incremental( false )
        , m_recursively( false )
        , m_restart( false )
        , m_idlePriority( false )
{
    setObjectName( "amarokcollectionscanner" );

    readArgs();

    if( m_idlePriority )
    {
        if( QThread::currentThread() )
            QThread::currentThread()->setPriority( QThread::IdlePriority );
    }

    TagLib::FileRef::addFileTypeResolver(new RealMediaFileTypeResolver);
    TagLib::FileRef::addFileTypeResolver(new AudibleFileTypeResolver);
    TagLib::FileRef::addFileTypeResolver(new ASFFileTypeResolver);
    TagLib::FileRef::addFileTypeResolver(new MP4FileTypeResolver);
    TagLib::FileRef::addFileTypeResolver(new WAVFileTypeResolver);
}


CollectionScanner::Scanner::~Scanner()
{
}

void
CollectionScanner::Scanner::readBatchFile( const QString &path )
{
    QFile batchFile( path );

    if( !batchFile.exists() )
        error( tr( "File \"%1\" not found." ).arg( path ) );

    if( !batchFile.open( QIODevice::ReadOnly ) )
        error( tr( "Could not open file \"%1\"." ).arg( path ) );

    BatchFile batch( path );
    foreach( const QString &str, batch.directories() )
    {
        m_folders.append( str );
    }

    foreach( const CollectionScanner::BatchFile::TimeDefinition &def, batch.timeDefinitions() )
    {
        m_mTimes.insert( def.first, def.second );
    }
}

void
CollectionScanner::Scanner::readNewerTime( const QString &path )
{
    QFileInfo file( path );

    if( !file.exists() )
        error( tr( "File \"%1\" not found." ).arg( path ) );

    m_newerTime = qMax( m_newerTime, file.lastModified().toTime_t() );
}


void
CollectionScanner::Scanner::doJob() //SLOT
{
    QFile xmlFile;
    xmlFile.open( stdout, QIODevice::WriteOnly );
    QXmlStreamWriter xmlWriter( &xmlFile );
    xmlWriter.setAutoFormatting( true );

    // --- determine the directories we have to scan
    QStringList entries;

    // -- when restarting read them from the shared memory
    if( m_restart && m_scanningState.isValid() )
    {
        m_scanningState.readFull();
        QString lastEntry = m_scanningState.lastDirectory();
        entries = m_scanningState.directories();

        // remove the entries we already scanned
        while( entries.front() != lastEntry && !entries.empty() )
            entries.pop_front();
    }

    // -- else use m_folders and do recursive
    else
    {
        QSet<QString> entriesSet;

        foreach( const QString &dir, m_folders ) // krazy:exclude=foreach
        {
            if( dir.isEmpty() )
                //apparently somewhere empty strings get into the mix
                //which results in a full-system scan! Which we can't allow
                continue;

            QString newdir( dir );

            // Make sure that all paths are absolute, not relative
            if( QDir::isRelativePath( dir ) )
                newdir = QDir::cleanPath( QDir::currentPath() + '/' + dir );

            if( !dir.endsWith( '/' ) )
                newdir += '/';

            addDir( newdir, &entriesSet );
        }

        entries = entriesSet.toList();
        m_scanningState.setLastDirectory( QString() );
        m_scanningState.setDirectories( entries );
    }

    if( !m_restart )
    {
        xmlWriter.writeStartDocument();
        xmlWriter.writeStartElement("scanner");
        xmlWriter.writeAttribute("count", QString::number( entries.count() ) );
        if( m_incremental )
            xmlWriter.writeAttribute("incremental", QString());
    }

    // --- now do the scanning
    foreach( QString path, entries )
    {
        CollectionScanner::Directory dir( path, &m_scanningState,
                                          m_incremental && !isModified( path ) );

        xmlWriter.writeStartElement( "directory" );
        dir.toXml( &xmlWriter );
        xmlWriter.writeEndElement();
        xmlFile.flush();
    }

    // --- write the end element (must be done by hand as we might not have written the start element when restarting)
    xmlFile.write("\n</scanner>\n");

    quit();
}

void
CollectionScanner::Scanner::addDir( const QString& dir, QSet<QString>* entries )
{
    // Linux specific, but this fits the 90% rule
    if( dir.startsWith( "/dev" ) || dir.startsWith( "/sys" ) || dir.startsWith( "/proc" ) )
        return;

    if( entries->contains( dir ) )
        return;

    QDir d( dir );
    if( !d.exists() )
        return;

    entries->insert( dir );

    if( !m_recursively )
        return; // finished

    d.setFilter( QDir::NoDotAndDotDot | QDir::Dirs );
    QFileInfoList fileInfos = d.entryInfoList();

    foreach( const QFileInfo &fi, fileInfos )
    {
        if( !fi.exists() )
            continue;

        const QFileInfo &f = fi.isSymLink() ? QFileInfo( fi.symLinkTarget() ) : fi;

        if( !f.exists() )
            continue;

        if( f.isDir() )
        {
            addDir( QString( f.absoluteFilePath() + '/' ), entries );
        }
    }
}

bool
CollectionScanner::Scanner::isModified( const QString& dir )
{
    QFileInfo info( dir );
    if( !info.exists() )
        return false;

    uint lastModified = info.lastModified().toTime_t();

    if( m_mTimes.contains( dir ) )
        return m_mTimes.value( dir ) != lastModified;
    else
        return m_newerTime < lastModified;
}

void
CollectionScanner::Scanner::readArgs()
{
    QStringList argslist = arguments();
    if( argslist.size() < 2 )
        displayHelp();

    bool missingArg = false;

    for( int argnum = 1; argnum < argslist.count(); argnum++ )
    {
        QString arg = argslist.at( argnum );

        if( arg.startsWith( "--" ) )
        {
            QString myarg = QString( arg ).remove( 0, 2 );
            if( myarg == "newer" )
            {
                if( argslist.count() > argnum + 1 )
                    readNewerTime( argslist.at( argnum + 1 ) );
                else
                    missingArg = true;
                argnum++;
            }
            else if( myarg == "batch" )
            {
                if( argslist.count() > argnum + 1 )
                    readBatchFile( argslist.at( argnum + 1 ) );
                else
                    missingArg = true;
                argnum++;
            }
            else if( myarg == "sharedmemory" )
            {
                if( argslist.count() > argnum + 1 )
                    m_scanningState.setKey( argslist.at( argnum + 1 ) );
                else
                    missingArg = true;
                argnum++;
            }
            else if( myarg == "version" )
                displayVersion();
            else if( myarg == "incremental" )
                m_incremental = true;
            else if( myarg == "recursive" )
                m_recursively = true;
            else if( myarg == "restart" )
                m_restart = true;
            else if( myarg == "idlepriority" )
                m_idlePriority = true;
            else if( myarg == "charset" )
                m_charset = true;
            else
                displayHelp();

        }
        else if( arg.startsWith( '-' ) )
        {
            QString myarg = QString( arg ).remove( 0, 1 );
            int pos = 0;
            while( pos < myarg.length() )
            {
                if( myarg[pos] == 'r' )
                    m_recursively = true;
                else if( myarg[pos] == 'v' )
                    displayVersion();
                else if( myarg[pos] == 's' )
                    m_restart = true;
                else if( myarg[pos] == 'c' )
                    m_charset = true;
                else if( myarg[pos] == 'i' )
                    m_incremental = true;
                else
                    displayHelp();

                ++pos;
            }
        }
        else
        {
            if( !arg.isEmpty() )
                m_folders.append( arg );
        }
    }

    if( missingArg )
        displayHelp( tr( "Missing argument for option %1" ).arg( argslist.last() ) );


    CollectionScanner::Track::setUseCharsetDetector( m_charset );

    // Start the actual scanning job
    QTimer::singleShot( 0, this, SLOT( doJob() ) );
}

void
CollectionScanner::Scanner::error( const QString &str )
{
    QTextStream stream( stderr );
    stream << str << endl;
    stream.flush();

    // Nothing else to do, so we exit directly
    ::exit( 0 );
}

/** This function is called by Amarok to verify that Amarok an Scanner versions match */
void
CollectionScanner::Scanner::displayVersion()
{
    QTextStream stream( stdout );
    stream << AMAROK_VERSION << endl;
    stream.flush();

    // Nothing else to do, so we exit directly
    ::exit( 0 );
}

void
CollectionScanner::Scanner::displayHelp( const QString &error )
{
    QTextStream stream( error.isEmpty() ? stdout : stderr );
    stream << error
        << tr( "Amarok Collection Scanner\n"
        "Scans directories and outputs a xml file with the results.\n"
        "For more information see http://amarok.kde.org/wiki/Batch_Mode\n\n"
        "Usage: amarokcollectionscanner [options] <Folder(s)>\n"
        "User-modifiable Options:\n"
        "<Folder(s)>             : list of folders to scan\n"
        "-h, --help              : This help text\n"
        "-v, --version           : Print the version of this tool\n"
        "-r, --recursive         : Scan folders recursively\n"
        "-i, --incremental       : Incremental scan (modified folders only)\n"
        "-s, --restart           : After a crash, restart the scanner in its last position\n"
        "    --idlepriority      : Run at idle priority\n"
        "    --sharedmemory <key> : A shared memory segment to be used for restarting a scan\n"
        "    --newer <path>      : Only scan directories if modification time is new than <path>\n"
        "                          Only useful in incremental scan mode\n"
        "    --batch <path>      : Add the directories from the batch xml file\n"
        "                          batch file format should look like this:\n"
        "   <scanner>\n"
        "    <directory>\n"
        "     <path>/absolute/path/of/directory</path>\n"
        "     <mtime>1234</mtime>   (this is optional)\n"
        "    </directory>\n"
        "   </scanner>\n"
        "                          You can also use a previous scan result for that.\n"
        )
        << endl;
    stream.flush();

    ::exit(0);
}

#include "CollectionScanner.moc"

