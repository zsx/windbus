/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */
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
#undef interface

#include <aclapi.h>
#include <lm.h>
#include <io.h>
#include <share.h>
#include <direct.h>

#define mkdir(path, mode) _mkdir (path)

#ifndef DBUS_WINCE
#ifndef S_ISREG
#define S_ISREG(mode) (((mode) & S_IFMT) == S_IFREG)
#endif
#endif

/* Declarations missing in mingw's headers */
extern BOOL WINAPI ConvertStringSidToSidA (LPCSTR  StringSid, PSID *Sid);
extern BOOL WINAPI ConvertSidToStringSidA (PSID Sid, LPSTR *StringSid);


#define DBUS_CONSOLE_DIR "/var/run/console/"


void _dbus_win_startup_winsock (void);
void _dbus_win_warn_win_error  (const char *message,
                                int         code);
extern const char* _dbus_lm_strerror  (int error_number);


dbus_bool_t _dbus_win_account_to_sid (const wchar_t *waccount,
                                      void         **ppsid,
                                      DBusError     *error);

dbus_bool_t
_dbus_win32_sid_to_name_and_domain (dbus_uid_t  uid,
                                    wchar_t   **wname,
                                    wchar_t   **wdomain,
                                    DBusError  *error);


/* Don't define DBUS_CONSOLE_DIR on Win32 */

wchar_t    *_dbus_win_utf8_to_utf16 (const char  *str,
                                     DBusError   *error);
char       *_dbus_win_utf16_to_utf8 (const wchar_t *str,
                                     DBusError *error);

void        _dbus_win_set_error_from_win_error (DBusError *error, int code);

dbus_bool_t
_dbus_win_sid_to_name_and_domain (dbus_uid_t uid,
                                  wchar_t  **wname,
                                  wchar_t  **wdomain,
                                  DBusError *error);

typedef struct DBusFile DBusFile;

dbus_bool_t _dbus_file_open (DBusFile   *file,
                             const char *filename,
                             int         oflag,
                             int         pmode);

dbus_bool_t _dbus_file_close (DBusFile  *file,
                              DBusError *error);


int _dbus_file_read  (DBusFile   *file,
                      DBusString *buffer,
                      int         count);

int _dbus_file_write (DBusFile         *file,
                      const DBusString *buffer,
                      int               start,
                      int               len);

dbus_bool_t _dbus_file_exists (const char *filename);


#define FDATA private_data
struct DBusFile
  {
    int FDATA;
  };


dbus_bool_t _dbus_get_config_file_name(DBusString *config_file, 
                                       char *s);



#endif

/** @} end of sysdeps-win.h */


