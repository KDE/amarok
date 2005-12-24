//
// C++ Interface: mediumpluginchooser
//
// Description: 
//
//
// Author: Jeff Mitchell <kde-dev@emailgoeshere.com>, (C) 2005
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef MEDIUMPLUGINCHOOSER_H
#define MEDIUMPLUGINCHOOSER_H

#include <kdialogbase.h>
#include <kguiitem.h>
#include <kcombobox.h>

class Medium;

/**
	@author Jeff Mitchell <kde-dev@emailgoeshere.com>
*/
class MediumPluginChooser : public KDialogBase
{
    //Q_OBJECT
    public:
        MediumPluginChooser( const Medium *medium, const KGuiItem ignoreButton );

        ~MediumPluginChooser();


    private:
        KComboBox*      m_chooserCombo;

};

#endif
