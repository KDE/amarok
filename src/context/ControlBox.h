/*
 *   Copyright 2007 by Matt Williams <matt@milliams.com>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License version 2 as
 *   published by the Free Software Foundation
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details
 *
 *   You should have received a copy of the GNU Library General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef CONTROL_BOX_H
#define CONTROL_BOX_H

#include <QtGui/QWidget>
#include <QtGui/QStandardItemModel>

#include <plasma/plasma.h>

class QLabel;
class QPushButton;
class QTreeView;
class QModelIndex;
class QStandardItemModel;
class QTimeLine;

class KComboBox;

class DisplayLabel;
class ControlWidget;
class PlasmoidListItemModel;

/**
 * @short The Desktop configuration widget
 * Just add one of these to the Corona and you've got an instant config box.
 */
class ControlBox : public QWidget
{
    Q_OBJECT

    public:
        ControlBox(QWidget* parent);
        ~ControlBox();
        bool eventFilter(QObject *watched, QEvent *event);

    Q_SIGNALS:
        void zoomIn();
        void zoomOut();
        void addApplet(const QString&);
        void lockInterface(bool);

    protected:
        //void mousePressEvent (QMouseEvent* event);
        void setupBox(); ///<Create contents of the config dialog

    protected Q_SLOTS:
        void showBox(); ///<Show the config widget
        void hideBox(); ///<Hide the config widget
        void finishBoxHiding();
        void animateBox(int frame); ///<Process the frames to create an animation

    private:
        ControlWidget* m_box; ///<The configuraion dialog widget
        DisplayLabel* m_displayLabel; ///<The 'show config' button
        QTimeLine* m_timeLine;
        QTimer* m_exitTimer;

        bool m_boxIsShown;
};

/**
 * @short The widget that contains the actual settings
 */
class ControlWidget : public QWidget
{
    Q_OBJECT

    public:
        ControlWidget(QWidget* parent);
        ~ControlWidget();
        QPushButton* zoomInButton;
        QPushButton* zoomOutButton;

    protected:
        void refreshPlasmoidList();

        QLabel* m_label;
        QTreeView *m_appletList;
        PlasmoidListItemModel* m_appletListModel;

    protected Q_SLOTS:
        void addApplet(const QModelIndex& plasmoidIndex);

    Q_SIGNALS:
        void addApplet(const QString&);
        void lockInterface(bool);
};

/**
 * A custom Item Model so that the correct MIME type can be set and so the name
 * of the plasmoid can be passed.
 */
class PlasmoidListItemModel : public QStandardItemModel
{
    public:
        enum ItemDataRole {
            AppletNameRole = Qt::UserRole
        };
        PlasmoidListItemModel(QWidget* parent = 0);

        QStringList mimeTypes() const;
        QMimeData* mimeData(const QModelIndexList &indexes) const;
};

#endif // multiple inclusion guard
