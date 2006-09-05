/***************************************************************************
 Setup dialog for the equalizer

 (c) 2004 Mark Kretschmann <markey@web.de>
 (c) 2005 Seb Ruiz <me@sebruiz.net>
 (c) 2005 Markus Brueffer <markus@brueffer.de>
***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef AMAROK_EQUALIZERSETUP_H
#define AMAROK_EQUALIZERSETUP_H

#include <qptrlist.h>           //stack alloc
#include <kdialogbase.h>

class EqualizerGraph;
class QGroupBox;
class QCheckBox;
class KComboBox;
class KPopupMenu;

namespace Amarok { class Slider; }


class EqualizerSetup : public KDialogBase
{
    Q_OBJECT

    public:
        static EqualizerSetup* instance() { return s_instance ? s_instance : new EqualizerSetup(); }
        static bool isInstantiated() { return s_instance ? true : false; }

        EqualizerSetup();
       ~EqualizerSetup();

        // for use by DCOP
        void setActive( bool active );
        void setBands( int preamp, QValueList<int> gains );
        void setPreset( QString name );

    private slots:
        void presetChanged( int id );
        void presetChanged( QString title );
        void sliderChanged();
        void setEqualizerEnabled( bool );
        void setEqualizerParameters();
        void editPresets();
        void addPreset();

    private:
        static EqualizerSetup* s_instance;

        void    loadPresets();
        void    savePresets();
        void    updatePresets(QString selectTitle = QString::null);
        QString presetsCache() const;

        Amarok::Slider* m_slider_preamp;
        EqualizerGraph* m_equalizerGraph;
        QPtrList<Amarok::Slider> m_bandSliders;

        QGroupBox*      m_groupBoxSliders;
        KComboBox*      m_presetCombo;
        uint            m_manualPos;

        QMap< QString, QValueList<int> > m_presets;
};

#endif /*AMAROK_EQUALIZERSETUP_H*/
