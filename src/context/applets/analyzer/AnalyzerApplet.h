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
    enum WidgetHeight { Tiny = 80, Small = 120, Medium = 170, Tall = 220, Default = Small };

    AnalyzerApplet( QObject* parent, const QVariantList& args );
    virtual ~AnalyzerApplet();

public Q_SLOTS:
    virtual void init();

private Q_SLOTS:
    void newGeometry();
    void heightActionTriggered();
    void analyzerAction( QAction* );

private:
    void hideEvent( QHideEvent* );
    void showEvent( QShowEvent* );
    void setNewHeight( WidgetHeight height );
    void setCurrentAnalyzer( const QString &name );
    QList<QAction *> contextualActions();

    QWidget *m_analyzer;
    QString m_analyzerName;
    QMap<QString, QString> m_analyzerNames;
    WidgetHeight m_currentHeight;
};

AMAROK_EXPORT_APPLET( analyzer, AnalyzerApplet )

#endif
