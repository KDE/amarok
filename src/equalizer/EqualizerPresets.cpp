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

#include "EqualizerPresets.h"

#include "amarokconfig.h"
#include "core/support/Debug.h"

#include <KLocalizedString>

#define NUM_EQ_VALUES 11

static int DEFAULT_PRESET_VALUES[][NUM_EQ_VALUES] =
{
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, // Manual
    {0, 0, 0, 0, 0, 0, 0, -40, -40, -40, -50}, // Classical
    {0, 0, 0, 20, 30, 30, 30, 20, 0, 0, 0}, // Club
    {-10, 50, 35, 10, 0, 0, -30, -40, -40, 0, 0}, // Dance
    {0, 29, 40, 23, 15, 0, 0, 0, 0, 0, 0}, // Full Bass
    {-83, -50, -50, -50, -25, 15, 55, 80, 80, 80, 85}, // Full Treble
    {-41, 35, 30, 0, -40, -25, 10, 45, 55, 60, 60}, // Full Bass + Treble
    {-16, 25, 50, 25, -20, 0, -30, -40, -40, 0, 0}, // Laptop/Headphones
    {-25, 50, 50, 30, 30, 0, -25, -25, -25, 0, 0}, // Large Hall
    {0, -25, 0, 20, 25, 30, 30, 20, 15, 15, 10}, // Live
    {0, 35, 35, 0, 0, 0, 0, 0, 0, 35, 35}, // Party
    {-15, -10, 25, 35, 40, 25, -5, -15, -15, -10, -10}, // Pop
    {0, 0, 0, -5, -30, 0, -35, -35, 0, 0, 0}, // Reggae
    {-28, 40, 25, -30, -40, -20, 20, 45, 55, 55, 55}, // Rock
    {-33, 25, 10, -5, -15, -5, 20, 45, 50, 55, 60}, // Soft
    {-29, -15, -25, -25, -5, 20, 30, 45, 50, 55, 50}, // Ska
    {0, 20, 20, 10, -5, -25, -30, -20, -5, 15, 45}, // Soft Rock
    {-26, 40, 30, 0, -30, -25, 0, 40, 50, 50, 45}, // Techno
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0} // Zero
};

QStringList
EqualizerPresets::eqDefaultPresetsList()
{
    QStringList presets;
    presets << "Manual"
            << "Classical"
            << "Club"
            << "Dance"
            << "Full Bass"
            << "Full Treble"
            << "Full Bass + Treble"
            << "Laptop/Headphones"
            << "Large Hall"
            << "Live"
            << "Party"
            << "Pop"
            << "Reggae"
            << "Rock"
            << "Soft"
            << "Ska"
            << "Soft Rock"
            << "Techno"
            << "Zero";
    return presets;
}

QStringList
EqualizerPresets::eqDefaultTranslatedPresetsList()
{
    QStringList strings;
    strings << i18n( "Manual" );
    strings << i18n( "Classical" );
    strings << i18n( "Club" );
    strings << i18n( "Dance" );
    strings << i18n( "Full Bass" );
    strings << i18n( "Full Treble" );
    strings << i18n( "Full Bass + Treble" );
    strings << i18n( "Laptop/Headphones" );
    strings << i18n( "Large Hall" );
    strings << i18n( "Live" );
    strings << i18n( "Party" );
    strings << i18n( "Pop" );
    strings << i18n( "Reggae" );
    strings << i18n( "Rock" );
    strings << i18n( "Soft" );
    strings << i18n( "Ska" );
    strings << i18n( "Soft Rock" );
    strings << i18n( "Techno");
    strings << i18n( "Zero" );
    return strings;
}

QStringList
EqualizerPresets::eqGlobalTranslatedList()
{
    QStringList globalTranslatedList = eqDefaultTranslatedPresetsList();
    globalTranslatedList += eqUserList();
    return globalTranslatedList;
}

QStringList
EqualizerPresets::eqGlobalList()
{
    // This function will build up a global list
    // first a default preset will comes
    // then user list is filtered to omit duplicates from default preset list
    QStringList mGlobalList;
    mGlobalList += eqDefaultPresetsList();
    mGlobalList += eqUserList();
    return mGlobalList;
}

QStringList
EqualizerPresets::eqUserList()
{
    const QStringList defaultList = eqDefaultPresetsList();

    QStringList userList;
    for( const QString &mUsrName : AmarokConfig::equalizerPresetsNames() )
    {
        if( !defaultList.contains( mUsrName ) )
            userList.append( mUsrName );
    }
    return userList;
}

// Equalizer preset management helper functions
bool
EqualizerPresets::eqCfgDeletePreset( const QString &presetName )
{
      // Idea is to delete the preset only if it is user preset:
      // present on user list & absent on default list
      const int idUsr = AmarokConfig::equalizerPresetsNames().indexOf( presetName );
      const int idDef = eqDefaultPresetsList().indexOf( presetName );

      if( idUsr >= 0 && idDef < 0 )
      {
          QStringList mNewNames = AmarokConfig::equalizerPresetsNames();
          QList<int> mNewValues = AmarokConfig::equalizerPresestValues();
          mNewNames.removeAt( idUsr );

          for( int it = 0; it < NUM_EQ_VALUES; it++ )
              mNewValues.removeAt( NUM_EQ_VALUES*idUsr );

          AmarokConfig::setEqualizerPresetsNames( mNewNames );
          AmarokConfig::setEqualizerPresestValues( mNewValues );
          return true;
      }

      return false;
}

bool
EqualizerPresets::eqCfgRestorePreset( const QString &presetName )
{
      // Idea is to delete the preset if it found on both
      // user list and default list - delete from the latter if so
      const int idUsr = AmarokConfig::equalizerPresetsNames().indexOf( presetName );
      const int idDef = eqDefaultPresetsList().indexOf( presetName );

      if( idUsr >= 0 && idDef >= 0 )
      {
          QStringList mNewNames = AmarokConfig::equalizerPresetsNames();
          QList<int> mNewValues = AmarokConfig::equalizerPresestValues();
          mNewNames.removeAt( idUsr );

          for( int it = 0; it < NUM_EQ_VALUES; it++ )
              mNewValues.removeAt( NUM_EQ_VALUES*idUsr );

          AmarokConfig::setEqualizerPresetsNames( mNewNames );
          AmarokConfig::setEqualizerPresestValues( mNewValues );
          return true;
      }

      return false;
}

bool
EqualizerPresets::eqCfgCanRestorePreset( const QString &presetName )
{
      // Idea is to delete the preset if it found on both
      // user list and default list - delete from the latter if so
      const int idUsr = AmarokConfig::equalizerPresetsNames().indexOf( presetName );
      const int idDef = eqDefaultPresetsList().indexOf( presetName );

      return ( idUsr >= 0 && idDef >= 0 );
}

void
EqualizerPresets::eqCfgSetPresetVal( const QString &presetName, const QList<int> &presetValues)
{
    DEBUG_BLOCK

    debug() << "Preset:" << presetName << presetValues;

    // Idea is to insert new values into user list
    // if preset exist on the list - replace it values
    const int idUsr = AmarokConfig::equalizerPresetsNames().indexOf( presetName );
    QStringList mNewNames = AmarokConfig::equalizerPresetsNames();
    QList<int> mNewValues = AmarokConfig::equalizerPresestValues();
    debug() << "Old preset found:" << (idUsr >= 0);

    if( idUsr < 0 )
    {
        mNewNames.append( presetName );
        mNewValues += presetValues;
    }
    else
    {
        for( int it = 0; it < NUM_EQ_VALUES; it++ )
            mNewValues.replace( idUsr * NUM_EQ_VALUES + it, presetValues.value(it) );
    }
    AmarokConfig::setEqualizerPresetsNames( mNewNames );
    AmarokConfig::setEqualizerPresestValues( mNewValues );
}

QList<int>
EqualizerPresets::eqCfgGetPresetVal( const QString &presetName )
{
      // Idea is to return user preset with request name first
      // if not look into the default preset names
      const int idUsr = AmarokConfig::equalizerPresetsNames().indexOf( presetName );
      const int idDef = eqDefaultPresetsList().indexOf( presetName );

      QList<int> mPresetVal;
      if( idUsr >= 0 )
          mPresetVal = AmarokConfig::equalizerPresestValues().mid( idUsr * NUM_EQ_VALUES, NUM_EQ_VALUES );
      else if( idDef >= 0) {
          for (int i = 0; i < NUM_EQ_VALUES; ++i)
              mPresetVal << DEFAULT_PRESET_VALUES[idDef][i];
      }

      return mPresetVal;
}
