/***************************************************************************
 Setup dialog for equalizer

 (c) 2004 Mark Kretschmann <markey@web.de>
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

#include "equalizersetupbase.h" //base class

#include <qptrlist.h>           //stack alloc
#include <qslider.h>

class EqualizerSetup : public EqualizerSetupBase
{
    Q_OBJECT

    public:
        static EqualizerSetup* instance() { return s_instance ? s_instance : new EqualizerSetup(); }

        EqualizerSetup();
        ~EqualizerSetup();

    private slots:
        void activateEqualizer( bool );
        void preampChanged();
        void bandChanged();

    private:
        static EqualizerSetup* s_instance;

        QPtrList<QSlider> m_bandSliders;
};


#endif /*AMAROK_EQUALIZERSETUP_H*/
