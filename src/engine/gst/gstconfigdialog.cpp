// (c) 2004 Mark Kretschmann <markey@web.de>
// See COPYING file for licensing information.

#include "config/gstconfig.h"
#include "enginecontroller.h"
#include "gstconfigdialog.h"
#include "gstengine.h"

#include <qcheckbox.h>

#include <kcombobox.h>
#include <kdebug.h>
#include <klineedit.h>
#include <klocale.h>


GstConfigDialog::GstConfigDialog( GstEngine* engine )
    : PluginConfig()
    , m_engine( engine )
{
    kdDebug() << k_funcinfo << endl;
    
    m_view = new GstConfigDialogBase();
            
    // Initialise widgets with current settings
    const QStringList outputs = m_engine->getOutputsList();
    m_view->kComboBox_output->insertStringList( outputs );
    
    if ( outputs.contains( GstConfig::soundOutput() ) )
        m_view->kComboBox_output->setCurrentText( GstConfig::soundOutput() );
    
    m_view->checkBox1->setChecked( GstConfig::customSoundDevice() );
    m_view->kLineEdit_device->setText( GstConfig::soundDevice() );

    // Connections for modification check
    connect( m_view->kComboBox_output, SIGNAL( activated( int ) ), this, SIGNAL( settingsChanged() ) );
    connect( m_view->checkBox1, SIGNAL( toggled( bool ) ), this, SIGNAL( settingsChanged() ) );
    connect( m_view->kLineEdit_device, SIGNAL( textChanged( const QString& ) ), this, SIGNAL( settingsChanged() ) );
}


GstConfigDialog::~GstConfigDialog()
{
    kdDebug() << k_funcinfo << endl;
}


/////////////////////////////////////////////////////////////////////////////////////
// PUBLIC 
/////////////////////////////////////////////////////////////////////////////////////

bool
GstConfigDialog::hasChanged() const
{
    return GstConfig::soundOutput()       != m_view->kComboBox_output->currentText() ||
           GstConfig::customSoundDevice() != m_view->checkBox1->isChecked() ||
           GstConfig::soundDevice()       != m_view->kLineEdit_device->text();
}


bool
GstConfigDialog::isDefault() const
{
    return false;
}


/////////////////////////////////////////////////////////////////////////////////////
// PUBLIC SLOTS
/////////////////////////////////////////////////////////////////////////////////////

void
GstConfigDialog::save() //SLOT
{
    kdDebug() << k_funcinfo << endl;
    
    GstConfig::setSoundOutput( m_view->kComboBox_output->currentText() );
    GstConfig::setCustomSoundDevice( m_view->checkBox1->isChecked() );
    GstConfig::setSoundDevice( m_view->kLineEdit_device->text() );
}


#include "gstconfigdialog.moc"

