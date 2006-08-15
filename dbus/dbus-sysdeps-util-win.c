/* -*- mode: C; c-file-style: "gnu" -*- */
/* dbus-sysdeps-util.c Would be in dbus-sysdeps.c, but not used in libdbus
 * 
 * Copyright (C) 2002, 2003, 2004, 2005  Red Hat, Inc.
 * Copyright (C) 2003 CodeFactory AB
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
#include "dbus-sysdeps.h"
#include "dbus-internals.h"
#include "dbus-protocol.h"
#include "dbus-string.h"
#define DBUS_USERDB_INCLUDES_PRIVATE 1
#include "dbus-userdb.h"
#include "dbus-test.h"

#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#ifdef HAVE_GRP_H
#include <grp.h>
#endif

#ifdef DBUS_WIN
#include <aclapi.h>
#define F_OK 0
#endif

#ifndef O_BINARY
#define O_BINARY 0
#endif


/**
 * Does the chdir, fork, setsid, etc. to become a daemon process.
 *
 * @param pidfile #NULL, or pidfile to create
 * @param print_pid_fd file descriptor to print daemon's pid to, or -1 for none
 * @param error return location for errors
 * @returns #FALSE on failure
 */
dbus_bool_t
_dbus_become_daemon (const DBusString *pidfile,
		     int               print_pid_fd,
                     DBusError        *error)
{
  return TRUE;
}

/**
 * Changes the user and group the bus is running as.
 *
 * @param uid the new user ID
 * @param gid the new group ID
 * @param error return location for errors
 * @returns #FALSE on failure
 */
dbus_bool_t
_dbus_change_identity  (dbus_uid_t     uid,
                        dbus_gid_t     gid,
                        DBusError     *error)
{
  return TRUE;
}

/** Checks if user is at the console
*
* @param username user to check
* @param error return location for errors
* @returns #TRUE is the user is at the consolei and there are no errors
*/
dbus_bool_t 
_dbus_user_at_console(const char *username,
                       DBusError  *error)
{
  dbus_bool_t retval = FALSE;
  wchar_t *wusername;
  DWORD sid_length;
  PSID user_sid, console_user_sid;
  HWINSTA winsta;

  wusername = _dbus_win_utf8_to_utf16 (username, error);
  if (!wusername)
    return FALSE;

  if (!_dbus_win_account_to_sid (wusername, &user_sid, error))
    goto out0;

  /* Now we have the SID for username. Get the SID of the
   * user at the "console" (window station WinSta0)
   */
  if (!(winsta = OpenWindowStation ("WinSta0", FALSE, READ_CONTROL)))
    {
      _dbus_win_set_error_from_win_error (error, GetLastError ());
      goto out2;
    }

  sid_length = 0;
  GetUserObjectInformation (winsta, UOI_USER_SID,
			    NULL, 0, &sid_length);
  if (sid_length == 0)
    {
      /* Nobody is logged on */
      goto out2;
    }
  
  if (sid_length < 0 || sid_length > 1000)
    {
      dbus_set_error_const (error, DBUS_ERROR_FAILED, "Invalid SID length");
      goto out3;
    }

  console_user_sid = dbus_malloc (sid_length);
  if (!console_user_sid)
    {
      _DBUS_SET_OOM (error);
      goto out3;
    }

  if (!GetUserObjectInformation (winsta, UOI_USER_SID,
				 console_user_sid, sid_length, &sid_length))
    {
      _dbus_win_set_error_from_win_error (error, GetLastError ());
      goto out4;
    }

  if (!IsValidSid (console_user_sid))
    {
      dbus_set_error_const (error, DBUS_ERROR_FAILED, "Invalid SID");
      goto out4;
    }

  retval = EqualSid (user_sid, console_user_sid);

 out4:
  dbus_free (console_user_sid);
 out3:
  CloseWindowStation (winsta);
 out2:
  dbus_free (user_sid);
 out0:
  dbus_free (wusername);

  return retval;
}

/** Installs a signal handler
 *
 * @param sig the signal to handle
 * @param handler the handler
 */
void
_dbus_set_signal_handler (int               sig,
                          DBusSignalHandler handler)
{
  _dbus_warn ("_dbus_set_signal_handler() has to be implemented\n");
}

/**
 * stat() wrapper.
 *
 * @param filename the filename to stat
 * @param statbuf the stat info to fill in
 * @param error return location for error
 * @returns #FALSE if error was set
 */
dbus_bool_t
_dbus_stat(const DBusString *filename,
            DBusStat         *statbuf,
            DBusError        *error)
{
  const char *filename_c;
#ifndef DBUS_WIN
  struct stat sb;
#else
  WIN32_FILE_ATTRIBUTE_DATA wfad;
  char *lastdot;
  DWORD rc;
  PSID owner_sid, group_sid;
  PSECURITY_DESCRIPTOR sd;
#endif

  _DBUS_ASSERT_ERROR_IS_CLEAR (error);
  
  filename_c = _dbus_string_get_const_data (filename);

  if (!GetFileAttributesEx (filename_c, GetFileExInfoStandard, &wfad))
    {
      _dbus_win_set_error_from_win_error (error, GetLastError ());
      return FALSE;
    }

  if (wfad.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
    statbuf->mode = _S_IFDIR;
  else
    statbuf->mode = _S_IFREG;

  statbuf->mode |= _S_IREAD;
  if (wfad.dwFileAttributes & FILE_ATTRIBUTE_READONLY)
    statbuf->mode |= _S_IWRITE;

  lastdot = strrchr (filename_c, '.');
  if (lastdot && stricmp (lastdot, ".exe") == 0)
    statbuf->mode |= _S_IEXEC;

  statbuf->mode |= (statbuf->mode & 0700) >> 3;
  statbuf->mode |= (statbuf->mode & 0700) >> 6;

  statbuf->nlink = 1;
    
  sd = NULL;
  rc = GetNamedSecurityInfo ((char *) filename_c, SE_FILE_OBJECT,
			     OWNER_SECURITY_INFORMATION |
			     GROUP_SECURITY_INFORMATION,
			     &owner_sid, &group_sid,
			     NULL, NULL,
			     &sd);
  if (rc != ERROR_SUCCESS)
    {
      _dbus_win_set_error_from_win_error (error, rc);
      if (sd != NULL)
	LocalFree (sd);
      return FALSE;
    }

  statbuf->uid = _dbus_win_sid_to_uid_t (owner_sid);
  statbuf->gid = _dbus_win_sid_to_uid_t (group_sid);

  LocalFree (sd);
			
  statbuf->size = ((dbus_int64_t) wfad.nFileSizeHigh << 32) + wfad.nFileSizeLow;
  
  statbuf->atime =
    (((dbus_int64_t) wfad.ftLastAccessTime.dwHighDateTime << 32) +
     wfad.ftLastAccessTime.dwLowDateTime) / 10000000 - DBUS_INT64_CONSTANT (116444736000000000);

  statbuf->mtime = 
    (((dbus_int64_t) wfad.ftLastWriteTime.dwHighDateTime << 32) +
     wfad.ftLastWriteTime.dwLowDateTime) / 10000000 - DBUS_INT64_CONSTANT (116444736000000000);

  statbuf->ctime =
    (((dbus_int64_t) wfad.ftCreationTime.dwHighDateTime << 32) +
     wfad.ftCreationTime.dwLowDateTime) / 10000000 - DBUS_INT64_CONSTANT (116444736000000000);

  return TRUE;
}

/**
 * Checks whether the filename is an absolute path
 *
 * @param filename the filename
 * @returns #TRUE if an absolute path
 */
dbus_bool_t
_dbus_path_is_absolute (const DBusString *filename)
{
  if (_dbus_string_get_length (filename) > 0)
    return _dbus_string_get_byte (filename, 1) == ':';
  else
    return FALSE;
}


dbus_bool_t
fill_group_info(DBusGroupInfo    *info,
                 dbus_gid_t        gid,
                 const DBusString *groupname,
                 DBusError        *error)
{
  const char *group_c_str;

  _dbus_assert (groupname != NULL || gid != DBUS_GID_UNSET);
  _dbus_assert (groupname == NULL || gid == DBUS_GID_UNSET);

  if (groupname)
    group_c_str = _dbus_string_get_const_data (groupname);
  else
    group_c_str = NULL;
  
  if (group_c_str)
    {
      PSID group_sid;
      wchar_t *wgroupname = _dbus_win_utf8_to_utf16 (group_c_str, error);

      if (!wgroupname)
	return FALSE;

      if (!_dbus_win_account_to_sid (wgroupname, &group_sid, error))
	{
	  dbus_free (wgroupname);
	  return FALSE;
	}

      info->gid = _dbus_win_sid_to_uid_t (group_sid);
      info->groupname = _dbus_strdup (group_c_str);

      dbus_free (group_sid);
      dbus_free (wgroupname);

      return TRUE;
    }
  else
    {
      dbus_bool_t retval = FALSE;
      wchar_t *wname, *wdomain;
      char *name, *domain;

      info->gid = gid;

      if (!_dbus_win_sid_to_name_and_domain (gid, &wname, &wdomain, error))
	return FALSE;

      name = _dbus_win_utf16_to_utf8 (wname, error);
      if (!name)
	goto out0;
      
      domain = _dbus_win_utf16_to_utf8 (wdomain, error);
      if (!domain)
	goto out1;

      info->groupname = dbus_malloc (strlen (domain) + 1 + strlen (name) + 1);

      strcpy (info->groupname, domain);
      strcat (info->groupname, "\\");
      strcat (info->groupname, name);

      retval = TRUE;

      dbus_free (domain);
    out1:
      dbus_free (name);
    out0:
      dbus_free (wname);
      dbus_free (wdomain);

      return retval;
    }
}

/* This file is part of the KDE project
   Copyright (C) 2000 Werner Almesberger

   libc/sys/linux/sys/dirent.h - Directory entry as returned by readdir

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#define HAVE_NO_D_NAMLEN	/* no struct dirent->d_namlen */
#define HAVE_DD_LOCK  		/* have locking mechanism */

#define MAXNAMLEN 255		/* sizeof(struct dirent.d_name)-1 */

#define __dirfd(dir) (dir)->dd_fd

/* struct dirent - same as Unix */
struct dirent {
    long d_ino;                    /* inode (always 1 in WIN32) */
    off_t d_off;                /* offset to this dirent */
    unsigned short d_reclen;    /* length of d_name */
    char d_name[_MAX_FNAME+1];    /* filename (null terminated) */
};

/* typedef DIR - not the same as Unix */
typedef struct {
    long handle;                /* _findfirst/_findnext handle */
    short offset;                /* offset into directory */
    short finished;             /* 1 if there are not more files */
    struct _finddata_t fileinfo;  /* from _findfirst/_findnext */
    char *dir;                  /* the dir we are reading */
    struct dirent dent;         /* the dirent to return */
} DIR;

/**********************************************************************
 * Implement dirent-style opendir/readdir/closedir on Window 95/NT
 *
 * Functions defined are opendir(), readdir() and closedir() with the
 * same prototypes as the normal dirent.h implementation.
 *
 * Does not implement telldir(), seekdir(), rewinddir() or scandir(). 
 * The dirent struct is compatible with Unix, except that d_ino is 
 * always 1 and d_off is made up as we go along.
 *
 * The DIR typedef is not compatible with Unix.
 **********************************************************************/

DIR * _dbus_opendir(const char *dir)
{
    DIR *dp;
    char *filespec;
    long handle;
    int index;

    filespec = malloc(strlen(dir) + 2 + 1);
    strcpy(filespec, dir);
    index = strlen(filespec) - 1;
    if (index >= 0 && (filespec[index] == '/' || filespec[index] == '\\'))
        filespec[index] = '\0';
    strcat(filespec, "\\*");

    dp = (DIR *)malloc(sizeof(DIR));
    dp->offset = 0;
    dp->finished = 0;
    dp->dir = strdup(dir);

    if ((handle = _findfirst(filespec, &(dp->fileinfo))) < 0) {
        if (errno == ENOENT)
            dp->finished = 1;
        else
        return NULL;
    }

    dp->handle = handle;
    free(filespec);

    return dp;
}

struct dirent * _dbus_readdir(DIR *dp)
{
    if (!dp || dp->finished) return NULL;

    if (dp->offset != 0) {
        if (_findnext(dp->handle, &(dp->fileinfo)) < 0) {
            dp->finished = 1;
			errno = 0;
            return NULL;
        }
    }
    dp->offset++;

    strncpy(dp->dent.d_name, dp->fileinfo.name, _MAX_FNAME);
    dp->dent.d_ino = 1;
    dp->dent.d_reclen = strlen(dp->dent.d_name);
    dp->dent.d_off = dp->offset;

    return &(dp->dent);
}


int _dbus_closedir(DIR *dp)
{
    if (!dp) return 0;
    _findclose(dp->handle);
    if (dp->dir) free(dp->dir);
    if (dp) free(dp);

    return 0;
}

/** @} */ /* End of DBusInternalsUtils functions */

/**
 * @addtogroup DBusString
 *
 * @{
 */
/**
 * Get the directory name from a complete filename
 * @param filename the filename
 * @param dirname string to append directory name to
 * @returns #FALSE if no memory
 */
dbus_bool_t
_dbus_string_get_dirname(const DBusString *filename,
                           DBusString       *dirname)
{
  int sep;
  
  _dbus_assert (filename != dirname);
  _dbus_assert (filename != NULL);
  _dbus_assert (dirname != NULL);

  /* Ignore any separators on the end */
  sep = _dbus_string_get_length (filename);
  if (sep == 0)
    return _dbus_string_append (dirname, "."); /* empty string passed in */
    
  while (sep > 0 &&
	 (_dbus_string_get_byte (filename, sep - 1) == '/' ||
	  _dbus_string_get_byte (filename, sep - 1) == '\\'))
    --sep;

  _dbus_assert (sep >= 0);
  
  if (sep == 0 ||
      (sep == 2 &&
       _dbus_string_get_byte (filename, 1) == ':' &&
       isalpha (_dbus_string_get_byte (filename, 0))))
    return _dbus_string_copy_len (filename, 0, sep + 1,
				  dirname, _dbus_string_get_length (dirname));

  {
    int sep1, sep2;
    _dbus_string_find_byte_backward (filename, sep, '/', &sep1);
    _dbus_string_find_byte_backward (filename, sep, '\\', &sep2);

    sep = MAX (sep1, sep2);
  }
  if (sep < 0)
    return _dbus_string_append (dirname, ".");
  
  while (sep > 0 &&
	 (_dbus_string_get_byte (filename, sep - 1) == '/' ||
	  _dbus_string_get_byte (filename, sep - 1) == '\\'))
    --sep;

  _dbus_assert (sep >= 0);
  
  if ((sep == 0 ||
       (sep == 2 &&
	_dbus_string_get_byte (filename, 1) == ':' &&
	isalpha (_dbus_string_get_byte (filename, 0))))
      &&
      (_dbus_string_get_byte (filename, sep) == '/' ||
       _dbus_string_get_byte (filename, sep) == '\\'))
    return _dbus_string_copy_len (filename, 0, sep + 1,
				  dirname, _dbus_string_get_length (dirname));
  else
    return _dbus_string_copy_len (filename, 0, sep - 0,
                                  dirname, _dbus_string_get_length (dirname));
}

/** @} */ /* DBusString stuff */

#ifdef DBUS_BUILD_TESTS
#include <stdlib.h>
static void
check_dirname (const char *filename,
               const char *dirname)
{
  DBusString f, d;
  
  _dbus_string_init_const (&f, filename);

  if (!_dbus_string_init (&d))
    _dbus_assert_not_reached ("no memory");

  if (!_dbus_string_get_dirname (&f, &d))
    _dbus_assert_not_reached ("no memory");

  if (!_dbus_string_equal_c_str (&d, dirname))
    {
      _dbus_warn ("For filename \"%s\" got dirname \"%s\" and expected \"%s\"\n",
                  filename,
                  _dbus_string_get_const_data (&d),
                  dirname);
      exit (1);
    }

  _dbus_string_free (&d);
}

static void
check_path_absolute (const char *path,
                     dbus_bool_t expected)
{
  DBusString p;

  _dbus_string_init_const (&p, path);

  if (_dbus_path_is_absolute (&p) != expected)
    {
      _dbus_warn ("For path \"%s\" expected absolute = %d got %d\n",
                  path, expected, _dbus_path_is_absolute (&p));
      exit (1);
    }
}

/**
 * Unit test for dbus-sysdeps.c.
 * 
 * @returns #TRUE on success.
 */
dbus_bool_t
_dbus_sysdeps_test (void)
{
  DBusString str;
  double val;
  int pos;
  
  check_dirname ("foo", ".");
  check_dirname ("foo/bar", "foo");
  check_dirname ("foo//bar", "foo");
  check_dirname ("foo///bar", "foo");
  check_dirname ("foo/bar/", "foo");
  check_dirname ("foo//bar/", "foo");
  check_dirname ("foo///bar/", "foo");
  check_dirname ("foo/bar//", "foo");
  check_dirname ("foo//bar////", "foo");
  check_dirname ("foo///bar///////", "foo");
  check_dirname ("/foo", "/");
  check_dirname ("////foo", "/");
  check_dirname ("/foo/bar", "/foo");
  check_dirname ("/foo//bar", "/foo");
  check_dirname ("/foo///bar", "/foo");
  check_dirname ("/", "/");
  check_dirname ("///", "/");
  check_dirname ("", ".");  


  _dbus_string_init_const (&str, "3.5");
  if (!_dbus_string_parse_double (&str,
				  0, &val, &pos))
    {
      _dbus_warn ("Failed to parse double");
      exit (1);
    }
  if (ABS(3.5 - val) > 1e-6)
    {
      _dbus_warn ("Failed to parse 3.5 correctly, got: %f", val);
      exit (1);
    }
  if (pos != 3)
    {
      _dbus_warn ("_dbus_string_parse_double of \"3.5\" returned wrong position %d", pos);
      exit (1);
    }
#if 0
  _dbus_string_init_const (&str, "0xff");
  if (!_dbus_string_parse_double (&str,
				  0, &val, &pos))
    {
      _dbus_warn ("Failed to parse double");
      exit (1);
    }
  if (ABS (0xff - val) > 1e-6)
    {
      _dbus_warn ("Failed to parse 0xff correctly, got: %f\n", val);
      exit (1);
    }
  if (pos != 4)
    {
      _dbus_warn ("_dbus_string_parse_double of \"0xff\" returned wrong position %d", pos);
      exit (1);
    }
#else
  _dbus_warn ("_dbus_string_parse_double(0xff) check disabled for now\n");
#endif
    
  check_path_absolute ("c:/", TRUE);
  check_path_absolute ("c:/foo", TRUE);
  check_path_absolute ("", FALSE);
  check_path_absolute ("foo", FALSE);
  check_path_absolute ("foo/bar", FALSE);
  
  return TRUE;
}
#endif /* DBUS_BUILD_TESTS */




