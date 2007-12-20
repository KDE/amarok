// (c) 2004 Mark Kretschmann <markey@web.de>
/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
 
#ifndef AMAROK_GSTCONFIGDIALOG_H
#define AMAROK_GSTCONFIGDIALOG_H

#include "gstconfigdialogbase.h"
#include "pluginconfig.h"

#include <QObject>

class GstEngine;

class GstConfigDialog : public Amarok::PluginConfig
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


