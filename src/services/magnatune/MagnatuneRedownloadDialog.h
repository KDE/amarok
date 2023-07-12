/****************************************************************************************
 * Copyright (c) 2006,2007 Nikolaj Hald Nielsen <nhn@kde.org>                           *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#ifndef MAGNATUNE_REDOWNLOAD_DIALOG_H
#define MAGNATUNE_REDOWNLOAD_DIALOG_H

#include "ui_MagnatuneRedownloadDialogBase.h"

#include "MagnatuneDownloadInfo.h"

#include <QStringList>

class MagnatuneRedownloadDialog : public QDialog, public Ui::magnatuneReDownloadDialogBase
{
    Q_OBJECT

public:
    explicit MagnatuneRedownloadDialog( QWidget* parent = nullptr, const char* name = nullptr, bool modal = false, Qt::WindowFlags fl = {} );
    ~MagnatuneRedownloadDialog();
    /*$PUBLIC_FUNCTIONS$*/

    void setRedownloadItems( const QStringList &items );
    void setRedownloadItems(const QList<MagnatuneDownloadInfo> &previousPurchases );

Q_SIGNALS:

//     void redownload( const QString &downloadInfoFileName );
    void redownload( const MagnatuneDownloadInfo &info );
    void cancelled();

public Q_SLOTS:
    /*$PUBLIC_SLOTS$*/

protected:
    QMap <QTreeWidgetItem*, MagnatuneDownloadInfo> m_infoMap;

protected Q_SLOTS:
    /*$PROTECTED_SLOTS$*/
    void slotRedownload();
    void selectionChanged();
    void reject() override;

};

#endif

