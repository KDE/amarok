/****************************************************************************************
 * Copyright (c) 2004-2009 Mark Kretschmann <kretschmann@kde.org>                       *
 * Copyright (c) 2009 Artur Szymiec <artur.szymiec@gmail.com>                           *
 * Copyright (c) 2013 Ralf Engels <ralf-engels@gmx.de>                                  *
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

#ifndef EQUALIZERDIALOG_H
#define EQUALIZERDIALOG_H

#include "ui_EqualizerDialog.h"

#include <equalizer/EqualizerPresets.h>

#include <KDialog>

class EqualizerDialog : public KDialog, public Ui_EqualizerDialog
{
    Q_OBJECT

    public:
        ~EqualizerDialog();

        static void showOnce( QWidget *parent = 0 );;

        /** Returns the untranslated current preset name. */
        QString selectedPresetName() const;

        /** Returns the current gain settings */
        QList<int> gains() const;

    private Q_SLOTS:
        /** Updates the enabled states of different ui components. */
        void updateUi();

        /** Set the index of the current preset.
            Will update the UI */
        void presetApplied( int index );

        /** Updates the tool tips, labels, configuration and engine */
        void bandsChanged();

        void updatePresets();

        void savePreset();
        void deletePreset();
        void restorePreset();
        void restoreOriginalSettings();
        void gainsChanged( const QList<int> &gains );
        void presetsChanged( const QString &name );
        void toggleEqualizer( bool enabled );

    private:
        EqualizerDialog( QWidget *parent = 0 );

        void updateToolTips();
        void updateLabels();
        void storeOriginalSettings();

        double mValueScale;
        QVector<QSlider*> m_bands;
        QVector<QLabel*> m_bandValues;
        QVector<QLabel*> m_bandLabels;

        /** The preset and gains at the time "showOnce" was called. */
        bool m_originalActivated;
        QString m_originalPreset;
        QList<int> m_originalGains;

        static EqualizerDialog *s_instance;
};


#endif // EQUALIZERDIALOG_H
