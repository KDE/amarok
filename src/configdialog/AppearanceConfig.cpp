#include "AppearanceConfig.h"

AppearanceConfig::AppearanceConfig( QWidget* parent )
    : ConfigDialogBase( parent )
    , m_gui( new Ui_AppearanceConfig() )
{
    m_gui->setupUi( this ); 
}


AppearanceConfig::~AppearanceConfig()
{
    delete m_gui;
}


bool
AppearanceConfig::hasChanged()
{
    return false;
}


bool
AppearanceConfig::isDefault()
{
    return false;
}


void AppearanceConfig::updateSettings()
{}

