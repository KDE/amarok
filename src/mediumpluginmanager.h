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

#include <qmap.h>
#include <qsignalmapper.h>
#include <kdialogbase.h>

class Medium;
class KComboBox;
class MediumPluginManager;
class MediumPluginDetailView;

typedef QMap<Medium*, KComboBox*> ComboMap;
typedef QMap<int, Medium*> ButtonMap;


/**
	@author Jeff Mitchell <kde-dev@emailgoeshere.com>
*/
class MediumPluginManager : public KDialogBase
{
    Q_OBJECT

    public:
        MediumPluginManager( );

    signals:
        void selectedPlugin( const Medium*, const QString );

    private slots:
        void slotOk( );
        void infoRequested( int buttonId );

    private:
        ComboMap m_cmap;
        ButtonMap m_bmap;
        QSignalMapper* m_sigmap;

};

class MediumPluginDetailView : public KDialogBase
{
    Q_OBJECT

    public:
        MediumPluginDetailView( const Medium* medium );

};

#endif

