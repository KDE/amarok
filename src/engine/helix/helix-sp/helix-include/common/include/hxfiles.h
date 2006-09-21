
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

#ifndef _HXFILES_H_
#define _HXFILES_H_

/*
 * Forward declarations of some interfaces defined here-in.
 */
typedef _INTERFACE      IHXFileObject                   IHXFileObject;
typedef _INTERFACE      IHXFileObjectExt                IHXFileObjectExt;
typedef _INTERFACE      IHXFileResponse         IHXFileResponse;
typedef _INTERFACE      IHXFileSystemObject             IHXFileSystemObject;
typedef _INTERFACE      IHXFileStat                     IHXFileStat;
typedef _INTERFACE      IHXFileStatResponse             IHXFileStatResponse;

typedef _INTERFACE      IHXFileSystemManager            IHXFileSystemManager;
typedef _INTERFACE      IHXFileSystemManagerResponse    IHXFileSystemManagerResponse;
typedef _INTERFACE      IHXFileExists                   IHXFileExists;
typedef _INTERFACE      IHXFileExistsResponse           IHXFileExistsResponse;
typedef _INTERFACE      IHXFileMimeMapper               IHXFileMimeMapper;
typedef _INTERFACE      IHXFileMimeMapperResponse       IHXFileMimeMapperResponse;
typedef _INTERFACE      IHXBroadcastMapper              IHXBroadcastMapper;
typedef _INTERFACE      IHXBroadcastMapperResponse      IHXBroadcastMapperResponse;
typedef _INTERFACE      IHXGetFileFromSamePoolResponse IHXGetFileFromSamePoolResponse;
typedef _INTERFACE      IHXBuffer                       IHXBuffer;
typedef _INTERFACE      IHXPacket                       IHXPacket;
typedef _INTERFACE      IHXValues                       IHXValues;
typedef _INTERFACE      IHXMetaCreation         IHXMetaCreation;

typedef _INTERFACE      IHXAuthenticator               IHXAuthenticator;
typedef _INTERFACE      IHXRequest                     IHXRequest;
typedef _INTERFACE      IHXFileRename                  IHXFileRename;
typedef _INTERFACE      IHXFileMove                     IHXFileMove;
typedef _INTERFACE      IHXDirHandler                   IHXDirHandler;
typedef _INTERFACE      IHXDirHandlerResponse           IHXDirHandlerResponse;
typedef _INTERFACE      IHXFileRemove                  IHXFileRemove;
// $Private:
typedef _INTERFACE      IHXFastFileFactory             IHXFastFileFactory;
typedef _INTERFACE      IHXHTTPPostObject               IHXHTTPPostObject;
typedef _INTERFACE      IHXHTTPPostResponse             IHXHTTPPostResponse;
typedef _INTERFACE      IHXHTTPRedirect         IHXHTTPRedirect;
typedef _INTERFACE      IHXHTTPRedirectResponse IHXHTTPRedirectResponse;
typedef _INTERFACE      IHXRM2Converter2               IHXRM2Converter2;
typedef _INTERFACE      IHXRM2Converter2Response       IHXRM2Converter2Response;
typedef _INTERFACE      IHXPoolPathAdjustment           IHXPoolPathAdjustment;
typedef _INTERFACE      IHXPostDataHandler              IHXPostDataHandler;
// $EndPrivate.

/****************************************************************************
 *  Defines:
 *      HX_FILE_XXXX
 *  Purpose:
 *      Flags for opening file objects
 */
#define HX_FILE_READ            1
#define HX_FILE_WRITE           2
#define HX_FILE_BINARY          4
#define HX_FILE_NOTRUNC         8
#define HX_FILE_NOPAC           16

/****************************************************************************
 *  Defines:
 *      HX_FILEADVISE_XXXX
 *  Purpose:
 *      Flags for file object Advise method
 */
#define HX_FILEADVISE_RANDOMACCESS              1
#define HX_FILEADVISE_SYNCACCESS                2
#define HX_FILEADVISE_ASYNCACCESS               3
#define HX_FILEADVISE_RANDOMACCESSONLY  4
#define HX_FILEADVISE_ANYACCESS         5

/****************************************************************************
 *  Defines:
 *      HX_FILERESPONSEADVISE_XXXX
 *  Purpose:
 *      Flags for file response Advise method
 */
#define HX_FILERESPONSEADVISE_REQUIREFULLREAD 1


#if defined(_UNIX) || defined(_WINDOWS)
#include "hlxclib/sys/stat.h"
/*
 * This is a subset of standard stat()/fstat() values that both Unix and
 * Windows support (or at least define).
 *
 * These flags are returned from IHXFileStatResponse::StatDone() in the
 * ulMode argument.
 */
#define HX_S_IFMT   S_IFMT
#define HX_S_IFDIR  S_IFDIR
#define HX_S_IFCHR  S_IFCHR
#define HX_S_IFIFO  S_IFIFO
#define HX_S_IFREG  S_IFREG
#else
/* Macintosh */
#define HX_S_IFMT   0170000
#define HX_S_IFDIR  0040000
#define HX_S_IFCHR  0020000
#define HX_S_IFIFO  0010000
#define HX_S_IFREG  0100000
#endif


/****************************************************************************
 *
 *  Interface:
 *
 *      IHXFileObject
 *
 *  Purpose:
 *
 *      Object that exports file control API
 *
 *  IID_IHXFileObject:
 *
 *      {00000200-0901-11d1-8B06-00A024406D59}
 *
 */
DEFINE_GUID(IID_IHXFileObject, 0x00000200, 0x901, 0x11d1, 0x8b, 0x6, 0x0,
                        0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXFileObject

DECLARE_INTERFACE_(IHXFileObject, IUnknown)
{
    /*
     *  IUnknown methods
     */
    STDMETHOD(QueryInterface)   (THIS_
                                REFIID riid,
                                void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)  (THIS) PURE;

    STDMETHOD_(ULONG32,Release) (THIS) PURE;

    /*
     *  IHXFileObject methods
     */

    /************************************************************************
     *  Method:
     *      IHXFileObject::Init
     *  Purpose:
     *      Associates a file object with the file response object it should
     *      notify of operation completness. This method should also check
     *      for validity of the object (for example by opening it if it is
     *      a local file).
     */
    STDMETHOD(Init)     (THIS_
                        ULONG32             /*IN*/  ulFlags,
                        IHXFileResponse*   /*IN*/  pFileResponse) PURE;

    /************************************************************************
     *  Method:
     *      IHXFileObject::GetFilename
     *  Purpose:
     *      Returns the filename (without any path information) associated
     *      with a file object.
     *
     *      Note: The returned pointer's lifetime expires as soon as the
     *      caller returns from a function which was called from the RMA
     *      core (i.e. when you return control to the RMA core)
     *
     */
    STDMETHOD(GetFilename)      (THIS_
                                REF(const char*)    /*OUT*/  pFilename) PURE;

    /************************************************************************
     *  Method:
     *      IHXFileObject::Close
     *  Purpose:
     *      Closes the file resource and releases all resources associated
     *      with the object.
     */
    STDMETHOD(Close)    (THIS) PURE;

    /************************************************************************
     *  Method:
     *      IHXFileObject::Read
     *  Purpose:
     *      Reads a buffer of data of the specified length from the file
     *      and asynchronously returns it to the caller via the
     *      IHXFileResponse interface passed in to Init.
     */
    STDMETHOD(Read)     (THIS_
                        ULONG32 ulCount) PURE;

    /************************************************************************
     *  Method:
     *      IHXFileObject::Write
     *  Purpose:
     *      Writes a buffer of data to the file and asynchronously notifies
     *      the caller via the IHXFileResponse interface passed in to Init,
     *      of the completeness of the operation.
     */
    STDMETHOD(Write)    (THIS_
                        IHXBuffer* pBuffer) PURE;

    /************************************************************************
     *  Method:
     *      IHXFileObject::Seek
     *  Purpose:
     *      Seeks to an offset in the file and asynchronously notifies
     *      the caller via the IHXFileResponse interface passed in to Init,
     *      of the completeness of the operation.
     *      If the bRelative flag is TRUE, it is a relative seek; else
     *      an absolute seek.
     */
    STDMETHOD(Seek)     (THIS_
                        ULONG32 ulOffset,
                        HXBOOL    bRelative) PURE;

    /************************************************************************
     *  Method:
     *      IHXFileObject::Advise
     *  Purpose:
     *      To pass information to the File Object advising it about usage
     *      heuristics.
     */
    STDMETHOD(Advise)   (THIS_
                        ULONG32 ulInfo) PURE;
};


/****************************************************************************
 *
 *  Interface:
 *
 *  IHXFileObjectExt
 *
 *  Purpose:
 *
 *  Object that exports file control API
 *
 *  IID_IHXFileObjectExt:
 *
 *  {96DD5EB5-7EFD-4084-95CD-4D192A9036AF}
 *
 */
 DEFINE_GUID(IID_IHXFileObjectExt, 0x96dd5eb5, 0x7efd, 0x4084, 0x95, 0xcd, 0x4d,
                       0x19, 0x2a, 0x90, 0x36, 0xaf);

#undef  INTERFACE
#define INTERFACE   IHXFileObjectExt

DECLARE_INTERFACE_(IHXFileObjectExt, IUnknown)
{
    /*
     *  IUnknown methods
     */
    STDMETHOD(QueryInterface)   (THIS_
                REFIID riid,
                void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)  (THIS) PURE;

    STDMETHOD_(ULONG32,Release) (THIS) PURE;

    /************************************************************************
     * Method:
     *     IHXFileObjectExt::GetFullFilename
     * Purpose:
     *     Returns the filename, with path information, associated
     *     with a file object.
     *
     *     Note: The returned pointer's lifetime expires as soon as the
     *     caller returns from a function which was called from the RMA
     *     core (i.e. when you return control to the RMA core)
     *
     */
    STDMETHOD(GetFullFilename) (THIS_
                               REF(IHXBuffer*)  /*OUT*/  pFullFilename) PURE;
};


/****************************************************************************
 *
 *  Interface:
 *
 *      IHXFileResponse
 *
 *  Purpose:
 *
 *      Object that exports file response API
 *
 *  IID_IHXFileResponse:
 *
 *      {00000201-0901-11d1-8B06-00A024406D59}
 *
 */
DEFINE_GUID(IID_IHXFileResponse, 0x00000201, 0x901, 0x11d1, 0x8b, 0x6, 0x0,
                        0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXFileResponse

DECLARE_INTERFACE_(IHXFileResponse, IUnknown)
{
    /*
     *  IUnknown methods
     */
    STDMETHOD(QueryInterface)           (THIS_
                                        REFIID riid,
                                        void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)          (THIS) PURE;

    STDMETHOD_(ULONG32,Release)         (THIS) PURE;

    /*
     *  IHXFileResponse methods
     */

    /************************************************************************
     *  Method:
     *      IHXFileResponse::InitDone
     *  Purpose:
     *      Notification interface provided by users of the IHXFileObject
     *      interface. This method is called by the IHXFileObject when the
     *      initialization of the file is complete. If the file is not valid
     *      for the file system, the status HXR_FAILED should be
     *      returned.
     */
    STDMETHOD(InitDone)                 (THIS_
                                        HX_RESULT           status) PURE;

    /************************************************************************
     *  Method:
     *      IHXFileResponse::CloseDone
     *  Purpose:
     *      Notification interface provided by users of the IHXFileObject
     *      interface. This method is called by the IHXFileObject when the
     *      close of the file is complete.
     */
    STDMETHOD(CloseDone)                (THIS_
                                        HX_RESULT           status) PURE;

    /************************************************************************
     *  Method:
     *      IHXFileResponse::ReadDone
     *  Purpose:
     *      Notification interface provided by users of the IHXFileObject
     *      interface. This method is called by the IHXFileObject when the
     *      last read from the file is complete and a buffer is available.
     */
    STDMETHOD(ReadDone)                 (THIS_
                                        HX_RESULT           status,
                                        IHXBuffer*          pBuffer) PURE;

    /************************************************************************
     *  Method:
     *      IHXFileResponse::WriteDone
     *  Purpose:
     *      Notification interface provided by users of the IHXFileObject
     *      interface. This method is called by the IHXFileObject when the
     *      last write to the file is complete.
     */
    STDMETHOD(WriteDone)                (THIS_
                                        HX_RESULT           status) PURE;

    /************************************************************************
     *  Method:
     *      IHXFileResponse::SeekDone
     *  Purpose:
     *      Notification interface provided by users of the IHXFileObject
     *      interface. This method is called by the IHXFileObject when the
     *      last seek in the file is complete.
     */
    STDMETHOD(SeekDone)                 (THIS_
                                        HX_RESULT           status) PURE;
};

/****************************************************************************
 *
 *  Interface:
 *
 *      IHXAdvise
 *
 *  Purpose:
 *
 *      Allow IHXFileObject to query its IHXFileResponse interface
 *
 *  IID_IHXAdvise:
 *
 *      {43C3A3B8-8F76-4394-A4F8-07AA9091A0CA}
 *
 */
DEFINE_GUID(IID_IHXAdvise, 0x43c3a3b8, 0x8f76, 0x4394, 0xa4, 0xf8, 0x7,
                        0xaa, 0x90, 0x91, 0xa0, 0xca);

#undef  INTERFACE
#define INTERFACE   IHXAdvise

DECLARE_INTERFACE_(IHXAdvise, IUnknown)
{
    /*
     *  IUnknown methods
     */
    STDMETHOD(QueryInterface)           (THIS_
                                        REFIID riid,
                                        void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)          (THIS) PURE;

    STDMETHOD_(ULONG32,Release)         (THIS) PURE;

    /*
     *  IHXAdvise methods
     */

    /************************************************************************
     *  Method:
     *      IHXAdvise::Advise
     *  Purpose:
     *      Allows IHXFileObject to query its IHXFileResponse about
     *      usage heuristics. It should pass HX_FILERESPONSEADVISE_xxx
     *      flags into this method.
     */
    STDMETHOD(Advise)   (THIS_ ULONG32 ulInfo) PURE;
};


/****************************************************************************
 *
 *  Interface:
 *
 *      IHXFileSystemObject
 *
 *  Purpose:
 *
 *      Object that allows a Controller to communicate with a specific
 *      File System plug-in session
 *
 *  IID_IHXFileSystemObject:
 *
 *      {00000202-0901-11d1-8B06-00A024406D59}
 *
 */
DEFINE_GUID(IID_IHXFileSystemObject, 0x00000202, 0x901, 0x11d1, 0x8b, 0x6, 0x0,
                        0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXFileSystemObject

DECLARE_INTERFACE_(IHXFileSystemObject, IUnknown)
{
    /*
     *  IUnknown methods
     */
    STDMETHOD(QueryInterface)   (THIS_
                                REFIID riid,
                                void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)  (THIS) PURE;

    STDMETHOD_(ULONG32,Release) (THIS) PURE;

    /*
     *  IHXFileSystemObject methods
     */

    /************************************************************************
     *  Method:
     *      IHXFileSystemObject::GetFileSystemInfo
     *  Purpose:
     *      Returns information vital to the instantiation of file system
     *      plugin.
     *
     *      pShortName should be a short, human readable name in the form
     *      of "company-fsname".  For example: pShortName = "pn-local".
     */
    STDMETHOD(GetFileSystemInfo)    (THIS_
                                    REF(const char*) /*OUT*/ pShortName,
                                    REF(const char*) /*OUT*/ pProtocol) PURE;

    STDMETHOD(InitFileSystem)   (THIS_
                                IHXValues* pOptions) PURE;

    STDMETHOD(CreateFile)       (THIS_
                                IUnknown**    /*OUT*/   ppFileObject) PURE;

    /*
     * The following method is deprecated and should return HXR_NOTIMPL
     */

    STDMETHOD(CreateDir)        (THIS_
                                IUnknown**   /*OUT*/ ppDirObject) PURE;

};


/****************************************************************************
 *
 *  Interface:
 *
 *      IHXFileStat
 *
 *  Purpose:
 *
 *      Gets information about a specific File object
 *
 *  IID_IHXFileStat:
 *
 *      {00000205-0901-11d1-8B06-00A024406D59}
 *
 */
DEFINE_GUID(IID_IHXFileStat, 0x00000205, 0x901, 0x11d1, 0x8b, 0x6, 0x0,
                        0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXFileStat

DECLARE_INTERFACE_(IHXFileStat, IUnknown)
{
    /*
     *  IUnknown methods
     */
    STDMETHOD(QueryInterface)   (THIS_
                                REFIID riid,
                                void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)  (THIS) PURE;

    STDMETHOD_(ULONG32,Release) (THIS) PURE;

    /*
     *  IHXFileStat methods
     */

    STDMETHOD(Stat)             (THIS_
                                IHXFileStatResponse* pFileStatResponse
                                ) PURE;
};


/****************************************************************************
 *
 *  Interface:
 *
 *      IHXFileStatResponse
 *
 *  Purpose:
 *
 *      Returns information about a specific File object
 *
 *  IID_IHXFileStatResponse:
 *
 *      {00000206-0901-11d1-8B06-00A024406D59}
 *
 */
DEFINE_GUID(IID_IHXFileStatResponse, 0x00000206, 0x901, 0x11d1, 0x8b, 0x6, 0x0,
                        0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXFileStatResponse

DECLARE_INTERFACE_(IHXFileStatResponse, IUnknown)
{
    /*
     *  IUnknown methods
     */
    STDMETHOD(QueryInterface)   (THIS_
                                REFIID riid,
                                void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)  (THIS) PURE;

    STDMETHOD_(ULONG32,Release) (THIS) PURE;

    /*
     *  IHXFileStat methods
     */

    STDMETHOD(StatDone)         (THIS_
                                 HX_RESULT status,
                                 UINT32 ulSize,
                                 UINT32 ulCreationTime,
                                 UINT32 ulAccessTime,
                                 UINT32 ulModificationTime,
                                 UINT32 ulMode) PURE;
};

/****************************************************************************
 *
 *  Interface:
 *
 *      IHXFileSystemManager
 *
 *  Purpose:
 *
 *      Gives out File Objects based on URLs
 *
 *  IID_IHXFileSystemManager:
 *
 *      {00000207-0901-11d1-8B06-00A024406D59}
 *
 */
DEFINE_GUID(IID_IHXFileSystemManager, 0x00000207, 0x901, 0x11d1, 0x8b, 0x6, 0x0,
                        0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXFileSystemManager

#define CLSID_IHXFileSystemManager IID_IHXFileSystemManager

DECLARE_INTERFACE_(IHXFileSystemManager, IUnknown)
{
    /*
     *  IUnknown methods
     */
    STDMETHOD(QueryInterface)   (THIS_
                                REFIID riid,
                                void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)  (THIS) PURE;

    STDMETHOD_(ULONG32,Release) (THIS) PURE;

    /*
     *  IHXFileSystemManager methods
     */

    STDMETHOD(Init) (THIS_
                    IHXFileSystemManagerResponse* /*IN*/  pFileManagerResponse
                    ) PURE;

    /* GetFileObject attempts to locate an existing file via the DoesExist
     * method in each file system's objects, and returns that object through
     * FSManagerResponse->FileObjectReady
     */
    STDMETHOD(GetFileObject)    (THIS_
                                 IHXRequest* pRequest,
                                 IHXAuthenticator* pAuthenticator) PURE;

    /* GetNewFileObject is similar to GetFileObject except that no DoesExist
     * checks are done.  The first file system that matches the mount point
     * or protocol for the path in the request object creates the file
     * which is then returned through FileObjectReady.  This is especially
     * useful for those who wish to open a brand new file for writing.
     */
    STDMETHOD(GetNewFileObject) (THIS_
                                 IHXRequest* pRequest,
                                 IHXAuthenticator* pAuthenticator) PURE;

    STDMETHOD(GetRelativeFileObject) (THIS_
                                      IUnknown* pOriginalObject,
                                      const char* pPath) PURE;

    /*
     * The following method is deprecated and should return HXR_NOTIMPL
     */

    STDMETHOD(GetDirObjectFromURL)      (THIS_
                                        const char* pURL) PURE;
};


/****************************************************************************
 *
 *  Interface:
 *
 *      IHXFileSystemManagerResponse
 *
 *  Purpose:
 *
 *      Gives out File System objects based on URLs
 *
 *  IID_IHXFileSystemManagerResponse:
 *
 *      {00000208-0901-11d1-8B06-00A024406D59}
 *
 */
DEFINE_GUID(IID_IHXFileSystemManagerResponse, 0x00000208, 0x901, 0x11d1, 0x8b, 0x6, 0x0,
                        0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXFileSystemManagerResponse

DECLARE_INTERFACE_(IHXFileSystemManagerResponse, IUnknown)
{
    /*
     *  IUnknown methods
     */
    STDMETHOD(QueryInterface)   (THIS_
                                REFIID riid,
                                void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)  (THIS) PURE;

    STDMETHOD_(ULONG32,Release) (THIS) PURE;

    /*
     *  IHXFileSystemManagerResponse methods
     */

    /************************************************************************
     *  Method:
     *  IHXFileSystemManagerResponse::InitDone
     *  Purpose:
     */
    STDMETHOD(InitDone)     (THIS_
                            HX_RESULT       status) PURE;

    STDMETHOD(FileObjectReady)  (THIS_
                                HX_RESULT status,
                                IUnknown* pObject) PURE;

    /*
     * The following method is deprecated and should return HXR_NOTIMPL
     */

    STDMETHOD(DirObjectReady)   (THIS_
                                HX_RESULT status,
                                IUnknown* pDirObject) PURE;
};


/****************************************************************************
 *
 *  Interface:
 *
 *      IHXFileExists
 *
 *  Purpose:
 *
 *      Checks for the existense of a file.  Must be implemented.
 *
 *  IID_IHXFileExists:
 *
 *      {00000209-0901-11d1-8B06-00A024406D59}
 *
 */
DEFINE_GUID(IID_IHXFileExists, 0x00000209, 0x901, 0x11d1, 0x8b, 0x6, 0x0,
                        0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXFileExists

DECLARE_INTERFACE_(IHXFileExists, IUnknown)
{
    /*
     *  IUnknown methods
     */
    STDMETHOD(QueryInterface)   (THIS_
                                REFIID riid,
                                void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)  (THIS) PURE;

    STDMETHOD_(ULONG32,Release) (THIS) PURE;

    /*
     *  IHXFileExists methods
     */

    /************************************************************************
     *  Method:
     *      IHXFileExists::DoesExist
     *  Purpose:
     */
    STDMETHOD(DoesExist) (THIS_
                        const char*             /*IN*/  pPath,
                        IHXFileExistsResponse* /*IN*/  pFileResponse) PURE;
};


/****************************************************************************
 *
 *  Interface:
 *
 *      IHXFileExistsResponse
 *
 *  Purpose:
 *
 *      Response interface for IHXFileExists.  Must be implemented.
 *
 *  IID_IHXFileExistsResponse:
 *
 *      {0000020A-0901-11d1-8B06-00A024406D59}
 *
 */
DEFINE_GUID(IID_IHXFileExistsResponse, 0x0000020a, 0x901, 0x11d1, 0x8b, 0x6, 0x0,
                        0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXFileExists

DECLARE_INTERFACE_(IHXFileExistsResponse, IUnknown)
{
    /*
     *  IUnknown methods
     */
    STDMETHOD(QueryInterface)   (THIS_
                                REFIID riid,
                                void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)  (THIS) PURE;

    STDMETHOD_(ULONG32,Release) (THIS) PURE;

    /*
     *  IHXFileExistsResponse methods
     */

    STDMETHOD(DoesExistDone) (THIS_
                              HXBOOL      bExist) PURE;
};


/****************************************************************************
 *
 *  Interface:
 *
 *      IHXFileMimeMapper
 *
 *  Purpose:
 *
 *      Allows you to specify a mime type for a specific file.
 *      Optional interface.
 *
 *  IID_IHXFileMimeMapper:
 *
 *      {0000020B-0901-11d1-8B06-00A024406D59}
 *
 */
DEFINE_GUID(IID_IHXFileMimeMapper, 0x0000020b, 0x901, 0x11d1, 0x8b, 0x6, 0x0,
                        0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXFileMimeMapper

DECLARE_INTERFACE_(IHXFileMimeMapper, IUnknown)
{
    /*
     *  IUnknown methods
     */
    STDMETHOD(QueryInterface)   (THIS_
                                REFIID riid,
                                void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)  (THIS) PURE;

    STDMETHOD_(ULONG32,Release) (THIS) PURE;

    /*
     *  IHXFileMimeMapper methods
     */

    /************************************************************************
     *  Method:
     *      IHXFileMimeMapper::FindMimeType
     *  Purpose:
     */
    STDMETHOD(FindMimeType) (THIS_
                            const char*             /*IN*/  pURL,
                            IHXFileMimeMapperResponse* /*IN*/  pMimeMapperResponse
                            ) PURE;
};


/****************************************************************************
 *
 *  Interface:
 *
 *      IHXFileMimeMapperResponse
 *
 *  Purpose:
 *
 *      Response interface for IHXFileMimeMapper.
 *      Optional interface.
 *
 *  IID_IHXFileMimeMapperResponse:
 *
 *      {0000020C-0901-11d1-8B06-00A024406D59}
 *
 */
DEFINE_GUID(IID_IHXFileMimeMapperResponse, 0x0000020c, 0x901, 0x11d1, 0x8b, 0x6, 0x0,
                        0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXFileMimeMapperResponse

DECLARE_INTERFACE_(IHXFileMimeMapperResponse, IUnknown)
{
    /*
     *  IUnknown methods
     */
    STDMETHOD(QueryInterface)   (THIS_
                                REFIID riid,
                                void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)  (THIS) PURE;

    STDMETHOD_(ULONG32,Release) (THIS) PURE;

    /*
     *  IHXFileMimeMapperResponse methods
     */

    /************************************************************************
     *  Method:
     *      IHXFileMimeMapperResponse::MimeTypeFound
     *  Purpose:
     *      Notification interface provided by users of the IHXFileMimeMapper
     *      interface. This method is called by the IHXFileObject when the
     *      initialization of the file is complete, and the Mime type is
     *      available for the request file. If the file is not valid for the
     *      file system, the status HXR_FAILED should be returned,
     *      with a mime type of NULL. If the file is valid but the mime type
     *      is unknown, then the status HXR_OK should be returned with
     *      a mime type of NULL.
     *
     */
    STDMETHOD(MimeTypeFound) (THIS_
                              HX_RESULT status,
                              const char* pMimeType) PURE;
};


/****************************************************************************
 *
 *  Interface:
 *
 *      IHXBroadcastMapper
 *
 *  Purpose:
 *
 *      Associates a file with a broadcast format plugin.
 *      Implementation only required by broadcast plugin file systems.
 *
 *  IID_IHXBroadcastMapper:
 *
 *      {0000020D-0901-11d1-8B06-00A024406D59}
 *
 */
DEFINE_GUID(IID_IHXBroadcastMapper, 0x0000020d, 0x901, 0x11d1, 0x8b, 0x6, 0x0,
                        0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXBroadcastMapper

DECLARE_INTERFACE_(IHXBroadcastMapper, IUnknown)
{
    /*
     *  IUnknown methods
     */
    STDMETHOD(QueryInterface)   (THIS_
                                REFIID riid,
                                void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)  (THIS) PURE;

    STDMETHOD_(ULONG32,Release) (THIS) PURE;

    /*
     *  IHXBroadcastMapper methods
     */

    /************************************************************************
     *  Method:
     *      IHXBroadcastMapper::FindBroadcastType
     *  Purpose:
     */
    STDMETHOD(FindBroadcastType) (THIS_
                                const char*                  /*IN*/  pURL,
                                IHXBroadcastMapperResponse* /*IN*/  pBroadcastMapperResponse) PURE;
};


/****************************************************************************
 *
 *  Interface:
 *
 *      IHXBroadcastMapperResponse
 *
 *  Purpose:
 *
 *      Response interface for IHXBroadcastMapper.
 *      Implementation only required by broadcast plugin file systems.
 *
 *  IID_IHXBroadcastMapperResponse:
 *
 *      {0000020E-0901-11d1-8B06-00A024406D59}
 *
 */
DEFINE_GUID(IID_IHXBroadcastMapperResponse, 0x0000020e, 0x901, 0x11d1, 0x8b, 0x6, 0x0,
                        0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXBroadcastMapperResponse

DECLARE_INTERFACE_(IHXBroadcastMapperResponse, IUnknown)
{
    /*
     *  IUnknown methods
     */
    STDMETHOD(QueryInterface)   (THIS_
                                REFIID riid,
                                void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)  (THIS) PURE;

    STDMETHOD_(ULONG32,Release) (THIS) PURE;

    /*
     *  IHXBroadcastMapperResponse methods
     */

    /************************************************************************
     *  Method:
     *      IHXBroadcastMapperResponse::BroadcastTypeFound
     *  Purpose:
     *      Notification interface provided by users of the IHXBroadcastMapper
     *      interface. This method is called by the File Object when the
     *      initialization of the file is complete, and the broadcast type is
     *      available for the request file. If the file is not valid for the
     *      file system, the status HXR_FAILED should be returned,
     *      with the broadcast type set to NULL.
     *
     */
    STDMETHOD(BroadcastTypeFound) (THIS_
                                  HX_RESULT     status,
                                  const char* pBroadcastType) PURE;
};


/****************************************************************************
 *
 *  Interface:
 *
 *      IHXGetFileFromSamePool
 *
 *  Purpose:
 *
 *      Gives out File Objects based on filenames and relative "paths"
 *
 *  IID_IHXGetFileFromSamePool:
 *
 *      {0000020f-0901-11d1-8B06-00A024406D59}
 *
 */
DEFINE_GUID(IID_IHXGetFileFromSamePool, 0x0000020f, 0x901, 0x11d1, 0x8b, 0x6, 0x0,
                        0xa0, 0x24, 0x40, 0x6d, 0x59);
#undef  INTERFACE
#define INTERFACE   IHXGetFileFromSamePool

#define CLSID_IHXGetFileFromSamePool IID_IHXGetFileFromSamePool

DECLARE_INTERFACE_(IHXGetFileFromSamePool, IUnknown)
{
    /*
     *  IUnknown methods
     */
    STDMETHOD(QueryInterface)   (THIS_
                                REFIID riid,
                                void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)  (THIS) PURE;

    STDMETHOD_(ULONG32,Release) (THIS) PURE;


    /*
     * IHXGetFileFromSamePool method
     */
    /************************************************************************
     *  Method:
     *      IHXGetFileFromSamePool::GetFileObjectFromPool
     *  Purpose:
     *      To get another FileObject from the same pool.
     */
    STDMETHOD(GetFileObjectFromPool)    (THIS_
                                         IHXGetFileFromSamePoolResponse*) PURE;
};


/****************************************************************************
 *
 *  Interface:
 *
 *      IHXGetFileFromSamePoolResponse
 *
 *  Purpose:
 *
 *      Gives out File Objects based on filenames and relative "paths"
 *
 *  IID_IHXGetFileFromSamePoolResponse:
 *
 *      {00000210-0901-11d1-8B06-00A024406D59}
 *
 */
DEFINE_GUID(IID_IHXGetFileFromSamePoolResponse, 0x00000210, 0x901, 0x11d1, 0x8b, 0x6, 0x0,
                        0xa0, 0x24, 0x40, 0x6d, 0x59);
#undef  INTERFACE
#define INTERFACE   IHXGetFileFromSamePoolResponse

#define CLSID_IHXGetFileFromSamePoolResponse IID_IHXGetFileFromSamePoolResponse

DECLARE_INTERFACE_(IHXGetFileFromSamePoolResponse, IUnknown)
{
    /*
     *  IUnknown methods
     */
    STDMETHOD(QueryInterface)   (THIS_
                                REFIID riid,
                                void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)  (THIS) PURE;

    STDMETHOD_(ULONG32,Release) (THIS) PURE;

    /*
     * IHXGetFileFromSamePoolResponse method
     */
    /************************************************************************
     *  Method:
     *      IHXGetFileFromSamePoolResponse::FileObjectReady
     *  Purpose:
     *      To return another FileObject from the same pool.
     */
    STDMETHOD(FileObjectReady) (THIS_
                                HX_RESULT status,
                                IUnknown* ppUnknown) PURE;
};


/****************************************************************************
 *
 *  Interface:
 *
 *      IHXFileAuthenticator
 *
 *  Purpose:
 *
 *      Set and Get a file object's authenticator object.
 *
 *  IID_IHXFileAuthenticator:
 *
 *      {00000211-0901-11d1-8B06-00A024406D59}
 *
 */
DEFINE_GUID(IID_IHXFileAuthenticator, 0x00000211, 0x901, 0x11d1, 0x8b, 0x6, 0x0,
                        0xa0, 0x24, 0x40, 0x6d, 0x59);
#undef  INTERFACE
#define INTERFACE   IHXFileAuthenticator

#define CLSID_IHXFileAuthenticator IID_IHXFileAuthenticator

DECLARE_INTERFACE_(IHXFileAuthenticator, IUnknown)
{
    /*
     *  IUnknown methods
     */
    STDMETHOD(QueryInterface)   (THIS_
                                REFIID riid,
                                void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)  (THIS) PURE;

    STDMETHOD_(ULONG32,Release) (THIS) PURE;

    /*
     * IHXFileAuthenticator methods
     */
    STDMETHOD(SetAuthenticator) (THIS_
                                 IHXAuthenticator* pAuthenticator) PURE;

    STDMETHOD(GetAuthenticator) (THIS_
                                 REF(IHXAuthenticator*) pAuthenticator) PURE;
};


/****************************************************************************
 *
 *  Interface:
 *
 *      IHXRequestHandler
 *
 *  Purpose:
 *
 *      Object to manage IHXRequest objects
 *
 *  IID_IHXRequestHandler:
 *
 *      {00000212-0901-11d1-8B06-00A024406D59}
 *
 */
DEFINE_GUID(IID_IHXRequestHandler, 0x00000212, 0x901, 0x11d1, 0x8b, 0x6, 0x0,
                        0xa0, 0x24, 0x40, 0x6d, 0x59);
#undef  INTERFACE
#define INTERFACE   IHXRequestHandler

#define CLSID_IHXRequestHandler IID_IHXRequestHandler

DECLARE_INTERFACE_(IHXRequestHandler, IUnknown)
{
    /*
     *  IUnknown methods
     */

    STDMETHOD(QueryInterface)   (THIS_
                                REFIID riid,
                                void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)  (THIS) PURE;

    STDMETHOD_(ULONG32,Release) (THIS) PURE;

    /************************************************************************
     *  Method:
     *      IHXRequestHandler::SetRequest
     *  Purpose:
     *      Associates an IHXRequest with an object
     */
    STDMETHOD(SetRequest)   (THIS_
                            IHXRequest*        /*IN*/  pRequest) PURE;

    /************************************************************************
     *  Method:
     *      IHXRequestHandler::GetRequest
     *  Purpose:
     *      Gets the IHXRequest object associated with an object
     */
    STDMETHOD(GetRequest)   (THIS_
                            REF(IHXRequest*)        /*OUT*/  pRequest) PURE;

};


/****************************************************************************
 *
 *  Interface:
 *
 *      IHXRequestContext
 *
 *  Purpose:
 *
 *      Object to manage the context of the Request
 *
 *  IID_IHXRequestContext:
 *
 *      {00000217-0901-11d1-8B06-00A024406D59}
 *
 */
DEFINE_GUID(IID_IHXRequestContext, 0x00000217, 0x901, 0x11d1, 0x8b, 0x6, 0x0,
                        0xa0, 0x24, 0x40, 0x6d, 0x59);
#undef  INTERFACE
#define INTERFACE   IHXRequestContext

#define CLSID_IHXRequestContext IID_IHXRequestContext

DECLARE_INTERFACE_(IHXRequestContext, IUnknown)
{
    /*
     *  IUnknown methods
     */

    STDMETHOD(QueryInterface)   (THIS_
                                REFIID riid,
                                void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)  (THIS) PURE;

    STDMETHOD_(ULONG32,Release) (THIS) PURE;

    /*
     * IHXRequestContext methods
     */

    /************************************************************************
     *  Method:
     *      IHXRequestContext::SetUserContext
     *  Purpose:
     *      Sets the Authenticated users Context.
     */
    STDMETHOD(SetUserContext)
    (
        THIS_
        IUnknown* pIUnknownNewContext
    ) PURE;

    /************************************************************************
     *  Method:
     *      IHXRequestContext::GetUserContext
     *  Purpose:
     *      Gets the Authenticated users Context.
     */
    STDMETHOD(GetUserContext)
    (
        THIS_
        REF(IUnknown*) pIUnknownCurrentContext
    ) PURE;

    /************************************************************************
     *  Method:
     *      IHXRequestContext::SetRequester
     *  Purpose:
     *      Sets the Object that made the request.
     */
    STDMETHOD(SetRequester)
    (
        THIS_
        IUnknown* pIUnknownNewRequester
    ) PURE;

    /************************************************************************
     *  Method:
     *      IHXRequestContext::GetRequester
     *  Purpose:
     *      Gets the Object that made the request.
     */
    STDMETHOD(GetRequester)
    (
        THIS_
        REF(IUnknown*) pIUnknownCurrentRequester
    ) PURE;
};

/****************************************************************************
 *
 *  Interface:
 *
 *      IHXRequest
 *
 *  Purpose:
 *
 *      Object to manage the RFC822 headers sent by the client
 *
 *  IID_IHXRequest:
 *
 *      {00000213-0901-11d1-8B06-00A024406D59}
 *
 */
DEFINE_GUID(IID_IHXRequest, 0x00000213, 0x901, 0x11d1, 0x8b, 0x6, 0x0,
                        0xa0, 0x24, 0x40, 0x6d, 0x59);
#undef  INTERFACE
#define INTERFACE   IHXRequest

#define CLSID_IHXRequest IID_IHXRequest

DECLARE_INTERFACE_(IHXRequest, IUnknown)
{
    /*
     *  IUnknown methods
     */

    STDMETHOD(QueryInterface)   (THIS_
                                REFIID riid,
                                void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)  (THIS) PURE;

    STDMETHOD_(ULONG32,Release) (THIS) PURE;

    /*
     * IHXRequest methods
     */

    /************************************************************************
     *  Method:
     *      IHXRequest::SetRequestHeaders
     *  Purpose:
     *      Sets the headers that will be sent in the RFC822 header section
     *      of the request message
     */
    STDMETHOD(SetRequestHeaders)        (THIS_
                                        IHXValues* pRequestHeaders) PURE;

    /************************************************************************
     *  Method:
     *      IHXRequest::GetRequestHeaders
     *  Purpose:
     *      Gets the headers that were sent in the RFC822 header section
     *      of the request message
     */
    STDMETHOD(GetRequestHeaders)        (THIS_
                                        REF(IHXValues*) pRequestHeaders) PURE;

    /************************************************************************
     *  Method:
     *      IHXRequest::SetResponseHeaders
     *  Purpose:
     *      Sets the headers that will be returned in the RFC822 header
     *      section of the response message
     */
    STDMETHOD(SetResponseHeaders)       (THIS_
                                        IHXValues* pResponseHeaders) PURE;

    /************************************************************************
     *  Method:
     *      IHXRequest::GetResponseHeaders
     *  Purpose:
     *      Gets the headers that were returned in the RFC822 header section
     *      of the response message
     */
    STDMETHOD(GetResponseHeaders)       (THIS_
                                        REF(IHXValues*) pResponseHeaders) PURE;

    /************************************************************************
     *  Method:
     *      IHXRequest::SetURL
     *  Purpose:
     *      Sets the fully qualified path associated with a file object.
     *      Note: On the server, this path does not include the file system
     *          mount point.
     */
    STDMETHOD(SetURL)                   (THIS_
                                        const char* pURL) PURE;

    /************************************************************************
     *  Method:
     *      IHXRequest::GetURL
     *  Purpose:
     *      Returns the fully qualified path associated with a file object.
     *      Note: On the server, this path does not include the file system
     *          mount point.
     *
     *      Note: The returned pointer's lifetime expires as soon as the
     *      caller returns from a function which was called from the RMA
     *      core (i.e. when you return control to the RMA core)
     */
    STDMETHOD(GetURL)                   (THIS_
                                        REF(const char*) pURL) PURE;
};

/****************************************************************************
 *
 *  Interface:
 *
 *      IHXFileRename
 *
 *  Purpose:
 *
 *      Interface to allow renaming of files.  Query off of the File Object.
 *      Not all filesystem plugins implement this feature.
 *
 *  IID_IHXFileRename:
 *
 *      {00000214-0901-11d1-8B06-00A024406D59}
 *
 */
DEFINE_GUID(IID_IHXFileRename, 0x00000214, 0x901, 0x11d1, 0x8b, 0x6, 0x0,
                        0xa0, 0x24, 0x40, 0x6d, 0x59);
#undef  INTERFACE
#define INTERFACE   IHXFileRename

DECLARE_INTERFACE_(IHXFileRename, IUnknown)
{
    /*
     *  IUnknown methods
     */

    STDMETHOD(QueryInterface)   (THIS_
                                REFIID riid,
                                void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)  (THIS) PURE;

    STDMETHOD_(ULONG32,Release) (THIS) PURE;

    /*
     * IHXFileRename methods
     */

    /************************************************************************
     *  Method:
     *      IHXFileRename::Rename
     *  Purpose:
     *      Renames a file to a new name.
     */
    STDMETHOD(Rename)                   (THIS_
                                        const char* pNewFileName) PURE;
};


/****************************************************************************
 *
 *  Interface:
 *
 *      IHXFileMove
 *
 *  Purpose:
 *
 *      Interface to allow removing of files.  Query off of the File Object.
 *      Not all filesystem plugins implement this feature.
 *
 *  IID_IHXFileMove:
 *
 *      {23E72FB0-DE0E-11d5-AA9A-00010251B340}
 *
 */
DEFINE_GUID(IID_IHXFileMove, 0x23e72fb0, 0xde0e, 0x11d5, 0xaa, 0x9a, 0x0,
                        0x1, 0x2, 0x51, 0xb3, 0x40);

#undef  INTERFACE
#define INTERFACE   IHXFileMove

DECLARE_INTERFACE_(IHXFileMove, IUnknown)
{
    /*
     *  IUnknown methods
     */

    STDMETHOD(QueryInterface)   (THIS_
                                REFIID riid,
                                void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)  (THIS) PURE;

    STDMETHOD_(ULONG32,Release) (THIS) PURE;

    /*
     * IHXFileMove methods
     */

    /************************************************************************
     *  Method:
     *      IHXFileMove::Move
     *  Purpose:
     *      Moves a file to a different location in the file system
     */
    STDMETHOD(Move)                     (THIS_
                                        const char* pNewFilePathName) PURE;
};


/****************************************************************************
 *
 *  Interface:
 *
 *      IHXDirHandler
 *
 *  Purpose:
 *
 *      Object that exports directory handler API
 *
 *  IID_IHXDirHandler:
 *
 *      {00000215-0901-11d1-8B06-00A024406D59}
 *
 */
DEFINE_GUID(IID_IHXDirHandler, 0x00000215, 0x901, 0x11d1, 0x8b, 0x6, 0x0, 0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXDirHandler

DECLARE_INTERFACE_(IHXDirHandler, IUnknown)
{
    /*
     *  IUnknown methods
     */
    STDMETHOD(QueryInterface)   (THIS_
                                REFIID riid,
                                void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)  (THIS) PURE;

    STDMETHOD_(ULONG32,Release) (THIS) PURE;

    /*
     *  IHXDirHandler methods
     */

    /************************************************************************
     *  Method:
     *      IHXDirHandler::InitDirHandler
     *  Purpose:
     *      Associates a directory handler with the directory handler
     *      response, it should notify of operation completness.
     */
    STDMETHOD(InitDirHandler)   (THIS_
                                IHXDirHandlerResponse*    /*IN*/  pDirResponse) PURE;

    /************************************************************************
     *  Method:
     *      IHXDirHandler::CloseDirHandler
     *  Purpose:
     *      Closes the directory handler resource and releases all resources
     *      associated with the object.
     */
    STDMETHOD(CloseDirHandler)  (THIS) PURE;

    /************************************************************************
     *  Method:
     *      IHXDirHandler::MakeDir
     *  Purpose:
     *      Create the directory
     */
    STDMETHOD(MakeDir)  (THIS) PURE;

    /************************************************************************
     *  Method:
     *      IHXDirHandler::ReadDir
     *  Purpose:
     *      Get a dump of the directory
     */
    STDMETHOD(ReadDir)  (THIS) PURE;
};


/****************************************************************************
 *
 *  Interface:
 *
 *      IHXDirHandlerResponse
 *
 *  Purpose:
 *
 *      Object that exports the directory handler response API
 *
 *  IID_IHXDirHandlerResponse:
 *
 *      {00000216-0901-11d1-8B06-00A024406D59}
 *
 */
DEFINE_GUID(IID_IHXDirHandlerResponse, 0x00000216, 0x901, 0x11d1, 0x8b, 0x6, 0x0, 0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXDirHandlerResponse

DECLARE_INTERFACE_(IHXDirHandlerResponse, IUnknown)
{
    /*
     *  IUnknown methods
     */
    STDMETHOD(QueryInterface)           (THIS_
                                        REFIID riid,
                                        void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)          (THIS) PURE;

    STDMETHOD_(ULONG32,Release)         (THIS) PURE;

    /*
     *  IHXDirHandlerResponse methods
     */

    /************************************************************************
     *  Method:
     *      IHXDirHandlerResponse::InitDirHandlerDone
     *  Purpose:
     *      Notification interface provided by users of the IHXDirHandler
     *      interface. This method is called by the IHXDirHandler when the
     *      initialization of the object is complete.
     */
    STDMETHOD(InitDirHandlerDone)       (THIS_
                                        HX_RESULT           status) PURE;

    /************************************************************************
     *  Method:
     *      IHXDirHandlerResponse::CloseDirHandlerDone
     *  Purpose:
     *      Notification interface provided by users of the IHXDirHandler
     *      interface. This method is called by the IHXDirHandler when the
     *      close of the directory is complete.
     */
    STDMETHOD(CloseDirHandlerDone)      (THIS_
                                        HX_RESULT           status) PURE;

    /************************************************************************
     *  Method:
     *      IHXDirHandler::MakeDirDone
     *  Purpose:
     *      Notification interface provided by users of the IHXDirHandler
     *      interface. This method is called by the IHXDirHandler when the
     *      attempt to create the directory is complete.
     */
    STDMETHOD(MakeDirDone)              (THIS_
                                        HX_RESULT           status) PURE;

    /************************************************************************
     *  Method:
     *      IHXDirHandler::ReadDirDone
     *  Purpose:
     *      Notification interface provided by users of the IHXDirHandler
     *      interface. This method is called by the IHXDirHandler when the
     *      read from the directory is complete and a buffer is available.
     */
    STDMETHOD(ReadDirDone)              (THIS_
                                        HX_RESULT           status,
                                        IHXBuffer*          pBuffer) PURE;
};


// $Private:
/****************************************************************************
 *
 *  Interface:
 *
 *      IHXGetRecursionLevel
 *
 *  Purpose:
 *
 *      Set's Recursion Level
 *
 *  IID_GetRecursionLevel:
 *
 *      {00000218-0901-11d1-8B06-00A024406D59}
 *
 */
DEFINE_GUID(IID_IHXGetRecursionLevel, 0x00000218, 0x901, 0x11d1, 0x8b, 0x6, 0x0, 0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXGetRecursionLevel

DECLARE_INTERFACE_(IHXGetRecursionLevel, IUnknown)
{
    /*
     *  IUnknown methods
     */
    STDMETHOD(QueryInterface)           (THIS_
                                        REFIID riid,
                                        void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)          (THIS) PURE;

    STDMETHOD_(ULONG32,Release)         (THIS) PURE;

    /*
     *  IHXGetRecursionLevel methods
     */

    STDMETHOD_(UINT32, GetRecursionLevel)       (THIS) PURE;
};

/****************************************************************************
 *
 *  Interface:
 *
 *      IHXFileRestrictor
 *
 *  Purpose:
 *
 *      Allows restrictions on per file basis.  This will only get called for
 *  HTTP and it will only send in the localport.  Maybe some day.....
 *
 *  IID_IHXFileRestrictor:
 *
 *      {00000219-0901-11d1-8B06-00A024406D59}
 *
 */
DEFINE_GUID(IID_IHXFileRestrictor, 0x00000219, 0x901, 0x11d1, 0x8b, 0x6, 0x0, 0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXFileRestrictor

DECLARE_INTERFACE_(IHXFileRestrictor, IUnknown)
{
    /* IUnknown methods */
    STDMETHOD(QueryInterface)       (THIS_ REFIID riid, void** ppvObj) PURE;
    STDMETHOD_(ULONG32,AddRef)      (THIS) PURE;
    STDMETHOD_(ULONG32,Release)     (THIS) PURE;

    /* IHXFileRestrictor methods */
    STDMETHOD_(HXBOOL, IsAllowed)     (THIS_ const char* url,
                                           const char* pLocalAddr,
                                           const char* pLocalPort,
                                           const char* pPeerAddr,
                                           const char* pPeerPort) PURE;
};

// $EndPrivate.

/****************************************************************************
 *
 *  Interface:
 *
 *      IHXFileRemove
 *
 *  Purpose:
 *
 *      Interface to allow removing of files.  Query off of the File Object.
 *      Not all filesystem plugins implement this feature.
 *
 *  IID_IHXFileRemove:
 *
 *      {0000021A-0901-11d1-8B06-00A024406D59}
 *
 */
DEFINE_GUID(IID_IHXFileRemove, 0x0000021A, 0x901, 0x11d1, 0x8b, 0x6, 0x0,
                        0xa0, 0x24, 0x40, 0x6d, 0x59);
#undef  INTERFACE
#define INTERFACE   IHXFileRemove

DECLARE_INTERFACE_(IHXFileRemove, IUnknown)
{
    /*
     *  IUnknown methods
     */

    STDMETHOD(QueryInterface)   (THIS_
                                REFIID riid,
                                void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)  (THIS) PURE;

    STDMETHOD_(ULONG32,Release) (THIS) PURE;

    /*
     * IHXFileRemove methods
     */

    /************************************************************************
     *  Method:
     *      IHXFileRemove::Remove
     *  Purpose:
     *      Removes a file from the file system.
     */
    STDMETHOD(Remove)                   (THIS) PURE;
};


// $Private:
/****************************************************************************
 *
 *  Interface:
 *
 *      IHXFastFileFactory
 *
 *  Purpose:
 *
 *      Interface to allow wrapping file objects with a buffer and block-sharing
 *      sceme.  Query off the server class factory.
 *
 *  IID_IHXFastFileFactory:
 *
 *      {0000021C-0901-11d1-8B06-00A024406D59}
 *
 */
DEFINE_GUID(IID_IHXFastFileFactory, 0x0000021C, 0x901, 0x11d1, 0x8b, 0x6, 0x0,
                        0xa0, 0x24, 0x40, 0x6d, 0x59);

#define CLSID_IHXFastFileFactory IID_IHXFastFileFactory

#undef  INTERFACE
#define INTERFACE   IHXFastFileFactory

DECLARE_INTERFACE_(IHXFastFileFactory, IUnknown)
{
    /*
     *  IUnknown methods
     */

    STDMETHOD(QueryInterface)   (THIS_
                                REFIID riid,
                                void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)  (THIS) PURE;

    STDMETHOD_(ULONG32,Release) (THIS) PURE;

    /*
     * IHXFastFileFactory methods
     */

    /************************************************************************
     *  Method:
     *      IHXFastFileFactory::Wrap
     *  Purpose:
     *      Return an instantiated wrapper around an existing (but
     *      uninitialized) file object.
     *
     */
    STDMETHOD(Wrap)             (THIS_
                                 REF(IUnknown*) /*OUT*/ pWrapper,
                                 IUnknown*      /*IN*/  pFileObj,
                                 UINT32         /*IN*/  ulBlockSize,
                                 HXBOOL           /*IN*/  bAlignReads,
                                 HXBOOL           /*IN*/  bCacheStats) PURE;
};


/****************************************************************************
 *
 *  Interface:
 *
 *      IHXFastFileFactory2
 *
 *  Purpose:
 *
 *      Interface to allow wrapping file objects with a buffer and block-sharing
 *      sceme.  Query off the server class factory.
 *
 *  IID_IHXFastFileFactory2:
 *
 *      {0000021F-0901-11d1-8B06-00A024406D59}
 *
 */
DEFINE_GUID(IID_IHXFastFileFactory2, 0x0000021F, 0x901, 0x11d1, 0x8b, 0x6, 0x0,
                        0xa0, 0x24, 0x40, 0x6d, 0x59);

#define CLSID_IHXFastFileFactory2 IID_IHXFastFileFactory2

#undef  INTERFACE
#define INTERFACE   IHXFastFileFactory2

DECLARE_INTERFACE_(IHXFastFileFactory2, IUnknown)
{
    /************************************************************************
     *  Method:
     *      IHXFastFileFactory2::Wrap
     *  Purpose:
     *      Return an instantiated wrapper around an existing (but
     *      uninitialized) file object.
     *
     */
    STDMETHOD(Wrap)             (THIS_
                                 REF(IUnknown*) /*OUT*/ pWrapper,
                                 IUnknown*      /*IN*/  pFileObj,
                                 UINT32         /*IN*/  ulBlockSize,
                                 HXBOOL           /*IN*/  bAlignReads,
                                 HXBOOL           /*IN*/  bCacheStats,
                                 UINT32         /*IN*/  ulMaxBlockSize) PURE;
};


/****************************************************************************
 *
 *  Interface:
 *
 *      IHXFilePlacementRead
 *
 *  Purpose:
 *
 *      Reads into the passed buffer
 *
 *  IID_IHXFilePlacementRead
 *
 *      {0000021D-0901-11d1-8B06-00A024406D59}
 *
 */
DEFINE_GUID(IID_IHXFilePlacementRead, 0x0000021D, 0x901, 0x11d1, 0x8b, 0x6, 0x0,
                        0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXFilePlacementRead

DECLARE_INTERFACE_(IHXFilePlacementRead, IUnknown)
{
    /*
     *  IUnknown methods
     */
    STDMETHOD(QueryInterface)   (THIS_
                                REFIID riid,
                                void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)  (THIS) PURE;

    STDMETHOD_(ULONG32,Release) (THIS) PURE;

    /*
     *  IHXFilePlacementRead methods
     */
    STDMETHOD(Read)             (THIS_
                                ULONG32 ulAmount,
                                ULONG32 ulOffset,
                                char*   pBuffer,
                                HXBOOL    bOffsetBuffer) PURE;

    STDMETHOD_(ULONG32,AlignmentBoundary) (THIS) PURE;
};

// $EndPrivate.

/****************************************************************************
 *
 *  Interface:
 *
 *      IHXFastFileStats
 *
 *  Purpose:
 *
 *      Interface to allow file objects to request that at close they be
 *      informed how many bytes have been "fast cached" on their behalf if any.
 *
 *      ulFastFileBytesSaved is the number of bytes that would have been read
 *      from the file object had FastFile not been in use (this includes some
 *      lookahead
 *
 *      ulFastFileBytesNeeded is the amount of data actually read by the file
 *      system.
 *
 *  IID_IHXFastFileStats:
 *
 *      {0000021E-0901-11d1-8B06-00A024406D59}
 *
 */
DEFINE_GUID(IID_IHXFastFileStats, 0x0000021E, 0x901, 0x11d1, 0x8b, 0x6, 0x0,
                        0xa0, 0x24, 0x40, 0x6d, 0x59);

#define CLSID_IHXFastFileStats IID_IHXFastFileStats

#undef  INTERFACE
#define INTERFACE   IHXFastFileStats

DECLARE_INTERFACE_(IHXFastFileStats, IUnknown)
{
    /*
     *  IUnknown methods
     */

    STDMETHOD(QueryInterface)   (THIS_
                                REFIID riid,
                                void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)  (THIS) PURE;

    STDMETHOD_(ULONG32,Release) (THIS) PURE;

    /*
     * IHXFastFileStats methods
     */

    STDMETHOD(UpdateFileObjectStats)    (THIS_
                                         UINT32         /*IN*/  ulFastFileBytesSaved,
                                         UINT32         /*IN*/  ulFastFileBytesNeeded) PURE;
};

// $Private:
/****************************************************************************
 *
 *  Interface:
 *
 *      IHXHTTPPostObject
 *
 *  Purpose:
 *
 *      Object that exports file control API
 *
 *  IID_IHXHTTPPostObject:
 *
 *      {00000112-0901-11d1-8B06-00A024406D59}
 *
 */

DEFINE_GUID(IID_IHXHTTPPostObject, 0x00000112, 0x901, 0x11d1, 0x8b, 0x6, 0x0,
                        0xa0, 0x24, 0x40, 0x6d, 0x59);

#define CLSID_IHXHTTPPostObject IID_IHXHTTPPostObject

#undef  INTERFACE
#define INTERFACE   IHXHTTPPostObject

DECLARE_INTERFACE_(IHXHTTPPostObject, IUnknown)
{
    /*
     *  IUnknown methods
     */
    STDMETHOD(QueryInterface)   (THIS_
                                REFIID riid,
                                void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)  (THIS) PURE;

    STDMETHOD_(ULONG32,Release) (THIS) PURE;

    /*
     *  IHXHTTPPostObject methods
     */

    /************************************************************************
     *  Method:
     *      IHXHTTPPostObject::Init
     *  Purpose:
     *      Associates a file object with the file response object it should
     *      notify of operation completness. This method should also check
     *      for validity of the object (for example by opening it if it is
     *      a local file).
     */
    STDMETHOD(Init)     (THIS_
                        ULONG32             /*IN*/  ulFlags,
                        IHXHTTPPostResponse*   /*IN*/  pFileResponse) PURE;

    /************************************************************************
     *  Method:
     *      IHXHTTPPostObject::Close
     *  Purpose:
     *      Closes the file resource and releases all resources associated
     *      with the object.
     */
    STDMETHOD(Close)    (THIS) PURE;

    /************************************************************************
     *  Method:
     *      IHXHTTPPostObject::GetResponse
     *  Purpose:
     *      Tells the object to retrieve any response data from the POST.
     *      Calls IHXHTTPPostResponse with ResponseReady(IHXValues*).
     */
    STDMETHOD(GetResponse)      (THIS) PURE;


    /************************************************************************
     *  Method:
     *      IHXHTTPPostObject::Post
     *  Purpose:
     *      Writes a buffer of data to the HTTP URL and asynchronously notifies
     *      the caller via the IHXFileResponse interface passed in to Init,
     *      of the completeness of the operation.
     */
    STDMETHOD(Post)     (THIS_
                        IHXBuffer* pBuffer) PURE;

                        /************************************************************************
     *  Method:
     *      IHXHTTPPostObject::SetSize
     *  Purpose:
     *      Set the total size of the Post(s) about to be made.
     */
    STDMETHOD(SetSize)          (THIS_
                                ULONG32         ulLength) PURE;
};


/****************************************************************************
 *
 *  Interface:
 *
 *      IHXHTTPPostResponse
 *
 *  Purpose:
 *
 *      Object that exports file response API
 *
 *  IID_IHXHTTPPostResponse:
 *
 *      {00000113-0901-11d1-8B06-00A024406D59}
 *
 */
DEFINE_GUID(IID_IHXHTTPPostResponse,    0x00000113, 0x901, 0x11d1, 0x8b, 0x6, 0x0,
                            0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXHTTPPostResponse

DECLARE_INTERFACE_(IHXHTTPPostResponse, IUnknown)
{
    /*
     *  IUnknown methods
     */
    STDMETHOD(QueryInterface)           (THIS_
                                        REFIID riid,
                                        void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)          (THIS) PURE;

    STDMETHOD_(ULONG32,Release)         (THIS) PURE;

    /*
     *  IHXHTTPPostResponse methods
     */

    /************************************************************************
     *  Method:
     *      IHXHTTPPostResponse::InitDone
     *  Purpose:
     *      Notification interface provided by users of the IHXHTTPPostObject
     *      interface. This method is called by the IHXHTTPPostObject when the
     *      initialization of the file is complete. If the file is not valid
     *      for the file system, the status HXR_FAILED should be
     *      returned.
     */
    STDMETHOD(InitDone)                 (THIS_
                                        HX_RESULT           status) PURE;

    /************************************************************************
     *  Method:
     *      IHXHTTPPostResponse::CloseDone
     *  Purpose:
     *      Notification interface provided by users of the IHXHTTPPostObject
     *      interface. This method is called by the IHXHTTPPostObject when the
     *      close of the file is complete.
     */
    STDMETHOD(CloseDone)                (THIS_
                                        HX_RESULT           status) PURE;

    /************************************************************************
     *  Method:
     *      IHXHTTPPostResponse::ResponseReady
     *  Purpose:
     *      Notification interface provided by users of the IHXHTTPPostObject
     *      interface. This method is called by the IHXHTTPPostObject when the
     *      POST response data has been completely read.
     */
    STDMETHOD(ResponseReady)            (THIS_
                                        HX_RESULT           status,
                                        IHXBuffer*          pContentBuffer) PURE;

    /************************************************************************
     *  Method:
     *      IHXHTTPPostResponse::WriteDone
     *  Purpose:
     *      Notification interface provided by users of the IHXHTTPPostObject
     *      interface. This method is called by the IHXHTTPPostObject when the
     *      last write to the file is complete.
     */
    STDMETHOD(PostDone)                 (THIS_
                                        HX_RESULT           status) PURE;
};
// $EndPrivate.


// $Private:
/****************************************************************************
 *
 *  Interface:
 *
 *      IHXHTTPRedirect
 *
 *  Purpose:
 *
 *      Allows you to get redirect URL
 *
 *  IID_IHXHTTPRedirect:
 *
 *      {00002E00-0901-11d1-8B06-00A024406D59}
 *
 */

DEFINE_GUID(IID_IHXHTTPRedirect, 0x21eae0b9, 0x2e0c, 0x4003, 0xbb, 0x79,
            0xbc, 0x8c, 0xc5, 0x67, 0x2c, 0x2d);

#undef  INTERFACE
#define INTERFACE   IHXHTTPRedirect

DECLARE_INTERFACE_(IHXHTTPRedirect, IUnknown)
{
    /*
     *  IUnknown methods
     */
    STDMETHOD(QueryInterface)           (THIS_
                                        REFIID riid,
                                        void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)          (THIS) PURE;

    STDMETHOD_(ULONG32,Release)         (THIS) PURE;

    /*
     *  IHXHTTPRedirect methods
     */

    /************************************************************************
     *  Method:
     *      IHXHTTPRedirect::Init
     *  Purpose:
     *      Initialize the response object
     */
    STDMETHOD(Init)         (THIS_
                             IHXHTTPRedirectResponse* pRedirectResponse) PURE;

    /************************************************************************
     *  Method:
     *      IHXHTTPRedirect::SetResponseObject
     *  Purpose:
     *      Initialize the response object w/o calling Init
     */
    STDMETHOD(SetResponseObject) (THIS_
                             IHXHTTPRedirectResponse* pRedirectResponse) PURE;

};

/****************************************************************************
 *
 *  Interface:
 *
 *      IHXHTTPRedirectResponse
 *
 *  Purpose:
 *
 *      Allows you to get redirect URL
 *
 *  IID_IHXHTTPRedirectResponse:
 *
 *      {00002E01-0901-11d1-8B06-00A024406D59}
 *
 */

DEFINE_GUID(IID_IHXHTTPRedirectResponse, 0x125a63b1, 0x669c, 0x42f9, 0xb1,
            0xf9, 0x1b, 0x53, 0xe9, 0x95, 0x82, 0x95);

#undef  INTERFACE
#define INTERFACE   IHXHTTPRedirectResponse

DECLARE_INTERFACE_(IHXHTTPRedirectResponse, IUnknown)
{
    /*
     *  IUnknown methods
     */
    STDMETHOD(QueryInterface)           (THIS_
                                        REFIID riid,
                                        void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)          (THIS) PURE;

    STDMETHOD_(ULONG32,Release)         (THIS) PURE;

    /*
     *  IHXHTTPRedirectResponse methods
     */

    /************************************************************************
     *  Method:
     *      IHXHTTPRedirectResponse::RedirectDone
     *  Purpose:
     *      return the redirect URL
     */
    STDMETHOD(RedirectDone)             (THIS_
                                         IHXBuffer* pURL) PURE;

};

/****************************************************************************
 *
 *  Interface:
 *
 *      IHXRM2Converter2
 *
 *  Purpose:
 *
 *      Interface to RM1->RM2 merge/converter module. This is a new improved
 *      interface which contains all of the functionality of the old one.
 *      However, this one is asynchronous where necessary and takes standard
 *      IHXFileObjects instead of pathnames for the required files.
 *
 *  IHXRM2Converter
 *
 *      {00002F00-0901-11D1-8B06-00A024406D59}
 */

DEFINE_GUID(IID_IHXRM2Converter2, 0x00002F00, 0x901, 0x11d1, 0x8b,
            0x6, 0x0, 0xa0, 0x24, 0x40, 0x6d, 0x59);

#define CLSID_IRMARM2Converter2 IID_IHXRM2Converter2

#undef  INTERFACE
#define INTERFACE   IHXRM2Converter2

DECLARE_INTERFACE_(IHXRM2Converter2, IUnknown)
{
    /*
     *  IUnknown methods
     */
    STDMETHOD(QueryInterface)       (THIS_
                                    REFIID riid,
                                    void** ppvObj) PURE;
    STDMETHOD_(ULONG32,AddRef)      (THIS) PURE;
    STDMETHOD_(ULONG32,Release)     (THIS) PURE;

    /*
     *  IHXRM2Converter2 methods
     */
    STDMETHOD(Init)                 (THIS_
                                    IHXRM2Converter2Response* pResponse) PURE;
    //
    // AddStream takes a stream from an RM1 file and
    // adds it to the new RM2 multirate section
    //
    STDMETHOD(AddStream)            (THIS_
                                    const char* pFilename,
                                    IHXFileObject* pFileObject,
                                    UINT16 nStreamNumber,
                                    HXBOOL bTagAsBackwardCompatible) PURE;

    //
    // AddInterleavedStream takes a stream from an
    // RM1 file and adds it to the initial interleaved
    // backward compatibility section
    //
    STDMETHOD(AddInterleavedStream) (THIS_
                                    const char* pFilename,
                                    IHXFileObject* pFileObject,
                                    UINT16 nStreamNumber) PURE;

    //
    // Add file slurps down all of the streams in
    // an RM1 file and adds them to the new RM2
    // multirate section.
    //
    STDMETHOD(AddFile)              (THIS_
                                    const char* pFilename,
                                    IHXFileObject* pFileObject) PURE;

    //
    // SetTitle,Author,Copyright, and Comment
    // should all be pretty self explanatory...
    //
    STDMETHOD(SetTitle)             (THIS_
                                    const char* pTitle) PURE;
    STDMETHOD(SetAuthor)            (THIS_
                                    const char* pAuthor) PURE;
    STDMETHOD(SetCopyright)         (THIS_
                                    const char* pCopyright) PURE;
    STDMETHOD(SetComment)           (THIS_
                                    const char* pComment) PURE;

    //
    // PairStreams lets you pair two different
    // streams together to play at a particular
    // bandwidth (i.e. the sum of the stream
    // bandwidths involved).
    //
    STDMETHOD(PairStreams)          (THIS_
                                    const char* pFilename1,
                                    UINT16 nStreamNumber1,
                                    const char* pFilename2,
                                    UINT16 nStreamNumber2) PURE;

    STDMETHOD(Merge)            (THIS_
                                const char* pFilename,
                                IHXFileObject* pOutputFile,
                                UINT32 ulBytesToWrite) PURE;

    STDMETHOD(Reset)                (THIS) PURE;
    STDMETHOD(Done)                 (THIS) PURE;
};


/****************************************************************************
 *
 *  Interface:
 *
 *      IHXRM2Converter2Response
 *
 *  Purpose:
 *
 *      Response Interface for IHXRM2Converter2.
 *
 *  IHXRM2Converter
 *
 *      {00002F01-0901-11D1-8B06-00A024406D59}
 */

DEFINE_GUID(IID_IHXRM2Converter2Response, 0x00002F01, 0x901, 0x11d1, 0x8b,
            0x6, 0x0, 0xa0, 0x24, 0x40, 0x6d, 0x59);


#undef  INTERFACE
#define INTERFACE   IHXRM2Converter2Response

DECLARE_INTERFACE_(IHXRM2Converter2Response, IUnknown)
{
    /*
     *  IUnknown methods
     */
    STDMETHOD(QueryInterface)           (THIS_
                                        REFIID riid,
                                        void** ppvObj) PURE;
    STDMETHOD_(ULONG32,AddRef)          (THIS) PURE;
    STDMETHOD_(ULONG32,Release)         (THIS) PURE;

    /*
     *  IHXRM2Converter2Response methods
     */
    STDMETHOD(InitDone)                 (THIS_
                                        HX_RESULT status) PURE;
    STDMETHOD(AddStreamDone)            (THIS_
                                        HX_RESULT status) PURE;
    STDMETHOD(AddInterleavedStreamDone) (THIS_
                                        HX_RESULT status) PURE;
    STDMETHOD(AddFileDone)              (THIS_
                                        HX_RESULT status) PURE;
    STDMETHOD(MergeDone)                (THIS_
                                        HX_RESULT status) PURE;
};


/****************************************************************************
 *
 *  Interface:
 *
 *      IHXPoolPathAdjustment
 *
 *  Purpose:
 *
 *      For file systems that use GetFileObjectFromPool to properly
 *      adjust absolute local URLs so they will be accessed relative
 *      to the pool filesystem mountpoint.
 *
 *  IID_IHXPoolPathAdjustment:
 *
 *      {00002F02-0901-11d1-8b06-00A024406D59}
 *
 */
DEFINE_GUID(IID_IHXPoolPathAdjustment, 0x00002F02, 0x901, 0x11d1, 0x8b, 0x6, 0x0, 0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXPoolPathAdjustment

DECLARE_INTERFACE_(IHXPoolPathAdjustment, IUnknown)
{
    /*
     *  IUnknown methods
     */
    STDMETHOD(QueryInterface)   (THIS_
                                REFIID riid,
                                void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)  (THIS) PURE;

    STDMETHOD_(ULONG32,Release) (THIS) PURE;

    /*
     * IHXPoolPathAdjustment Methods.
     */
    STDMETHOD(AdjustAbsolutePath)   (THIS_
                                    IHXBuffer*      /*IN*/  pOldPath,
                                    REF(IHXBuffer*) /*OUT*/ pNewPath) PURE;

};

/*
 * Interface:
 *      IHXPostDataHandler
 *
 * Purpose:
 *
 *      Allows a file object to receive out of band POST data
 *      from an HTTP request.  If this interface is not implemented
 *      data will be placed in request header as "PostData"
 *
 *      When there is no more Data remaining, PostData will be
 *      called with a NULL argument.
 *
 * IID_IHXPostDataHandler:
 *
 * {0x0222a1b5-49fc-47e2-b69098befa0a588e}
 *
 */
DEFINE_GUID(IID_IHXPostDataHandler, 0x0222a1b5, 0x49fc, 0x47e2, 0xb6,
        0x90, 0x98, 0xbe, 0xfa, 0x0a, 0x58, 0x8e);

#undef INTERFACE
#define INTERFACE IHXPostDataHandler

DECLARE_INTERFACE_(IHXPostDataHandler, IUnknown)
{
    /*
     * IUnknownMethods
     */

    STDMETHOD(QueryInterface) (THIS_
            REFIID riid,
            void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef) (THIS) PURE;

    STDMETHOD_(ULONG32,Release) (THIS) PURE;
    /*
     * IHXPostDataHandler methods
     */
    STDMETHOD(PostData) (THIS_
            IHXBuffer* pBuf) PURE;
};

// $EndPrivate.

#endif  /* _HXFILES_H_ */
