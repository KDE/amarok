// (c) 2004 Christian Muehlhaeuser <chris@chris.de>
// (c) 2004 Sami Nieminen <sami.nieminen@iki.fi>
// (c) 2006 Shane King <kde@dontletsstart.com>
// (c) 2006 Iain Benson <iain@arctos.me.uk>
// See COPYING file for licensing information

#ifndef AMAROK_SCROBBLER_H
#define AMAROK_SCROBBLER_H

#include "engineobserver.h"
#include <qdom.h>
#include <qobject.h>
#include <qptrdict.h>
#include <qptrlist.h>
#include <qtimer.h>

//some setups require this
#undef PROTOCOL_VERSION

namespace KIO { class Job; }

class QStringList;
class SubmitItem;
class SubmitQueue;
class ScrobblerSubmitter;

class Scrobbler : public QObject, public EngineObserver
{
    friend class MediaDevice;

    Q_OBJECT

    public:
        static Scrobbler *instance();

        void similarArtists( const QString & /*artist*/ );
        void applySettings();

    signals:
        void similarArtistsFetched( const QString& artist, const QStringList& suggestions );

    public slots:
        void subTrack( long currentPos, long startPos, long endPos ); // cuefiles can update length without track change

    protected:
        Scrobbler();
        ~Scrobbler();

        void engineNewMetaData( const MetaBundle& /*bundle*/, bool /*state*/ );
        void engineTrackPositionChanged( long /*position*/ , bool /*userSeek*/ );

    private slots:
        void audioScrobblerSimilarArtistsResult( KIO::Job* /*job*/ );
        void audioScrobblerSimilarArtistsData(
            KIO::Job* /*job*/, const QByteArray& /*data*/ );

    private:
        QTimer m_timer; //works around xine bug
                        //http://sourceforge.net/tracker/index.php?func=detail&aid=1401026&group_id=9655&atid=109655
        QByteArray m_similarArtistsBuffer;
        KIO::Job* m_similarArtistsJob;
        QString m_artist;
        bool m_validForSending;
        long m_startPos;
        ScrobblerSubmitter* m_submitter;
        SubmitItem* m_item;
};


class SubmitItem
{
    friend class ScrobblerSubmitter;

    public:
        SubmitItem(
            const QString& /*artist*/,
            const QString& /*album*/,
            const QString& /*title*/,
            int /*length*/,
            bool now = true );
        SubmitItem( const QDomElement& /* domElement */ );
        SubmitItem();

        bool operator==( const SubmitItem& item );

        void setArtist( const QString& artist ) { m_artist = artist; }
        void setAlbum( const QString& album ) { m_album = album; }
        void setTitle( const QString& title ) { m_title = title; }
        const QString artist() const { return m_artist; }
        const QString album() const { return m_album; }
        const QString title() const { return m_title; }
        int length() const { return m_length; }
        uint playStartTime() const { return m_playStartTime; }

        QDomElement toDomElement( QDomDocument& /* document */ ) const;

        bool valid() const { return !m_artist.isEmpty() && !m_title.isEmpty() && m_length >= 30; }

    private:
        QString m_artist;
        QString m_album;
        QString m_title;
        int m_length;
        uint m_playStartTime;
};


class SubmitQueue : public QPtrList<SubmitItem>
{
    protected:
        int compareItems( QPtrCollection::Item item1, QPtrCollection::Item item2 );
};


class ScrobblerSubmitter : public QObject
{
    Q_OBJECT

    public:
        static QString PROTOCOL_VERSION;
        static QString CLIENT_ID;
        static QString CLIENT_VERSION;
        static QString HANDSHAKE_URL;

        ScrobblerSubmitter();
        ~ScrobblerSubmitter();

        void submitItem( SubmitItem* /* item */ );

        void configure( const QString& /*username*/, const QString& /* password*/, bool /*enabled*/ );

        void syncComplete();

    private slots:
        void scheduledTimeReached();
        void audioScrobblerHandshakeResult( KIO::Job* /*job*/ );
        void audioScrobblerSubmitResult( KIO::Job* /*job*/ );
        void audioScrobblerSubmitData(
            KIO::Job* /*job*/, const QByteArray& /*data*/ );

    private:
        bool canSubmit() const;
        void enqueueItem( SubmitItem* /* item */ );
        SubmitItem* dequeueItem();
        void enqueueJob( KIO::Job* /* job */ );
        void finishJob( KIO::Job* /* job */ );
        void announceSubmit(
            SubmitItem* /* item */, int /* tracks */, bool /* success */ ) const;
        void saveSubmitQueue();
        void readSubmitQueue();
        bool schedule( bool failure );
        void performHandshake();
        void performSubmit();

        // on failure, start at MIN_BACKOFF, and double on subsequent failures
        // until MAX_BACKOFF is reached
        static const int MIN_BACKOFF = 60;
        static const int MAX_BACKOFF = 60 * 60;

        QString m_submitResultBuffer;
        QString m_username;
        QString m_password;
        QString m_submitUrl;
        QString m_challenge;
        QString m_savePath;
        bool m_scrobblerEnabled;
        bool m_holdFakeQueue;
        bool m_inProgress;
        bool m_needHandshake;
        uint m_prevSubmitTime;
        uint m_interval;
        uint m_backoff;
        uint m_lastSubmissionFinishTime;
        uint m_fakeQueueLength;

        QPtrDict<SubmitItem> m_ongoingSubmits;
        SubmitQueue m_submitQueue; // songs played by Amarok
        SubmitQueue m_fakeQueue; // songs for which play times have to be faked (e.g. when submitting from media device)

        QTimer m_timer;
};


#endif /* AMAROK_SCROBBLER_H */
