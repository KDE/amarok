/****************************************************************************************
 * Copyright (c) 2009 Jakob Kummerow <jakob.kummerow@gmail.com>                         *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#define DEBUG_PREFIX "ScriptUpdater"

#include "ScriptUpdater.h"

#include "core/support/Debug.h"
#include "ScriptManager.h"

#include <KIO/Job>
#include <QStandardPaths>
#include <KTar>


#include <QDir>
#include <QFileInfo>

#ifdef QCA2_FOUND
#include <QtCrypto>
#endif

ScriptUpdater::ScriptUpdater( QObject *parent ) : QObject( parent )
{
}

ScriptUpdater::~ScriptUpdater()
{
}

void
ScriptUpdater::setScriptPath( const QString& scriptPath )
{
    m_scriptPath = scriptPath;
}

void
ScriptUpdater::updateScript()
{
    DEBUG_BLOCK

    // 1a. detect currently installed version
    QFileInfo info( m_scriptPath );
    const QString specPath = info.path() + QLatin1Char('/') + "script.spec";
    if( !QFile::exists( specPath ) )
    {
        // no .spec file found, can't continue
        Q_EMIT finished( m_scriptPath );
        return;
    }
    KPluginMetaData pInfo = ScriptManager::createMetadaFromSpec( specPath );
    if ( !pInfo.isValid() || pInfo.name().isEmpty() || pInfo.version().isEmpty() )
    {
        // invalid or unusable .spec file, can't continue
        Q_EMIT finished( m_scriptPath );
        return;
    }
    m_scriptversion = pInfo.version();

    // 1b. detect script name
    QFile file( m_scriptPath );
    m_fileName = file.fileName();
    QRegExp rxname( "amarok/scripts/(.+)/main.js" );
    rxname.indexIn( m_fileName );
    m_scriptname = rxname.cap( 1 );

    // 2. check if there are updates: get 'version' file from server
    QUrl versionUrl( updateBaseUrl );
    versionUrl = versionUrl.adjusted(QUrl::StripTrailingSlash);
    versionUrl.setPath(versionUrl.path() + QLatin1Char('/') + ( m_scriptname ));
    versionUrl = versionUrl.adjusted(QUrl::StripTrailingSlash);
    versionUrl.setPath(versionUrl.path() + QLatin1Char('/') + ( '/' + versionFilename ));
    m_versionFile.open();
    debug() << m_scriptname << ": Accessing " << versionUrl.toDisplayString() << " ...";
    QUrl versionDest( m_versionFile.fileName() );
    KIO::FileCopyJob *versionJob = KIO::file_copy( versionUrl, versionDest, -1, KIO::Overwrite | KIO::HideProgressInfo );
    connect ( versionJob, &KIO::FileCopyJob::result, this, &ScriptUpdater::phase2 );
}

void
ScriptUpdater::phase2( KJob * job )
{
    DEBUG_BLOCK
    if ( job->error() )
    {
        // if no 'version' file was found, cancel the update
        Q_EMIT finished( m_scriptPath );
        return;
    }
    QFile file( m_versionFile.fileName() );
    if ( !file.open( QIODevice::ReadOnly ) ) {
        debug() << m_scriptname << ": Failed to open version file for reading!";
        Q_EMIT finished( m_scriptPath );
        return;
    }
    QString response( file.readAll() );
    file.close();
    debug() << m_scriptname << ": online version: " << response;
    if ( !isNewer( response, m_scriptversion ) )
    {
        // if no newer version is available, cancel update
        Q_EMIT finished( m_scriptPath );
        return;
    }
    debug() << m_scriptname << ": newer version found, starting update :-)";

    // 3. get the update archive, download it to a temporary file
    QUrl archiveSrc( updateBaseUrl );
    archiveSrc = archiveSrc.adjusted(QUrl::StripTrailingSlash);
    archiveSrc.setPath(archiveSrc.path() + QLatin1Char('/') + ( m_scriptname ));
    archiveSrc = archiveSrc.adjusted(QUrl::StripTrailingSlash);
    archiveSrc.setPath(archiveSrc.path() + QLatin1Char('/') + ( '/' + archiveFilename ));
    m_archiveFile.open(); // temporary files only have a fileName() after they've been opened
    QUrl archiveDest( m_archiveFile.fileName() );
    KIO::FileCopyJob *archiveJob = KIO::file_copy( archiveSrc, archiveDest, -1, KIO::Overwrite | KIO::HideProgressInfo );
    connect ( archiveJob, &KIO::FileCopyJob::result, this, &ScriptUpdater::phase3 );
}

void ScriptUpdater::phase3( KJob * job )
{
    if ( job->error() )
    {
        // if the file wasn't found, cancel the update
        Q_EMIT finished( m_scriptPath );
        return;
    }

    // 4. get the archive's signature, download it to a temporary file as well
    QUrl sigSrc( updateBaseUrl );
    sigSrc = sigSrc.adjusted(QUrl::StripTrailingSlash);
    sigSrc.setPath(sigSrc.path() + QLatin1Char('/') + ( m_scriptname ));
    sigSrc = sigSrc.adjusted(QUrl::StripTrailingSlash);
    sigSrc.setPath(sigSrc.path() + QLatin1Char('/') + ( '/' + signatureFilename ));
    m_sigFile.open();
    QUrl sigDest( m_sigFile.fileName() );
    KIO::FileCopyJob *sigJob = KIO::file_copy( sigSrc, sigDest, -1, KIO::Overwrite | KIO::HideProgressInfo );
    connect ( sigJob, &KIO::FileCopyJob::result, this, &ScriptUpdater::phase4 );
}

void
ScriptUpdater::phase4( KJob * job )
{
    if ( job->error() )
    {
        // if the signature couldn't be downloaded, cancel the update
        Q_EMIT finished( m_scriptPath );
        return;
    }

    // 5. compare the signature to the archive's hash
#ifdef QCA2_FOUND
    QCA::Initializer init;
    QCA::ConvertResult conversionResult;
    QCA::PublicKey pubkey = QCA::PublicKey::fromPEM( publicKey, &conversionResult );
    if ( !( QCA::ConvertGood == conversionResult ) )
    {
        debug() << m_scriptname << ": Failed to read public key!";
        Q_EMIT finished( m_scriptPath );
        return;
    }
    QFile file( m_archiveFile.fileName() );
    if ( !file.open( QIODevice::ReadOnly ) )
    {
        debug() << m_scriptname << ": Failed to open archive file for reading!";
        Q_EMIT finished( m_scriptPath );
        return;
    }
    QCA::Hash hash( "sha1" );
    hash.update( &file );
    file.close();
    QFile versionFile( m_versionFile.fileName() );
    if ( !versionFile.open( QIODevice::ReadOnly ) )
    {
        debug() << m_scriptname << ": Failed to open version file for reading!";
        Q_EMIT finished( m_scriptPath );
        return;
    }
    QCA::Hash versionHash( "sha1" );
    versionHash.update( &versionFile );
    versionFile.close();
    QFile sigFile( m_sigFile.fileName() );
    if ( !sigFile.open( QIODevice::ReadOnly ) )
    {
        debug() << m_scriptname << ": Failed to open signature file for reading!";
        Q_EMIT finished( m_scriptPath );
        return;
    }
    QByteArray signature = QByteArray::fromBase64( sigFile.readAll() );
    sigFile.close();
    pubkey.startVerify( QCA::EMSA3_SHA1 );
    pubkey.update( hash.final() );
    pubkey.update( versionHash.final() );
    if ( !pubkey.validSignature( signature ) )
    {
        debug() << m_scriptname << ": Invalid signature, no update performed.";
        Q_EMIT finished( m_scriptPath );
        return;
    }
    debug() << m_scriptname << ": Signature matches. Performing update now.";

#else
    debug() << m_scriptname << ": Amarok build without QtCrypto. Cannot verify signature";
    return;
#endif

    // 6. everything OK, perform the update by extracting the archive
    KTar archive( m_archiveFile.fileName() );
    if( !archive.open( QIODevice::ReadOnly ) )
    {
        // in case of errors: bad luck, cancel the update
        debug() << m_scriptname << ": Error opening the update package.";
        Q_EMIT finished( m_scriptPath );
        return;
    }
    const QStringList locations = QStandardPaths::standardLocations( QStandardPaths::GenericDataLocation );
    QString relativePath;
    for( const auto &location : locations )
        if( m_fileName.startsWith( location ) )
            relativePath = m_fileName.remove( location );
    if( relativePath.startsWith( QLatin1Char('/') ) )
       relativePath.remove( 0, 1 );
    const QFileInfo fileinfo( relativePath );
    const QString destination = QStandardPaths::writableLocation( QStandardPaths::GenericDataLocation ) + QLatin1Char('/') + fileinfo.path();
    const QDir dir;
    if( !dir.exists( destination ) )
    {
        dir.mkpath( destination );
    }
    const KArchiveDirectory* const archiveDir = archive.directory();
    archiveDir->copyTo( destination );
    // update m_scriptPath so that the updated version of the script will be loaded
    m_scriptPath = destination + "/main.js";
    debug() << m_scriptname << ": Updating finished successfully :-)";

    // all done, temporary files are deleted automatically by Qt
    Q_EMIT finished( m_scriptPath );
}

// decide whether a version string 'update' is newer than 'installed'
bool
ScriptUpdater::isNewer( const QString & update, const QString & installed )
{
    // only dots are supported as separators, and only integers are supported
    // between the dots (so no fancy stuff like 2.1-1 or 2.1.beta2)
    QStringList uList = update.split( QLatin1Char('.') );
    QStringList iList = installed.split( QLatin1Char('.') );
    int i = 0;
    // stop working if the end of both lists is reached
    while ( i < uList.length() || i < iList.length() ) {
        // read current number, or use 0 if it isn't present (so that "2" == "2.0" == "2.0.0.0.0.0" == "2...0")
        int up = ( ( uList.length() > i && ( !uList[i].isEmpty() ) ) ? uList[i].toInt() : 0 );
        int in = ( ( iList.length() > i && ( !iList[i].isEmpty() ) ) ? iList[i].toInt() : 0 );
        if ( up > in )
        {
            return true;
        }
        else if ( up < in )
        {
            return false;
        }
        else
        {
            // both strings are equal up to this point -> look at the next pair of numbers
            i++;
        }
    }
    // if we reach this point, both versions are considered equal
    return false;
}
