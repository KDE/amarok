//(C) 2005 Ian Monroe <ian@monroe.nu> 
/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "debug.h"
#include "xine-config.h"
#include "xineconfigbase.h"
#include "xinecfg.h"

#include <kstandarddirs.h>
#include <kcombobox.h>
#include <klocale.h>

#include <qlabel.h>
#include <qpixmap.h>


#include <xine.h>

///////////////////////
/// XineConfigDialog
///////////////////////

XineConfigDialog::XineConfigDialog( const xine_t* const xine)
        : PluginConfig()
        , m_xine (const_cast<xine_t*>(xine))
{
    m_view = new XineConfigBase( 0 );
    m_view->xineLogo->setPixmap( QPixmap( locate( "data", "amarok/images/xine_logo.png" ) ) );
    //sound output combo box
    m_view->deviceComboBox->insertItem(i18n("Autodetect"));
    if(m_xine) debug() << "its not null\n";
    const char* const* drivers = xine_list_audio_output_plugins(m_xine);
    for(int i =0; drivers[i]; ++i)
    {
        m_view->deviceComboBox->insertItem(drivers[i]);
    }
     //xine_cfg_entry_t ent;
     //if(xine_config_lookup_entry(m_xine, "audio.driver", &ent)) 
     //   for( int i = 0; ent.enum_values[i]; ++i )
     //       m_view->deviceComboBox->insertItem( QString::fromLocal8Bit( ent.enum_values[i] ) );
    //else
     //   debug() << "unsuccessful lookup\n";
    connect( m_view->deviceComboBox, SIGNAL( activated( int ) ), SIGNAL( viewChanged() ) );
    m_view->deviceComboBox->setCurrentItem( (XineCfg::outputPlugin() == "auto" ) ? "Autodetect" : XineCfg::outputPlugin() );
}

XineConfigDialog::~XineConfigDialog()
{
    XineCfg::writeConfig();
    delete m_view;
}

bool
XineConfigDialog::hasChanged() const
{
	return XineCfg::outputPlugin() != ((m_view->deviceComboBox->currentItem() == 0) ? "auto" : m_view->deviceComboBox->currentText());
}

bool
XineConfigDialog::isDefault() const
{
    return false;
}

void
XineConfigDialog::save()
{
    bool changed =  hasChanged();
    //its not Autodetect really, its just auto
    debug() << XineCfg::outputPlugin() << endl;
    XineCfg::setOutputPlugin( (m_view->deviceComboBox->currentItem() == 0) ? "auto" : m_view->deviceComboBox->currentText() );
    debug() << XineCfg::outputPlugin() << changed << endl;
    if(changed) emit settingsSaved();
}

#include "xine-config.moc"
