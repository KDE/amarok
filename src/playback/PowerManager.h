/****************************************************************************************
 * Copyright (c) 2013 Anmol Ahuja <darthcodus@gmail.com>                                *
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

#ifndef AMAROK_POWERMANAGER_H
#define AMAROK_POWERMANAGER_H

#include <QObject>

class EngineController;
class QDBusInterface;

/**
 * Handle Amarok behavior on system suspend
 */
class PowerManager : public QObject
{
    Q_OBJECT

    public:
        PowerManager( EngineController *engine );
        ~PowerManager();

    private Q_SLOTS:
        void slotSettingsChanged();
        void slotHandleSuspend();
        void slotPlaying();
        void slotNotPlaying();

    private:
        void startInhibitingSuspend();
        void stopInhibitingSuspend();

        int m_inhibitionCookie;
        QDBusInterface *m_interface;
};

#endif // AMAROK_POWERMANAGER_H
