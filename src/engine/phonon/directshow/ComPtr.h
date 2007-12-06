/***************************************************************************
 *   Copyright (C) 2007 Shane King <kde@dontletsstart.com>                 *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef AMAROK_COMPTR_H
#define AMAROK_COMPTR_H

#include <objbase.h>
#include <algorithm>
#include <stdio.h>
#include <ostream>

// helper to output HRESULTs as hex
class HResult
{
public:
    HResult() : m_hr(S_OK) {}
    HResult(HRESULT hr) : m_hr(hr) {}

    HResult &operator=(HRESULT hr) { m_hr = hr; return *this; }
    operator HRESULT() const { return m_hr; }
private:
    HRESULT m_hr;
};

inline std::ostream &operator<<(std::ostream &stream, const HResult &hr)
{
    char buf[16];
    sprintf( buf, "0x%08x", static_cast<HRESULT>( hr ) );
    stream << buf;
    return stream;
}

// basic COM smart pointer
template <typename T>
class ComPtr
{
public:
    ComPtr() : m_ptr(0) {}
    ComPtr(const ComPtr<T> &other)
        : m_ptr(other.m_ptr)
    {
        m_ptr->AddRef();
    }

    ~ComPtr()
    {
        if( m_ptr )
            m_ptr->Release();
    }

    ComPtr<T> &operator=(ComPtr<T> other)
    {
        std::swap(m_ptr, other.m_ptr);
        return *this;
    }

    T **operator&()
    {
        if( m_ptr )
        {
            m_ptr->Release();
            m_ptr = 0;
        }
        return &m_ptr;
    }

    T *operator->()
    {
        return m_ptr;
    }

    operator T*()
    {
        return m_ptr;
    }

    operator bool() const
    {
        return m_ptr != 0;
    }

    HResult CreateInstance(CLSID clsid, IID iid)
    {
        return CoCreateInstance( clsid, NULL, CLSCTX_INPROC_SERVER, iid, reinterpret_cast<void **>( &*this ) );
    }

    template <typename U>
    HResult QueryInterface(IID iid, ComPtr<U> &other)
    {
        if( !m_ptr )
            return E_NOINTERFACE;

        return m_ptr->QueryInterface( iid, reinterpret_cast<void **>( &other ) );
    }

private:
    T *m_ptr;
};

#endif // AMAROK_COMPTR_H
