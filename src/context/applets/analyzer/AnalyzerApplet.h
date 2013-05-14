/****************************************************************************************
 * Copyright (c) 2013 Mark Kretschmann <kretschmann@kde.org>                            *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#ifndef ANALYZER_APPLET_H
#define ANALYZER_APPLET_H

#include "context/Applet.h"
#include "context/DataEngine.h"


class AnalyzerApplet : public Context::Applet
{
    Q_OBJECT

public:
    AnalyzerApplet( QObject* parent, const QVariantList& args );
    virtual ~AnalyzerApplet();

public slots:
    virtual void init();
};

AMAROK_EXPORT_APPLET( analyzer, AnalyzerApplet )

#endif
