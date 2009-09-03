/****************************************************************************************
 * Copyright (c) 1992-2007 Trolltech ASA <copyright@trolltech.com>                      *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) version 3 or any    *
 * later version publicly approved by Trolltech ASA (or its successor, if any) and the  *
 * KDE Free Qt Foundation.                                                              *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 *                                                                                      *
 * In addition, Trolltech gives you certain additional rights as described in the       *
 * Trolltech GPL Exception version 1.2 which can be found at                            *
 * http://www.trolltech.com/products/qt/gplexception/                                   *
 ****************************************************************************************/

#ifndef QTSCRIPTEXTENSIONS_GLOBAL_H
#define QTSCRIPTEXTENSIONS_GLOBAL_H

#include <QtCore/QSharedData>

#define DECLARE_SELF(Class, __fn__) \
    Class* self = qscriptvalue_cast<Class*>(ctx->thisObject()); \
    if (!self) { \
        return ctx->throwError(QScriptContext::TypeError, \
            QString::fromLatin1("%0.prototype.%1: this object is not a %0") \
            .arg(#Class).arg(#__fn__)); \
    }

#define DECLARE_SELF2(Class, __fn__, __ret__) \
    Class* self = qscriptvalue_cast<Class*>(thisObject()); \
    if (!self) { \
        context()->throwError(QScriptContext::TypeError, \
            QString::fromLatin1("%0.prototype.%1: this object is not a %0") \
            .arg(#Class).arg(#__fn__)); \
        return __ret__; \
    }



#define ADD_METHOD(__p__, __f__) \
    __p__.setProperty(#__f__, __p__.engine()->newFunction(__f__))

#define ADD_GET_METHOD(__p__, __get__) \
    ADD_METHOD(__p__, __get__)

#define ADD_GET_SET_METHODS(__p__, __get__, __set__) \
do { \
    ADD_METHOD(__p__, __get__); \
    ADD_METHOD(__p__, __set__); \
} while (0);

#define ADD_CTOR_FUNCTION(__c__, __f__) ADD_METHOD(__c__, __f__)

#define ADD_ENUM_VALUE(__c__, __ns__, __v__) \
    __c__.setProperty(#__v__, QScriptValue(__c__.engine(), __ns__::__v__))


#define BEGIN_DECLARE_METHOD(Class, __mtd__) \
static QScriptValue __mtd__(QScriptContext *ctx, QScriptEngine *eng) \
{ \
    DECLARE_SELF(Class, __mtd__);

#define END_DECLARE_METHOD \
}


#define DECLARE_GET_METHOD(Class, __get__) \
BEGIN_DECLARE_METHOD(Class, __get__) { \
    return qScriptValueFromValue(eng, self->__get__()); \
} END_DECLARE_METHOD

#define DECLARE_SET_METHOD(Class, T, __set__) \
BEGIN_DECLARE_METHOD(Class, __set__) { \
    self->__set__(qscriptvalue_cast<T>(ctx->argument(0))); \
    return eng->undefinedValue(); \
} END_DECLARE_METHOD

#define DECLARE_GET_SET_METHODS(Class, T, __get__, __set__) \
DECLARE_GET_METHOD(Class, /*T,*/ __get__) \
DECLARE_SET_METHOD(Class, T, __set__)



#define DECLARE_SIMPLE_GET_METHOD(Class, __get__) \
BEGIN_DECLARE_METHOD(Class, __get__) { \
    return QScriptValue(eng, self->__get__()); \
} END_DECLARE_METHOD

#define DECLARE_SIMPLE_SET_METHOD(Class, ToType, __set__) \
BEGIN_DECLARE_METHOD(Class, __set__) { \
    self->__set__(ctx->argument(0).ToType()); \
    return eng->undefinedValue(); \
} END_DECLARE_METHOD

#define DECLARE_BOOLEAN_GET_METHOD(Class, __set__) \
    DECLARE_SIMPLE_GET_METHOD(Class, __set__)
#define DECLARE_BOOLEAN_SET_METHOD(Class, __set__) \
    DECLARE_SIMPLE_SET_METHOD(Class, toBoolean, __set__)

#define DECLARE_INT_GET_METHOD(Class, __set__) \
    DECLARE_SIMPLE_GET_METHOD(Class, __set__)
#define DECLARE_INT_SET_METHOD(Class, __set__) \
    DECLARE_SIMPLE_SET_METHOD(Class, toInt32, __set__)

#define DECLARE_NUMBER_GET_METHOD(Class, __set__) \
    DECLARE_SIMPLE_GET_METHOD(Class, __set__)
#define DECLARE_NUMBER_SET_METHOD(Class, __set__) \
    DECLARE_SIMPLE_SET_METHOD(Class, toNumber, __set__)

#define DECLARE_STRING_GET_METHOD(Class, __set__) \
    DECLARE_SIMPLE_GET_METHOD(Class, __set__)
#define DECLARE_STRING_SET_METHOD(Class, __set__) \
    DECLARE_SIMPLE_SET_METHOD(Class, toString, __set__)

#define DECLARE_QOBJECT_GET_METHOD(Class, __get__) \
BEGIN_DECLARE_METHOD(Class, __get__) { \
    return eng->newQObject(self->__get__()); \
} END_DECLARE_METHOD
#define DECLARE_QOBJECT_SET_METHOD(Class, __set__) \
    DECLARE_SIMPLE_SET_METHOD(Class, toQObject, __set__)

#define DECLARE_BOOLEAN_GET_SET_METHODS(Class, __get__, __set__) \
    DECLARE_BOOLEAN_GET_METHOD(Class, __get__) \
    DECLARE_BOOLEAN_SET_METHOD(Class, __set__)

#define DECLARE_NUMBER_GET_SET_METHODS(Class, __get__, __set__) \
    DECLARE_NUMBER_GET_METHOD(Class, __get__) \
    DECLARE_NUMBER_SET_METHOD(Class, __set__)

#define DECLARE_INT_GET_SET_METHODS(Class, __get__, __set__) \
    DECLARE_INT_GET_METHOD(Class, __get__) \
    DECLARE_INT_SET_METHOD(Class, __set__)

#define DECLARE_STRING_GET_SET_METHODS(Class, __get__, __set__) \
    DECLARE_STRING_GET_METHOD(Class, __get__) \
    DECLARE_STRING_SET_METHOD(Class, __set__)

#define DECLARE_QOBJECT_GET_SET_METHODS(Class, __get__, __set__) \
    DECLARE_QOBJECT_GET_METHOD(Class, __get__) \
    DECLARE_QOBJECT_SET_METHOD(Class, __set__)


#define DECLARE_VOID_METHOD(Class, __fun__) \
BEGIN_DECLARE_METHOD(Class, __fun__) { \
    self->__fun__(); \
    return eng->undefinedValue(); \
} END_DECLARE_METHOD

#define DECLARE_VOID_NUMBER_METHOD(Class, __fun__) \
BEGIN_DECLARE_METHOD(Class, __fun__) { \
    self->__fun__(ctx->argument(0).toNumber()); \
    return eng->undefinedValue(); \
} END_DECLARE_METHOD

#define DECLARE_VOID_NUMBER_NUMBER_METHOD(Class, __fun__) \
BEGIN_DECLARE_METHOD(Class, __fun__) { \
    self->__fun__(ctx->argument(0).toNumber(), ctx->argument(1).toNumber()); \
    return eng->undefinedValue(); \
} END_DECLARE_METHOD

#define DECLARE_VOID_QUAD_NUMBER_METHOD(Class, __fun__) \
BEGIN_DECLARE_METHOD(Class, __fun__) { \
    self->__fun__(ctx->argument(0).toNumber(), ctx->argument(1).toNumber(), ctx->argument(2).toNumber(), ctx->argument(3).toNumber()); \
    return eng->undefinedValue(); \
} END_DECLARE_METHOD

#define DECLARE_VOID_1ARG_METHOD(Class, ArgType, __fun__) \
BEGIN_DECLARE_METHOD(Class, __fun__) { \
    self->__fun__(qscriptvalue_cast<ArgType>(ctx->argument(0))); \
    return eng->undefinedValue(); \
} END_DECLARE_METHOD

#define DECLARE_BOOLEAN_1ARG_METHOD(Class, ArgType, __fun__) \
BEGIN_DECLARE_METHOD(Class, __fun__) { \
    return QScriptValue(eng, self->__fun__(qscriptvalue_cast<ArgType>(ctx->argument(0)))); \
} END_DECLARE_METHOD


#define DECLARE_POINTER_METATYPE(T) \
    Q_DECLARE_METATYPE(T*) \
    Q_DECLARE_METATYPE(QScript::Pointer<T>::wrapped_pointer_type)

namespace QScript
{

enum {
    UserOwnership = 1
};

template <typename T>
class Pointer : public QSharedData
{
public:
    typedef T* pointer_type;
    typedef QExplicitlySharedDataPointer<Pointer<T> > wrapped_pointer_type;

    ~Pointer()
    {
        if (!(m_flags & UserOwnership))
            delete m_value;
    }

    operator T*()
    {
        return m_value;
    }

    operator const T*() const
    {
        return m_value;
    }

    static wrapped_pointer_type create(T *value, uint flags = 0)
    {
        return wrapped_pointer_type(new Pointer(value, flags));
    }

    static QScriptValue toScriptValue(QScriptEngine *engine, T* const &source)
    {
        if (!source)
            return engine->nullValue();
        return engine->newVariant(qVariantFromValue(source));
    }

    static void fromScriptValue(const QScriptValue &value, T* &target)
    {
        if (value.isVariant()) {
            QVariant var = value.toVariant();
            if (qVariantCanConvert<T*>(var)) {
                target = qvariant_cast<T*>(var);
            } else if (qVariantCanConvert<wrapped_pointer_type>(var)) {
                target = qvariant_cast<wrapped_pointer_type>(var)->operator T*();
            } else {
                // look in prototype chain
                target = 0;
                int type = qMetaTypeId<T*>();
                int pointerType = qMetaTypeId<wrapped_pointer_type>();
                QScriptValue proto = value.prototype();
                while (proto.isObject() && proto.isVariant()) {
                    int protoType = proto.toVariant().userType();
                    if ((type == protoType) || (pointerType == protoType)) {
                        QByteArray name = QMetaType::typeName(var.userType());
                        if (name.startsWith("QScript::Pointer<")) {
                            target = (*reinterpret_cast<wrapped_pointer_type*>(var.data()))->operator T*();
                            break;
                        } else {
                            target = static_cast<T*>(var.data());
                            break;
                        }
                    }
                    proto = proto.prototype();
                }
            }
        } else if (value.isQObject()) {
            QObject *qobj = value.toQObject();
            QByteArray typeName = QMetaType::typeName(qMetaTypeId<T*>());
            target = reinterpret_cast<T*>(qobj->qt_metacast(typeName.left(typeName.size()-1)));
        } else {
            target = 0;
        }
    }

    uint flags() const
    { return m_flags; }
    void setFlags(uint flags)
    { m_flags = flags; }
    void unsetFlags(uint flags)
    { m_flags &= ~flags; }

protected:
    Pointer(T* value, uint flags)
        : m_flags(flags), m_value(value)
    {}

private:
    uint m_flags;
    T* m_value;
};

template <typename T>
int registerPointerMetaType(
    QScriptEngine *eng,
    const QScriptValue &prototype = QScriptValue(),
    T * /* dummy */ = 0
)
{
    QScriptValue (*mf)(QScriptEngine *, T* const &) = Pointer<T>::toScriptValue;
    void (*df)(const QScriptValue &, T* &) = Pointer<T>::fromScriptValue;
    const int id = qMetaTypeId<T*>();
    qScriptRegisterMetaType_helper(
        eng, id, reinterpret_cast<QScriptEngine::MarshalFunction>(mf),
        reinterpret_cast<QScriptEngine::DemarshalFunction>(df),
        prototype);
    eng->setDefaultPrototype(qMetaTypeId<typename Pointer<T>::wrapped_pointer_type>(), prototype);
    return id;
}

inline void maybeReleaseOwnership(const QScriptValue &value)
{
    if (value.isVariant()) {
        QVariant var = value.toVariant();
        QByteArray name = QMetaType::typeName(var.userType());
        if (name.startsWith("QScript::Pointer<"))
            (*reinterpret_cast<Pointer<void*>::wrapped_pointer_type *>(var.data()))->setFlags(UserOwnership);
    }
}

inline void maybeTakeOwnership(const QScriptValue &value)
{
    if (value.isVariant()) {
        QVariant var = value.toVariant();
        QByteArray name = QMetaType::typeName(var.userType());
        if (name.startsWith("QScript::Pointer<"))
            (*reinterpret_cast<Pointer<void*>::wrapped_pointer_type *>(var.data()))->unsetFlags(UserOwnership);
    }
}

template <class T>
inline QScriptValue wrapPointer(QScriptEngine *eng, T *ptr, uint flags = 0)
{
    return eng->newVariant(qVariantFromValue(Pointer<T>::create(ptr, flags)));
}

} // namespace QScript

#ifdef QGRAPHICSITEM_H

namespace QScript {

template <class T>
inline QScriptValue wrapGVPointer(QScriptEngine *eng, T *item)
{
    uint flags = item->parentItem() ? UserOwnership : 0;
    return wrapPointer<T>(eng, item, flags);
}

} // namespace QScript

#endif // QGRAPHICSITEM_H

#endif // QTSCRIPTEXTENSIONS_GLOBAL_H
