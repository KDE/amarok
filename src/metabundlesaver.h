// Jeff Mitchell <kde-dev@emailgoeshere.com>, (C) 2006
// License: GNU General Public License V2

#ifndef METABUNDLESAVER_H
#define METABUNDLESAVER_H

#include <qobject.h>
#include <qstringlist.h>
#include <kurl.h>    //inline functions
#include <klocale.h> //inline functions
#include <taglib/audioproperties.h>
#include "expression.h"
#include "atomicstring.h"
#include "atomicurl.h"

#include "amarok_export.h"

namespace TagLib {
    class FileRef;
}
namespace KIO{
    class Job;
}

/**
 * @class MetaBundleSaver
 * @author Jeff Mitchell <kde-dev@emailgoeshere.com>
 */

class LIBAMAROK_EXPORT MetaBundleSaver : public QObject
{
    Q_OBJECT
public:
     MetaBundleSaver( MetaBundle *bundle );
    ~MetaBundleSaver();

    //bool scannerSafeSave( TagLib::File* file );
    TagLib::FileRef* prepareToSave();
    bool doSave();
    bool cleanupSave();
    void abortSave( const QString message );

public slots:
    void kioDone( KIO::Job *job );

private:
    MetaBundle *m_bundle;
    int m_waitingOnKIO;
    QString m_tempSavePath;
    QString m_origRenamedSavePath;
    QCString m_tempSaveDigest;
    TagLib::FileRef* m_saveFileref;
};

#endif
