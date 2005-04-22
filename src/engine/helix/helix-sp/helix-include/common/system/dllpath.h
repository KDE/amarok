
/*
 *
 * This software is released under the provisions of the GPL version 2.
 * see file "COPYING".  If that file is not available, the full statement 
 * of the license can be found at
 *
 * http://www.fsf.org/licensing/licenses/gpl.txt
 *
 * Portions Copyright (c) 1995-2004 RealNetworks, Inc. All Rights Reserved.
 *
 */

#ifndef _DLL_PATH
#define _DLL_PATH

#include "hxcom.h"
#include "hxmap.h"
#include "hxstring.h"

#ifdef _MACINTOSH                                                           
#pragma export on                                                          
STDAPI SetDLLAccessPath(const char* pPathDescriptor);
#pragma export off 
#endif

#if defined(HELIX_CONFIG_NOSTATICS)
#include "globals/hxglobals.h"
#endif

/*
 *      Used to identify dll types.  
 */
typedef enum dll_types
{
        DLLTYPE_NOT_DEFINED = 0,              // Arbitrary DLLs (no predefined path used)
        DLLTYPE_PLUGIN,                       // Plug-ins
        DLLTYPE_CODEC,                        // Codecs
        DLLTYPE_ENCSDK,                       // Encoder SDK DLLs 
        DLLTYPE_COMMON,                       // Common libraries       
        DLLTYPE_UPDATE,                       // Setup/Upgrade libraries
        DLLTYPE_OBJBROKR,                     // Special entry for the object broker
        DLLTYPE_RCAPLUGIN,                    // Gemini plugins
        DLLTYPE_NUMBER                        // Not a type, used as number of predefined types.
} DLLTYPES;           

typedef HX_RESULT (HXEXPORT_PTR FPSETDLLACCESSPATH) (const char*);


class DLLAccessPath
{
public:
    DLLAccessPath();
    virtual ~DLLAccessPath();

    // This class is only ref-counted if it is used as such. 
    // Most of the system uses this as a non-refcounted class.
    STDMETHOD_(ULONG32,AddRef)  (THIS);
    STDMETHOD_(ULONG32,Release) (THIS);

    HX_RESULT SetAccessPaths(const char* pPathDescriptor);  
    HX_RESULT SetPath(UINT16 nLibType, const char* szPath);
    HX_RESULT SetPath(const char* szLibType, const char* szPath);

    const char* GetPath(UINT16 nLibType);
    const char* GetPath(const char* szLibType);
    const char* GetLibTypeName(UINT16 nLibType);

    HX_RESULT PassDLLAccessPath(FPSETDLLACCESSPATH pSetDLLAccessPath);

    HX_RESULT AddPathToEnvironment(const char* szPath);
    HX_RESULT RestoreEnvironment();
    UINT32 GetNumPaths() {return m_mapPathes.GetCount();}

protected:

    static const char* const zm_pszDllTypeNames[DLLTYPE_NUMBER];

private:

    LONG32               m_lRefCount;

    CHXMapStringToString m_mapPathes;
    CHXString            m_strPathEnvVar;
};

extern DLLAccessPath* GetDLLAccessPath();  

class DLLAccessDestructor
{
public:
    DLLAccessDestructor() {};
    ~DLLAccessDestructor() 
    { 
#ifndef _VXWORKS
        if (GetDLLAccessPath())
        {
            GetDLLAccessPath()->Release(); 
        }
#endif
    }
};


//
// Macros for setting DLL loading paths
//
#ifndef _VXWORKS

#if defined(_STATICALLY_LINKED) && !defined(HELIX_FEATURE_SERVER)

// We need this since many DLLs have this listed
// as an export, so we have to have it defined
#define ENABLE_DLLACCESS_PATHS(GLOBAL)                           \
STDAPI ENTRYPOINT(SetDLLAccessPath)(const char* pPathDescriptor) \
{                                                                \
    return HXR_OK;                                               \
}

#define ENABLE_MULTILOAD_DLLACCESS_PATHS(GLOBAL)                 \
STDAPI ENTRYPOINT(SetDLLAccessPath)(const char* pPathDescriptor) \
{                                                                \
        return HXR_OK;                                           \
}

#elif defined(HELIX_CONFIG_NOSTATICS)

#define ENABLE_DLLACCESS_PATHS(GLOBAL)                                      \
    static const DLLAccessPath* const _g_##GLOBAL = NULL;                   \
                                                                            \
    DLLAccessPath* ENTRYPOINT(GetDLLAccessPath)()                           \
    {                                                                       \
        return &HXGlobalDLLAccessPath::Get(&_g_##GLOBAL);                    \
    }                                                                       \
                                                                            \
    STDAPI ENTRYPOINT(SetDLLAccessPath)(const char* pPathDescriptor)        \
    {                                                                       \
        return (GetDLLAccessPath()->SetAccessPaths(pPathDescriptor));       \
    }                                                                       

#else /* #if defined(_STATICALLY_LINKED) && !defined(HELIX_FEATURE_SERVER) */

#define ENABLE_DLLACCESS_PATHS(GLOBAL)                                      \
    DLLAccessPath GLOBAL;                                                   \
                                                                            \
    DLLAccessPath* ENTRYPOINT(GetDLLAccessPath)()                           \
    {                                                                       \
        return &GLOBAL;                                                     \
    }                                                                       \
                                                                            \
    STDAPI ENTRYPOINT(SetDLLAccessPath)(const char* pPathDescriptor)        \
    {                                                                       \
        return (GetDLLAccessPath()->SetAccessPaths(pPathDescriptor));       \
    }                                                                       

#ifdef _UNIX

#define ENABLE_MULTILOAD_DLLACCESS_PATHS(GLOBAL)                            \
    DLLAccessPath* GLOBAL = NULL;                                           \
                                                                            \
    DLLAccessPath* GetDLLAccessPath()                                       \
    {                                                                       \
        if (!GLOBAL)                                                        \
        {                                                                   \
            GLOBAL = new DLLAccessPath();                                   \
            GLOBAL->AddRef();                                               \
        }                                                                   \
        return GLOBAL;                                                      \
    }                                                                       \
                                                                            \
    STDAPI ENTRYPOINT(SetDLLAccessPath)(const char* pPathDescriptor)                \
    {                                                                       \
        if (!GLOBAL)                                                        \
        {                                                                   \
            GLOBAL = new DLLAccessPath();                                   \
            GLOBAL->AddRef();                                               \
        }                                                                   \
                                                                            \
        return (GLOBAL->SetAccessPaths(pPathDescriptor));                   \
    }
                                                                            
#else /* #ifdef _UNIX */

#define ENABLE_MULTILOAD_DLLACCESS_PATHS(GLOBAL)                            \
    DLLAccessPath* GLOBAL = NULL;                                           \
    DLLAccessDestructor GLOBALDestructor;                                   \
                                                                            \
    DLLAccessPath* GetDLLAccessPath()                                       \
    {                                                                       \
        if (!GLOBAL)                                                        \
        {                                                                   \
            GLOBAL = new DLLAccessPath();                                   \
            GLOBAL->AddRef();                                               \
        }                                                                   \
        return GLOBAL;                                                      \
    }                                                                       \
                                                                            \
    STDAPI ENTRYPOINT(SetDLLAccessPath)(const char* pPathDescriptor)                \
    {                                                                       \
        if (!GLOBAL)                                                        \
        {                                                                   \
            GLOBAL = new DLLAccessPath();                                   \
            GLOBAL->AddRef();                                               \
        }                                                                   \
                                                                            \
        return (GLOBAL->SetAccessPaths(pPathDescriptor));                   \
    }                                                                       

#endif /* #ifdef _UNIX #else */

#endif /* #if defined(_STATICALLY_LINKED) #else */

#else /* #ifndef _VXWORKS */

#define ENABLE_DLLACCESS_PATHS(GLOBAL)                                      \
    STDAPI ENTRYPOINT(SetDLLAccessPath)(const char* pPathDescriptor)        \
    {                                                                       \
        return 0;                                                           \
    }                                                                       

#ifdef _SERVER
#define ENABLE_MULTILOAD_DLLACCESS_PATHS(GLOBAL)                            \
    STDAPI ENTRYPOINT(SetDLLAccessPath)(const char* pPathDescriptor)        \
    {                                                                       \
        return 0;                                                           \
    }                                                                       
#else
#define ENABLE_MULTILOAD_DLLACCESS_PATHS(GLOBAL)                            \
    STDAPI ENTRYPOINT(SetDLLAccessPath)(const char* pPathDescriptor)        \
    {                                                                       \
        return 0;                                                           \
    }                                                                       
#endif

#endif /* #ifndef _VXWORKS #else */

#endif /* #ifndef _DLL_PATH */

