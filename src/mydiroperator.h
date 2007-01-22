#ifndef MYDIROPERATOR_H
#define MYDIROPERATOR_H

#include "mydirlister.h"

#include <kaction.h>
#include <kdiroperator.h>
#include <kpopupmenu.h>

class Medium;

class MyDirOperator : public KDirOperator {

    Q_OBJECT

    public:
        MyDirOperator( const KURL &url, QWidget *parent, Medium *medium = 0 );

    public slots:
        //reimplemented due to a bug in KDirOperator::activatedMenu ( KDE 3.4.2 ) - See Bug #103305
        virtual void activatedMenu (const KFileItem *, const QPoint &pos) {
            updateSelectionDependentActions();
            reenableDeleteKey();
            static_cast<KActionMenu*>(actionCollection()->action("popupMenu"))->popupMenu()->popup( pos );
        }
        void myHome();
        void myCdUp();

    private:
        void reenableDeleteKey();
    Medium *m_medium;

};

#endif
