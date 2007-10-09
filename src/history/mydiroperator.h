#ifndef MYDIROPERATOR_H
#define MYDIROPERATOR_H

#include "mydirlister.h"

#include <kaction.h>
#include <kdiroperator.h>
#include <kmenu.h>

class Medium;

class MyDirOperator : public KDirOperator {

    Q_OBJECT

    public:
        MyDirOperator( const KUrl &url, QWidget *parent, Medium *medium = 0 );

    public slots:
        void myHome();
        void myCdUp();

    private:
        void reenableDeleteKey();
    Medium *m_medium;

};

#endif
