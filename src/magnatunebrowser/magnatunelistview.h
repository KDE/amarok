//
// C++ Interface: magnatunelistview
//
// Description: 
//
//
// Author: Mark Kretschmann <markey@web.de>, (C) 2006
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef MAGNATUNELISTVIEW_H
#define MAGNATUNELISTVIEW_H

#include "magnatunelistviewitems.h"

#include <qlistview.h>
#include <kurldrag.h>

/**
A specialised QListView that provides drag and drop functionality and decoration

	@author Mark Kretschmann <markey@web.de>
*/
class MagnatuneListView : public QListView
{
public:
    MagnatuneListView(QWidget * parent);

    ~MagnatuneListView();

protected:

    KURLDrag * dragObject();

};

#endif
