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
#include <qptrlist.h>
#include <qtabwidget.h>
#include <qvbox.h>

class QGridLayout;
class KLineEdit;
class KComboBox;
class QSpinBox;
class QCheckBox;

typedef struct xine_s xine_t;
typedef struct xine_cfg_entry_s xine_cfg_entry_t;


///stores a single config entry of the config file

class XineConfigEntry : public QObject
{
Q_OBJECT
public:
    XineConfigEntry( QWidget *parent, amaroK::PluginConfig*, int row, xine_cfg_entry_t* );

    bool isChanged() const { return m_valueChanged; }
    void setUnchanged() { m_valueChanged = false; }
    const QCString& key() const { return m_key; }
    const QCString& stringValue() const { return m_stringValue; }
    int numValue() const { return m_numValue; }

private slots:
    void slotBoolChanged( bool );
    void slotNumChanged( int );
    void slotStringChanged( const QString& );

private:
    bool     m_valueChanged;
    int      m_numValue;
    QCString m_key;
    QCString m_stringValue;
};


class XineConfigDialog : public amaroK::PluginConfig, public QTabWidget
{
public:
    XineConfigDialog( const xine_t* const xine, QWidget *parent = 0 );

    virtual QWidget *view() { return this; }
    virtual bool hasChanged() const;
    virtual bool isDefault() const;

    /** Save view state into configuration */
    virtual void save();

private:
    QPtrList<XineConfigEntry> entrys;
    xine_t *m_xine;
};

#endif
