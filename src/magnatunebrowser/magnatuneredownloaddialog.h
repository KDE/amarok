/*
 Copyright (c) 2006  Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Library General Public
 License as published by the Free Software Foundation; either
 version 2 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Library General Public License for more details.

 You should have received a copy of the GNU Library General Public License
 along with this library; see the file COPYING.LIB.  If not, write to
 the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 Boston, MA 02110-1301, USA.
*/


#ifndef MAGNATUNE_REDOWNLOAD_DIALOG_H
#define MAGNATUNE_REDOWNLOAD_DIALOG_H

#include "magnatuneredownloaddialogbase.h"

#include <qstringlist.h>

class MagnatuneRedownloadDialog : public magnatuneReDownloadDialogBase
{
    Q_OBJECT

public:
    MagnatuneRedownloadDialog( QWidget* parent = 0, const char* name = 0, bool modal = false, WFlags fl = 0 );
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

