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

#include <kcombobox.h>
#include <klocale.h>
#include <knuminput.h>
#include <kstandarddirs.h>

#include <qlabel.h>
#include <qpixmap.h>


#include <xine.h>
////////////////////
/// XineGeneralEntry
////////////////////
XineGeneralEntry::XineGeneralEntry(const QString& key, xine_t *xine, XineConfigDialog* xcf)
    : m_valueChanged(false)
    , m_key(key)
    , m_xine(xine)
{
    debug() << "new entry " << m_key << endl;
    connect(this, SIGNAL(viewChanged()), xcf, SIGNAL(viewChanged() ) );
}
////////////////////
/// XineStrEntry
////////////////////
XineStrEntry::XineStrEntry(QLineEdit* input, const QCString & key, xine_t *xine, XineConfigDialog* xcf)
    : XineGeneralEntry(key,xine,xcf)
{
    xine_cfg_entry_t ent;
    if( xine_config_lookup_entry(m_xine,  m_key.ascii(), &ent) )
    {
        input->setText(ent.str_value);
        m_val = ent.str_value; 
    }
     connect( input, SIGNAL( textChanged( const QString & ) ), this, SLOT( entryChanged(const QString &) ) );
}

void XineStrEntry::save()
{
    if(m_xine) debug() << "its not null " << m_key << ' ' << m_val << endl;
    xine_cfg_entry_t ent;
    if(xine_config_lookup_entry(m_xine, m_key.ascii(), &ent))
    {
        ent.str_value = const_cast<char*>(m_val.ascii());
        xine_config_update_entry(m_xine, &ent);
        m_valueChanged = false;
    }
    else
        debug()<<"Error saving " << m_val << " with key " << m_key;
}

void XineStrEntry::entryChanged(const QString & val)
{
    m_valueChanged = true;
    m_val = val;
    emit viewChanged();
}

////////////////////
/// XineIntEntry
////////////////////
XineIntEntry::XineIntEntry(KIntSpinBox* input, const QCString & key, xine_t *xine, XineConfigDialog* xcf)
    : XineGeneralEntry(key,xine,xcf)
{
    xine_cfg_entry_t ent;
    if(xine_config_lookup_entry(m_xine,  m_key.ascii(), &ent)) 
        {
            input->setValue(ent.num_value);
            m_val = ent.num_value;
        }
     connect( input,  SIGNAL( valueChanged( int ) ), this, SLOT( entryChanged( int ) ) );
}

void XineIntEntry::save()
{
    xine_cfg_entry_t ent;
    if(xine_config_lookup_entry(m_xine,  m_key.ascii(), &ent))
    {
        ent.num_value = m_val;
        xine_config_update_entry(m_xine, &ent);
        m_valueChanged = false;
    }
    else
        debug()<<"Error saving " << m_val << " with key " << m_key;
}

void XineIntEntry::entryChanged(int val)
{
    debug() << m_key << " is now set to " << val << endl;
    m_valueChanged = true;
    m_val = val;
    emit viewChanged();
}
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
    const char* const* drivers = xine_list_audio_output_plugins(m_xine);
    for(int i =0; drivers[i]; ++i)
    {
        if(qstrcmp(drivers[i],"none") != 0) //returns 0 if equal
            m_view->deviceComboBox->insertItem(drivers[i]);
    }
    
    connect( m_view->deviceComboBox, SIGNAL( activated( int ) ), SIGNAL( viewChanged() ) );
    m_entries.setAutoDelete(TRUE);
    m_view->deviceComboBox->setCurrentItem( (XineCfg::outputPlugin() == "auto" ) ? "Autodetect" : XineCfg::outputPlugin() );
    init();
}

XineConfigDialog::~XineConfigDialog()
{
    XineCfg::writeConfig();
    delete m_view;
}
void XineConfigDialog::init()
{
    #define add(X) m_entries.append(X)
    add(new XineStrEntry(m_view->hostLineEdit, "media.network.http_proxy_host", m_xine, this));
    add(new XineIntEntry(m_view->portIntBox,"media.network.http_proxy_port", m_xine, this));
    add(new XineStrEntry(m_view->userLineEdit, "media.network.http_proxy_user", m_xine, this));
    add(new XineStrEntry(m_view->passLineEdit, "media.network.http_proxy_password", m_xine, this));
    #undef add
}

bool
XineConfigDialog::hasChanged() const
{
	if(XineCfg::outputPlugin() != ((m_view->deviceComboBox->currentItem() == 0) ? "auto" : m_view->deviceComboBox->currentText()))
        return true;
    QPtrListIterator<XineGeneralEntry> it( m_entries );
    XineGeneralEntry* entry;
    while( (entry = it.current()) != 0 )
    {
        ++it;
        if(entry->hasChanged())
            return true; 
    }
    return false;
}

bool
XineConfigDialog::isDefault() const
{
    return false;
}
void
XineConfigDialog::reset(xine_t* xine) //SLOT
{
    debug() << &m_xine << " " << &xine << endl;
    m_entries.clear();
    m_xine = xine;
    debug() << "m_entries now empty " << m_entries.isEmpty() << endl;
    init();
}

void
XineConfigDialog::save()//SLOT
{
    if(hasChanged())
    {
        //its not Autodetect really, its just auto
        XineCfg::setOutputPlugin((m_view->deviceComboBox->currentItem() == 0) 
            ? "auto" : m_view->deviceComboBox->currentText());
        XineGeneralEntry* entry;
        for(entry = m_entries.first(); entry; entry=m_entries.next())
        {
            if(entry->hasChanged())
                entry->save();
        }
        emit settingsSaved();
    }
}

#include "xine-config.moc"
