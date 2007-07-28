/*
 *   Copyright (C) 2007 by Matt Williams <matt@milliams.com>
 *                      by Leo Franchi <lfranchi@gmail.com>
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

#include "ControlBox.h"

#include <QApplication>
#include <QCheckBox>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMouseEvent>
#include <QPainter>
#include <QPushButton>
#include <QStandardItemModel>
#include <QStringList>
#include <QTimeLine>
#include <QTimer>
#include <QTreeView>
#include <QVBoxLayout>

#include <KComboBox>
#include <KLocale>
#include <KDebug>
#include <KIcon>
#include <KLibrary>
//#include <KLibLoader>

#include "plasma/applet.h"
#include "plasma/svg.h"
#include "debug.h"

//BEGIN - DisplayLabel

class DisplayLabel : public QLabel
{
    public:
        DisplayLabel(const QString& text, QWidget *parent);
        //QSize minimumSizeHint();

    protected:
        void paintEvent(QPaintEvent *event);

    private:
        Plasma::Svg m_background;
};

DisplayLabel::DisplayLabel(const QString& text, QWidget *parent)
    : QLabel(text, parent),
      m_background("widgets/toolbox-button")
{
    setAlignment(Qt::AlignCenter);
    resize(100, 100);
    m_background.resize(size());
}

/*
QSize DisplayLabel::minimumSizeHint()
{
    QSize size = QLabel::minimumSizeHint();
    size.setHeight(size.height() * 2);
    size.setWidth(size.width() * 5 / 3);
    return size;
}
*/

void DisplayLabel::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event)

    QPainter p(this);
    m_background.paint(&p, rect());
    QLabel::paintEvent(event);
}

//BEGIN- PlasmoidListItemModel

PlasmoidListItemModel::PlasmoidListItemModel(QWidget* parent)
    : QStandardItemModel(parent)
{
}

QStringList PlasmoidListItemModel::mimeTypes() const
{
    QStringList types;
    types << QLatin1String("text/x-amarokappletservicename");
    return types;
}

QMimeData* PlasmoidListItemModel::mimeData(const QModelIndexList &indexes) const
{
    if (indexes.count() <= 0) {
        return 0;
    }

    QStringList types = mimeTypes();

    if (types.isEmpty()) {
        return 0;
    }

    QMimeData *data = new QMimeData();
    QString format = types.at(0);
    QByteArray byteArray;
    QStandardItem* selectedItem = item(indexes[0].row(), 1);
    byteArray.append(selectedItem->data(Qt::DisplayRole).toByteArray());
    data->setData(format, byteArray);
    return data;
}

//BEGIN - ControlWidget

ControlWidget::ControlWidget(QWidget *parent)
    : QWidget(parent)
{
    QPushButton* hideBoxButton = new QPushButton(i18n("Hide Config Box"), this);
    zoomInButton = new QPushButton(i18n("Zoom In"), this);
    zoomOutButton = new QPushButton(i18n("Zoom Out"), this);
    connect(hideBoxButton, SIGNAL(pressed()), parent, SLOT(hideBox()));

    m_formFactorSelector = new KComboBox(this);
    QStringList formFactors;
    formFactors << "Desktop" << "Media Center" << "Horizontal Panel" << "Vertical Panel";
    m_formFactorSelector->addItems(formFactors);
    connect(m_formFactorSelector, SIGNAL(activated(int)), this, SLOT(switchFormFactor(int)));

    m_appletList = new QTreeView(this);
    m_appletList->header()->hide();
    m_appletList->setDragEnabled(true);
    m_appletListModel = new PlasmoidListItemModel(this);
    m_appletList->setModel(m_appletListModel);
    m_appletList->setEditTriggers(QAbstractItemView::NoEditTriggers);
    connect(m_appletList, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(addApplet(QModelIndex)));

    m_label = new QLabel("Plasmoids:", this);

    //TODO: this should be delayed until (if) the box is actually shown.
    refreshPlasmoidList();

    QCheckBox* lockApplets = new QCheckBox(i18n("Lock Desktop"), this);
    connect(lockApplets, SIGNAL(toggled(bool)), this, SIGNAL(lockInterface(bool)));

    //This is all to change of course
    QVBoxLayout* boxLayout = new QVBoxLayout(this);
    boxLayout->addWidget(hideBoxButton);
    boxLayout->addWidget(m_formFactorSelector);
    boxLayout->addWidget(lockApplets);
    boxLayout->addWidget(zoomInButton);
    boxLayout->addWidget(zoomOutButton);
    boxLayout->addWidget(m_label);
    boxLayout->addWidget(m_appletList);
    //setLayout(boxLayout);
}

ControlWidget::~ControlWidget() {}

void ControlWidget::refreshPlasmoidList()
{
    m_appletListModel->clear();
    m_appletListModel->setColumnCount(1);

    foreach (const QString &category, Plasma::Applet::knownCategories( "amarok" )) {
        KPluginInfo::List applets = Plasma::Applet::knownApplets(category, "amarok");

        if (applets.count() < 1) {
            continue;
        }

        QStandardItem* parent = new QStandardItem(category);
        m_appletListModel->appendRow(parent);
        int rowCount = 0;

        foreach (KPluginInfo info, applets) {
            QString category = Plasma::Applet::category(info);
//            QStandardItem *item = new PlasmoidItem(info);
            QStandardItem *item = new QStandardItem(info.name());
            item->setData(info.pluginName(), Qt::UserRole);
            parent->setChild(rowCount, 0, item);
            ++rowCount;
        }
    }

#ifndef NDEBUG
    kDebug() << "Known categories: " << Plasma::Applet::knownCategories("amarok") << endl;
#endif

    m_appletListModel->sort(0);
    m_appletList->expandAll();
}

void ControlWidget::addApplet(const QModelIndex& plasmoidIndex)
{
    QStandardItem* item = m_appletListModel->itemFromIndex(plasmoidIndex);
    if (!item || !item->data(Qt::UserRole).isValid()) {
        kDebug() << "ControlWidget::addApplet no item at " << plasmoidIndex << endl;
        return;
    }

    emit addApplet(item->data(Qt::UserRole).toString());
}

void ControlWidget::switchFormFactor(int formFactor)
{
    emit setFormFactor(static_cast<Plasma::FormFactor>(formFactor));
}

//BEGIN - ControlBox

ControlBox::ControlBox(QWidget* parent)
    : QWidget(parent),
      m_box(0),
      m_boxIsShown(false)
{
    //The display box label/button
    m_displayLabel = new DisplayLabel(i18n("Desktop Toolbox"), this);
    m_displayLabel->show();
    m_displayLabel->installEventFilter(this);
    resize(m_displayLabel->size());

    //The hide box timer
    m_exitTimer = new QTimer(this);
    m_exitTimer->setInterval(300);
    m_exitTimer->setSingleShot(true);
    connect(m_exitTimer, SIGNAL(timeout()), this, SLOT(hideBox()));

    //Set up the animation timeline
    m_timeLine = new QTimeLine(300, this);
    m_timeLine->setFrameRange(0, 25); //25 step anumation
    m_timeLine->setCurveShape(QTimeLine::EaseInOutCurve);
    connect(m_timeLine, SIGNAL(frameChanged(int)), this, SLOT(animateBox(int)));
    connect(m_timeLine, SIGNAL(finished()), this, SLOT(finishBoxHiding()));
}

ControlBox::~ControlBox()
{
}

bool ControlBox::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == m_displayLabel) {
        if (event->type() == QEvent::Enter) {
            showBox();
        }
    } else if (watched == m_box) {
        if (event->type() == QEvent::Leave) {
            
            // TODO not sure why this doesn't work
            //QWidget* widget = kApp->activePopupWidget();

//             if ( !widget ||!m_box->m_formFactorSelector->isActiveWindow()) {
                m_exitTimer->start();
//             }
        } else if (event->type() == QEvent::Enter) {
            m_exitTimer->stop(); //If not a leave event, stop the box from closing
        }
    }

    return QWidget::eventFilter(watched, event);
}

void ControlBox::showBox()
{
    if (m_boxIsShown) {
        return;
    }

    // set up the actual widget here on first show
    if (!m_box) {
        m_box = new ControlWidget(this);
        m_box->installEventFilter(this);
        //m_box->hide();
        connect(m_box, SIGNAL(addApplet(const QString&)), this, SIGNAL(addApplet(const QString&)));
        connect(m_box, SIGNAL(setFormFactor(Plasma::FormFactor)), this, SIGNAL(setFormFactor(Plasma::FormFactor)));
        connect(m_box, SIGNAL(lockInterface(bool)), this, SIGNAL(lockInterface(bool)));
    }

    m_boxIsShown = true;
    m_box->move(-m_box->size().width(),-m_box->size().height());
    resize(m_box->sizeHint()); //resize this widget so the full contents of m_box can be seen.
    m_box->show();
    m_timeLine->setDirection(QTimeLine::Forward);
    m_timeLine->start();
}

void ControlBox::hideBox()
{
    if (!m_boxIsShown) {
        return;
    }

    m_boxIsShown = false;
    m_timeLine->setDirection(QTimeLine::Backward);
    m_timeLine->start();
}

void ControlBox::animateBox(int frame)
{
    //Display the config box
    qreal boxWidth = m_box->size().width();
    qreal boxHeight = m_box->size().height();
    qreal boxStep = ((qreal(frame)/25) - 1.0);
    m_box->move(static_cast<int>(boxWidth*boxStep),static_cast<int>(boxHeight*boxStep));

    //And hide the label
    qreal labelWidth = m_displayLabel->size().width();
    qreal labelHeight = m_displayLabel->size().height();
    qreal labelStep = (-qreal(frame)/25);
    m_displayLabel->move(static_cast<int>(labelWidth*labelStep),static_cast<int>(labelHeight*labelStep));
}

void ControlBox::finishBoxHiding()
{
    if (m_timeLine->direction() == QTimeLine::Backward) {
        resize(m_displayLabel->size()); //resize this widget so it's only the size of the label
        m_box->hide();
    }
}

/*void ControlBox::mousePressEvent(QMouseEvent* event)
{
    QWidget::mousePressEvent(event);
    if (event->button() == Qt::LeftButton) {
        showBox();
    }
}*/

#include "ControlBox.moc"
