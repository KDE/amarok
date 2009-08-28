/****************************************************************************************
 * Copyright (c) 2004-2009 Mark Kretschmann <kretschmann@kde.org>                       *
 * Copyright (c) 2009 Artur Szymiec <artur.szymiec@gmail.com>                           *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Pulic License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#ifndef PLAYBACKCONFIG_H
#define PLAYBACKCONFIG_H

#include "ui_PlaybackConfig.h"
#include "ConfigDialogBase.h"



class PlaybackConfig : public ConfigDialogBase, public Ui_PlaybackConfig
{
    Q_OBJECT

    public:
        PlaybackConfig( QWidget* parent );
        virtual ~PlaybackConfig();

        virtual bool hasChanged();
        virtual bool isDefault();
        virtual void updateSettings();
    public Q_SLOTS:
        void eqUpdateUI( int index );
    private Q_SLOTS:
        void configurePhonon();
        void eqPresetChanged( int index );
        void eqBandsChanged();
        void eqSavePreset();
        void eqDeletePreset();
        void eqRestorePreset();
    private:
        double mValueScale;
        QVector<QSlider*> mBands;
        QVector<QLabel*> mBandsValues;
        QVector<QLabel*> mBandsLabels;
        void eqSetupUI();
        void eqUpdateToolTips();
        void eqUpdateLabels( QList<int> & mEqGains );
        bool eqCfgDeletePreset( QString & mPresetName );
        bool eqCfgRestorePreset( QString mPresetName );
        void eqCfgSetPresetVal( QString & mPresetName, QList<int> & mPresetValues);
        QList<int> eqCfgGetPresetVal ( QString mPresetName );
        QStringList eqGlobalList();        
};

#endif
