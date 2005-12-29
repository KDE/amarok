/*THIS IS NOT BUILT YET BECAUSE IT's PRETTY MUCH ONLY AN IDEA AT THIS POINT*/

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
#ifndef MEDIUMPLUGINMANAGER_H
#define MEDIUMPLUGINMANAGER_H

#include <kdialogbase.h>

class Medium;
class KComboBox;

/**
	@author Jeff Mitchell <kde-dev@emailgoeshere.com>
*/
class MediumPluginManager : public KDialogBase
{
    Q_OBJECT

    public:
        MediumPluginManager( );
        ~MediumPluginManager( );

    private slots:
        void slotOk( );
        void slotCancel( );

    private:
        KTable*     m_tombo;

};

#endif