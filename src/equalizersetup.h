/***************************************************************************
 Setup dialog for the equalizer

 (c) 2004 Mark Kretschmann <markey@web.de>
 (c) 2005 Seb Ruiz <me@sebruiz.net>
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
#include <qvbox.h>

class EqualizerGraph;
class QCheckBox;
class KPopupMenu;

namespace amaroK { class Slider; }


class EqualizerSetup : public QVBox
{
    Q_OBJECT

    public:
        static EqualizerSetup* instance() { return s_instance ? s_instance : new EqualizerSetup(); }
        static bool isInstantiated() { return s_instance ? true : false; }

        EqualizerSetup();
       ~EqualizerSetup();
        //for use by DCOP to update GUI
        void updateSliders( int, QValueList<int> );

    private slots:
        void presetChanged( int id );
        void sliderChanged();
        void setEqualizerEnabled( bool );
        void setEqualizerParameters();

    private:
        static EqualizerSetup* s_instance;

        void    loadPresets();
        void    savePresets();
        QString presetsCache() const;

        amaroK::Slider* m_slider_preamp;
        EqualizerGraph* m_equalizerGraph;
        QPtrList<amaroK::Slider> m_bandSliders;

        KPopupMenu*     m_presetPopup;
        int             m_currentPreset;
        uint            m_totalPresets;
        QMap< int, QValueList<int> > m_presets;
};


#endif /*AMAROK_EQUALIZERSETUP_H*/
