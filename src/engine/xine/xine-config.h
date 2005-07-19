//Copyright: (C) 2004 Max Howell, <max.howell@methylblue.com>
//Copyright: (C) 2003-2004 J. Kofler, <kaffeine@gmx.net>

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef XINECONFIG_H
#define XINECONFIG_H

#include "plugin/pluginconfig.h"
#include "xineconfigbase.h"

#include <qobject.h>

#include <xine.h>

class XineConfigDialog : public amaroK::PluginConfig
{
Q_OBJECT
    public:
        XineConfigDialog( const xine_t* const xine);
        ~XineConfigDialog();
        QWidget* view() { return m_view; }
        /** Return true if any of the view settings are different to the currently saved state */
        bool hasChanged() const;
        /** Return true if all view settings are in their default states */
        bool isDefault() const;
    public slots:
        /** Save view state using, eg KConfig */
        void save();
    private:
        xine_t *m_xine;
        XineConfigBase* m_view;
};

#endif
