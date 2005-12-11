// (c) 2004 Mark Kretschmann <markey@web.de>
// See COPYING file for licensing information.

#ifndef AMAROK_GSTCONFIGDIALOG_H
#define AMAROK_GSTCONFIGDIALOG_H

#include "gstconfigdialogbase.h"
#include "pluginconfig.h"

#include <qobject.h>

class GstEngine;

class GstConfigDialog : public amaroK::PluginConfig
{
    Q_OBJECT

    public:
        GstConfigDialog( GstEngine const * const engine );
        ~GstConfigDialog();

        QWidget* view() { return m_view; }

        bool hasChanged() const;
        bool isDefault() const;

    public slots:
        void save();

    private:
        GstEngine* m_engine;
        GstConfigDialogBase* m_view;
};


#endif /*AMAROK_GSTCONFIGDIALOG_H*/


