/* -*- mode: C; c-file-style: "gnu" -*- */
/* dbus-sysdeps.c Wrappers around system/libc features (internal to D-BUS implementation)
 * 
 * Copyright (C) 2002, 2003  Red Hat, Inc.
 * Copyright (C) 2003 CodeFactory AB
 * Copyright (C) 2005 Novell, Inc.
 *
 * Licensed under the Academic Free License version 2.1
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#ifndef DBUS_SYSDEPS_WIN_H
#define DBUS_SYSDEPS_WIN_H

#define _WINSOCKAPI_

#include "dbus-hash.h"
#include "dbus-string.h"
#include <ctype.h>
#include <malloc.h>
#include <windows.h>
#include <aclapi.h>
#include <lm.h>
#include <io.h>
#include <share.h>
#include <direct.h>

#define mkdir(path, mode) _mkdir (path)

#ifndef S_ISREG
#define    S_ISREG(mode) (((mode) & S_IFMT) == S_IFREG)
#endif

/* Declarations missing in mingw's headers */
extern BOOL WINAPI ConvertStringSidToSidA (LPCSTR  StringSid, PSID *Sid);
extern BOOL WINAPI ConvertSidToStringSidA (PSID Sid, LPSTR *StringSid);

typedef enum
{
  DBUS_WIN_FD_UNUSED,
  DBUS_WIN_FD_BEING_OPENED,
  DBUS_WIN_FD_C_LIB,           /* Unix-style file descriptor
                                * (implemented by the C runtime)
                                */
  DBUS_WIN_FD_SOCKET,            /* Winsock SOCKET */
  DBUS_WIN_FD_NAMED_PIPE_HANDLE /* HANDLE for a named pipe */
} DBusWin32FDType;

#define DBUS_CONSOLE_DIR "/var/run/console/"     

typedef struct
{
  DBusWin32FDType type;
  int fd;               /* File descriptor, SOCKET or file HANDLE */
  int port_file_fd;        /* File descriptor for file containing
                         * port number for "pseudo-unix" sockets
                         */
  DBusString port_file; /* File name for said file */
  dbus_bool_t close_on_exec;
  dbus_bool_t non_blocking;
} DBusWin32FD;

extern DBusWin32FD *win_fds;
extern int win32_n_fds;


#if 0
#define RANDOMIZE(n) ((n)^win32_encap_randomizer)
#define UNRANDOMIZE(n) ((n)^win32_encap_randomizer)
#else
#define TO_HANDLE(n)   ((n)+0x10000000)
#define FROM_HANDLE(n) ((n)-0x10000000)
#define IS_HANDLE(n)   ((n)&0x10000000)

#endif

#define _dbus_decapsulate_quick(i) win_fds[UNRANDOMIZE (i)].fd

extern const char*
_dbus_strerror_win (int error_number);
int  _dbus_win_allocate_fd     (void);
void _dbus_win_deallocate_fd   (int  fd);
void _dbus_win_startup_winsock (void);
void _dbus_win_warn_win_error  (const char *message,
                                int         code);
extern const char* _dbus_lm_strerror  (int error_number);


int 
_dbus_write_two_win (int               fd,
                     const DBusString *buffer1,
                     int               start1,
                     int               len1,
                     const DBusString *buffer2,
                     int               start2,
                     int               len2);
int
_dbus_read_win (int               fd,
                DBusString       *buffer,
                int               count);
int
_dbus_write_win (int               fd,
                 const DBusString *buffer,
                 int               start,
                 int               len);

int
_dbus_connect_unix_socket_win (const char     *path,
                               dbus_bool_t     abstract,
                               DBusError      *error);
int
_dbus_listen_unix_socket_win (const char     *path,
                              dbus_bool_t     abstract,
                              DBusError      *error);
dbus_bool_t
fill_win_user_info_from_uid (dbus_uid_t    uid,
                             DBusUserInfo *info,
                             DBusError    *error);
dbus_bool_t
fill_win_user_info_from_name (wchar_t      *wname,
                              DBusUserInfo *info,
                              DBusError    *error);

dbus_bool_t _dbus_win_account_to_sid (const wchar_t *waccount,
                                      void         **ppsid,
                                      DBusError     *error);

dbus_bool_t 
_dbus_win32_sid_to_name_and_domain (dbus_uid_t  uid,
                                    wchar_t   **wname,
                                    wchar_t   **wdomain,
                                    DBusError  *error);
dbus_bool_t
_dbus_full_duplex_pipe_win (int         *fd1,
                            int         *fd2,
                            dbus_bool_t  blocking,
                            DBusError   *error);

/* Don't define DBUS_CONSOLE_DIR on Win32 */                                                                                       
                                                                                                                                  
wchar_t    *_dbus_win_utf8_to_utf16 (const char  *str, 
                                     DBusError   *error);
char       *_dbus_win_utf16_to_utf8 (const wchar_t *str, 
                                     DBusError *error);                                                                                              
                                                                                                                                  
void        _dbus_win_set_error_from_win_error (DBusError *error, int code); 

dbus_uid_t  _dbus_win_sid_to_uid_t (void        *psid);        
dbus_bool_t _dbus_uid_t_to_win_sid (dbus_uid_t   uid, 
                                    void       **ppsid);
dbus_bool_t
_dbus_account_to_win_sid (const wchar_t  *waccount,
                          void          **ppsid,
                          DBusError      *error);
dbus_bool_t
_dbus_win_sid_to_name_and_domain (dbus_uid_t uid,
                                  wchar_t  **wname,
                                  wchar_t  **wdomain,
                                  DBusError *error);

dbus_uid_t    _dbus_getuid_win (void);
dbus_gid_t    _dbus_getgid_win (void);

dbus_bool_t   _dbus_close (int        fd,
                           DBusError *error);
int
_dbus_poll_win (DBusPollFD *fds,
                int         n_fds,
                int         timeout_milliseconds);
void
_dbus_fd_set_close_on_exec_win (int fd);

dbus_bool_t
_dbus_close_win (int        fd,
                 DBusError *error);
dbus_bool_t
_dbus_set_fd_nonblocking_win (int         fd,
                              DBusError  *error);

#endif

/** @} end of sysdeps-win.h */


