// (c) 2004 Mark Kretschmann <markey@web.de>
// See COPYING file for licensing information.

#ifndef AMAROK_COVERFETCHER_H
#define AMAROK_COVERFETCHER_H

#include <qobject.h>         //baseclass
#include <qstring.h>         //stack alloc

namespace KIO {
    class Job;
}

class CoverFetcher : public QObject
{
    Q_OBJECT
    
    public:
        CoverFetcher( QObject* parent = 0 );
        ~CoverFetcher();
    
        void setLicense( const QString license ) { m_license = license; }
        void getCover( const QString& keyword );
        
    private slots:
        void slotData( KIO::Job* job, const QByteArray& data );
        void slotResult( KIO::Job* job );        
        
    private:
        QString m_license;
        QString m_resultStr;
};    
    

#endif /* AMAROK_COVERFETCHER_H */
