/****************************************************************************************
 * Copyright (c) 2012 Sven Krohlas <sven@asbest-online.de>                              *
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

#ifndef AMZDOWNLOADER_H
#define AMZDOWNLOADER_H

#include "ui_AmzDownloader.h"

#include <QDialog>
#include <QDir>
#include <QProcess>

class AmzDownloader : public QDialog
{
    Q_OBJECT

public:
    explicit AmzDownloader( QWidget *parent = 0 );
    ~AmzDownloader();

public Q_SLOTS:
    void selectAmzClicked();
    void selectDirectoryClicked();
    void startClicked();
    void quitClicked();
    
private:
    void checkAmzList();

    Ui::AmzDownloader* ui;
    QStringList m_amzList;
    QDir m_downloadDir;
    QProcess m_clamzProcess;

private Q_SLOTS:
    void clamzError();
    void clamzFinished( int exitCode, QProcess::ExitStatus exitStatus );
    void clamzOutputAvailable();
};

#endif // AMZDOWNLOADER_H
