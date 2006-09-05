//Copyright: (C) 2004 Max Howell, <max.howell@methylblue.com>
//Copyright: (C) 2003-2004 J. Kofler, <kaffeine@gmx.net>
//Copyright: (C) 2005 Ian Monroe
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

#include <xine.h>

class XineConfigDialog;
class KLineEdit;

class XineGeneralEntry : public QObject
{
    Q_OBJECT

    public:
        virtual void save() = 0;
        bool hasChanged()  const { return m_valueChanged; };

    signals:
        void viewChanged();

    protected:
        XineGeneralEntry(const QString& key, xine_t *xine, XineConfigDialog* xcf);
        void entryChanged();

        bool m_valueChanged;
        QString m_key;
        xine_t *m_xine;
};


class XineStrFunctor
{
    public:
        void operator()( xine_cfg_entry_t* ent, const QString& val );
};


class XineIntFunctor
{
    public:
        void operator()( xine_cfg_entry_t* ent, int val );
};


template<class T, class Functor>
void saveXineEntry(Functor& storeEntry, T val, const QString& key, xine_t *xine);


class XineStrEntry : public XineGeneralEntry
{
    Q_OBJECT

    public:
        XineStrEntry(QLineEdit* input, const QCString & key, xine_t *m_xine, XineConfigDialog* xcf);
        void save();

    private slots:
        void entryChanged(const QString& newEntry);

    private:
        QString m_val;
};


class XineIntEntry : public XineGeneralEntry
{
    Q_OBJECT

    public:
        XineIntEntry(KIntSpinBox* input, const QCString & key, xine_t *xine, XineConfigDialog* xcf);
        XineIntEntry(const QString& key, xine_t *xine, XineConfigDialog* xcf);
        void save();

    protected slots:
        void entryChanged(int newEntry);

    protected:
        int m_val;
};


class XineEnumEntry : public XineIntEntry
{
    Q_OBJECT
public:
    XineEnumEntry(QComboBox* input, const QCString & key, xine_t *xine, XineConfigDialog* xcf);
};


class XineConfigDialog : public Amarok::PluginConfig
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
        void reset(xine_t *xine);

    private:
        /** All data structures with m_xine initiated **/
        void init();
        void showHidePluginConfigs() const;
        xine_t *m_xine;
        QPtrList<XineGeneralEntry> m_entries;
        XineConfigBase* m_view;
};

#endif
