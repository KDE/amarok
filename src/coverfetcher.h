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
        enum QueryMode { lite, heavy };
        
        CoverFetcher( QObject* parent = 0 );
        ~CoverFetcher();
    
        void setLicense( const QString license ) { m_license = license; }
        void getCover( const QString& keyword, QueryMode mode = lite );
        
    signals:
        void imageReady( const QString& keyword, const QPixmap& image );        
        
    private slots:
        void xmlData( KIO::Job* job, const QByteArray& data );
        void xmlResult( KIO::Job* job );        
        void imageData( KIO::Job* job, const QByteArray& data );
        void imageResult( KIO::Job* job );        
        
    private:
        QString m_license;
        QString m_xmlDocument;
        QString m_keyword;
        QByteArray m_image;
};    
    

#endif /* AMAROK_COVERFETCHER_H */
