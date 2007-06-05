/***************************************************************************
 *   Copyright (c) 2006, 2007                                              *
 *        Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>                   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02111-1307, USA.          *
 ***************************************************************************/ 


#ifndef MAGNATUNE_REDOWNLOAD_DIALOG_H
#define MAGNATUNE_REDOWNLOAD_DIALOG_H

#include "magnatuneredownloaddialogbase.h"

#include <QStringList>

class MagnatuneRedownloadDialog : public magnatuneReDownloadDialogBase
{
    Q_OBJECT

public:
    explicit MagnatuneRedownloadDialog( QWidget* parent = 0, const char* name = 0, bool modal = false, Qt::WFlags fl = 0 );
    ~MagnatuneRedownloadDialog();
    /*$PUBLIC_FUNCTIONS$*/

    void setRedownloadItems(const QStringList &items);

signals:

    void redownload(const QString &downloadInfoFileName);
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

