/***************************************************************************
 *   Copyright (C) 2007 Shane King <kde@dontletsstart.com>                 *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#define DEBUG_PREFIX "phonon-directshow"

#include "DirectShowBackend.h"
#include "DirectShowAudioOutput.h"
#include "DirectShowGraph.h"
#include "DirectShowMediaObject.h"
#include "ComVariant.h"

#include "debug.h"

#include <kpluginfactory.h>

#include <QtGui>

#include <dshow.h>
#include <ocidl.h>


K_PLUGIN_FACTORY(DirectShowBackendFactory, registerPlugin<DirectShowBackend>();)
K_EXPORT_PLUGIN(DirectShowBackendFactory("amarokdirectshowbackend"))


DirectShowBackend::DirectShowBackend(QObject *parent, const QVariantList &args)
    : QObject(parent)
{
    m_initialized = createMessageWindow() && loadAudioDevices();
}


DirectShowBackend::~DirectShowBackend()
{
    destroyMessageWindow();
}


QObject *
DirectShowBackend::createObject(BackendInterface::Class objectClass, QObject *parent, const QList<QVariant> &args)
{
    if( m_initialized )
    {
        switch( objectClass )
        {
            case BackendInterface::MediaObjectClass:
                return new DirectShowMediaObject(parent);

            case BackendInterface::AudioOutputClass:
                return new DirectShowAudioOutput(parent);
        }
    }
    return 0;
}


QList<int> 
DirectShowBackend::objectDescriptionIndexes(Phonon::ObjectDescriptionType type) const
{
    QList<int> result;

    if( type == Phonon::AudioOutputDeviceType )
    {
        for( int i = 0; i < m_audioDevices.size(); ++i )
        {
            result.push_back( i );
        }
    }

    return result;
}


QHash<QByteArray, QVariant> 
DirectShowBackend::objectDescriptionProperties(Phonon::ObjectDescriptionType type, int index) const
{
    QHash<QByteArray, QVariant> result;

    if( type == Phonon::AudioOutputDeviceType )
    {
        ComPtr<IMoniker> moniker = m_audioDevices[ index ];

        HResult hr;
        ComPtr<IPropertyBag> propBag;
        hr = moniker->BindToStorage( 0, 0, IID_IPropertyBag, reinterpret_cast<void **>( &propBag ) );
        if( SUCCEEDED( hr ) )
        {
            ComVariant name;
            hr = propBag->Read(L"FriendlyName", &name, 0);
            if( SUCCEEDED( hr ) )
            {
                result.insert( "name", name.AsString() );
            }
            else
            {
                debug() << "Failed to get FriendlyName: " << hr;
            }
        }
        else
        {
            debug() << "Failed to get properties for audio device: " << hr;
        }
    }

    return result;
}


bool 
DirectShowBackend::startConnectionChange(QSet<QObject *>)
{
    return true;
}


bool 
DirectShowBackend::connectNodes(QObject *source, QObject *sink)
{
    DirectShowMediaObject *mediaObject = qobject_cast<DirectShowMediaObject *>( source );
    DirectShowAudioOutput *audioOutput = qobject_cast<DirectShowAudioOutput *>( sink );
    if( mediaObject && audioOutput )
    {
        if( mediaObject->getGraph() )
            debug() << "Can't connect multiple outputs to a media object";
        else
            return ( new DirectShowGraph( this, mediaObject, audioOutput ) )->initialized();
    }
    else
    {
        debug() << "Can't connect nodes: mediaObject = " << (mediaObject == 0) << " audioOutput = " << (audioOutput == 0);
    }
    return false;
}


bool 
DirectShowBackend::disconnectNodes(QObject *source, QObject *sink)
{
    DirectShowMediaObject *mediaObject = qobject_cast<DirectShowMediaObject *>( source );
    DirectShowAudioOutput *audioOutput = qobject_cast<DirectShowAudioOutput *>( sink );
    
    delete mediaObject->getGraph();
    mediaObject->setGraph( 0 );

    return false;
}


bool 
DirectShowBackend::endConnectionChange(QSet<QObject *>)
{
    return true;
}


QStringList 
DirectShowBackend::availableMimeTypes() const
{
    // no way to get a list from DirectShow here :(
    return QStringList();
}


extern "C" LRESULT CALLBACK WndProc( HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    if( uMsg == DirectShowBackend::WM_GRAPH_EVENT )
    {
        return reinterpret_cast<DirectShowGraph *>( lParam )->onEvent() ? 0 : -1;
    }
    return DefWindowProc( hwnd, uMsg, wParam, lParam );
}


static const wchar_t * WINDOW_CLASS = L"PhononAmarokDirectShow";
const int DirectShowBackend::WM_GRAPH_EVENT = WM_APP + 1;

bool
DirectShowBackend::createMessageWindow()
{
    WNDCLASS cls;
    cls.style = 0;
    cls.lpfnWndProc = WndProc;
    cls.cbClsExtra = 0;
    cls.cbWndExtra = 0;
    cls.hInstance = qWinAppInst();
    cls.hIcon = 0;
    cls.hCursor = 0;
    cls.hbrBackground = 0;
    cls.lpszMenuName = 0;
    cls.lpszClassName = WINDOW_CLASS;

    if( RegisterClass(&cls) )
    {
        m_window = CreateWindowEx( 0, WINDOW_CLASS, 
            L"PhononAmarokDirectShowWindow",0 ,0 ,0 ,0 ,0 ,HWND_MESSAGE,0, cls.hInstance, 0);
        if(m_window)
            return true;
        else
            debug() << "Failed to create message window";
    }
    else
    {
        debug() << "Failed to register window class";
    }
    return false;
}


void
DirectShowBackend::destroyMessageWindow()
{
    DestroyWindow(m_window);
    UnregisterClass(WINDOW_CLASS, qWinAppInst());
}


bool
DirectShowBackend::loadAudioDevices()
{
    // Create the System Device Enumerator.
    HResult hr;
    ComPtr<ICreateDevEnum> sysDevEnum;
    hr = sysDevEnum.CreateInstance(CLSID_SystemDeviceEnum, IID_ICreateDevEnum);
    if ( SUCCEEDED( hr ) )
    {
        ComPtr<IEnumMoniker> enumCat;
        hr = sysDevEnum->CreateClassEnumerator(CLSID_AudioRendererCategory, &enumCat, 0);
        if ( SUCCEEDED( hr ) && enumCat )
        {
            ComPtr<IMoniker> moniker;
            unsigned long fetched;
            int i = 1;
            while( enumCat->Next( 1, &moniker, &fetched ) == S_OK )
            {
                m_audioDevices.push_back( moniker );
            }
            return true;
        }
        else
        {
            debug() << "Failure enumerating device: " << hr;
        }
    }
    else
    {
        debug() << "Failed to create system device enumerator: " << hr;
    }
    return false;
}
