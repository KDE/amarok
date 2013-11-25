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

#ifndef AMAROK_EQUALIZER_SCRIPT_H
#define AMAROK_EQUALIZER_SCRIPT_H

#include <QObject>

class QScriptEngine;

namespace AmarokScript
{
    // SCRIPTDOX Amarok.Engine.Equalizer
    class AmarokEqualizerScript : public QObject
    {
        Q_OBJECT

        /**
         * @return true if the current backend supports equalizer.
         */
        Q_PROPERTY( bool enabled READ enabled )

        /**
         * @return true if the current backend supports equalizer.
         */
        Q_PROPERTY( bool isSupported READ isSupported )

        /**
         * The currently set equalizer preset.
         * Set to "" to disable.
         */
        Q_PROPERTY( QString selectedPreset READ selectedPreset WRITE setEqualizerPreset )

        /**
         * @return list of user presets.
         */
        Q_PROPERTY( QStringList userPresets READ userPresets )

        /**
         * Return a localized list of all presets.
         */
        Q_PROPERTY( QStringList translatedGlobalPresetList READ translatedGlobalPresetList )

        /**
         * Return an untranslated list of all presets.
         */
        Q_PROPERTY( QStringList globalPresetList READ globalPresetList )

        /**
         * @return list of all default Amarok presets.
         */
        Q_PROPERTY( QStringList translatedDefaultPresetList READ translatedDefaultPresetList )

        /**
         * @return translated list of all default Amarok presets.
         */
        Q_PROPERTY( QStringList defaultPresetList READ defaultPresetList )

        /**
         * The current equalizer gains values.
         */
        Q_PROPERTY( QList<int> gains READ gains WRITE setGains )

        Q_PROPERTY( int maxGain READ maxGain )

        Q_PROPERTY( QStringList bandFrequencies READ bandFrequencies )

        public:
            /**
            * Will return a "user" preset if present. Else a "global".
            */
            Q_INVOKABLE QList<int> getPresetGains( const QString &presetName );

            /**
            * Returns true if the preset was deleted.
            * @param presetName The untranslated preset name.
            */
            Q_INVOKABLE bool deletePreset( const QString &presetName );

            /**
             * Save the @param presetGains as a preset with name @param name.
             */
            Q_INVOKABLE void savePreset( const QString &name, const QList<int> &presetGains );

        signals:
            /**
             * Emitted when preset with name @param name is applied or the equalizer is disabled.
             * name is "" when disabled.
             */
            void presetApplied( QString name );

            /**
             * Emitted when the current gains are changed.
             */
            void gainsChanged( QList<int> gains );

            /**
             * Emitted when preset @param name is added, removed or modified.
             */
            void presetsChanged( QString name );

        private slots:
            void equalizerPresetApplied( int index );

        private:
            AmarokEqualizerScript( QScriptEngine* scriptEngine );
            bool enabled();
            void setEqualizerPreset( const QString &name ) const;
            QStringList bandFrequencies() const;
            bool isSupported() const;
            QString selectedPreset() const;
            QStringList userPresets() const;
            QStringList translatedGlobalPresetList() const;
            QStringList globalPresetList() const;
            QStringList translatedDefaultPresetList() const;
            QStringList defaultPresetList() const;
            QList<int> gains() const;
            void setGains( QList<int> gains );
            int maxGain() const;

            friend class AmarokEngineScript;
    };
}

#endif
