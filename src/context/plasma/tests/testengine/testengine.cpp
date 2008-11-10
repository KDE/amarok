/*
 *   Copyright (C) 2008 Gilles CHAUVIN <gcnweb+kde@gmail.com>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License either version 2, or
 *   (at your option) any later version as published by the Free Software
 *   Foundation.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details
 *
 *   You should have received a copy of the GNU General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */


#include "testengine.h"

#include <QtCore/QBitArray>
#include <QtCore/QDate>
#include <QtCore/QLocale>
#include <QtCore/QUrl>
#include <QtGui/QBitmap>
#include <QtGui/QBrush>
#include <QtGui/QCursor>
#include <QtGui/QFont>
#include <QtGui/QIcon>
#include <QtGui/QPalette>
#include <QtGui/QPen>
#include <QtGui/QSizePolicy>
#include <QtGui/QTextFormat>


Q_DECLARE_METATYPE(TestEngine::MyUserType)


TestEngine::TestEngine(QObject *parent, const QVariantList &args)
    : Plasma::DataEngine(parent, args)
{
} // ctor()


TestEngine::~TestEngine()
{
} // dtor()


void TestEngine::init()
{
    QString dsn("TestEngine");
    
    // QVariant::Invalid
    // QVariant::BitArray
    setData(dsn, "QBitArray", QVariant(QBitArray(97, false)));
    // QVariant::Bitmap
    setData(dsn, "QBitmap", QVariant(QBitmap(12, 57)));
    // QVariant::Bool
    setData(dsn, "bool", QVariant((bool)true));
    // QVariant::Brush
    setData(dsn, "QBrush", QVariant(QBrush(Qt::SolidPattern)));
    // QVariant::ByteArray
    QByteArray byteArray;
    for (int i=0; i<256; ++i) {
        byteArray.append(i);
    }
    setData(dsn, "QByteArray1", QVariant(byteArray));
    setData(dsn, "QByteArray2", QVariant(QByteArray("KDE4")));
    // QVariant::Char
    setData(dsn, "QChar", QVariant(QChar(0x4B)));
    // QVariant::Color
    setData(dsn, "QColor", QVariant(QColor("#031337")));
    // QVariant::Cursor
    setData(dsn, "QCursor", QVariant(QCursor(Qt::ArrowCursor)));
    // QVariant::Date
    setData(dsn, "QDate", QVariant(QDate(2008, 1, 11)));
    // QVariant::DateTime
    setData(dsn, "QDateTime", QVariant(QDateTime(QDate(2008, 1, 11), QTime(12, 34, 56))));
    // QVariant::Double
    setData(dsn, "double", QVariant((double)12.34));
    // QVariant::Font
    setData(dsn, "QFont", QVariant(QFont()));
    // QVariant::Icon
    setData(dsn, "QIcon", QVariant(QIcon(QPixmap(12, 34))));
    // QVariant::Image
    setData(dsn, "QImage", QVariant(QImage(56, 78, QImage::Format_Mono)));
    // QVariant::Int
    setData(dsn, "int", QVariant((int)-4321));
    // QVariant::KeySequence (???)
    // QVariant::Line
    setData(dsn, "QLine", QVariant(QLine(12, 34, 56, 78)));
    // QVariant::LineF
    setData(dsn, "QLineF", QVariant(QLineF(1.2, 3.4, 5.6, 7.8)));
    // QVariant::List
    QList<QVariant> list;
    list << QString("KDE4") << QBrush() << QPen();
    setData(dsn, "QList", QVariant(list));
    // QVariant::Locale
    setData(dsn, "QLocale", QVariant(QLocale("fr_FR")));
    // QVariant::LongLong
    setData(dsn, "qlonglong", QVariant((qlonglong)-4321));
    // QVariant::Map
    QMap<QString, QVariant> map;
    for (int i=0; i<123; ++i) {
        QString key = QString("key%1").arg(i);
        QString val = QString("value%1").arg(i);
        map[key] = val;
    }
    setData(dsn, "QMap", QVariant(map));
    // QVariant::Matrix
    setData(dsn, "QMatrix", QVariant(QMatrix()));
    // QVariant::Transform
    setData(dsn, "QTransform", QVariant(QTransform()));
    // QVariant::Palette
    setData(dsn, "QPalette", QVariant(QPalette()));
    // QVariant::Pen
    setData(dsn, "QPen", QVariant(QPen(Qt::SolidLine)));
    // QVariant::Pixmap
    setData(dsn, "QPixmap", QVariant(QPixmap(12, 34)));
    // QVariant::Point
    setData(dsn, "QPoint", QVariant(QPoint(12, 34)));
    // QVariant::PointArray (obsoloted in Qt4, see QPolygon)
    // QVariant::PointF
    setData(dsn, "QPointF", QVariant(QPointF(12.34, 56.78)));
    // QVariant::Polygon
    setData(dsn, "QPolygon", QVariant(QPolygon(42)));
    // QVariant::Rect
    setData(dsn, "QRect", QVariant(QRect(12, 34, 56, 78)));
    // QVariant::RectF
    setData(dsn, "QRectF", QVariant(QRectF(1.2, 3.4, 5.6, 7.8)));
    // QVariant::RegExp
    setData(dsn, "QRegExp", QVariant(QRegExp("^KDE4$")));
    // QVariant::Region
    setData(dsn, "QRegion", QVariant(QRegion(10, 20, 30, 40)));
    // QVariant::Size
    setData(dsn, "QSize", QVariant(QSize(12, 34)));
    // QVariant::SizeF
    setData(dsn, "QSizeF", QVariant(QSizeF(12.34, 56.78)));
    // QVariant::SizePolicy
    setData(dsn, "QSizePolicy", QVariant(QSizePolicy()));
    // QVariant::String
    setData(dsn, "QString", QVariant(QString("KDE4 ROCKS!")));
    // QVariant::StringList
    QStringList stringList;
    stringList << "K" << "D" << "E" << "4";
    setData(dsn, "QStringList", QVariant(stringList));
    // QVariant::TextFormat
    setData(dsn, "QTextFormat", QVariant(QTextFormat()));
    // QVariant::TextLength
    setData(dsn, "QTextLength", QVariant(QTextLength()));
    // QVariant::Time
    setData(dsn, "QTime", QVariant(QTime(12, 34, 56)));
    // QVariant::UInt
    setData(dsn, "uint", QVariant((uint)4321));
    // QVariant::ULongLong
    setData(dsn, "qulonglong", QVariant((qulonglong)4321));
    // QVariant::Url
    setData(dsn, "QUrl", QVariant(QUrl("http://user:password@example.com:80/test.php?param1=foo&param2=bar")));
    // QVariant::UserType
    MyUserType userType;
    QVariant v;
    v.setValue(userType);
    setData(dsn, "UserType", v);
} // init()


bool TestEngine::sourceRequestEvent(const QString &source)
{
    // Nothing to do...
    Q_UNUSED(source)
    return true;
} // sourceRequestEvent()


#include "testengine.moc"
