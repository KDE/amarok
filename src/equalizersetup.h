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

#include "sliderwidget.h"

#include <qptrlist.h>           //stack alloc
#include <qvbox.h>

using namespace amaroK;

class EqualizerGraph;
class QCheckBox;

class EqualizerSetup : public QVBox
{
    Q_OBJECT

    public:
        static EqualizerSetup* instance() { return s_instance ? s_instance : new EqualizerSetup(); }

        EqualizerSetup();
       ~EqualizerSetup();

    private slots:
        void enableEqualizer( bool );
        void parametersChanged();

    private:
        static EqualizerSetup* s_instance;

        Slider* m_slider_preamp;
        EqualizerGraph* m_equalizerGraph;

        QPtrList<Slider> m_bandSliders;
};


#endif /*AMAROK_EQUALIZERSETUP_H*/
