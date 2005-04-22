
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

#ifndef _HXRESULT_H_
#define _HXRESULT_H_

/* Some files include this before pntypes.h. */
#include "hxtypes.h"

typedef LONG32  HX_RESULT;

#ifndef _WIN32
    typedef HX_RESULT HRESULT;
#   undef NOERROR
#   define NOERROR 0
#   define FACILITY_ITF 4
#   define MAKE_HRESULT(sev,fac,code)                                           \
        ((HRESULT) (((unsigned long)(sev)<<31) | ((unsigned long)(fac)<<16) |   \
        ((unsigned long)(code))) )
#   define SUCCEEDED(Status) (((unsigned long)(Status)>>31) == 0)
#   define FAILED(Status) (((unsigned long)(Status)>>31) != 0)
#else
#   ifndef _HRESULT_DEFINED
        typedef LONG32 HRESULT;
#   endif       /* _HRESULT_DEFINED */
#   include <winerror.h>
#endif /* _WIN32 */

#define MAKE_HX_RESULT(sev,fac,code) MAKE_HRESULT(sev, FACILITY_ITF,        \
    ((fac << 6) | (code)))

#define SS_GLO 0     /* General errors                             */
#define SS_NET 1     /* Networking errors                          */
#define SS_FIL 2     /* File errors                                */
#define SS_PRT 3     /* Protocol Error                             */
#define SS_AUD 4     /* Audio error                                */
#define SS_INT 5     /* General internal errors                    */
#define SS_USR 6     /* The user is broken.                        */
#define SS_MSC 7     /* Miscellaneous                              */
#define SS_DEC 8     /* Decoder errors                             */
#define SS_ENC 9     /* Encoder errors                             */
#define SS_REG 10    /* Registry (not Windows registry ;) errors   */
#define SS_PPV 11    /* Pay Per View errors                        */
#define SS_RSC 12    /* Errors for HXXRES */
#define SS_UPG 13    /* Auto-upgrade & Certificate Errors          */
#define SS_PLY 14    /* RealPlayer/Plus specific errors (USE ONLY IN /rpmisc/pub/rpresult.h) */
#define SS_RMT 15    /* RMTools Errors                             */
#define SS_CFG 16    /* AutoConfig Errors                          */
#define SS_RPX 17    /* RealPix-related Errors */
#define SS_XML 18    /* XML-related Errors                         */
// $Private:
#define SS_TKO 19    /* Taiko specific errors (USE ONLY IN /taiko/tresult.h) */
#define SS_SEC 20    /* Security (key handling, encrypt,decrypt, CSP,...) errors */
// $EndPrivate.
#define SS_RCA 21    /* RCA errors                                 */
#define SS_ENC_AX 22 /* Encoder Active x error                     */
#define SS_SOCK 24   /* Socket errors */
#define SS_RSLV 25   /* Resolver errors */

#define SS_DPR 63    /* Deprecated errors                          */
#define SS_SAM 100   /* ServerAlert errors                         */


#define HXR_NOTIMPL                     MAKE_HRESULT(1,0,0x4001)                    // 80004001
#define HXR_OUTOFMEMORY                 MAKE_HRESULT(1,7,0x000e)                    // 8007000e
#define HXR_INVALID_PARAMETER           MAKE_HRESULT(1,7,0x0057)                    // 80070057
#define HXR_NOINTERFACE                 MAKE_HRESULT(1,0,0x4002)                    // 80004002
#define HXR_POINTER                     MAKE_HRESULT(1,0,0x4003)                    // 80004003
#define HXR_HANDLE                      MAKE_HRESULT(1,7,0x0006)                    // 80070006
#define HXR_ABORT                       MAKE_HRESULT(1,0,0x4004)                    // 80004004
#define HXR_FAIL                        MAKE_HRESULT(1,0,0x4005)                    // 80004005
#define HXR_ACCESSDENIED                MAKE_HRESULT(1,7,0x0005)                    // 80070005
#define HXR_IGNORE                      MAKE_HRESULT(1,0,0x0006)                    // 80000006
#define HXR_OK                          MAKE_HRESULT(0,0,0)                         // 00000000


#define HXR_INVALID_OPERATION           MAKE_HX_RESULT(1,SS_GLO,4)                  // 80040004
#define HXR_INVALID_VERSION             MAKE_HX_RESULT(1,SS_GLO,5)                  // 80040005
#define HXR_INVALID_REVISION            MAKE_HX_RESULT(1,SS_GLO,6)                  // 80040006
#define HXR_NOT_INITIALIZED             MAKE_HX_RESULT(1,SS_GLO,7)                  // 80040007
#define HXR_DOC_MISSING                 MAKE_HX_RESULT(1,SS_GLO,8)                  // 80040008
#define HXR_UNEXPECTED                  MAKE_HX_RESULT(1,SS_GLO,9)                  // 80040009
#define HXR_INCOMPLETE                  MAKE_HX_RESULT(1,SS_GLO,12)                 // 8004000c
#define HXR_BUFFERTOOSMALL              MAKE_HX_RESULT(1,SS_GLO,13)                 // 8004000d
#define HXR_UNSUPPORTED_VIDEO           MAKE_HX_RESULT(1,SS_GLO,14)                 // 8004000e
#define HXR_UNSUPPORTED_AUDIO           MAKE_HX_RESULT(1,SS_GLO,15)                 // 8004000f
#define HXR_INVALID_BANDWIDTH           MAKE_HX_RESULT(1,SS_GLO,16)                 // 80040010
/* HXR_NO_RENDERER and HXR_NO_FILEFORMAT old value is being deprecated
#define HXR_NO_FILEFORMAT               MAKE_HX_RESULT(1,SS_GLO,10)
#define HXR_NO_RENDERER                 MAKE_HX_RESULT(1,SS_GLO,11)*/
#define HXR_NO_RENDERER                 MAKE_HX_RESULT(1,SS_GLO,17)                 // 80040011
#define HXR_NO_FILEFORMAT               MAKE_HX_RESULT(1,SS_GLO,17)                 // 80040011
#define HXR_MISSING_COMPONENTS          MAKE_HX_RESULT(1,SS_GLO,17)                 // 80040011
#define HXR_ELEMENT_NOT_FOUND           MAKE_HX_RESULT(0,SS_GLO,18)                 // 00040012
#define HXR_NOCLASS                     MAKE_HX_RESULT(0,SS_GLO,19)                 // 00040013
#define HXR_CLASS_NOAGGREGATION         MAKE_HX_RESULT(0,SS_GLO,20)                 // 00040014
#define HXR_NOT_LICENSED                MAKE_HX_RESULT(1,SS_GLO,21)                 // 80040015
#define HXR_NO_FILESYSTEM               MAKE_HX_RESULT(1,SS_GLO,22)                 // 80040016
#define HXR_REQUEST_UPGRADE             MAKE_HX_RESULT(1,SS_GLO,23)                 // 80040017

#define HXR_CHECK_RIGHTS               MAKE_HX_RESULT(1,SS_GLO,24)                 // 80040018
#define HXR_RESTORE_SERVER_DENIED      MAKE_HX_RESULT(1,SS_GLO,25)                 // 80040019
#define HXR_DEBUGGER_DETECTED          MAKE_HX_RESULT(1,SS_GLO,26)                 // 8004001a
#define HXR_RESTORE_SERVER_CONNECT     MAKE_HX_RESULT(1,SS_NET,28)                 // 8004005c
#define HXR_RESTORE_SERVER_TIMEOUT     MAKE_HX_RESULT(1,SS_NET,29)                 // 8004005d
#define HXR_REVOKE_SERVER_CONNECT      MAKE_HX_RESULT(1,SS_NET,30)                 // 8004005e
#define HXR_REVOKE_SERVER_TIMEOUT      MAKE_HX_RESULT(1,SS_NET,31)                 // 8004005f
#define HXR_VIEW_RIGHTS_NODRM          MAKE_HX_RESULT(1,SS_MSC,13)                 // 800401cd
#define HXR_VSRC_NODRM                 MAKE_HX_RESULT(1,SS_MSC,19)                 // 800401d3
#define HXR_WM_OPL_NOT_SUPPORTED       MAKE_HX_RESULT(1,SS_GLO,36)                 // 80040024

// $Private:
/* Status Code for backup/restore*/
#define HXR_RESTORATION_COMPLETE        MAKE_HX_RESULT(1,SS_GLO,27)                 // 8004001b
#define HXR_BACKUP_COMPLETE             MAKE_HX_RESULT(1,SS_GLO,28)                 // 8004001c
#define HXR_TLC_NOT_CERTIFIED           MAKE_HX_RESULT(1,SS_GLO,29)                 // 8004001d
#define HXR_CORRUPTED_BACKUP_FILE       MAKE_HX_RESULT(1,SS_GLO,30)                 // 8004001e
// $EndPrivate.
#define HXR_AWAITING_LICENSE            MAKE_HX_RESULT(1,SS_GLO,31)                 // 8004001f
#define HXR_ALREADY_INITIALIZED         MAKE_HX_RESULT(1,SS_GLO,32)                 // 80040020
#define HXR_NOT_SUPPORTED               MAKE_HX_RESULT(1,SS_GLO,33)                 // 80040021
#define HXR_S_FALSE                     MAKE_HX_RESULT(0,SS_GLO,34)                 // 00040022
#define HXR_WARNING                     MAKE_HX_RESULT(0,SS_GLO,35)                 // 00040023

#define HXR_BUFFERING                   MAKE_HX_RESULT(0,SS_NET,0)                  // 00040040
#define HXR_PAUSED                      MAKE_HX_RESULT(0,SS_NET,1)                  // 00040041
#define HXR_NO_DATA                     MAKE_HX_RESULT(0,SS_NET,2)                  // 00040042
#define HXR_STREAM_DONE                 MAKE_HX_RESULT(0,SS_NET,3)                  // 00040043
#define HXR_NET_SOCKET_INVALID          MAKE_HX_RESULT(1,SS_NET,3)                  // 80040043
#define HXR_NET_CONNECT                 MAKE_HX_RESULT(1,SS_NET,4)                  // 80040044
#define HXR_BIND                        MAKE_HX_RESULT(1,SS_NET,5)                  // 80040045
#define HXR_SOCKET_CREATE               MAKE_HX_RESULT(1,SS_NET,6)                  // 80040046
#define HXR_INVALID_HOST                MAKE_HX_RESULT(1,SS_NET,7)                  // 80040047
#define HXR_NET_READ                    MAKE_HX_RESULT(1,SS_NET,8)                  // 80040048
#define HXR_NET_WRITE                   MAKE_HX_RESULT(1,SS_NET,9)                  // 80040049
#define HXR_NET_UDP                     MAKE_HX_RESULT(1,SS_NET,10)                 // 8004004a
#define HXR_RETRY                       MAKE_HX_RESULT(1,SS_NET,11) /* XXX */       // 8004004b
#define HXR_SERVER_TIMEOUT              MAKE_HX_RESULT(1,SS_NET,12)                 // 8004004c
#define HXR_SERVER_DISCONNECTED         MAKE_HX_RESULT(1,SS_NET,13)                 // 8004004d
#define HXR_WOULD_BLOCK                 MAKE_HX_RESULT(1,SS_NET,14)                 // 8004004e
#define HXR_GENERAL_NONET               MAKE_HX_RESULT(1,SS_NET,15)                 // 8004004f
#define HXR_BLOCK_CANCELED              MAKE_HX_RESULT(1,SS_NET,16) /* XXX */       // 80040050
#define HXR_MULTICAST_JOIN              MAKE_HX_RESULT(1,SS_NET,17)                 // 80040051
#define HXR_GENERAL_MULTICAST           MAKE_HX_RESULT(1,SS_NET,18)                 // 80040052
#define HXR_MULTICAST_UDP               MAKE_HX_RESULT(1,SS_NET,19)                 // 80040053
#define HXR_AT_INTERRUPT                MAKE_HX_RESULT(1,SS_NET,20)                 // 80040054
#define HXR_MSG_TOOLARGE                MAKE_HX_RESULT(1,SS_NET,21)                 // 80040055
#define HXR_NET_TCP                     MAKE_HX_RESULT(1,SS_NET,22)                 // 80040056
#define HXR_TRY_AUTOCONFIG              MAKE_HX_RESULT(1,SS_NET,23)                 // 80040057
#define HXR_NOTENOUGH_BANDWIDTH         MAKE_HX_RESULT(1,SS_NET,24)                 // 80040058
#define HXR_HTTP_CONNECT                MAKE_HX_RESULT(1,SS_NET,25)                 // 80040059
#define HXR_PORT_IN_USE                 MAKE_HX_RESULT(1,SS_NET,26)                 // 8004005a
#define HXR_LOADTEST_NOT_SUPPORTED      MAKE_HX_RESULT(1,SS_NET,27)                 // 8004005b
#define HXR_TCP_CONNECT                 MAKE_HX_RESULT(0,SS_NET,32)                 // 00040060
#define HXR_TCP_RECONNECT               MAKE_HX_RESULT(0,SS_NET,33)                 // 00040061
#define HXR_TCP_FAILED                  MAKE_HX_RESULT(1,SS_NET,34)                 // 80040062
#define HXR_AUTH_SOCKET_CREATE_FAILURE  MAKE_HX_RESULT(1,SS_NET,35)                 // 80040063
#define HXR_AUTH_TCP_CONNECT_FAILURE    MAKE_HX_RESULT(1,SS_NET,36)                 // 80040064
#define HXR_AUTH_TCP_CONNECT_TIMEOUT    MAKE_HX_RESULT(1,SS_NET,37)                 // 80040065
#define HXR_AUTH_FAILURE                MAKE_HX_RESULT(1,SS_NET,38)                 // 80040066
#define HXR_AUTH_REQ_PARAMETER_MISSING  MAKE_HX_RESULT(1,SS_NET,39)                 // 80040067
#define HXR_DNS_RESOLVE_FAILURE         MAKE_HX_RESULT(1,SS_NET,40)                 // 80040068
#define HXR_AUTH_SUCCEEDED              MAKE_HX_RESULT(0,SS_NET,40)                 // 00040068
#define HXR_PULL_AUTHENTICATION_FAILED  MAKE_HX_RESULT(1,SS_NET,41)                 // 80040069
#define HXR_BIND_ERROR                  MAKE_HX_RESULT(1,SS_NET,42)                 // 8004006a
#define HXR_PULL_PING_TIMEOUT           MAKE_HX_RESULT(1,SS_NET,43)                 // 8004006b
#define HXR_AUTH_TCP_FAILED             MAKE_HX_RESULT(1,SS_NET,44)                 // 8004006c
#define HXR_UNEXPECTED_STREAM_END       MAKE_HX_RESULT(1,SS_NET,45)                 // 8004006d
#define HXR_AUTH_READ_TIMEOUT           MAKE_HX_RESULT(1,SS_NET,46)                 // 8004006e
#define HXR_AUTH_CONNECTION_FAILURE     MAKE_HX_RESULT(1,SS_NET,47)                 // 8004006f
#define HXR_BLOCKED                     MAKE_HX_RESULT(1,SS_NET,48)                 // 80040070
#define HXR_NOTENOUGH_PREDECBUF         MAKE_HX_RESULT(1,SS_NET,49)                 // 80040071
#define HXR_END_WITH_REASON             MAKE_HX_RESULT(1,SS_NET,50)                 // 80040072
#define HXR_SOCKET_NOBUFS               MAKE_HX_RESULT(1,SS_NET,51)                 // 80040073

#define HXR_AT_END                      MAKE_HX_RESULT(0,SS_FIL,0)                  // 00040080
#define HXR_INVALID_FILE                MAKE_HX_RESULT(1,SS_FIL,1)                  // 80040081
#define HXR_INVALID_PATH                MAKE_HX_RESULT(1,SS_FIL,2)                  // 80040082
#define HXR_RECORD                      MAKE_HX_RESULT(1,SS_FIL,3)                  // 80040083
#define HXR_RECORD_WRITE                MAKE_HX_RESULT(1,SS_FIL,4)                  // 80040084
#define HXR_TEMP_FILE                   MAKE_HX_RESULT(1,SS_FIL,5)                  // 80040085
#define HXR_ALREADY_OPEN                MAKE_HX_RESULT(1,SS_FIL,6)                  // 80040086
#define HXR_SEEK_PENDING                MAKE_HX_RESULT(1,SS_FIL,7)                  // 80040087
#define HXR_CANCELLED                   MAKE_HX_RESULT(1,SS_FIL,8)                  // 80040088
#define HXR_FILE_NOT_FOUND              MAKE_HX_RESULT(1,SS_FIL,9)                  // 80040089
#define HXR_WRITE_ERROR                 MAKE_HX_RESULT(1,SS_FIL,10)                 // 8004008a
#define HXR_FILE_EXISTS                 MAKE_HX_RESULT(1,SS_FIL,11)                 // 8004008b
#define HXR_FILE_NOT_OPEN               MAKE_HX_RESULT(1,SS_FIL,12)                 // 8004008c
#define HXR_ADVISE_PREFER_LINEAR        MAKE_HX_RESULT(0,SS_FIL,13)                 // 0004008d
#define HXR_PARSE_ERROR                 MAKE_HX_RESULT(1,SS_FIL,14)                 // 8004008e
#define HXR_ADVISE_NOASYNC_SEEK         MAKE_HX_RESULT(0,SS_FIL,15)                 // 0004008f
#define HXR_HEADER_PARSE_ERROR          MAKE_HX_RESULT(1,SS_FIL,16)                 // 80040090
#define HXR_CORRUPT_FILE                MAKE_HX_RESULT(1,SS_FIL,17)                 // 80040091

#define HXR_BAD_SERVER                  MAKE_HX_RESULT(1,SS_PRT,0)                  // 800400c0
#define HXR_ADVANCED_SERVER             MAKE_HX_RESULT(1,SS_PRT,1)                  // 800400c1
#define HXR_OLD_SERVER                  MAKE_HX_RESULT(1,SS_PRT,2)                  // 800400c2
#define HXR_REDIRECTION                 MAKE_HX_RESULT(0,SS_PRT,3) /* XXX */        // 000400c3
#define HXR_SERVER_ALERT                MAKE_HX_RESULT(1,SS_PRT,4)                  // 800400c4
#define HXR_PROXY                       MAKE_HX_RESULT(1,SS_PRT,5)                  // 800400c5
#define HXR_PROXY_RESPONSE              MAKE_HX_RESULT(1,SS_PRT,6)                  // 800400c6
#define HXR_ADVANCED_PROXY              MAKE_HX_RESULT(1,SS_PRT,7)                  // 800400c7
#define HXR_OLD_PROXY                   MAKE_HX_RESULT(1,SS_PRT,8)                  // 800400c8
#define HXR_INVALID_PROTOCOL            MAKE_HX_RESULT(1,SS_PRT,9)                  // 800400c9
#define HXR_INVALID_URL_OPTION          MAKE_HX_RESULT(1,SS_PRT,10)                 // 800400ca
#define HXR_INVALID_URL_HOST            MAKE_HX_RESULT(1,SS_PRT,11)                 // 800400cb
#define HXR_INVALID_URL_PATH            MAKE_HX_RESULT(1,SS_PRT,12)                 // 800400cc
#define HXR_HTTP_CONTENT_NOT_FOUND      MAKE_HX_RESULT(1,SS_PRT,13)                 // 800400cd
#define HXR_NOT_AUTHORIZED              MAKE_HX_RESULT(1,SS_PRT,14)                 // 800400ce
#define HXR_UNEXPECTED_MSG              MAKE_HX_RESULT(1,SS_PRT,15)                 // 800400cf
#define HXR_BAD_TRANSPORT               MAKE_HX_RESULT(1,SS_PRT,16)                 // 800400d0
#define HXR_NO_SESSION_ID               MAKE_HX_RESULT(1,SS_PRT,17)                 // 800400d1
#define HXR_PROXY_DNR                   MAKE_HX_RESULT(1,SS_PRT,18)                 // 800400d2
#define HXR_PROXY_NET_CONNECT           MAKE_HX_RESULT(1,SS_PRT,19)                 // 800400d3
#define HXR_AGGREGATE_OP_NOT_ALLOWED    MAKE_HX_RESULT(1,SS_PRT,20)                 // 800400d4
#define HXR_RIGHTS_EXPIRED              MAKE_HX_RESULT(1,SS_PRT,21)                 // 800400d5
#define HXR_NOT_MODIFIED                MAKE_HX_RESULT(1,SS_PRT,22)                 // 800400d6
#define HXR_FORBIDDEN                   MAKE_HX_RESULT(1,SS_PRT,23)                 // 800400d7

#define HXR_AUDIO_DRIVER                MAKE_HX_RESULT(1,SS_AUD,0)                  // 80040100
#define HXR_LATE_PACKET                 MAKE_HX_RESULT(1,SS_AUD,1)                  // 80040101
#define HXR_OVERLAPPED_PACKET           MAKE_HX_RESULT(1,SS_AUD,2)                  // 80040102
#define HXR_OUTOFORDER_PACKET           MAKE_HX_RESULT(1,SS_AUD,3)                  // 80040103
#define HXR_NONCONTIGUOUS_PACKET        MAKE_HX_RESULT(1,SS_AUD,4)                  // 80040104

#define HXR_OPEN_NOT_PROCESSED          MAKE_HX_RESULT(1,SS_INT,0)                  // 80040140
#define HXR_WINDRAW_EXCEPTION           MAKE_HX_RESULT(1,SS_INT,1)                  // 80040141

#define HXR_EXPIRED                     MAKE_HX_RESULT(1,SS_USR,0)                  // 80040180

#define HXR_INVALID_INTERLEAVER         MAKE_HX_RESULT(1,SS_DPR,0)                  // 80040fc0
#define HXR_BAD_FORMAT                  MAKE_HX_RESULT(1,SS_DPR,1)                  // 80040fc1
#define HXR_CHUNK_MISSING               MAKE_HX_RESULT(1,SS_DPR,2)                  // 80040fc2
#define HXR_INVALID_STREAM              MAKE_HX_RESULT(1,SS_DPR,3)                  // 80040fc3
#define HXR_DNR                         MAKE_HX_RESULT(1,SS_DPR,4)                  // 80040fc4
#define HXR_OPEN_DRIVER                 MAKE_HX_RESULT(1,SS_DPR,5)                  // 80040fc5
#define HXR_UPGRADE                     MAKE_HX_RESULT(1,SS_DPR,6)                  // 80040fc6
#define HXR_NOTIFICATION                MAKE_HX_RESULT(1,SS_DPR,7)                  // 80040fc7
#define HXR_NOT_NOTIFIED                MAKE_HX_RESULT(1,SS_DPR,8)                  // 80040fc8
#define HXR_STOPPED                     MAKE_HX_RESULT(1,SS_DPR,9)                  // 80040fc9
#define HXR_CLOSED                      MAKE_HX_RESULT(1,SS_DPR,10)                 // 80040fca
#define HXR_INVALID_WAV_FILE            MAKE_HX_RESULT(1,SS_DPR,11)                 // 80040fcb
#define HXR_NO_SEEK                     MAKE_HX_RESULT(1,SS_DPR,12)                 // 80040fcc

#define HXR_DEC_INITED                  MAKE_HX_RESULT(1,SS_DEC,0)                  // 80040200
#define HXR_DEC_NOT_FOUND               MAKE_HX_RESULT(1,SS_DEC,1)                  // 80040201
#define HXR_DEC_INVALID                 MAKE_HX_RESULT(1,SS_DEC,2)                  // 80040202
#define HXR_DEC_TYPE_MISMATCH           MAKE_HX_RESULT(1,SS_DEC,3)                  // 80040203
#define HXR_DEC_INIT_FAILED             MAKE_HX_RESULT(1,SS_DEC,4)                  // 80040204
#define HXR_DEC_NOT_INITED              MAKE_HX_RESULT(1,SS_DEC,5)                  // 80040205
#define HXR_DEC_DECOMPRESS              MAKE_HX_RESULT(1,SS_DEC,6)                  // 80040206
#define HXR_OBSOLETE_VERSION            MAKE_HX_RESULT(1,SS_DEC,7)                  // 80040207
#define HXR_DEC_AT_END                  MAKE_HX_RESULT(0,SS_DEC,8)                  // 00040208

#define HXR_ENC_FILE_TOO_SMALL          MAKE_HX_RESULT(1,SS_ENC,0)                  // 80040240
#define HXR_ENC_UNKNOWN_FILE            MAKE_HX_RESULT(1,SS_ENC,1)                  // 80040241
#define HXR_ENC_BAD_CHANNELS            MAKE_HX_RESULT(1,SS_ENC,2)                  // 80040242
#define HXR_ENC_BAD_SAMPSIZE            MAKE_HX_RESULT(1,SS_ENC,3)                  // 80040243
#define HXR_ENC_BAD_SAMPRATE            MAKE_HX_RESULT(1,SS_ENC,4)                  // 80040244
#define HXR_ENC_INVALID                 MAKE_HX_RESULT(1,SS_ENC,5)                  // 80040245
#define HXR_ENC_NO_OUTPUT_FILE          MAKE_HX_RESULT(1,SS_ENC,6)                  // 80040246
#define HXR_ENC_NO_INPUT_FILE           MAKE_HX_RESULT(1,SS_ENC,7)                  // 80040247
#define HXR_ENC_NO_OUTPUT_PERMISSIONS   MAKE_HX_RESULT(1,SS_ENC,8)                  // 80040248
#define HXR_ENC_BAD_FILETYPE            MAKE_HX_RESULT(1,SS_ENC,9)                  // 80040249
#define HXR_ENC_INVALID_VIDEO           MAKE_HX_RESULT(1,SS_ENC,10)                 // 8004024a
#define HXR_ENC_INVALID_AUDIO           MAKE_HX_RESULT(1,SS_ENC,11)                 // 8004024b
#define HXR_ENC_NO_VIDEO_CAPTURE        MAKE_HX_RESULT(1,SS_ENC,12)                 // 8004024c
#define HXR_ENC_INVALID_VIDEO_CAPTURE   MAKE_HX_RESULT(1,SS_ENC,13)                 // 8004024d
#define HXR_ENC_NO_AUDIO_CAPTURE        MAKE_HX_RESULT(1,SS_ENC,14)                 // 8004024e
#define HXR_ENC_INVALID_AUDIO_CAPTURE   MAKE_HX_RESULT(1,SS_ENC,15)                 // 8004024f
#define HXR_ENC_TOO_SLOW_FOR_LIVE       MAKE_HX_RESULT(1,SS_ENC,16)                 // 80040250
#define HXR_ENC_ENGINE_NOT_INITIALIZED  MAKE_HX_RESULT(1,SS_ENC,17)                 // 80040251
#define HXR_ENC_CODEC_NOT_FOUND         MAKE_HX_RESULT(1,SS_ENC,18)                 // 80040252
#define HXR_ENC_CODEC_NOT_INITIALIZED   MAKE_HX_RESULT(1,SS_ENC,19)                 // 80040253
#define HXR_ENC_INVALID_INPUT_DIMENSIONS MAKE_HX_RESULT(1,SS_ENC,20)                // 80040254
#define HXR_ENC_MESSAGE_IGNORED         MAKE_HX_RESULT(1,SS_ENC,21)                 // 80040255
#define HXR_ENC_NO_SETTINGS             MAKE_HX_RESULT(1,SS_ENC,22)                 // 80040256
#define HXR_ENC_NO_OUTPUT_TYPES         MAKE_HX_RESULT(1,SS_ENC,23)                 // 80040257
#define HXR_ENC_IMPROPER_STATE          MAKE_HX_RESULT(1,SS_ENC,24)                 // 80040258
#define HXR_ENC_INVALID_SERVER          MAKE_HX_RESULT(1,SS_ENC,25)                 // 80040259
#define HXR_ENC_INVALID_TEMP_PATH       MAKE_HX_RESULT(1,SS_ENC,26)                 // 8004025a
#define HXR_ENC_MERGE_FAIL              MAKE_HX_RESULT(1,SS_ENC,27)                 // 8004025b
#define HXR_BIN_DATA_NOT_FOUND          MAKE_HX_RESULT(0,SS_ENC,28)                 // 0004025c
#define HXR_BIN_END_OF_DATA             MAKE_HX_RESULT(0,SS_ENC,29)                 // 0004025d
#define HXR_BIN_DATA_PURGED             MAKE_HX_RESULT(1,SS_ENC,30)                 // 8004025e
#define HXR_BIN_FULL                    MAKE_HX_RESULT(1,SS_ENC,31)                 // 8004025f
#define HXR_BIN_OFFSET_PAST_END         MAKE_HX_RESULT(1,SS_ENC,32)                 // 80040260
#define HXR_ENC_NO_ENCODED_DATA         MAKE_HX_RESULT(1,SS_ENC,33)                 // 80040261
#define HXR_ENC_INVALID_DLL             MAKE_HX_RESULT(1,SS_ENC,34)                 // 80040262
#define HXR_NOT_INDEXABLE               MAKE_HX_RESULT(1,SS_ENC,35)                 // 80040263
#define HXR_ENC_NO_BROWSER              MAKE_HX_RESULT(1,SS_ENC,36)                 // 80040264
#define HXR_ENC_NO_FILE_TO_SERVER       MAKE_HX_RESULT(1,SS_ENC,37)                 // 80040265
#define HXR_ENC_INSUFFICIENT_DISK_SPACE MAKE_HX_RESULT(1,SS_ENC,38)                 // 80040266
#define HXR_ENC_SAMPLE_DISCARDED MAKE_HX_RESULT(0,SS_ENC,39)                        // 00040267
#define HXR_ENC_RV10_FRAME_TOO_LARGE    MAKE_HX_RESULT(1,SS_ENC,40)                 // 80040268
#define HXR_S_NOT_HANDLED               MAKE_HX_RESULT(0,SS_ENC,41)                 // 00040269
#define HXR_S_END_OF_STREAM             MAKE_HX_RESULT(0,SS_ENC,42)                 // 0004026a
#define HXR_S_JOBFILE_INCOMPLETE        MAKE_HX_RESULT(0, SS_ENC, 43 )              // 0004026b
#define HXR_S_NOTHING_TO_SERIALIZE      MAKE_HX_RESULT(0, SS_ENC, 44 )              // 0004026c
#define HXR_SIZENOTSET                  MAKE_HX_RESULT(1,SS_ENC,45)                 // 8004026d
#define HXR_ALREADY_COMMITTED           MAKE_HX_RESULT(1,SS_ENC,46)                 // 8004026e
#define HXR_BUFFERS_OUTSTANDING         MAKE_HX_RESULT(1,SS_ENC,47)                 // 8004026f
#define HXR_NOT_COMMITTED               MAKE_HX_RESULT(1,SS_ENC,48)                 // 80040270
#define HXR_SAMPLE_TIME_NOT_SET         MAKE_HX_RESULT(1,SS_ENC,49)                 // 80040271
#define HXR_TIMEOUT                     MAKE_HX_RESULT(1,SS_ENC,50)                 // 80040272
#define HXR_WRONGSTATE                  MAKE_HX_RESULT(1,SS_ENC,51)                 // 80040273

#define HXR_RMT_USAGE_ERROR                     MAKE_HX_RESULT(1,SS_RMT,1)          // 800403c1
#define HXR_RMT_INVALID_ENDTIME                 MAKE_HX_RESULT(1,SS_RMT,2)          // 800403c2
#define HXR_RMT_MISSING_INPUT_FILE              MAKE_HX_RESULT(1,SS_RMT,3)          // 800403c3
#define HXR_RMT_MISSING_OUTPUT_FILE             MAKE_HX_RESULT(1,SS_RMT,4)          // 800403c4
#define HXR_RMT_INPUT_EQUALS_OUTPUT_FILE        MAKE_HX_RESULT(1,SS_RMT,5)          // 800403c5
#define HXR_RMT_UNSUPPORTED_AUDIO_VERSION       MAKE_HX_RESULT(1,SS_RMT,6)          // 800403c6
#define HXR_RMT_DIFFERENT_AUDIO                 MAKE_HX_RESULT(1,SS_RMT,7)          // 800403c7
#define HXR_RMT_DIFFERENT_VIDEO                 MAKE_HX_RESULT(1,SS_RMT,8)          // 800403c8
#define HXR_RMT_PASTE_MISSING_STREAM            MAKE_HX_RESULT(1,SS_RMT,9)          // 800403c9
#define HXR_RMT_END_OF_STREAM                   MAKE_HX_RESULT(1,SS_RMT,10)         // 800403ca
#define HXR_RMT_IMAGE_MAP_PARSE_ERROR           MAKE_HX_RESULT(1,SS_RMT,11)         // 800403cb
#define HXR_RMT_INVALID_IMAGEMAP_FILE           MAKE_HX_RESULT(1,SS_RMT,12)         // 800403cc
#define HXR_RMT_EVENT_PARSE_ERROR               MAKE_HX_RESULT(1,SS_RMT,13)         // 800403cd
#define HXR_RMT_INVALID_EVENT_FILE              MAKE_HX_RESULT(1,SS_RMT,14)         // 800403ce
#define HXR_RMT_INVALID_OUTPUT_FILE             MAKE_HX_RESULT(1,SS_RMT,15)         // 800403cf
#define HXR_RMT_INVALID_DURATION                MAKE_HX_RESULT(1,SS_RMT,16)         // 800403d0
#define HXR_RMT_NO_DUMP_FILES                   MAKE_HX_RESULT(1,SS_RMT,17)         // 800403d1
#define HXR_RMT_NO_EVENT_DUMP_FILE              MAKE_HX_RESULT(1,SS_RMT,18)         // 800403d2
#define HXR_RMT_NO_IMAP_DUMP_FILE               MAKE_HX_RESULT(1,SS_RMT,19)         // 800403d3
#define HXR_RMT_NO_DATA                         MAKE_HX_RESULT(1,SS_RMT,20)         // 800403d4
#define HXR_RMT_EMPTY_STREAM                    MAKE_HX_RESULT(1,SS_RMT,21)         // 800403d5
#define HXR_RMT_READ_ONLY_FILE                  MAKE_HX_RESULT(1,SS_RMT,22)         // 800403d6
#define HXR_RMT_PASTE_MISSING_AUDIO_STREAM      MAKE_HX_RESULT(1,SS_RMT,23)         // 800403d7
#define HXR_RMT_PASTE_MISSING_VIDEO_STREAM      MAKE_HX_RESULT(1,SS_RMT,24)         // 800403d8
#define HXR_RMT_ENCRYPTED_CONTENT               MAKE_HX_RESULT(1,SS_RMT,25)         // 800403d9

#define HXR_PROP_NOT_FOUND              MAKE_HX_RESULT(1,SS_REG,1)                  // 80040281
#define HXR_PROP_NOT_COMPOSITE          MAKE_HX_RESULT(1,SS_REG,2)                  // 80040282
#define HXR_PROP_DUPLICATE              MAKE_HX_RESULT(1,SS_REG,3)                  // 80040283
#define HXR_PROP_TYPE_MISMATCH          MAKE_HX_RESULT(1,SS_REG,4)                  // 80040284
#define HXR_PROP_ACTIVE                 MAKE_HX_RESULT(1,SS_REG,5)                  // 80040285
#define HXR_PROP_INACTIVE               MAKE_HX_RESULT(1,SS_REG,6)                  // 80040286
#define HXR_PROP_VAL_UNDERFLOW          MAKE_HX_RESULT(1,SS_REG,7)                  // 80040287
#define HXR_PROP_VAL_OVERFLOW           MAKE_HX_RESULT(1,SS_REG,8)                  // 80040288
#define HXR_PROP_VAL_LT_LBOUND          MAKE_HX_RESULT(1,SS_REG,9)                  // 80040289
#define HXR_PROP_VAL_GT_UBOUND          MAKE_HX_RESULT(1,SS_REG,10)                 // 8004028a
#define HXR_PROP_DELETE_PENDING         MAKE_HX_RESULT(0,SS_REG,11)                 // 0004028b

#define HXR_COULDNOTINITCORE            MAKE_HX_RESULT(1,SS_MSC,1)                  // 800401c1
#define HXR_PERFECTPLAY_NOT_SUPPORTED   MAKE_HX_RESULT(1,SS_MSC,2)                  // 800401c2
#define HXR_NO_LIVE_PERFECTPLAY         MAKE_HX_RESULT(1,SS_MSC,3)                  // 800401c3
#define HXR_PERFECTPLAY_NOT_ALLOWED     MAKE_HX_RESULT(1,SS_MSC,4)                  // 800401c4
#define HXR_NO_CODECS                   MAKE_HX_RESULT(1,SS_MSC,5)                  // 800401c5
#define HXR_SLOW_MACHINE                MAKE_HX_RESULT(1,SS_MSC,6)                  // 800401c6
#define HXR_FORCE_PERFECTPLAY           MAKE_HX_RESULT(1,SS_MSC,7)                  // 800401c7
#define HXR_INVALID_HTTP_PROXY_HOST     MAKE_HX_RESULT(1,SS_MSC,8)                  // 800401c8
#define HXR_INVALID_METAFILE            MAKE_HX_RESULT(1,SS_MSC,9)                  // 800401c9
#define HXR_BROWSER_LAUNCH              MAKE_HX_RESULT(1,SS_MSC,10)                 // 800401ca
#define HXR_VIEW_SOURCE_NOCLIP          MAKE_HX_RESULT(1,SS_MSC,11)                 // 800401cb
#define HXR_VIEW_SOURCE_DISSABLED       MAKE_HX_RESULT(1,SS_MSC,12)                 // 800401cc
#define HXR_TIMELINE_SUSPENDED          MAKE_HX_RESULT(1,SS_MSC,14)                 // 800401ce
#define HXR_BUFFER_NOT_AVAILABLE        MAKE_HX_RESULT(1,SS_MSC,15)                 // 800401cf
#define HXR_COULD_NOT_DISPLAY           MAKE_HX_RESULT(1,SS_MSC,16)                 // 800401d0
#define HXR_VSRC_DISABLED               MAKE_HX_RESULT(1,SS_MSC,17)                 // 800401d1
#define HXR_VSRC_NOCLIP                 MAKE_HX_RESULT(1,SS_MSC,18)                 // 800401d2

#define HXR_RESOURCE_NOT_CACHED         MAKE_HX_RESULT(1,SS_RSC,1)                  // 80040301
#define HXR_RESOURCE_NOT_FOUND          MAKE_HX_RESULT(1,SS_RSC,2)                  // 80040302
#define HXR_RESOURCE_CLOSE_FILE_FIRST   MAKE_HX_RESULT(1,SS_RSC,3)                  // 80040303
#define HXR_RESOURCE_NODATA             MAKE_HX_RESULT(1,SS_RSC,4)                  // 80040304
#define HXR_RESOURCE_BADFILE            MAKE_HX_RESULT(1,SS_RSC,5)                  // 80040305
#define HXR_RESOURCE_PARTIALCOPY        MAKE_HX_RESULT(1,SS_RSC,6)                  // 80040306

#define HXR_PPV_NO_USER                 MAKE_HX_RESULT(1,SS_PPV,0)                  // 800402c0
#define HXR_PPV_GUID_READ_ONLY          MAKE_HX_RESULT(1,SS_PPV,1)                  // 800402c1
#define HXR_PPV_GUID_COLLISION          MAKE_HX_RESULT(1,SS_PPV,2)                  // 800402c2
#define HXR_REGISTER_GUID_EXISTS        MAKE_HX_RESULT(1,SS_PPV,3)                  // 800402c3
#define HXR_PPV_AUTHORIZATION_FAILED    MAKE_HX_RESULT(1,SS_PPV,4)                  // 800402c4
#define HXR_PPV_OLD_PLAYER              MAKE_HX_RESULT(1,SS_PPV,5)                  // 800402c5
#define HXR_PPV_ACCOUNT_LOCKED          MAKE_HX_RESULT(1,SS_PPV,6)                  // 800402c6
// #define HXR_PPV_PROTOCOL_IGNORES     MAKE_HX_RESULT(1,SS_PPV,7)
#define HXR_PPV_DBACCESS_ERROR          MAKE_HX_RESULT(1,SS_PPV,8)                  // 800402c8
#define HXR_PPV_USER_ALREADY_EXISTS     MAKE_HX_RESULT(1,SS_PPV,9)                  // 800402c9

// auto-upgrade (RealUpdate) errors
#define HXR_UPG_AUTH_FAILED             MAKE_HX_RESULT(1,SS_UPG,0)                  // 80040340
#define HXR_UPG_CERT_AUTH_FAILED        MAKE_HX_RESULT(1,SS_UPG,1)                  // 80040341
#define HXR_UPG_CERT_EXPIRED            MAKE_HX_RESULT(1,SS_UPG,2)                  // 80040342
#define HXR_UPG_CERT_REVOKED            MAKE_HX_RESULT(1,SS_UPG,3)                  // 80040343
#define HXR_UPG_RUP_BAD                 MAKE_HX_RESULT(1,SS_UPG,4)                  // 80040344
#define HXR_UPG_SYSTEM_BUSY             MAKE_HX_RESULT(1,SS_UPG,5)                  // 80040345

// auto-config errors
#define HXR_AUTOCFG_SUCCESS             MAKE_HX_RESULT(1,SS_CFG,0)
#define HXR_AUTOCFG_FAILED              MAKE_HX_RESULT(1,SS_CFG,1)
#define HXR_AUTOCFG_ABORT               MAKE_HX_RESULT(1,SS_CFG,2)

//producer activex errors.
#define HXR_ENC_AX_INIT_FAILED                  MAKE_HX_RESULT(1,SS_ENC_AX,0)
#define HXR_ENC_AX_NOTVALID_WHILE_ENCODING      MAKE_HX_RESULT(1,SS_ENC_AX,1)
#define HXR_ENC_AX_REALMEDIAEVENTS_DISABLED     MAKE_HX_RESULT(1,SS_ENC_AX,2)
#define HXR_ENC_AX_EVENT_START_TIME_DECREASING  MAKE_HX_RESULT(1,SS_ENC_AX,3)
#define HXR_ENC_AX_FAILED_MEDIASINK_INPUT       MAKE_HX_RESULT(1,SS_ENC_AX,4)
#define HXR_ENC_AX_INVALID_EVENT_TYPE           MAKE_HX_RESULT(1,SS_ENC_AX,5)
#define HXR_ENC_AX_JOB_NOT_SET                  MAKE_HX_RESULT(1,SS_ENC_AX,6)
#define HXR_ENC_AX_NOTVALID_WHILE_NOTENCODING   MAKE_HX_RESULT(1,SS_ENC_AX,7)
#define HXR_ENC_AX_NO_AUDIO_GAIN_SET            MAKE_HX_RESULT(1,SS_ENC_AX,8)
#define HXR_ENC_AX_UPDATE_CODECS_FAILED         MAKE_HX_RESULT(1,SS_ENC_AX,9)
#define HXR_ENC_AX_EVENT_LATE                   MAKE_HX_RESULT(1,SS_ENC_AX,10)
#define HXR_ENC_AX_EVENT_INVALID_START_TIME     MAKE_HX_RESULT(1,SS_ENC_AX,11)
#define HXR_ENC_AX_JOB_START_TIME_NOT_SET       MAKE_HX_RESULT(1,SS_ENC_AX,12)
#define HXR_ENC_AX_INVALID_JOB_FILE             MAKE_HX_RESULT(1,SS_ENC_AX,13)
#define HXR_ENC_AX_INVALID_JOB_XML              MAKE_HX_RESULT(1,SS_ENC_AX,14)
#define HXR_ENC_AX_INVALID_ARGUMENTS            MAKE_HX_RESULT(1,SS_ENC_AX,15)

// RealPix errors
#define HXR_UNKNOWN_IMAGE               MAKE_HX_RESULT(1,SS_RPX,0)
#define HXR_UNKNOWN_EFFECT              MAKE_HX_RESULT(1,SS_RPX,1)
#define HXR_SENDIMAGE_ABORTED           MAKE_HX_RESULT(0,SS_RPX,2)
#define HXR_SENDEFFECT_ABORTED          MAKE_HX_RESULT(0,SS_RPX,3)

// server alert errors
#define HXR_SE_MIN_VALUE                    MAKE_HX_RESULT(1, SS_SAM, 0)            // 80041800
#define HXR_SE_NO_ERROR                     MAKE_HX_RESULT(1, SS_SAM, 1)            // 80041901
#define HXR_SE_INVALID_VERSION              MAKE_HX_RESULT(1, SS_SAM, 2)            // 80041902
#define HXR_SE_INVALID_FORMAT               MAKE_HX_RESULT(1, SS_SAM, 3)            // 80041903
#define HXR_SE_INVALID_BANDWIDTH            MAKE_HX_RESULT(1, SS_SAM, 4)            // 80041904
#define HXR_SE_INVALID_PATH                 MAKE_HX_RESULT(1, SS_SAM, 5)            // 80041905
#define HXR_SE_UNKNOWN_PATH                 MAKE_HX_RESULT(1, SS_SAM, 6)            // 80041906
#define HXR_SE_INVALID_PROTOCOL             MAKE_HX_RESULT(1, SS_SAM, 7)            // 80041907
#define HXR_SE_INVALID_PLAYER_ADDR          MAKE_HX_RESULT(1, SS_SAM, 8)            // 80041908
#define HXR_SE_LOCAL_STREAMS_PROHIBITED     MAKE_HX_RESULT(1, SS_SAM, 9)            // 80041909
#define HXR_SE_SERVER_FULL                  MAKE_HX_RESULT(1, SS_SAM, 10)           // 8004190a
#define HXR_SE_REMOTE_STREAMS_PROHIBITED    MAKE_HX_RESULT(1, SS_SAM, 11)           // 8004190b
#define HXR_SE_EVENT_STREAMS_PROHIBITED     MAKE_HX_RESULT(1, SS_SAM, 12)           // 8004190c
#define HXR_SE_INVALID_HOST                 MAKE_HX_RESULT(1, SS_SAM, 13)           // 8004190d
#define HXR_SE_NO_CODEC                     MAKE_HX_RESULT(1, SS_SAM, 14)           // 8004190e
#define HXR_SE_LIVEFILE_INVALID_BWN         MAKE_HX_RESULT(1, SS_SAM, 15)           // 8004190f
#define HXR_SE_UNABLE_TO_FULFILL            MAKE_HX_RESULT(1, SS_SAM, 16)           // 80041910
#define HXR_SE_MULTICAST_DELIVERY_ONLY      MAKE_HX_RESULT(1, SS_SAM, 17)           // 80041911
#define HXR_SE_LICENSE_EXCEEDED             MAKE_HX_RESULT(1, SS_SAM, 18)           // 80041912
#define HXR_SE_LICENSE_UNAVAILABLE          MAKE_HX_RESULT(1, SS_SAM, 19)           // 80041913
#define HXR_SE_INVALID_LOSS_CORRECTION      MAKE_HX_RESULT(1, SS_SAM, 20)           // 80041914
#define HXR_SE_PROTOCOL_FAILURE             MAKE_HX_RESULT(1, SS_SAM, 21)           // 80041915
#define HXR_SE_REALVIDEO_STREAMS_PROHIBITED MAKE_HX_RESULT(1, SS_SAM, 22)           // 80041916
#define HXR_SE_REALAUDIO_STREAMS_PROHIBITED MAKE_HX_RESULT(1, SS_SAM, 23)           // 80041917
#define HXR_SE_DATATYPE_UNSUPPORTED         MAKE_HX_RESULT(1, SS_SAM, 24)           // 80041918
#define HXR_SE_DATATYPE_UNLICENSED          MAKE_HX_RESULT(1, SS_SAM, 25)           // 80041919
#define HXR_SE_RESTRICTED_PLAYER            MAKE_HX_RESULT(1, SS_SAM, 26)           // 8004191a
#define HXR_SE_STREAM_INITIALIZING          MAKE_HX_RESULT(1, SS_SAM, 27)           // 8004191b
#define HXR_SE_INVALID_PLAYER               MAKE_HX_RESULT(1, SS_SAM, 28)           // 8004191c
#define HXR_SE_PLAYER_PLUS_ONLY             MAKE_HX_RESULT(1, SS_SAM, 29)           // 8004191d
#define HXR_SE_NO_EMBEDDED_PLAYERS          MAKE_HX_RESULT(1, SS_SAM, 30)           // 8004191e
#define HXR_SE_PNA_PROHIBITED               MAKE_HX_RESULT(1, SS_SAM, 31)           // 8004191f
#define HXR_SE_AUTHENTICATION_UNSUPPORTED   MAKE_HX_RESULT(1, SS_SAM, 32)           // 80041920
#define HXR_SE_MAX_FAILED_AUTHENTICATIONS   MAKE_HX_RESULT(1, SS_SAM, 33)           // 80041921
#define HXR_SE_AUTH_ACCESS_DENIED           MAKE_HX_RESULT(1, SS_SAM, 34)           // 80041922
#define HXR_SE_AUTH_UUID_READ_ONLY          MAKE_HX_RESULT(1, SS_SAM, 35)           // 80041923
#define HXR_SE_AUTH_UUID_NOT_UNIQUE         MAKE_HX_RESULT(1, SS_SAM, 36)           // 80041924
#define HXR_SE_AUTH_NO_SUCH_USER            MAKE_HX_RESULT(1, SS_SAM, 37)           // 80041925
#define HXR_SE_AUTH_REGISTRATION_SUCCEEDED  MAKE_HX_RESULT(1, SS_SAM, 38)           // 80041926
#define HXR_SE_AUTH_REGISTRATION_FAILED     MAKE_HX_RESULT(1, SS_SAM, 39)           // 80041927
#define HXR_SE_AUTH_REGISTRATION_GUID_REQUIRED MAKE_HX_RESULT(1, SS_SAM, 40)        // 80041928
#define HXR_SE_AUTH_UNREGISTERED_PLAYER     MAKE_HX_RESULT(1, SS_SAM, 41)           // 80041929
#define HXR_SE_AUTH_TIME_EXPIRED            MAKE_HX_RESULT(1, SS_SAM, 42)           // 8004192a
#define HXR_SE_AUTH_NO_TIME_LEFT            MAKE_HX_RESULT(1, SS_SAM, 43)           // 8004192b
#define HXR_SE_AUTH_ACCOUNT_LOCKED          MAKE_HX_RESULT(1, SS_SAM, 44)           // 8004192c
#define HXR_SE_AUTH_INVALID_SERVER_CFG      MAKE_HX_RESULT(1, SS_SAM, 45)           // 8004192d
#define HXR_SE_NO_MOBILE_DOWNLOAD           MAKE_HX_RESULT(1, SS_SAM, 46)           // 8004192e
#define HXR_SE_NO_MORE_MULTI_ADDR           MAKE_HX_RESULT(1, SS_SAM, 47)           // 8004192f
#define HXR_PE_PROXY_MAX_CONNECTIONS        MAKE_HX_RESULT(1, SS_SAM, 48)           // 80041930
#define HXR_PE_PROXY_MAX_GW_BANDWIDTH       MAKE_HX_RESULT(1, SS_SAM, 49)           // 80041931
#define HXR_PE_PROXY_MAX_BANDWIDTH          MAKE_HX_RESULT(1, SS_SAM, 50)           // 80041932
#define HXR_SE_BAD_LOADTEST_PASSWORD        MAKE_HX_RESULT(1, SS_SAM, 51)           // 80041933
#define HXR_SE_PNA_NOT_SUPPORTED            MAKE_HX_RESULT(1, SS_SAM, 52)           // 80041934
#define HXR_PE_PROXY_ORIGIN_DISCONNECTED    MAKE_HX_RESULT(1, SS_SAM, 53)           // 80041935
#define HXR_SE_INTERNAL_ERROR               MAKE_HX_RESULT(1, SS_SAM, 54)           // 80041936
#define HXR_SE_MAX_VALUE                    MAKE_HX_RESULT(1, SS_SAM, 55)           // 80041937


#define HXR_SOCK_INTR                   MAKE_HX_RESULT(1, SS_SOCK, 0)               // 80040600
#define HXR_SOCK_BADF                   MAKE_HX_RESULT(1, SS_SOCK, 1)               // 80040601
#define HXR_SOCK_ACCES                  MAKE_HX_RESULT(1, SS_SOCK, 2)               // 80040602
#define HXR_SOCK_FAULT                  MAKE_HX_RESULT(1, SS_SOCK, 3)               // 80040603
#define HXR_SOCK_INVAL                  MAKE_HX_RESULT(1, SS_SOCK, 4)               // 80040604
#define HXR_SOCK_MFILE                  MAKE_HX_RESULT(1, SS_SOCK, 5)               // 80040605
#define HXR_SOCK_WOULDBLOCK             MAKE_HX_RESULT(1, SS_SOCK, 6)               // 80040606
#define HXR_SOCK_INPROGRESS             MAKE_HX_RESULT(1, SS_SOCK, 7)               // 80040607
#define HXR_SOCK_ALREADY                MAKE_HX_RESULT(1, SS_SOCK, 8)               // 80040608
#define HXR_SOCK_NOTSOCK                MAKE_HX_RESULT(1, SS_SOCK, 9)               // 80040609
#define HXR_SOCK_DESTADDRREQ            MAKE_HX_RESULT(1, SS_SOCK, 10)              // 8004060a
#define HXR_SOCK_MSGSIZE                MAKE_HX_RESULT(1, SS_SOCK, 11)              // 8004060b
#define HXR_SOCK_PROTOTYPE              MAKE_HX_RESULT(1, SS_SOCK, 12)              // 8004060c
#define HXR_SOCK_NOPROTOOPT             MAKE_HX_RESULT(1, SS_SOCK, 13)              // 8004060d
#define HXR_SOCK_PROTONOSUPPORT         MAKE_HX_RESULT(1, SS_SOCK, 14)              // 8004060e
#define HXR_SOCK_SOCKTNOSUPPORT         MAKE_HX_RESULT(1, SS_SOCK, 15)              // 8004060f
#define HXR_SOCK_OPNOTSUPP              MAKE_HX_RESULT(1, SS_SOCK, 16)              // 80040610
#define HXR_SOCK_PFNOSUPPORT            MAKE_HX_RESULT(1, SS_SOCK, 17)              // 80040611
#define HXR_SOCK_AFNOSUPPORT            MAKE_HX_RESULT(1, SS_SOCK, 18)              // 80040612
#define HXR_SOCK_ADDRINUSE              MAKE_HX_RESULT(1, SS_SOCK, 19)              // 80040613
#define HXR_SOCK_ADDRNOTAVAIL           MAKE_HX_RESULT(1, SS_SOCK, 20)              // 80040614
#define HXR_SOCK_NETDOWN                MAKE_HX_RESULT(1, SS_SOCK, 21)              // 80040615
#define HXR_SOCK_NETUNREACH             MAKE_HX_RESULT(1, SS_SOCK, 22)              // 80040616
#define HXR_SOCK_NETRESET               MAKE_HX_RESULT(1, SS_SOCK, 23)              // 80040617
#define HXR_SOCK_CONNABORTED            MAKE_HX_RESULT(1, SS_SOCK, 24)              // 80040618
#define HXR_SOCK_CONNRESET              MAKE_HX_RESULT(1, SS_SOCK, 25)              // 80040619
#define HXR_SOCK_NOBUFS                 MAKE_HX_RESULT(1, SS_SOCK, 26)              // 8004061a
#define HXR_SOCK_ISCONN                 MAKE_HX_RESULT(1, SS_SOCK, 27)              // 8004061b
#define HXR_SOCK_NOTCONN                MAKE_HX_RESULT(1, SS_SOCK, 28)              // 8004061c
#define HXR_SOCK_SHUTDOWN               MAKE_HX_RESULT(1, SS_SOCK, 29)              // 8004061d
#define HXR_SOCK_TOOMANYREFS            MAKE_HX_RESULT(1, SS_SOCK, 30)              // 8004061e
#define HXR_SOCK_TIMEDOUT               MAKE_HX_RESULT(1, SS_SOCK, 31)              // 8004061f
#define HXR_SOCK_CONNREFUSED            MAKE_HX_RESULT(1, SS_SOCK, 32)              // 80040620
#define HXR_SOCK_LOOP                   MAKE_HX_RESULT(1, SS_SOCK, 33)              // 80040621
#define HXR_SOCK_NAMETOOLONG            MAKE_HX_RESULT(1, SS_SOCK, 34)              // 80040622
#define HXR_SOCK_HOSTDOWN               MAKE_HX_RESULT(1, SS_SOCK, 35)              // 80040623
#define HXR_SOCK_HOSTUNREACH            MAKE_HX_RESULT(1, SS_SOCK, 36)              // 80040624
#define HXR_SOCK_PIPE                   MAKE_HX_RESULT(1, SS_SOCK, 37)              // 80040625
#define HXR_SOCK_ENDSTREAM              MAKE_HX_RESULT(1, SS_SOCK, 38)              // 80040626
#define HXR_SOCK_BUFFERED               MAKE_HX_RESULT(0, SS_SOCK, 39)              // 00040627


#define HXR_RSLV_NONAME                 MAKE_HX_RESULT(1, SS_RSLV, 0)               // 80040640
#define HXR_RSLV_NODATA                 MAKE_HX_RESULT(1, SS_RSLV, 1)               // 80040641

#define SA_OFFSET 2
#define MAKE_SA(sa) HXR_SE_MIN_VALUE+sa+SA_OFFSET
#define IS_SERVER_ALERT(sa) ((HXR_SE_MIN_VALUE < sa && sa < HXR_SE_MAX_VALUE) || sa == HXR_SERVER_ALERT)


#define HXR_FAILED                      HXR_FAIL

#ifdef _WIN16
/*typedef UINT                          MMRESULT;*/
#else
#ifdef _WIN32

#if defined(WIN32_PLATFORM_PSPC)
#undef _HRESULT_TYPEDEF_
#undef E_NOTIMPL
#undef E_OUTOFMEMORY
#undef E_INVALIDARG
#undef E_NOINTERFACE
#undef E_POINTER
#undef E_HANDLE
#undef E_ABORT
#undef E_FAIL
#undef E_ACCESSDENIED
#endif /* defined(WIN32_PLATFORM_PSPC) */
#define _HRESULT_TYPEDEF_(_sc) ((HRESULT)_sc)
#define E_NOTIMPL                        _HRESULT_TYPEDEF_(0x80004001L)
#define E_OUTOFMEMORY                    _HRESULT_TYPEDEF_(0x8007000EL)
#define E_INVALIDARG                     _HRESULT_TYPEDEF_(0x80070057L)
#define E_NOINTERFACE                    _HRESULT_TYPEDEF_(0x80004002L)
#define E_POINTER                        _HRESULT_TYPEDEF_(0x80004003L)
#define E_HANDLE                         _HRESULT_TYPEDEF_(0x80070006L)
#define E_ABORT                          _HRESULT_TYPEDEF_(0x80004004L)
#define E_FAIL                           _HRESULT_TYPEDEF_(0x80004005L)
#define E_ACCESSDENIED                   _HRESULT_TYPEDEF_(0x80070005L)
#else
#define S_OK                    HXR_OK
#define E_NOTIMPL               HXR_NOTIMPL
#define E_INVALIDARG            HXR_INVALID_PARAMETER
#define E_NOINTERFACE           HXR_NOINTERFACE
#define E_POINTER               HXR_POINTER
#define E_HANDLE                HXR_HANDLE
#define E_ABORT                 HXR_ABORT
#define E_FAIL                  HXR_FAIL
#define E_ACCESSDENIES          HXR_ACCESSDENIED
#endif  /* _WIN32 */
#endif  /* _WIN16 */

#define HX_STATUS_OK            HXR_OK
#define HX_STATUS_FAILED        E_FAIL

#endif /* _HXRESULT_H_ */
