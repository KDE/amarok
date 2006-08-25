// Jeff Mitchell <kde-dev@emailgoeshere.com>, (C) 2006
// License: GNU General Public License V2


#define DEBUG_PREFIX "MetaBundleSaver"

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <fcntl.h>

#include "amarok.h"
#include "amarokconfig.h"
#include "collectiondb.h"
#include "debug.h"
#include "metabundlesaver.h"
#include "scancontroller.h"
#include <kapplication.h>
#include <kfilemetainfo.h>
#include <kio/global.h>
#include <kio/job.h>
#include <kio/jobclasses.h>
#include <kio/netaccess.h>
#include <kmdcodec.h>
#include <kurl.h>
#include <qfile.h> //decodePath()
#include <qcstring.h>
#include <qstring.h>
#include <taglib/fileref.h>

#include <config.h>

#include "metabundle.h"

MetaBundleSaver::MetaBundleSaver( MetaBundle *bundle )
    : QObject()
    , m_bundle( bundle )
    , m_tempSavePath( QString::null )
    , m_origRenamedSavePath( QString::null )
    , m_tempSaveDigest( 0 )
    , m_saveFileref( 0 )

{
    DEBUG_BLOCK
}

MetaBundleSaver::~MetaBundleSaver()
{
    DEBUG_BLOCK
    if( m_saveFileref )
        delete m_saveFileref;
}

/*
bool
MetaBundleSaver::scannerSafeSave( TagLib::File* file )
{

    //NOTE: this function will probably be changed around a bit later, as the algorithm may be obsoleted
    //by that in doSave...alternately it maybe should be kept to help reduce contention when writing to a file
    //between amarokapp and the collection scanner
    //note however that this save is not *yet* scanner safe with ATF turned on...
    DEBUG_BLOCK
    if( ( QString( kapp->name() ) == QString( "amarokcollectionscanner" ) ) //default
            || !ScanController::instance() //no scan, we're good
            || !AmarokConfig::advancedTagFeatures() ) //if no ATF, we're not writing to files with scanner
    {
        return file->save();
        //return doSave( file );
    }

    m_safeToSave = false;

    if( ScanController::instance() ) //yes check again, it can pull out from under us at any time
    {
        ScanController::instance()->notifyThisBundle( this );
        if( !ScanController::instance()->requestPause() )
        {
            debug() << "DCOP call to pause scanner failed, aborting save" << endl;
            return false;
        }
    }
    else //scanner seems to have exited in the interim
    {
        return file->save();
        //return doSave( file );
    }

    int count = 0;
    bool result = false;

    debug() << "entering loop to wait on scanner" << endl;

    while( ScanController::instance() && !m_safeToSave && count < 50 ) //time out after five seconds, just in case
    {
        kapp->processEvents( 100 );
        usleep( 100000 );
        count++;
        if( count % 10 == 0 )
            debug() << "waitcount is " << count << endl;
    }

    if( m_safeToSave )
    {
        debug() << "Starting tag save" << endl;
        result = file->save();
        //result = doSave( file );
        debug() << "done, result is " << (result?"success":"failure") << endl;
    }
    else
        debug() << "Did not write tag for file " << m_bundle->url().path() << endl;

    ScanController::instance()->notifyThisBundle( 0 );
    if( !ScanController::instance()->requestUnpause() )
    {
        debug() << "DCOP call to unpause scanner failed, it may be hung" << endl;
        //TODO: Have ScanController kill it and restart?
    }

    return result;
}
*/

TagLib::FileRef *
MetaBundleSaver::prepareToSave()
{
    DEBUG_BLOCK

    KMD5* md5sum;
    const KURL origPath = m_bundle->url();
    char hostbuf[32];
    int hostname = gethostname( hostbuf, 32 );
    if( hostname != 0 )
    {
        debug() << "Could not determine hostname!" << endl;
        return 0;
    }
    QString pid;
    QString randomString = m_bundle->getRandomString( 8, true );
    m_tempSavePath = origPath.path() + ".amaroktemp.host-" + QString( hostbuf ) +
                        ".pid-" + pid.setNum( getpid() ) + ".random-" + randomString + "." + m_bundle->type();
    m_origRenamedSavePath = origPath.path() + ".amarokoriginal.host-" + QString( hostbuf ) +
                        ".pid-" + pid.setNum( getpid() ) + ".random-" + randomString + "." + m_bundle->type();


    //The next long step is to copy the file over.  We can't use KIO because it's not thread save,
    //and std and QFile only have provisions for renaming and removing, so manual it is
    //doing it block-by-block results it not needing a huge amount of memory overhead

    debug() << "Copying original file to copy" << endl;

    if( QFile::exists( m_tempSavePath ) )
    {
        debug() << "Temp file already exists!" << endl;
        return 0;
    }

    QFile *orig = new QFile( m_bundle->url().path() );
    QFile *copy = new QFile( m_tempSavePath );

    if( !orig->open( IO_Raw | IO_ReadOnly ) )
    {
        debug() << "Could not open original file!" << endl;
        return 0;
    }

    //Do this separately so as not to create a zero-length file if you can't read from input
    if( !copy->open( IO_Raw | IO_WriteOnly | IO_Truncate ) )
    {
        debug() << "Could not create file copy" << endl;
        return 0;
    }

    char databuf[8192];

    Q_ULONG maxlen = 8192;
    Q_LONG actualreadlen, actualwritelen;

    while( ( actualreadlen = orig->readBlock( databuf, maxlen ) ) > 0 )
    {
        if( ( actualwritelen = copy->writeBlock( databuf, actualreadlen ) ) != actualreadlen )
        {
            debug() << "Error during copying of original file data to copy!" << endl;
            delete orig;
            delete copy;
            return 0;
        }
    }

    if( actualreadlen == -1 )
    {
        delete orig;
        delete copy;
        debug() << "Error during reading original file!" << endl;
        return 0;
    }

    delete orig;
    delete copy;

    //By this point, we have the following:
    //The original file is copied at path m_tempSavePath
    //We have generated what will be the filename to rename the original to in m_origRenamedSavePath
    //We have successfully copied the original file to the temp location

    debug() << "Calculating MD5 of " << m_tempSavePath << endl;

    QFile* tempFile = new QFile( m_tempSavePath );
    tempFile->open( IO_ReadOnly );
    md5sum = new KMD5( tempFile->readAll() );
    tempFile->close();
    delete tempFile;
    m_tempSaveDigest = md5sum->hexDigest();

    debug() << "MD5 sum of temp file: " << m_tempSaveDigest.data() << endl;

    //Now, we have a MD5 sum of the original file at the time of copying saved in m_tempSaveDigest
    //Create a fileref on the copied file, for modification

    m_saveFileref = new TagLib::FileRef( QFile::encodeName( m_tempSavePath ), false );

    if( m_saveFileref && !m_saveFileref->isNull() )
        return m_saveFileref;

    debug() << "Error creating temp file's fileref!" << endl;
    return 0;
}

bool
MetaBundleSaver::doSave()
{
    //TODO: much commenting needed.  For now this pretty much follows algorithm laid out in bug 131353,
    //but isn't useable since I need to find a good way to switch the file path with taglib, or a good way
    //to get all the metadata copied over.

    DEBUG_BLOCK
    bool revert = false;

    QFile* origRenamedFile;
    KMD5* md5sum;

    int errcode;

    QCString origRenamedDigest;

    debug() << "Saving tag changes to the temporary file..." << endl;

    //We've made our changes to the fileref; save it first, then do the logic to move the correct file back
    if( !m_saveFileref->save() )
    {
        debug() << "Could not save the new file!" << endl;
        goto fail_remove_copy;
    }

    debug() << "Renaming original file to temporary name " << m_origRenamedSavePath << endl;

    errcode = std::rename( QFile::encodeName( m_bundle->url().path() ).data(),
                               QFile::encodeName( m_origRenamedSavePath ).data() );
    if( errcode != 0 )
    {
        debug() << "Could not move original!" << endl;
        perror( "Could not move original!" );
        goto fail_remove_copy;
    }

    origRenamedFile = new QFile( m_origRenamedSavePath );
    origRenamedFile->open( IO_ReadOnly );
    md5sum = new KMD5( origRenamedFile->readAll() );
    origRenamedFile->close();
    delete origRenamedFile;
    origRenamedDigest = md5sum->hexDigest();

    debug() << "md5sum of original file: " << origRenamedDigest.data() << endl;

    if( origRenamedDigest != m_tempSaveDigest )
    {
        debug() << "Original checksum did not match current checksum!" << endl;
        revert = true;
        goto fail_remove_copy;
    }

    debug() << "Renaming temp file to original's filename" << endl;

    errcode = std::rename( QFile::encodeName( m_tempSavePath ).data(),
                                QFile::encodeName( m_bundle->url().path() ).data() );
    if( errcode != 0 )
    {
        debug() << "Could not rename newly-tagged file to original!" << endl;
        perror( "Could not rename newly-tagged file to original!" );
        goto fail_remove_copy;
    }

    debug() << "Deleting original" << endl;

    errcode = std::remove( QFile::encodeName( m_origRenamedSavePath ) );
    if( errcode != 0 )
    {
        debug() << "Could not delete the original file!" << endl;
        perror( "Could not delete the original file!" );
        return false;
    }

    debug() << "Save done, returning true!" << endl;

    return true;

    fail_remove_copy:

        errcode = std::remove( QFile::encodeName( m_tempSavePath ).data() );
        if( errcode != 0 )
        {
            debug() << "Could not delete the temporary file!" << endl;
            perror( "Could not delete the temporary file!" );
            return false;
        }

        if( !revert )
            return false;


        errcode = std::rename( QFile::encodeName( m_origRenamedSavePath ).data(),
                                QFile::encodeName( m_bundle->url().path() ).data() );
        if( errcode != 0 )
        {
            debug() << "Could not revert file to original filename!" << endl;
            perror( "Could not revert file to original filename!" );
        }

        return false;
}

bool
MetaBundleSaver::cleanupSave()
{
    DEBUG_BLOCK

    bool dirty = false;

    if( !(m_tempSavePath == QString::null) && QFile::exists( m_tempSavePath ) )
    {
        int errcode;
        errcode = std::remove( QFile::encodeName( m_tempSavePath ).data() );
        if( errcode != 0 )
        {
            dirty = true;
            debug() << "Could not delete the temporary file!" << endl;
        }
    }

    m_tempSavePath = QString::null;
    m_origRenamedSavePath = QString::null;
    m_tempSaveDigest = QCString( 0 );
    if( m_saveFileref )
        delete m_saveFileref;

    return !dirty;
}

#include "metabundlesaver.moc"
