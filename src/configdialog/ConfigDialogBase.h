#include <QWidget>


class ConfigDialogBase : public QWidget
{
    public:
        virtual void updateSettings();

        virtual bool hasChanged();
        virtual bool isDefault();
};

