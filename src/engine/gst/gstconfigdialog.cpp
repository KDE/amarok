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
    
    m_view->checkBox_outputDevice->setChecked( GstConfig::useCustomSoundDevice() );
    m_view->kLineEdit_outputDevice->setText( GstConfig::soundDevice() );

    m_view->checkBox_outputParams->setChecked( GstConfig::useCustomOutputParams() );
    m_view->kLineEdit_outputParams->setText( GstConfig::outputParams() );
    
    // Connections for modification check
    connect( m_view->kComboBox_output, SIGNAL( activated( int ) ), this, SIGNAL( settingsChanged() ) );
    connect( m_view->checkBox_outputDevice, SIGNAL( toggled( bool ) ), this, SIGNAL( settingsChanged() ) );
    connect( m_view->kLineEdit_outputDevice, SIGNAL( textChanged( const QString& ) ), this, SIGNAL( settingsChanged() ) );
    connect( m_view->checkBox_outputParams, SIGNAL( toggled( bool ) ), this, SIGNAL( settingsChanged() ) );
    connect( m_view->kLineEdit_outputParams, SIGNAL( textChanged( const QString& ) ), this, SIGNAL( settingsChanged() ) );
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
    return GstConfig::soundOutput()           != m_view->kComboBox_output->currentText() ||
           GstConfig::useCustomSoundDevice()  != m_view->checkBox_outputDevice->isChecked() ||
           GstConfig::soundDevice()           != m_view->kLineEdit_outputDevice->text() ||
           GstConfig::useCustomOutputParams() != m_view->checkBox_outputParams->isChecked() ||
           GstConfig::outputParams()          != m_view->kLineEdit_outputParams->text();
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
    GstConfig::setUseCustomSoundDevice( m_view->checkBox_outputDevice->isChecked() );
    GstConfig::setSoundDevice( m_view->kLineEdit_outputDevice->text() );
    GstConfig::setUseCustomOutputParams( m_view->checkBox_outputParams->isChecked() );
    GstConfig::setOutputParams( m_view->kLineEdit_outputParams->text() );
}


#include "gstconfigdialog.moc"

