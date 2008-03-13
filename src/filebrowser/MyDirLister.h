#ifndef MYDIRLISTER_H
#define MYDIRLISTER_H

//TODO wait for lister to finish, if there are no files shown, but there are
//     media files in that directory show a longMessage (preferably one that disappears when given a signal)

#include "enginecontroller.h"

#include <kdirlister.h>
#include <kfileitem.h>

class MyDirLister : public KDirLister {
    Q_OBJECT
public:
    MyDirLister( bool delayedMimeTypes ) : KDirLister() { setDelayedMimeTypes( delayedMimeTypes ); setAutoUpdate( true ); }

protected:
    virtual bool matchesFilter( const KFileItem &item ) const;
};

#endif
