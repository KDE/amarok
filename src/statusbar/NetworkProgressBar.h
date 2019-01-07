/****************************************************************************************
 * Copyright (c) 2010 Rick W. Chen <stuffcorpse@archlinux.us>                           *
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

#ifndef AMAROK_NETWORK_PROGRESS_BAR_H
#define AMAROK_NETWORK_PROGRESS_BAR_H

#include "statusbar/ProgressBar.h"

#include <QNetworkReply>

/**
 * A specialized progress bar that takes a QNetworkReply in the constructor and keeps itself updated
 */
class NetworkProgressBar : public ProgressBar
{
    Q_OBJECT

    public:
        explicit NetworkProgressBar( QWidget *parent, QNetworkReply *reply );
        ~NetworkProgressBar();

    private Q_SLOTS:
        void progressChanged( qint64 bytesChanged, qint64 bytesTotal );
        void infoMessage( QNetworkReply::NetworkError code );
};

#endif
