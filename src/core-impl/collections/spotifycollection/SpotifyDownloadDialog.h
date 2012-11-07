/****************************************************************************************
 * Copyright (c) 2012 Ryan Feng <odayfans@gmail.com>                                    *
 * Copyright (c) 2012 Bart Cerneels <bart.cerneels@kde.org>                             *
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
#ifndef SPOTIFYDOWNLOADDIALOG_H
#define SPOTIFYDOWNLOADDIALOG_H

#include <QDialog>
#include <QNetworkReply>

namespace Ui {
class SpotifyDownloadDialog;
}

class SpotifyDownloadDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SpotifyDownloadDialog(QWidget *parent = 0);
    ~SpotifyDownloadDialog();

private Q_SLOTS:
    void slotDownloadError( QNetworkReply::NetworkError error );
    void slotDownloadProgress( qint64 current, qint64 total );
    void slotDownloadFinished();
    void tryDownloadResolver();

private:
    Ui::SpotifyDownloadDialog *m_ui;
    QNetworkReply *m_downloadReply;
};

#endif // SPOTIFYDOWNLOADDIALOG_H
