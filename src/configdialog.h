/***************************************************************************
begin                : 2004/02/07
copyright            : (C) Mark Kretschmann
email                : markey@web.de
***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef AMAROKCONFIGDIALOG_H
#define AMAROKCONFIGDIALOG_H

#include <qmap.h>
#include <qvaluelist.h>

#include <kconfigdialog.h>

class QComboBox;
class QGroupBox;
class QVBox;

namespace Amarok {
    class PluginConfig;
}

class MediumPluginManager;

class AmarokConfigDialog : public KConfigDialog
{
    Q_OBJECT

    public:
        AmarokConfigDialog( QWidget *parent, const char* name, KConfigSkeleton *config );
        ~AmarokConfigDialog();

        void addPage( QWidget *page, const QString &itemName, const QString &pixmapName,
                      const QString &header=QString::null, bool manage=true);

        void showPageByName( const QCString& page );

        static int s_currentPage;

    protected slots:
        void updateButtons();
        void updateSettings();
        void updateWidgets();
        void updateWidgetsDefault();

    private slots:
        void aboutEngine();

    protected:
        bool hasChanged();
        bool isDefault();

    private:
        void soundSystemChanged();
        QString externalBrowser() const;

        QComboBox* m_soundSystem;
        Amarok::PluginConfig *m_engineConfig;
        QGroupBox            *m_engineConfigFrame;
        class Options1       *m_opt1;
        class Options2       *m_opt2;
        class Options4       *m_opt4;
        class Options7       *m_opt7;
        MediumPluginManager  *m_deviceManager;

        QValueList<QWidget*> m_pageList;
        QMap<QString, QString> m_pluginName;
        QMap<QString, QString> m_pluginAmarokName;
};


#endif // AMAROKCONFIGDIALOG_H
