// (c) 2004 Christian Muehlhaeuser <chris@chris.de>
// See COPYING file for licensing information


#ifndef AMAROK_SCROBBLER_H
#define AMAROK_SCROBBLER_H

#include <kio/job.h>
#include <kio/jobclasses.h>

#include <qobject.h>         //baseclass
#include <qstringlist.h>

class Scrobbler : public QObject
{
    Q_OBJECT

    public:
        Scrobbler();
        ~Scrobbler();

        void relatedArtists( QString artist );

    signals:
        void relatedArtistsFetched( const QString& artist, const QStringList& suggestions );

    private slots:
        void audioScrobblerRelatedArtistData( KIO::Job* job, const QByteArray& data );
        void audioScrobblerRelatedArtistResult( KIO::Job* job );

    private:
        QString m_buffer;
        QString m_artist;

};

#endif /* AMAROK_SCROBBLER_H */
