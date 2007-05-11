#ifndef CONFIGDIALOGBASE_H
#define CONFIGDIALOGBASE_H

#include <QWidget>


class ConfigDialogBase : public QWidget
{
    public:
        ConfigDialogBase( QWidget* parent ) : QWidget( parent ) {}

        virtual void updateSettings() = 0;

        virtual bool hasChanged() = 0;
        virtual bool isDefault() = 0;
};


#endif

