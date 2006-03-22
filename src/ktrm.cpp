/***************************************************************************
    copyright            : (C) 2004 by Scott Wheeler
    email                : wheeler@kde.org
 ***************************************************************************/

/***************************************************************************
 *   This library is free software; you can redistribute it and/or modify  *
 *   it  under the terms of the GNU Lesser General Public License version  *
 *   2.1 as published by the Free Software Foundation.                     *
 *                                                                         *
 *   This library is distributed in the hope that it will be useful, but   *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU     *
 *   Lesser General Public License for more details.                       *
 *                                                                         *
 *   You should have received a copy of the GNU Lesser General Public      *
 *   License along with this library; if not, write to the Free Software   *
 *   Foundation, Inc., 51 Franklin Steet, Fifth Floor, Boston, MA  02111-1307  *
 *   USA                                                                   *
 ***************************************************************************/

#include "config.h"
#include "debug.h"
#define DEBUG_PREFIX "KTRM"

#include "ktrm.h"

#include <kapplication.h>
#include <kprotocolmanager.h>
#include <kurl.h>

#include <qmutex.h>
#include <qevent.h>
#include <qobject.h>
#include <qfile.h>
#include <qregexp.h>

#if HAVE_TUNEPIMP

#include <tunepimp/tp_c.h>

class KTRMLookup;

extern "C"
{
#if HAVE_TUNEPIMP >= 4
    static void TRMNotifyCallback(tunepimp_t pimp, void *data, TPCallbackEnum type, int fileId, TPFileStatus status);
#else
    static void TRMNotifyCallback(tunepimp_t pimp, void *data, TPCallbackEnum type, int fileId);
#endif
}

/**
 * This represents the main TunePimp instance and handles incoming requests.
 */

class KTRMRequestHandler
{
public:
    static KTRMRequestHandler *instance()
    {
        static QMutex mutex;
        mutex.lock();
        static KTRMRequestHandler handler;
        mutex.unlock();
        return &handler;
    }

    int startLookup(KTRMLookup *lookup)
    {
        int id;

        if(!m_fileMap.contains(lookup->file())) {
#if HAVE_TUNEPIMP >= 4
            id = tp_AddFile(m_pimp, QFile::encodeName(lookup->file()), 0);
#else
            id = tp_AddFile(m_pimp, QFile::encodeName(lookup->file()));
#endif
            m_fileMap.insert(lookup->file(), id);
        }
        else {
            id = m_fileMap[lookup->file()];
            tp_IdentifyAgain(m_pimp, id);
        }
        m_lookupMap[id] = lookup;
        return id;
    }

    void endLookup(KTRMLookup *lookup)
    {
        tp_ReleaseTrack(m_pimp, tp_GetTrack(m_pimp, lookup->fileId()));
        tp_Remove(m_pimp, lookup->fileId());
        m_lookupMap.remove(lookup->fileId());
    }

    bool lookupMapContains(int fileId) const
    {
        m_lookupMapMutex.lock();
        bool contains = m_lookupMap.contains(fileId);
        m_lookupMapMutex.unlock();
        return contains;
    }

    KTRMLookup *lookup(int fileId) const
    {
        m_lookupMapMutex.lock();
        KTRMLookup *l = m_lookupMap[fileId];
        m_lookupMapMutex.unlock();
        return l;
    }

    void removeFromLookupMap(int fileId)
    {
        m_lookupMapMutex.lock();
        m_lookupMap.remove(fileId);
        m_lookupMapMutex.unlock();
    }

    const tunepimp_t &tunePimp() const
    {
        return m_pimp;
    }

protected:
    KTRMRequestHandler()
    {
        m_pimp = tp_New("KTRM", "0.1");
        //tp_SetDebug(m_pimp, true);
        tp_SetTRMCollisionThreshold(m_pimp, 100);
        tp_SetAutoFileLookup(m_pimp,true);
        tp_SetAutoSaveThreshold(m_pimp, -1);
        tp_SetMoveFiles(m_pimp, false);
        tp_SetRenameFiles(m_pimp, false);
#if HAVE_TUNEPIMP >= 4
        tp_SetFileNameEncoding(m_pimp, "UTF-8");
#else
        tp_SetUseUTF8(m_pimp, true);
#endif
        tp_SetNotifyCallback(m_pimp, TRMNotifyCallback, 0);

        if(KProtocolManager::useProxy()) {
            KURL proxy = KProtocolManager::proxyFor("http");
            tp_SetProxy(m_pimp, proxy.host().latin1(), short(proxy.port()));
        }
    }

    ~KTRMRequestHandler()
    {
        tp_Delete(m_pimp);
    }

private:
    tunepimp_t m_pimp;
    QMap<int, KTRMLookup *> m_lookupMap;
    QMap<QString, int> m_fileMap;
    mutable QMutex m_lookupMapMutex;
};


/**
 * A custom event type used for signalling that a TRM lookup is finished.
 */

class KTRMEvent : public QCustomEvent
{
public:
    enum Status {
        Recognized,
        Unrecognized,
        Collision,
        Error
    };

    KTRMEvent(int fileId, Status status) :
        QCustomEvent(id),
        m_fileId(fileId),
        m_status(status) {}

    int fileId() const
    {
        return m_fileId;
    }

    Status status() const
    {
        return m_status;
    }

    static const int id = User + 1984; // random, unique, event id

private:
    int m_fileId;
    Status m_status;
};

/**
 * A helper class to intercept KTRMQueryEvents and call recognized() (from the GUI
 * thread) for the lookup.
 */

class KTRMEventHandler : public QObject
{
public:
    static void send(int fileId, KTRMEvent::Status status)
    {
        KApplication::postEvent(instance(), new KTRMEvent(fileId, status));
    }

protected:
    KTRMEventHandler() : QObject() {}

    static KTRMEventHandler *instance()
    {
        static QMutex mutex;
        mutex.lock();
        static KTRMEventHandler handler;
        mutex.unlock();
        return &handler;
    }

    virtual void customEvent(QCustomEvent *event)
    {
        if(!event->type() == KTRMEvent::id)
            return;

        KTRMEvent *e = static_cast<KTRMEvent *>(event);

        static QMutex mutex;
        mutex.lock();

        if(!KTRMRequestHandler::instance()->lookupMapContains(e->fileId())) {
            mutex.unlock();
            return;
        }

        KTRMLookup *lookup = KTRMRequestHandler::instance()->lookup(e->fileId());
#if HAVE_TUNEPIMP >= 4
        if ( e->status() != KTRMEvent::Unrecognized)
#endif
            KTRMRequestHandler::instance()->removeFromLookupMap(e->fileId());

        mutex.unlock();

        switch(e->status()) {
        case KTRMEvent::Recognized:
            lookup->recognized();
            break;
        case KTRMEvent::Unrecognized:
            lookup->unrecognized();
            break;
        case KTRMEvent::Collision:
            lookup->collision();
            break;
        case KTRMEvent::Error:
            lookup->error();
            break;
        }
    }
};

/**
 * Callback fuction for TunePimp lookup events.
 */
#if HAVE_TUNEPIMP >= 4
static void TRMNotifyCallback(tunepimp_t /*pimp*/, void */*data*/, TPCallbackEnum type, int fileId, TPFileStatus status)
#else
static void TRMNotifyCallback(tunepimp_t pimp, void */*data*/, TPCallbackEnum type, int fileId)
#endif
{
    if(type != tpFileChanged)
        return;

#if HAVE_TUNEPIMP < 4
    track_t track = tp_GetTrack(pimp, fileId);
    TPFileStatus status = tr_GetStatus(track);
#endif

    switch(status) {
    case eRecognized:
        KTRMEventHandler::send(fileId, KTRMEvent::Recognized);
        break;
    case eUnrecognized:
        KTRMEventHandler::send(fileId, KTRMEvent::Unrecognized);
        break;
    case eTRMCollision:
#if HAVE_TUNEPIMP >= 4
    case eUserSelection:
#endif
        KTRMEventHandler::send(fileId, KTRMEvent::Collision);
        break;
    case eError:
        KTRMEventHandler::send(fileId, KTRMEvent::Error);
        break;
    default:
        break;
    }
#if HAVE_TUNEPIMP < 4
    tp_ReleaseTrack(pimp, track);
#endif
}

////////////////////////////////////////////////////////////////////////////////
// KTRMResult implementation
////////////////////////////////////////////////////////////////////////////////

class KTRMResult::KTRMResultPrivate
{
public:
    KTRMResultPrivate() : track(0), year(0), relevance(0) {}
    QString title;
    QString artist;
    QString album;
    int track;
    int year;
    double relevance;

    bool operator== (const KTRMResultPrivate &r) const;
};

bool KTRMResult::KTRMResultPrivate::operator==(const KTRMResultPrivate &r) const
{
	return (
			title == r.title &&
			artist == r.artist &&
			album == r.album &&
			track == r.track &&
			year == r.year &&
			relevance == r.relevance
	       );
}

////////////////////////////////////////////////////////////////////////////////
// KTRMResult public methods
////////////////////////////////////////////////////////////////////////////////
#endif
KTRMResult::KTRMResult()
{
#if HAVE_TUNEPIMP
    d = new KTRMResultPrivate;
#endif
}

KTRMResult::KTRMResult(const KTRMResult &result)
{
#if HAVE_TUNEPIMP
    d = new KTRMResultPrivate(*result.d);
#endif
}

KTRMResult::~KTRMResult()
{
#if HAVE_TUNEPIMP
    delete d;
#endif
}

QString KTRMResult::title() const
{
#if HAVE_TUNEPIMP
    return d->title;
#else
    return QString::null;
#endif
}

QString KTRMResult::artist() const
{
#if HAVE_TUNEPIMP
    return d->artist;
#else
    return QString::null;
#endif
}

QString KTRMResult::album() const
{
#if HAVE_TUNEPIMP
    return d->album;
#else
    return QString::null;
#endif
}

int KTRMResult::track() const
{
#if HAVE_TUNEPIMP
    return d->track;
#else
    return 0;
#endif
}

int KTRMResult::year() const
{
#if HAVE_TUNEPIMP
    return d->year;
#else
    return 0;
#endif
}

bool KTRMResult::operator<(const KTRMResult &r) const
{
#if HAVE_TUNEPIMP
    return r.d->relevance < d->relevance;
#else
    return false;
#endif
}

bool KTRMResult::operator>(const KTRMResult &r) const
{
#if HAVE_TUNEPIMP
    return r.d->relevance > d->relevance;
#else
    return true;
#endif
}

KTRMResult &KTRMResult::operator= (const KTRMResult &r)
{
#if HAVE_TUNEPIMP
    d = new KTRMResultPrivate(*r.d);
#endif
    return *this;
}

bool KTRMResult::operator== (const KTRMResult &r) const
{
#if HAVE_TUNEPIMP
    return *d == *(r.d);
#endif
    return false;
}


bool KTRMResult::isEmpty() const
{
#if HAVE_TUNEPIMP
    return d->title.isEmpty() && d->artist.isEmpty() && d->album.isEmpty() &&
        d->track == 0 && d->year == 0;
#else
    return true;
#endif
}
#if HAVE_TUNEPIMP
////////////////////////////////////////////////////////////////////////////////
// KTRMLookup implementation
////////////////////////////////////////////////////////////////////////////////

class KTRMLookup::KTRMLookupPrivate
{
public:
    KTRMLookupPrivate() :
        fileId(-1) {}
    QString file;
    QString errorString;
    KTRMResultList results;
    int fileId;
    bool autoDelete;
};
#endif
////////////////////////////////////////////////////////////////////////////////
// KTRMLookup public methods
////////////////////////////////////////////////////////////////////////////////

KTRMLookup::KTRMLookup(const QString &file, bool autoDelete)
    : QObject()
{
#if HAVE_TUNEPIMP
    d = new KTRMLookupPrivate;
    d->file = file;
    d->autoDelete = autoDelete;
    d->fileId = KTRMRequestHandler::instance()->startLookup(this);
#endif
}

KTRMLookup::~KTRMLookup()
{
#if HAVE_TUNEPIMP
    KTRMRequestHandler::instance()->endLookup(this);
    delete d;
#endif
}

QString KTRMLookup::file() const
{
#if HAVE_TUNEPIMP
    return d->file;
#else
    return QString::null;
#endif
}

int KTRMLookup::fileId() const
{
#if HAVE_TUNEPIMP
    return d->fileId;
#else
    return -1;
#endif
}

void KTRMLookup::recognized()
{
#if HAVE_TUNEPIMP
    debug() << k_funcinfo << d->file << endl;

    d->results.clear();

    metadata_t *metaData = md_New();
    track_t track = tp_GetTrack(KTRMRequestHandler::instance()->tunePimp(), d->fileId);
    tr_Lock(track);
    tr_GetServerMetadata(track, metaData);

    KTRMResult result;

    result.d->title = QString::fromUtf8(metaData->track);
    result.d->artist = QString::fromUtf8(metaData->artist);
    result.d->album = QString::fromUtf8(metaData->album);
    result.d->track = metaData->trackNum;
    result.d->year = metaData->releaseYear;

    d->results.append(result);

    md_Delete(metaData);
    tr_Unlock(track);
    finished();
#endif
}

void KTRMLookup::unrecognized()
{
#if HAVE_TUNEPIMP
    debug() << k_funcinfo << d->file << endl;
    #if HAVE_TUNEPIMP >= 4
    char trm[255];
    bool finish = false;
    trm[0] = 0;
    track_t track = tp_GetTrack(KTRMRequestHandler::instance()->tunePimp(), d->fileId);
    tr_Lock(track);
    tr_GetTRM(track, trm, 255);
    if ( !trm[0] ) {
        tr_SetStatus(track, ePending);
        tp_Wake(KTRMRequestHandler::instance()->tunePimp(), track);
    }
    else
        finish = true;
    tr_Unlock(track);
    tp_ReleaseTrack(KTRMRequestHandler::instance()->tunePimp(), track);
    if ( !finish )
        return;
    #endif
    d->results.clear();
    finished();

#endif
}

void KTRMLookup::collision()
{
#if HAVE_TUNEPIMP
    debug() << k_funcinfo << d->file << endl;

    track_t track = tp_GetTrack(KTRMRequestHandler::instance()->tunePimp(), d->fileId);

    if(track <= 0) {
        debug() << "invalid track number" << endl;
        return;
    }

    tr_Lock(track);
    int resultCount = tr_GetNumResults(track);

    QStringList strList = QStringList::split ( '/', d->file );

    metadata_t *mdata = md_New();
    strList.append( QString::fromUtf8(mdata->track) );
    strList.append( QString::fromUtf8(mdata->artist) );
    strList.append( QString::fromUtf8(mdata->album) );
    md_Clear(mdata);

    if(resultCount > 0) {
        TPResultType type;
        result_t *results = new result_t[resultCount];
        tr_GetResults(track, &type, results, &resultCount);

        switch(type) {
        case eNone:
            debug() << k_funcinfo << "eNone" << endl;
            break;
        case eArtistList:
            debug() << "eArtistList" << endl;
            break;
        case eAlbumList:
            debug() << "eAlbumList" << endl;
            break;
        case eTrackList:
        {
            debug() << "eTrackList" << endl;
            albumtrackresult_t **tracks = (albumtrackresult_t **) results;
            d->results.clear();

            for(int i = 0; i < resultCount; i++) {
                KTRMResult result;

                result.d->title = QString::fromUtf8(tracks[i]->name);
#if HAVE_TUNEPIMP >= 4
                result.d->artist = QString::fromUtf8(tracks[i]->artist.name);
                result.d->album = QString::fromUtf8(tracks[i]->album.name);
                result.d->year = tracks[i]->album.releaseYear;
#else
                result.d->artist = QString::fromUtf8(tracks[i]->artist->name);
                result.d->album = QString::fromUtf8(tracks[i]->album->name);
                result.d->year = tracks[i]->album->releaseYear;
#endif
                result.d->track = tracks[i]->trackNum;
                result.d->relevance =
                    4 * stringSimilarity(strList,result.d->title) +
                    2 * stringSimilarity(strList,result.d->artist) +
                    1 * stringSimilarity(strList,result.d->album);

                if(!d->results.contains(result)) d->results.append(result);
            }
            break;
        }
        case eMatchedTrack:
            debug() << k_funcinfo << "eMatchedTrack" << endl;
            break;
        }

        delete [] results;
    }

    tr_Unlock(track);
    qHeapSort(d->results);

    finished();
#endif
}

void KTRMLookup::error()
{
#if HAVE_TUNEPIMP
    debug() << k_funcinfo << d->file << endl;
    track_t track = tp_GetTrack(KTRMRequestHandler::instance()->tunePimp(), d->fileId);
    char error[1000];
    tr_GetError( track, error, 1000);
    debug() << "Error: " << error << endl;
    d->errorString = error;
    d->results.clear();
    finished();
#endif
}

KTRMResultList KTRMLookup::results() const
{
#if HAVE_TUNEPIMP
    return d->results;
#else
    return KTRMResultList();
#endif
}

////////////////////////////////////////////////////////////////////////////////
// KTRMLookup protected methods
////////////////////////////////////////////////////////////////////////////////

void KTRMLookup::finished()

{
#if HAVE_TUNEPIMP
    emit sigResult( results(), d->errorString );

    if(d->autoDelete)
        delete this;
#endif
}

////////////////////////////////////////////////////////////////////////////////
// Helper Functions used for sorting MusicBrainz results
////////////////////////////////////////////////////////////////////////////////
double stringSimilarity(QStringList &l, QString &s)
{
    double max = 0, current = 0;
    for ( QStringList::Iterator it = l.begin(); it != l.end(); ++it ) {
       if( max < (current = stringSimilarity((*it),s)))
            max = current;
    }
    return max;
}

double stringSimilarity(QString s1, QString s2)
{
    s1.remove( QRegExp("[\\s\\t\\r\\n]") );
    s2.remove( QRegExp("[\\s\\t\\r\\n]") );

    double nCommon = 0;
    int p1 = 0, p2 = 0, x1 = 0, x2 = 0;
    int l1 = s1.length(), l2 = s2.length(), l3 = l1 + l2;
    QChar c1 = 0, c2 = 0;

    while(p1 < l1 && p2 < l2) {
        c1 = s1.at(p1); c2 = s2.at(p2);
        if( c1.upper() == c2.upper()) {
            ++nCommon;
            ++p1; ++p2;
        }
        else {
            x1 = s1.find(c2,p1,false);
            x2 = s2.find(c1,p2,false);

            if( (x1 == x2 || -1 == x1) || (-1 != x2 && x1 > x2) )
                ++p2;
            else
                ++p1;
        }
    }
    return l3 ? (double)(nCommon*2) / (double)(l3) : 1;
}





#include "ktrm.moc"

