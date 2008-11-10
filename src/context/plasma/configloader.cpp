/*
 *   Copyright 2007 Aaron Seigo <aseigo@kde.org>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License as
 *   published by the Free Software Foundation; either version 2, or
 *   (at your option) any later version.
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

#include "configloader.h"

#include <QColor>
#include <QFont>
#include <QHash>
#include <QXmlContentHandler>
#include <QXmlInputSource>
#include <QXmlSimpleReader>

#include <KDebug>
#include <KUrl>

namespace Plasma
{

class ConfigLoaderPrivate
{
    public:
        ~ConfigLoaderPrivate()
        {
            qDeleteAll(bools);
            qDeleteAll(strings);
            qDeleteAll(stringlists);
            qDeleteAll(colors);
            qDeleteAll(fonts);
            qDeleteAll(ints);
            qDeleteAll(uints);
            qDeleteAll(urls);
            qDeleteAll(dateTimes);
            qDeleteAll(doubles);
            qDeleteAll(intlists);
            qDeleteAll(longlongs);
            qDeleteAll(points);
            qDeleteAll(rects);
            qDeleteAll(sizes);
            qDeleteAll(ulonglongs);
            qDeleteAll(urllists);
        }

        bool *newBool()
        {
            bool *v = new bool;
            bools.append(v);
            return v;
        }

        QString *newString()
        {
            QString *v = new QString;
            strings.append(v);
            return v;
        }

        QStringList *newStringList()
        {
            QStringList *v = new QStringList;
            stringlists.append(v);
            return v;
        }

        QColor *newColor()
        {
            QColor *v = new QColor;
            colors.append(v);
            return v;
        }

        QFont *newFont()
        {
            QFont *v = new QFont;
            fonts.append(v);
            return v;
        }

        qint32 *newInt()
        {
            qint32 *v = new qint32;
            ints.append(v);
            return v;
        }

        quint32 *newUint()
        {
            quint32 *v = new quint32;
            uints.append(v);
            return v;
        }

        KUrl *newUrl()
        {
            KUrl *v = new KUrl;
            urls.append(v);
            return v;
        }

        QDateTime *newDateTime()
        {
            QDateTime *v = new QDateTime;
            dateTimes.append(v);
            return v;
        }

        double *newDouble()
        {
            double *v = new double;
            doubles.append(v);
            return v;
        }

        QList<qint32>* newIntList()
        {
            QList<qint32> *v = new QList<qint32>;
            intlists.append(v);
            return v;
        }

        qint64 *newLongLong()
        {
            qint64 *v = new qint64;
            longlongs.append(v);
            return v;
        }

        QPoint *newPoint()
        {
            QPoint *v = new QPoint;
            points.append(v);
            return v;
        }

        QRect *newRect()
        {
            QRect *v = new QRect;
            rects.append(v);
            return v;
        }

        QSize *newSize()
        {
            QSize *v = new QSize;
            sizes.append(v);
            return v;
        }

        quint64 *newULongLong()
        {
            quint64 *v = new quint64;
            ulonglongs.append(v);
            return v;
        }

        KUrl::List *newUrlList()
        {
            KUrl::List *v = new KUrl::List;
            urllists.append(v);
            return v;
        }

        void parse(ConfigLoader *loader, QIODevice *xml);

        QList<bool *> bools;
        QList<QString *> strings;
        QList<QStringList *> stringlists;
        QList<QColor *> colors;
        QList<QFont *> fonts;
        QList<qint32 *> ints;
        QList<quint32 *> uints;
        QList<KUrl *> urls;
        QList<QDateTime *> dateTimes;
        QList<double *> doubles;
        QList<QList<qint32> *> intlists;
        QList<qint64 *> longlongs;
        QList<QPoint *> points;
        QList<QRect *> rects;
        QList<QSize *> sizes;
        QList<quint64 *> ulonglongs;
        QList<KUrl::List *> urllists;
        QString baseGroup;
        QStringList groups;
        QHash<QString, QString> keysToNames;
};

class ConfigLoaderHandler : public QXmlDefaultHandler
{
public:
    ConfigLoaderHandler(ConfigLoader *config, ConfigLoaderPrivate *d);
    bool startElement(const QString &namespaceURI, const QString &localName,
                      const QString &qName, const QXmlAttributes &atts);
    bool endElement(const QString &namespaceURI, const QString &localName,
                    const QString &qName);
    bool characters(const QString &ch);

private:
    void addItem();
    void resetState();

    ConfigLoader *m_config;
    ConfigLoaderPrivate *d;
    int m_min;
    int m_max;
    QString m_name;
    QString m_key;
    QString m_type;
    QString m_label;
    QString m_default;
    QString m_cdata;
    QString m_whatsThis;
    KConfigSkeleton::ItemEnum::Choice m_choice;
    QList<KConfigSkeleton::ItemEnum::Choice> m_enumChoices;
    bool m_haveMin;
    bool m_haveMax;
    bool m_inChoice;
};

void ConfigLoaderPrivate::parse(ConfigLoader *loader, QIODevice *xml)
{
    QXmlInputSource source(xml);
    QXmlSimpleReader reader;
    ConfigLoaderHandler handler(loader, this);
    reader.setContentHandler(&handler);
    reader.parse(&source, false);
}

ConfigLoaderHandler::ConfigLoaderHandler(ConfigLoader *config, ConfigLoaderPrivate *d)
    : QXmlDefaultHandler(),
      m_config(config),
      d(d)
{
    resetState();
}

bool ConfigLoaderHandler::startElement(const QString &namespaceURI, const QString &localName,
                                    const QString &qName, const QXmlAttributes &attrs)
{
    Q_UNUSED(namespaceURI)
    Q_UNUSED(qName)

//     kDebug() << "ConfigLoaderHandler::startElement(" << localName << qName;
    int numAttrs = attrs.count();
    QString tag = localName.toLower();
    if (tag == "group") {
        QString group;
        for (int i = 0; i < numAttrs; ++i) {
            QString name = attrs.localName(i).toLower();
            if (name == "name") {
                //kDebug() << "set group to" << attrs.value(i);
                group = attrs.value(i);
            }
        }
        if (group.isEmpty()) {
            group = d->baseGroup;
        } else {
            d->groups.append(group);
            if (!d->baseGroup.isEmpty()) {
                group = d->baseGroup + '\x1d' + group;
            }
        }
        m_config->setCurrentGroup(group);
    } else if (tag == "entry") {
        for (int i = 0; i < numAttrs; ++i) {
            QString name = attrs.localName(i).toLower();
            if (name == "name") {
                m_name = attrs.value(i);
            } else if (name == "type") {
                m_type = attrs.value(i).toLower();
            } else if (name == "key") {
                m_key = attrs.value(i);
            }
        }
    } else if (tag == "choice") {
        m_choice.name.clear();
        m_choice.label.clear();
        m_choice.whatsThis.clear();
        for (int i = 0; i < numAttrs; ++i) {
            QString name = attrs.localName(i).toLower();
            if (name == "name") {
                m_choice.name = attrs.value(i);
            }
        }
        m_inChoice = true;
    }

    return true;
}

bool ConfigLoaderHandler::characters(const QString &ch)
{
    m_cdata.append(ch);
    return true;
}

bool ConfigLoaderHandler::endElement(const QString &namespaceURI,
                                  const QString &localName, const QString &qName)
{
    Q_UNUSED(namespaceURI)
    Q_UNUSED(qName)

//     kDebug() << "ConfigLoaderHandler::endElement(" << localName << qName;
    QString tag = localName.toLower();
    if (tag == "entry") {
        addItem();
        resetState();
    } else if (tag == "label") {
        if (m_inChoice) {
            m_choice.label = m_cdata.trimmed();
        } else {
            m_label = m_cdata.trimmed();
        }
    } else if (tag == "whatsthis") {
        if (m_inChoice) {
            m_choice.whatsThis = m_cdata.trimmed();
        } else {
            m_whatsThis = m_cdata.trimmed();
        }
    } else if (tag == "default") {
        m_default = m_cdata.trimmed();
    } else if (tag == "min") {
        m_min = m_cdata.toInt(&m_haveMin);
    } else if (tag == "max") {
        m_max = m_cdata.toInt(&m_haveMax);
    } else if (tag == "choice") {
        m_enumChoices.append(m_choice);
        m_inChoice = false;
    }

    m_cdata.clear();
    return true;
}

void ConfigLoaderHandler::addItem()
{
    if (m_name.isEmpty()) {
        return;
    }

    KConfigSkeletonItem *item = 0;

    if (m_type == "bool") {
        bool defaultValue = m_default.toLower() == "true";
        item = m_config->addItemBool(m_name, *d->newBool(), defaultValue, m_key);
    } else if (m_type == "color") {
        item = m_config->addItemColor(m_name, *d->newColor(), QColor(m_default), m_key);
    } else if (m_type == "datetime") {
        item = m_config->addItemDateTime(m_name, *d->newDateTime(),
                                         QDateTime::fromString(m_default), m_key);
    } else if (m_type == "enum") {
        m_key = (m_key.isEmpty()) ? m_name : m_key;
        KConfigSkeleton::ItemEnum *enumItem =
            new KConfigSkeleton::ItemEnum(m_config->currentGroup(),
                                          m_key, *d->newInt(),
                                          m_enumChoices,
                                          m_default.toUInt());
        m_config->addItem(enumItem, m_name);
        item = enumItem;
    } else if (m_type == "font") {
        item = m_config->addItemFont(m_name, *d->newFont(), QFont(m_default), m_key);
    } else if (m_type == "int") {
        KConfigSkeleton::ItemInt *intItem = m_config->addItemInt(m_name, *d->newInt(),
                                                                 m_default.toInt(), m_key);

        if (m_haveMin) {
            intItem->setMinValue(m_min);
        }

        if (m_haveMax) {
            intItem->setMaxValue(m_max);
        }

        item = intItem;
    } else if (m_type == "password") {
        item = m_config->addItemPassword(m_name, *d->newString(), m_default, m_key);
    } else if (m_type == "path") {
        item = m_config->addItemPath(m_name, *d->newString(), m_default, m_key);
    } else if (m_type == "string") {
        item = m_config->addItemString(m_name, *d->newString(), m_default, m_key);
    } else if (m_type == "stringlist") {
        //FIXME: the split() is naive and will break on lists with ,'s in them
        item = m_config->addItemStringList(m_name, *d->newStringList(),
                                           m_default.split(','), m_key);
    } else if (m_type == "uint") {
        KConfigSkeleton::ItemUInt *uintItem =
            m_config->addItemUInt(m_name, *d->newUint(), m_default.toUInt(), m_key);
        if (m_haveMin) {
            uintItem->setMinValue(m_min);
        }
        if (m_haveMax) {
            uintItem->setMaxValue(m_max);
        }
        item = uintItem;
    } else if (m_type == "url") {
        m_key = (m_key.isEmpty()) ? m_name : m_key;
        KConfigSkeleton::ItemUrl *urlItem =
            new KConfigSkeleton::ItemUrl(m_config->currentGroup(),
                                         m_key, *d->newUrl(),
                                         m_default);
        m_config->addItem(urlItem, m_name);
        item = urlItem;
    } else if (m_type == "double") {
        KConfigSkeleton::ItemDouble *doubleItem = m_config->addItemDouble(m_name,
                *d->newDouble(), m_default.toDouble(), m_key);
        if (m_haveMin) {
            doubleItem->setMinValue(m_min);
        }
        if (m_haveMax) {
            doubleItem->setMaxValue(m_max);
        }
        item = doubleItem;
    } else if (m_type == "intlist") {
        QStringList tmpList = m_default.split(',');
        QList<qint32> defaultList;
        foreach (const QString &tmp, tmpList) {
            defaultList.append(tmp.toInt());
        }
        item = m_config->addItemIntList(m_name, *d->newIntList(), defaultList, m_key);
    } else if (m_type == "longlong") {
        KConfigSkeleton::ItemLongLong *longlongItem = m_config->addItemLongLong(m_name,
                *d->newLongLong(), m_default.toLongLong(), m_key);
        if (m_haveMin) {
            longlongItem->setMinValue(m_min);
        }
        if (m_haveMax) {
            longlongItem->setMaxValue(m_max);
        }
        item = longlongItem;
    /* No addItemPathList in KConfigSkeleton ?
    } else if (m_type == "PathList") {
        //FIXME: the split() is naive and will break on lists with ,'s in them
        item = m_config->addItemPathList(m_name, *d->newStringList(), m_default.split(","), m_key);
    */
    } else if (m_type == "point") {
        QPoint defaultPoint;
        QStringList tmpList = m_default.split(',');
        while (tmpList.size() >= 2) {
            defaultPoint.setX(tmpList[0].toInt());
            defaultPoint.setY(tmpList[1].toInt());
        }
        item = m_config->addItemPoint(m_name, *d->newPoint(), defaultPoint, m_key);
    } else if (m_type == "rect") {
        QRect defaultRect;
        QStringList tmpList = m_default.split(',');
        while (tmpList.size() >= 4) {
            defaultRect.setCoords(tmpList[0].toInt(), tmpList[1].toInt(),
                                  tmpList[2].toInt(), tmpList[3].toInt());
        }
        item = m_config->addItemRect(m_name, *d->newRect(), defaultRect, m_key);
    } else if (m_type == "size") {
        QSize defaultSize;
        QStringList tmpList = m_default.split(',');
        while (tmpList.size() >= 2) {
            defaultSize.setWidth(tmpList[0].toInt());
            defaultSize.setHeight(tmpList[1].toInt());
        }
        item = m_config->addItemSize(m_name, *d->newSize(), defaultSize, m_key);
    } else if (m_type == "ulonglong") {
        KConfigSkeleton::ItemULongLong *ulonglongItem =
            m_config->addItemULongLong(m_name, *d->newULongLong(), m_default.toULongLong(), m_key);
        if (m_haveMin) {
            ulonglongItem->setMinValue(m_min);
        }
        if (m_haveMax) {
            ulonglongItem->setMaxValue(m_max);
        }
        item = ulonglongItem;
    /* No addItemUrlList in KConfigSkeleton ?
    } else if (m_type == "urllist") {
        //FIXME: the split() is naive and will break on lists with ,'s in them
        QStringList tmpList = m_default.split(",");
        KUrl::List defaultList;
        foreach (const QString& tmp, tmpList) {
            defaultList.append(KUrl(tmp));
        }
        item = m_config->addItemUrlList(m_name, *d->newUrlList(), defaultList, m_key);*/
    }

    if (item) {
        item->setLabel(m_label);
        item->setWhatsThis(m_whatsThis);
        d->keysToNames.insert(item->group() + item->key(), item->name());
    }
}

void ConfigLoaderHandler::resetState()
{
    m_haveMin = false;
    m_min = 0;
    m_haveMax = false;
    m_max = 0;
    m_name.clear();
    m_type.clear();
    m_label.clear();
    m_default.clear();
    m_key.clear();
    m_whatsThis.clear();
    m_enumChoices.clear();
    m_inChoice = false;
}

ConfigLoader::ConfigLoader(const QString &configFile, QIODevice *xml, QObject *parent)
    : KConfigSkeleton(configFile, parent),
      d(new ConfigLoaderPrivate)
{
    d->parse(this, xml);
}

ConfigLoader::ConfigLoader(KSharedConfigPtr config, QIODevice *xml, QObject *parent)
    : KConfigSkeleton(config, parent),
      d(new ConfigLoaderPrivate)
{
    d->parse(this, xml);
}

//FIXME: obviously this is broken and should be using the group as the root,
//       but KConfigSkeleton does not currently support this. it will eventually though,
//       at which point this can be addressed properly
ConfigLoader::ConfigLoader(const KConfigGroup *config, QIODevice *xml, QObject *parent)
    : KConfigSkeleton(KSharedConfig::openConfig(config->config()->name()), parent),
      d(new ConfigLoaderPrivate)
{
    KConfigGroup group = config->parent();
    d->baseGroup = config->name();
    while (group.isValid() && group.name() != "<default>") {
        d->baseGroup = group.name() + '\x1d' + d->baseGroup;
        group = group.parent();
    }
    d->parse(this, xml);
}

ConfigLoader::~ConfigLoader()
{
    delete d;
}

KConfigSkeletonItem *ConfigLoader::findItem(const QString &group, const QString &key)
{
    return KConfigSkeleton::findItem(d->keysToNames[group + key]);
}

bool ConfigLoader::hasGroup(const QString &group) const
{
    return d->groups.contains(group);
}

QStringList ConfigLoader::groupList() const
{
    return d->groups;
}

} // Plasma namespace
