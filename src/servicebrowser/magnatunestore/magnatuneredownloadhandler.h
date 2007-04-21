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
 *   51 Franklin Steet, Fifth Floor, Boston, MA  02111-1307, USA.          *
 ***************************************************************************/ 

#ifndef MAGNATUNE_REDOWNLOAD_HANDLER_H
#define MAGNATUNE_REDOWNLOAD_HANDLER_H

#include "magnatunealbumdownloader.h"
#include "magnatunedownloaddialog.h"
#include "magnatunedownloadinfo.h"
#include "magnatuneredownloaddialog.h"

#include <QObject>

/**
This class handles the redownloading of previously purchased albums

	@author Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>
*/

class MagnatuneRedownloadHandler : public QObject {
Q_OBJECT
public:
    MagnatuneRedownloadHandler(QWidget * parent);

    ~MagnatuneRedownloadHandler();

    /**
     * Calls forth the redownload dialog.
     */
    void showRedownloadDialog();

signals:

    void reDownloadCompleted( bool success );

protected:

    QWidget * m_parent;
    QStringList GetPurchaseList( );
    MagnatuneRedownloadDialog * m_redownloadDialog;
    MagnatuneDownloadDialog * m_downloadDialog;
    MagnatuneAlbumDownloader * m_albumDownloader;

protected slots:

    void redownload(const QString &storedInfoFileName);
    void selectionDialogCancelled();
    void albumDownloadComplete( bool success );



};

#endif
