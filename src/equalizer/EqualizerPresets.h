/****************************************************************************************
 * Copyright (c) 2009 Artur Szymiec <artur.szymiec@gmail.com>                           *
 * Copyright (c) 2011 Kevin Funk <krf@electrostorm.net>                                 *
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

#ifndef EQUALIZERPRESETS_H
#define EQUALIZERPRESETS_H

#include <QList>
#include <QStringList>

class EqualizerPresets
{
public:
    EqualizerPresets();
    virtual ~EqualizerPresets();

    static QStringList eqDefaultPresetsList();
    static QStringList eqDefaultTranslatedPresetsList();

    static QStringList eqGlobalList();

    static QList<int> eqCfgGetPresetVal( const QString &presetName );

    bool eqCfgDeletePreset( const QString &presetName );
    bool eqCfgRestorePreset( const QString &presetName );
    void eqCfgSetPresetVal( const QString &presetName, const QList<int> &presetValues );
};

#endif // EQUALIZERPRESETS_H
