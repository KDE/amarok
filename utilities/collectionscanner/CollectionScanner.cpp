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

#include "Version.h"  // for AMAROK_VERSION
#include "collectionscanner/BatchFile.h"
#include "collectionscanner/Directory.h"
#include "collectionscanner/Track.h"

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

#include <algorithm>

#ifdef Q_OS_LINUX
// for ioprio
#include <unistd.h>
#include <sys/syscall.h>
enum {
    IOPRIO_CLASS_NONE,
    IOPRIO_CLASS_RT,
    IOPRIO_CLASS_BE,
    IOPRIO_CLASS_IDLE
};

enum {
    IOPRIO_WHO_PROCESS = 1,
    IOPRIO_WHO_PGRP,
    IOPRIO_WHO_USER
};
#define IOPRIO_CLASS_SHIFT  13
#endif


int
main( int argc, char *argv[] )
{
    CollectionScanner::Scanner scanner( argc, argv );
    return scanner.exec();
}

CollectionScanner::Scanner::Scanner( int &argc, char **argv )
        : QCoreApplication( argc, argv )
        , m_charset( false )
        , m_newerTime(0)
        , m_incremental( false )
        , m_recursively( false )
        , m_restart( false )
        , m_idlePriority( false )
{
    setObjectName( QStringLiteral("amarokcollectionscanner") );

    readArgs();

    if( m_idlePriority )
    {
        bool ioPriorityWorked = false;
#if defined(Q_OS_LINUX) && defined(SYS_ioprio_set)
        // try setting the idle priority class
        ioPriorityWorked = ( syscall( SYS_ioprio_set, IOPRIO_WHO_PROCESS, 0,
                                      IOPRIO_CLASS_IDLE << IOPRIO_CLASS_SHIFT ) >= 0 );
        // try setting the lowest priority in the best-effort priority class (the default class)
        if( !ioPriorityWorked )
            ioPriorityWorked = ( syscall( SYS_ioprio_set, IOPRIO_WHO_PROCESS, 0,
                                          7 | ( IOPRIO_CLASS_BE << IOPRIO_CLASS_SHIFT ) ) >= 0 );
#endif
        if( !ioPriorityWorked && QThread::currentThread() )
            QThread::currentThread()->setPriority( QThread::IdlePriority );
    }
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
    for( const QString &str : batch.directories() )
    {
        m_folders.append( str );
    }

    for( const CollectionScanner::BatchFile::TimeDefinition &def : batch.timeDefinitions() )
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

    m_newerTime = qMax<qint64>( m_newerTime, file.lastModified().toSecsSinceEpoch() );
}


void
CollectionScanner::Scanner::doJob() //SLOT
{
    QFile xmlFile;
    xmlFile.open( stdout, QIODevice::WriteOnly );
    QXmlStreamWriter xmlWriter( &xmlFile );
    xmlWriter.setAutoFormatting( true );

    // get a list of folders to scan. We do it even if resuming because we don't want
    // to save the (perhaps very big) list of directories into shared memory, bug 327812
    QStringList entries;
    {
        QSet<QString> entriesSet;

        for( QString dir : m_folders )
        {
            if( dir.isEmpty() )
                //apparently somewhere empty strings get into the mix
                //which results in a full-system scan! Which we can't allow
                continue;

            // Make sure that all paths are absolute, not relative
            if( QDir::isRelativePath( dir ) )
                dir = QDir::cleanPath( QDir::currentPath() + QLatin1Char('/') + dir );

            if( !dir.endsWith( QLatin1Char('/') ) )
                dir += QLatin1Char('/');

            addDir( dir, &entriesSet ); // checks m_recursively
        }

        entries = entriesSet.values();
        std::sort( entries.begin(), entries.end() ); // the sort is crucial because of restarts and lastDirectory handling
    }

    if( m_restart )
    {
        m_scanningState.readFull();
        QString lastEntry = m_scanningState.lastDirectory();

        int index = entries.indexOf( lastEntry );
        if( index >= 0 )
            // strip already processed entries, but *keep* the lastEntry
            entries = entries.mid( index );
        else
            qWarning() << Q_FUNC_INFO << "restarting scan after a crash, but lastDirectory"
                       << lastEntry << "not found in folders to scan (size" << entries.size()
                       << "). Starting scanning from the beginning.";
    }
    else // first attempt
    {
        m_scanningState.writeFull(); // just trigger write to initialise memory

        xmlWriter.writeStartDocument();
        xmlWriter.writeStartElement(QStringLiteral("scanner"));
        xmlWriter.writeAttribute(QStringLiteral("count"), QString::number( entries.count() ) );
        if( m_incremental )
            xmlWriter.writeAttribute(QStringLiteral("incremental"), QString());
        // write some information into the file and close previous tag
        xmlWriter.writeComment("Created by amarokcollectionscanner " AMAROK_VERSION " on "+QDateTime::currentDateTime().toString());
        xmlFile.flush();
    }

    // --- now do the scanning
    for( const QString &path : entries )
    {
        CollectionScanner::Directory dir( path, &m_scanningState,
                                          m_incremental && !isModified( path ) );

        xmlWriter.writeStartElement( QStringLiteral("directory") );
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
    if( dir.startsWith( QLatin1String("/dev") ) || dir.startsWith( QLatin1String("/sys") ) || dir.startsWith( QLatin1String("/proc") ) )
        return;

    if( entries->contains( dir ) )
        return;

    QDir d( dir );
    if( !d.exists() )
    {
        QTextStream stream( stderr );
        stream << "Directory \""<<dir<<"\" does not exist." << Qt::endl;
        return;
    }

    entries->insert( dir );

    if( !m_recursively )
        return; // finished

    d.setFilter( QDir::NoDotAndDotDot | QDir::Dirs );
    const QFileInfoList fileInfos = d.entryInfoList();

    for ( const QFileInfo &fi : fileInfos )
    {
        if( !fi.exists() )
            continue;

        const QFileInfo &f = fi.isSymLink() ? QFileInfo( fi.symLinkTarget() ) : fi;

        if( !f.exists() )
            continue;

        if( f.isDir() )
        {
            addDir( QString( f.absoluteFilePath() + QLatin1Char( '/' ) ), entries );
        }
    }
}

bool
CollectionScanner::Scanner::isModified( const QString& dir )
{
    QFileInfo info( dir );
    if( !info.exists() )
        return false;

    uint lastModified = info.lastModified().toSecsSinceEpoch();

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

        if( arg.startsWith( QLatin1String("--") ) )
        {
            QString myarg = QString( arg ).remove( 0, 2 );
            if( myarg == QLatin1String("newer") )
            {
                if( argslist.count() > argnum + 1 )
                    readNewerTime( argslist.at( argnum + 1 ) );
                else
                    missingArg = true;
                argnum++;
            }
            else if( myarg == QLatin1String("batch") )
            {
                if( argslist.count() > argnum + 1 )
                    readBatchFile( argslist.at( argnum + 1 ) );
                else
                    missingArg = true;
                argnum++;
            }
            else if( myarg == QLatin1String("sharedmemory") )
            {
                if( argslist.count() > argnum + 1 )
                    m_scanningState.setKey( argslist.at( argnum + 1 ) );
                else
                    missingArg = true;
                argnum++;
            }
            else if( myarg == QLatin1String("version") )
                displayVersion();
            else if( myarg == QLatin1String("incremental") )
                m_incremental = true;
            else if( myarg == QLatin1String("recursive") )
                m_recursively = true;
            else if( myarg == QLatin1String("restart") )
                m_restart = true;
            else if( myarg == QLatin1String("idlepriority") )
                m_idlePriority = true;
            else if( myarg == QLatin1String("charset") )
                m_charset = true;
            else
                displayHelp();

        }
        else if( arg.startsWith( QLatin1Char( '-' ) ) )
        {
            QString myarg = QString( arg ).remove( 0, 1 );
            int pos = 0;
            while( pos < myarg.length() )
            {
                if( myarg[pos] == QLatin1Char( 'r' ) )
                    m_recursively = true;
                else if( myarg[pos] == QLatin1Char( 'v' ) )
                    displayVersion();
                else if( myarg[pos] == QLatin1Char( 's' ) )
                    m_restart = true;
                else if( myarg[pos] == QLatin1Char( 'c' ) )
                    m_charset = true;
                else if( myarg[pos] == QLatin1Char( 'i' ) )
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
    QTimer::singleShot( 0, this, &Scanner::doJob );
}

void
CollectionScanner::Scanner::error( const QString &str )
{
    QTextStream stream( stderr );
    stream << str << Qt::endl;
    stream.flush();

    // Nothing else to do, so we exit directly
    ::exit( 0 );
}

/** This function is called by Amarok to verify that Amarok an Scanner versions match */
void
CollectionScanner::Scanner::displayVersion()
{
    QTextStream stream( stdout );
    stream << AMAROK_VERSION << Qt::endl;
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
        "For more information see http://community.kde.org/Amarok/Development/BatchMode\n\n"
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
        "    --newer <path>      : Only scan directories if modification time is newer than <path>\n"
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
        << Qt::endl;
    stream.flush();

    ::exit(0);
}


