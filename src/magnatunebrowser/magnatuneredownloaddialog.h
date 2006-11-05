//
// C++ Interface:
//
// Description:
//
//
// Author: Mark Kretschmann <markey@web.de>, (C) 2006
//
// Copyright: See COPYING file that comes with this distribution
//
//

#ifndef MAGNATUNE_REDOWNLOAD_DIALOG_H
#define MAGNATUNE_REDOWNLOAD_DIALOG_H

#include "magnatuneredownloaddialogbase.h"

#include <qstringlist.h>

class MagnatuneRedownloadDialog : public magnatuneReDownloadDialogBase
{
    Q_OBJECT

public:
    MagnatuneRedownloadDialog( QWidget* parent = 0, const char* name = 0, bool modal = FALSE, WFlags fl = 0 );
    ~MagnatuneRedownloadDialog();
    /*$PUBLIC_FUNCTIONS$*/

    void setRedownloadItems(QStringList items);

signals:

    void redownload(QString downloadInfoFileName);
    void cancelled();

public slots:
    /*$PUBLIC_SLOTS$*/

protected:
    /*$PROTECTED_FUNCTIONS$*/

protected slots:
    /*$PROTECTED_SLOTS$*/
    void redownload();
    void selectionChanged();
    void reject();

};

#endif

