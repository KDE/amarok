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
        MyDirOperator( const KUrl &url, QWidget *parent );

};

#endif
