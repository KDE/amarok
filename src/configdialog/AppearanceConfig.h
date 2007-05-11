#ifndef APPEARANCECONFIG_H
#define APPEARANCECONFIG_H

#include "ui_AppearanceConfig.h"
#include "ConfigDialogBase.h"


class AppearanceConfig : public ConfigDialogBase
{
    public:
        AppearanceConfig( QWidget* parent );
        virtual ~AppearanceConfig();

        virtual bool hasChanged();
        virtual bool isDefault();
        virtual void updateSettings();

    private:
        Ui_AppearanceConfig* m_gui;
};


#endif

