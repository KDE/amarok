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

/** Functions that handles equalizer presets.
    There are two lists of presets. The globals are pre-defined and cannot
    be changed.
    The user presets can be created and deleted.
*/
namespace EqualizerPresets
{
    QStringList eqGlobalTranslatedList();
    QStringList eqGlobalList();

    /** Will return a list of all non default preset names */
    QStringList eqUserList();

    /** Will return a "user" preset if present. Else a "global". */
    QList<int> eqCfgGetPresetVal( const QString &presetName );

    /** Returns true if the preset was deleted.
        @param presetName The untranslated preset name.
    */
    bool eqCfgDeletePreset( const QString &presetName );

    /** Returns true if the preset was restored (meaning that there was
        a user preset and a global one and we just deleted the user preset)
        @param presetName The untranslated preset name.
    */
    bool eqCfgRestorePreset( const QString &presetName );

    /** Returns true if it is possible to restore the preset
        Meaning that there is a user and a default preset with the given name.
    */
    bool eqCfgCanRestorePreset( const QString &presetName );

    /** Sets the preset (create a user preset).
        @param presetName The untranslated preset name.
        @param presetValues The preset values.
    */
    void eqCfgSetPresetVal( const QString &presetName, const QList<int> &presetValues );

    /**
     * Will return a list of all default preset names untranslated.
     */
    QStringList eqDefaultPresetsList();

    /**
     * Will return a list of all default preset names translated.
     */
    QStringList eqDefaultTranslatedPresetsList();
};

#endif // EQUALIZERPRESETS_H
