/****************************************************************************************
 * Copyright (c) 2012 Ryan Feng <odayfans@gmail.com>                                    *
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
#ifndef SPOTIFYSETTINGS_H_
#define SPOTIFYSETTINGS_H_

#include "SpotifyConfig.h"

#include <KDialog>

namespace Ui {
    class SpotifySettingsWidget;
    class SpotifyDownloadDialog;
}

class KMessageWidget;

class SpotifySettingsDialog: public KDialog
{
    Q_OBJECT

public:
    explicit SpotifySettingsDialog( QWidget *parent = 0 );
    virtual ~SpotifySettingsDialog();

signals:
    void changed( bool );

public Q_SLOTS:
    virtual void save();
    virtual void load();
    virtual void defaults();
    void slotSettingsChanged();
    void slotCancel();

private Q_SLOTS:
    void slotTryLogin();
    void slotLoginSuccess(const QString &user);
    void slotLoginFailed(const QString &message);
    void slotCustomMessage(const QString &messageType, const QVariantMap &map);

private:
    Ui::SpotifySettingsWidget *m_settingsWidget;
    SpotifyConfig m_config;
};

#endif
