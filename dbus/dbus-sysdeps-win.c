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

#include "dbus-internals.h"
#include "dbus-sysdeps.h"
#include "dbus-sysdeps-win.h"
#include "dbus-threads.h"
#include "dbus-protocol.h"
#include "dbus-hash.h"
#include "dbus-sockets-win.h"
#include "dbus-string.h"

#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>

#include "dbus-sysdeps-win.h"

#ifdef _DBUS_WIN_USE_RANDOMIZER
static int  win_encap_randomizer;
#endif
static DBusHashTable *sid_atom_cache = NULL;


_DBUS_DEFINE_GLOBAL_LOCK (win_fds);
_DBUS_DEFINE_GLOBAL_LOCK (sid_atom_cache);

/************************************************************************
 
 handle <-> fd/socket functions

 ************************************************************************/

static DBusWin32FD *win_fds = NULL;
static int win_n_fds = 0; // is this the size? rename to win_fds_size? #

#if 0
#define TO_HANDLE(n)   ((n)^win32_encap_randomizer)
#define FROM_HANDLE(n) ((n)^win32_encap_randomizer)
#else
#define TO_HANDLE(n)   ((n)+0x10000000)
#define FROM_HANDLE(n) ((n)-0x10000000)
#define IS_HANDLE(n)   ((n)&0x10000000)
#endif

// do we need this? 
// doesn't the compiler optimize within one file?
#define _dbus_decapsulate_quick(i) win_fds[FROM_HANDLE (i)].fd

static
void
_dbus_win_deallocate_fd (int fd)
{
  _DBUS_LOCK (win_fds);
  win_fds[FROM_HANDLE (fd)].type = DBUS_WIN_FD_UNUSED;
  _DBUS_UNLOCK (win_fds);
}

static
int
_dbus_win_allocate_fd (void)
{
  int i;

  _DBUS_LOCK (win_fds);

  if (win_fds == NULL)
    {
#ifdef _DBUS_WIN_USE_RANDOMIZER
      DBusString random;
#endif

      win_n_fds = 16;
      /* Use malloc to avoid memory leak failure in dbus-test */
      win_fds = malloc (win_n_fds * sizeof (*win_fds));

      _dbus_assert (win_fds != NULL);

      for (i = 0; i < win_n_fds; i++)
        win_fds[i].type = DBUS_WIN_FD_UNUSED;

#ifdef _DBUS_WIN_USE_RANDOMIZER
      _dbus_string_init (&random);
      _dbus_generate_random_bytes (&random, sizeof (int));
      memmove (&win_encap_randomizer, _dbus_string_get_const_data (&random), sizeof (int));
      win_encap_randomizer &= 0xFF;
      _dbus_string_free (&random);
#endif
    }

  for (i = 0; i < win_n_fds && win_fds[i].type != DBUS_WIN_FD_UNUSED; i++)
    ;

  if (i == win_n_fds)
    {
      int oldn = win_n_fds;
      int j;

      win_n_fds += 16;
      win_fds = realloc (win_fds, win_n_fds * sizeof (*win_fds));

      _dbus_assert (win_fds != NULL);

      for (j = oldn; j < win_n_fds; j++)
        win_fds[i].type = DBUS_WIN_FD_UNUSED;
    }

  win_fds[i].type = DBUS_WIN_FD_BEING_OPENED;
  win_fds[i].fd = -1;
  win_fds[i].port_file_fd = -1;
  win_fds[i].close_on_exec = FALSE;
  win_fds[i].non_blocking = FALSE;

  _DBUS_UNLOCK (win_fds);

  return i;
}

static
int                                                                     
_dbus_create_handle_from_value (DBusWin32FDType type, int value)                                   
{    
  int i;
  int handle = -1;      

  // check: parameter must be a valid value
  _dbus_assert(value != -1);
  _dbus_assert(!IS_HANDLE(value));
 
  // get index of a new position in the map
  i = _dbus_win_allocate_fd ();                                   
  
   // fill new posiiton in the map: value->index
  win_fds[i].fd = value;                                             
  win_fds[i].type = type;
                              
  // create handle from the index: index->handle
  handle = TO_HANDLE (i);                                               
                                                                        
  _dbus_verbose ("_dbus_create_handle_from_value, value: %d, handle: %d\n", value, handle);  
         
  return handle;
}

static
int                 
_dbus_value_to_handle (DBusWin32FDType type, int value)
{
  int i;
  int handle = -1;

  // check: parameter must be a valid value
  _dbus_assert(value != -1);
  _dbus_assert(!IS_HANDLE(value));

  _DBUS_LOCK (win_fds);

  // at the first call there is no win_fds
  // will be constructed  _dbus_create_handle_from_value
  // because handle = -1
  if (win_fds != NULL)
  {
    // search for the value in the map
    // find the index of the value: value->index
    for (i = 0; i < win_n_fds; i++)
      if (win_fds[i].type == type && win_fds[i].fd == value)
        {
          // create handle from the index: index->handle
          handle = TO_HANDLE (i);
          break;
        }
  
    _DBUS_UNLOCK (win_fds);
  }

  if (handle == -1)
    {
      handle = _dbus_create_handle_from_value(type, value);
    }

  _dbus_assert(handle != -1);
  
  return handle;
}


int                 
_dbus_socket_to_handle (int socket)
{     
  return _dbus_value_to_handle (DBUS_WIN_FD_SOCKET, socket);
}

int                 
_dbus_fd_to_handle (int fd)
{     
  return _dbus_value_to_handle (DBUS_WIN_FD_C_LIB, fd);
}

                                              
static
int
_dbus_handle_to_value (DBusWin32FDType type, int handle)
{
  int i;
  int value;

  // check: parameter must be a valid handle
  _dbus_assert(handle != -1);
  _dbus_assert(IS_HANDLE(handle));

  // map from handle to index: handle->index
  i = FROM_HANDLE (handle);

  _dbus_assert (win_fds != NULL);
  _dbus_assert (i >= 0 && i < win_n_fds);

  // check for correct type
  _dbus_assert (win_fds[i].type == type);

  // get value from index: index->value
  value = win_fds[i].fd;
  
  _dbus_verbose ("deencapsulated C value fd=%d i=%d dfd=%x\n", value, i, handle);

  return value;
}

int
_dbus_handle_to_socket (int handle)
{
  return _dbus_handle_to_value (DBUS_WIN_FD_SOCKET, handle);
}

int
_dbus_handle_to_fd (int handle)
{
  return _dbus_handle_to_value (DBUS_WIN_FD_C_LIB, handle);
}

#undef TO_HANDLE
#undef IS_HANDLE
#undef FROM_HANDLE

#ifdef DBUS_WIN_FIXME
#define FROM_HANDLE(n) ((n)-0x10000000)
#else
// FIXME: don't use FROM_HANDLE directly,
// use _handle_to functions
#define FROM_HANDLE(n) 1==DBUS_WIN_FIXME__FROM_HANDLE
#endif






/************************************************************************
  
  ????????????????????

 ************************************************************************/





int
_dbus_read_win (int               fd,
            DBusString       *buffer,
            int               count)
{
#ifdef DBUS_WIN
  DBusWin32FDType type;
#endif
  int bytes_read;
  int start;
  char *data;

  _dbus_assert (count >= 0);
  
  start = _dbus_string_get_length (buffer);

  if (!_dbus_string_lengthen (buffer, count))
    {
      errno = ENOMEM;
      return -1;
    }

  data = _dbus_string_get_data_len (buffer, start, count);

#ifndef DBUS_WIN
#else

  _DBUS_LOCK (win_fds);

  fd = FROM_HANDLE (fd);

  _dbus_assert (fd >= 0 && fd < win_n_fds);
  _dbus_assert (win_fds != NULL);

  type = win_fds[fd].type;
  fd = win_fds[fd].fd;

  _DBUS_UNLOCK (win_fds);
    
  switch (type)
    {
    case DBUS_WIN_FD_SOCKET:
      _dbus_verbose ("recv: count=%d socket=%d\n", count, fd);
      bytes_read = recv (fd, data, count, 0);
      if (bytes_read == SOCKET_ERROR)
	{
	  DBUS_SOCKET_SET_ERRNO();
	  _dbus_verbose ("recv: failed: %s\n", _dbus_strerror (errno));
	  bytes_read = -1;
	}
      else
	_dbus_verbose ("recv: = %d\n", bytes_read); 
      break;

    case DBUS_WIN_FD_C_LIB:
      _dbus_verbose ("read: count=%d fd=%d\n", count, fd);
      bytes_read = read (fd, data, count);
      if (bytes_read == -1)
	_dbus_verbose ("read: failed: %s\n", _dbus_strerror (errno));
      else
	_dbus_verbose ("read: = %d\n", bytes_read); 
      break;

    default:
      _dbus_assert_not_reached ("unhandled fd type");
    }

#endif

  if (bytes_read < 0)
        {
          /* put length back (note that this doesn't actually realloc anything) */
          _dbus_string_set_length (buffer, start);
          return -1;
        }
  else
    {
      /* put length back (doesn't actually realloc) */
      _dbus_string_set_length (buffer, start + bytes_read);

#if 0
      if (bytes_read > 0)
        _dbus_verbose_bytes_of_string (buffer, start, bytes_read);
#endif
      
      return bytes_read;
    }
}

int
_dbus_write_win (int               fd,
                 const DBusString *buffer,
                 int               start,
                 int               len)
{
#ifdef DBUS_WIN
  DBusWin32FDType type;
#endif
  const char *data;
  int bytes_written;
  
  data = _dbus_string_get_const_data_len (buffer, start, len);
  
#ifndef DBUS_WIN
#else

  _DBUS_LOCK (win_fds);

  fd = FROM_HANDLE (fd);

  _dbus_assert (fd >= 0 && fd < win_n_fds);
  _dbus_assert (win_fds != NULL);

  type = win_fds[fd].type;
  fd = win_fds[fd].fd;

  _DBUS_UNLOCK (win_fds);

  switch (type)
    {
    case DBUS_WIN_FD_SOCKET:
      _dbus_verbose ("send: len=%d socket=%d\n", len, fd);
      bytes_written = send (fd, data, len, 0);
      if (bytes_written == SOCKET_ERROR)
	{
	  DBUS_SOCKET_SET_ERRNO();
	  _dbus_verbose ("send: failed: %s\n", _dbus_strerror (errno));
	  bytes_written = -1;
	}
      else
	_dbus_verbose ("send: = %d\n", bytes_written); 
      break;

    case DBUS_WIN_FD_C_LIB:
      _dbus_verbose ("write: len=%d fd=%d\n", len, fd);
      bytes_written = write (fd, data, len);
      if (bytes_written == -1)
	_dbus_verbose ("write: failed: %s\n", _dbus_strerror (errno));
      else
	_dbus_verbose ("write: = %d\n", bytes_written); 
      break;

    default:
      _dbus_assert_not_reached ("unhandled fd type");
    }

#endif

#if 0
  if (bytes_written > 0)
    _dbus_verbose_bytes_of_string (buffer, start, bytes_written);
#endif
  
  return bytes_written;
}



dbus_bool_t
_dbus_close_win (int        fd,
                 DBusError *error)
{
  const int encapsulated_fd = fd;

  _DBUS_ASSERT_ERROR_IS_CLEAR (error);
  
  _DBUS_LOCK (win_fds);

  fd = FROM_HANDLE (fd);

  _dbus_assert (fd >= 0 && fd < win_n_fds);
  _dbus_assert (win_fds != NULL);

  switch (win_fds[fd].type)
    {
    case DBUS_WIN_FD_SOCKET:
      if (win_fds[fd].port_file_fd >= 0)
	{
	  _chsize (win_fds[fd].port_file_fd, 0);
	  close (win_fds[fd].port_file_fd);
	  win_fds[fd].port_file_fd = -1;
	  unlink (_dbus_string_get_const_data (&win_fds[fd].port_file));
	  free ((char *) _dbus_string_get_const_data (&win_fds[fd].port_file));
	}
      
      if (closesocket (win_fds[fd].fd) == SOCKET_ERROR)
	{
	  DBUS_SOCKET_SET_ERRNO ();
	  dbus_set_error (error, _dbus_error_from_errno (errno),
			  "Could not close socket %d:%d:%d %s",
			  encapsulated_fd, fd, win_fds[fd].fd,
			  _dbus_strerror (errno));
	  _DBUS_UNLOCK (win_fds);
	  return FALSE;
	}
      _dbus_verbose ("closed socket %d:%d:%d\n",
		     encapsulated_fd, fd, win_fds[fd].fd);
      _DBUS_UNLOCK (win_fds);
      break;

    case DBUS_WIN_FD_C_LIB:
      if (close (win_fds[fd].fd) == -1)
	{
	  dbus_set_error (error, _dbus_error_from_errno (errno),
			  "Could not close fd %d:%d:%d: %s",
			  encapsulated_fd, fd, win_fds[fd].fd,
			  _dbus_strerror (errno));
	  _DBUS_UNLOCK (win_fds);
	  return FALSE;
	}
      _dbus_verbose ("closed C file descriptor %d:%d:%d\n",
		     encapsulated_fd, fd, win_fds[fd].fd);
      _DBUS_UNLOCK (win_fds);
      break;

    default:
      _dbus_assert_not_reached ("unhandled fd type");
    }

  _DBUS_UNLOCK (win_fds);

  _dbus_win_deallocate_fd (encapsulated_fd);

  return TRUE;

}

void
_dbus_fd_set_close_on_exec_win (int fd)
{
  int fd2;
  if (fd < 0) 
    return;
  _DBUS_LOCK (win_fds);

  fd2 = FROM_HANDLE (fd);
  _dbus_verbose("fd %d %d %d\n",fd,fd2,win_n_fds);
  _dbus_assert (fd2 >= 0 && fd2 < win_n_fds);
  _dbus_assert (win_fds != NULL);

  win_fds[fd2].close_on_exec = TRUE;

  _DBUS_UNLOCK (win_fds);
}

dbus_bool_t
_dbus_set_fd_nonblocking_win (int             fd,
                              DBusError      *error)
{
  u_long one = 1;
  const int encapsulated_fd = fd;

  _DBUS_ASSERT_ERROR_IS_CLEAR (error);
  
  _DBUS_LOCK (win_fds);

  fd = FROM_HANDLE (fd);

  _dbus_assert (fd >= 0 && fd < win_n_fds);
  _dbus_assert (win_fds != NULL);

  switch (win_fds[fd].type)
    {
    case DBUS_WIN_FD_SOCKET:
      if (ioctlsocket (win_fds[fd].fd, FIONBIO, &one) == SOCKET_ERROR)
	{
	  dbus_set_error (error, _dbus_error_from_errno (WSAGetLastError ()),
			  "Failed to set socket %d:%d to nonblocking: %s",
			  encapsulated_fd, win_fds[fd].fd,
			  _dbus_strerror (WSAGetLastError ()));
	  _DBUS_UNLOCK (win_fds);
	  return FALSE;
	}
      break;

    case DBUS_WIN_FD_C_LIB:
      _dbus_assert_not_reached ("only sockets can be set to nonblocking");
      break;

    default:
      _dbus_assert_not_reached ("unhandled fd type");
    }

  _DBUS_UNLOCK (win_fds);

  return TRUE;
}


int
_dbus_write_two_win (int               fd,
                     const DBusString *buffer1,
                     int               start1,
                     int               len1,
                     const DBusString *buffer2,
                     int               start2,
                     int               len2)
{
  DBusWin32FDType type;
  WSABUF vectors[2];
  const char *data1;
  const char *data2;
  int rc;
  DWORD bytes_written;
  int ret1;

  _DBUS_LOCK (win_fds);

  fd = FROM_HANDLE (fd);

  _dbus_assert (fd >= 0 && fd < win_n_fds);
  _dbus_assert (win_fds != NULL);

  type = win_fds[fd].type;
  fd = win_fds[fd].fd;

  _DBUS_UNLOCK (win_fds);

  data1 = _dbus_string_get_const_data_len (buffer1, start1, len1);
  
  if (buffer2 != NULL)
    data2 = _dbus_string_get_const_data_len (buffer2, start2, len2);
  else
    {
      data2 = NULL;
      start2 = 0;
      len2 = 0;
    }
   
  switch (type)
    {
    case DBUS_WIN_FD_SOCKET:
      vectors[0].buf = (char*) data1;
      vectors[0].len = len1;
      vectors[1].buf = (char*) data2;
      vectors[1].len = len2;
      
      _dbus_verbose ("WSASend: len1+2=%d+%d socket=%d\n", len1, len2, fd);
      rc = WSASend (fd, vectors, data2 ? 2 : 1, &bytes_written,
		    0, NULL, NULL);
      if (rc < 0)
	    {
          DBUS_SOCKET_SET_ERRNO ();
	      _dbus_verbose ("WSASend: failed: %s\n", _dbus_strerror (errno));
	      bytes_written = -1;
	    }
      else
        _dbus_verbose ("WSASend: = %ld\n", bytes_written);
      return bytes_written;

    case DBUS_WIN_FD_C_LIB:
      ret1 = _dbus_write (fd, buffer1, start1, len1);
      if (ret1 == len1 && buffer2 != NULL)
	    {
	      int ret2 = _dbus_write (fd, buffer2, start2, len2);
	      if (ret2 < 0)
	      ret2 = 0; /* we can't report an error as the first write was OK */
          return ret1 + ret2;
	    }
      else
	    return ret1;

    default:
      _dbus_assert_not_reached ("unhandled fd type");
    }
  return 0;
}

int
_dbus_connect_unix_socket_win (const char     *path,
                               dbus_bool_t     abstract,
                               DBusError      *error)
{
  int fd, n, port;
  char buf[7];

  _DBUS_ASSERT_ERROR_IS_CLEAR (error);

  _dbus_verbose ("connecting to pseudo-unix socket at %s\n",
                 path);
  
  if (abstract)
    {
      dbus_set_error (error, DBUS_ERROR_NOT_SUPPORTED,
                      "Implementation does not support abstract socket namespace\n");
      return -1;
    }
    
  fd = _sopen (path, O_RDONLY, SH_DENYNO);

  if (fd == -1)
    {
      dbus_set_error (error, _dbus_error_from_errno (errno),
		      "Failed to open file %s: %s",
		      path, _dbus_strerror (errno));
      return -1;
    }

  n = read (fd, buf, sizeof (buf) - 1);
  close (fd);

  if (n == 0)
    {
      dbus_set_error (error, DBUS_ERROR_FAILED,
		      "Failed to read port number from file %s",
		      path);
      return -1;
    }

  buf[n] = '\0';
  port = atoi (buf);

  if (port <= 0 || port > 0xFFFF)
    {
      dbus_set_error (error, DBUS_ERROR_FAILED,
		      "Invalid port numer in file %s",
		      path);
      return -1;
    }
 
  return _dbus_connect_tcp_socket (NULL, port, error);

}
int
_dbus_listen_unix_socket_win (const char     *path,
                              dbus_bool_t     abstract,
                              DBusError      *error)
{
  int listen_fd;
  SOCKET sock;
  struct sockaddr sa;
  int addr_len;
  int filefd;
  int n, l;
  DBusString portstr;

  _DBUS_ASSERT_ERROR_IS_CLEAR (error);

  _dbus_verbose ("listening on pseudo-unix socket at %s\n",
                 path);

  if (abstract)
    {
      dbus_set_error (error, DBUS_ERROR_NOT_SUPPORTED,
                      "Implementation does not support abstract socket namespace\n");
      return -1;
    }
    
  listen_fd = _dbus_listen_tcp_socket (NULL, 0, error);

  if (listen_fd == -1)
    return -1;

  sock = win_fds[FROM_HANDLE (listen_fd)].fd;

  addr_len = sizeof (sa);
  if (getsockname (sock, &sa, &addr_len) == SOCKET_ERROR)
    {
      DBUS_SOCKET_SET_ERRNO ();
      dbus_set_error (error, _dbus_error_from_errno (errno),
		      "getsockname failed: %s",
		      _dbus_strerror (errno));
      _dbus_close (listen_fd, NULL);
      return -1;
    }

  _dbus_assert (((struct sockaddr_in*) &sa)->sin_family == AF_INET);

  filefd = _sopen (path, O_CREAT|O_WRONLY|_O_SHORT_LIVED, SH_DENYWR, 0666);

  if (filefd == -1)
    {
      dbus_set_error (error, _dbus_error_from_errno (errno),
		      "Failed to create pseudo-unix socket port number file %s: %s",
		      path, _dbus_strerror (errno));
      _dbus_close (listen_fd, NULL);
      return -1;
    }

  win_fds[FROM_HANDLE (listen_fd)].port_file_fd = filefd;

  /* Use strdup() to avoid memory leak in dbus-test */
  path = strdup (path);
  if (!path)
    {
      _DBUS_SET_OOM (error);
      _dbus_close (listen_fd, NULL);
      return -1;
    }

  _dbus_string_init_const (&win_fds[FROM_HANDLE (listen_fd)].port_file, path);

  if (!_dbus_string_init (&portstr))
    {
      _DBUS_SET_OOM (error);
      _dbus_close (listen_fd, NULL);
      return -1;
    }

  if (!_dbus_string_append_int (&portstr, ntohs (((struct sockaddr_in*) &sa)->sin_port)))
    {
      _DBUS_SET_OOM (error);
      _dbus_close (listen_fd, NULL);
      return -1;
    }

  l = _dbus_string_get_length (&portstr);
  n = write (filefd, _dbus_string_get_const_data (&portstr), l);
  _dbus_string_free (&portstr);

  if (n == -1)
    {
      dbus_set_error (error, _dbus_error_from_errno (errno),
		      "Failed to write port number to file %s: %s",
		      path, _dbus_strerror (errno));
      _dbus_close (listen_fd, NULL);
      return -1;
    }
  else if (n < l)
    {
      dbus_set_error (error, _dbus_error_from_errno (errno),
		      "Failed to write port number to file %s",
		      path);
      _dbus_close (listen_fd, NULL);
      return -1;
    }

  return listen_fd;
}

#ifdef DBUS_WIN
#if 0

/**
 * Opens the client side of a Windows named pipe. The connection D-BUS
 * file descriptor index is returned. It is set up as nonblocking.
 * 
 * @param path the path to named pipe socket
 * @param error return location for error code
 * @returns connection D-BUS file descriptor or -1 on error
 */
int
_dbus_connect_named_pipe (const char     *path,
			  DBusError      *error)
{
  _dbus_assert_not_reached ("not implemented");
}

#endif
#endif


dbus_bool_t
_dbus_account_to_win_sid (const wchar_t  *waccount,
                          void          **ppsid,
                          DBusError      *error)
{
  dbus_bool_t retval = FALSE;
  DWORD sid_length, wdomain_length;
  SID_NAME_USE use;
  wchar_t *wdomain;
		     
  *ppsid = NULL;

  sid_length = 0;
  wdomain_length = 0;
  if (!LookupAccountNameW (NULL, waccount, NULL, &sid_length,
	     NULL, &wdomain_length, &use) 
      && GetLastError () != ERROR_INSUFFICIENT_BUFFER)
    {
      _dbus_win_set_error_from_win_error (error, GetLastError ());
      return FALSE;
    }

  *ppsid = dbus_malloc (sid_length);
  if (!*ppsid)
    {
      _DBUS_SET_OOM (error);
      return FALSE;
    }

  wdomain = dbus_new (wchar_t, wdomain_length);
  if (!wdomain)
    {
      _DBUS_SET_OOM (error);
      goto out1;
    }

  if (!LookupAccountNameW (NULL, waccount, (PSID) *ppsid, &sid_length,
			   wdomain, &wdomain_length, &use))
    {
      _dbus_win_set_error_from_win_error (error, GetLastError ());
      goto out2;
    }

  if (!IsValidSid ((PSID) *ppsid))
    {
      dbus_set_error_const (error, DBUS_ERROR_FAILED, "Invalid SID");
      goto out2;
    }

  retval = TRUE;

 out2:
  dbus_free (wdomain);
 out1:
  if (!retval)
    {
      dbus_free (*ppsid);
      *ppsid = NULL;
    }

  return retval;
}


dbus_bool_t
fill_win_user_info_name_and_groups (wchar_t 	  *wname,
				      wchar_t 	  *wdomain,
				      DBusUserInfo *info,
				      DBusError    *error)
{
  dbus_bool_t retval = FALSE;
  char *name, *domain;
  LPLOCALGROUP_USERS_INFO_0 local_groups = NULL;
  LPGROUP_USERS_INFO_0 global_groups = NULL;
  DWORD nread, ntotal;

  name = _dbus_win_utf16_to_utf8 (wname, error);
  if (!name)
    return FALSE;

  domain = _dbus_win_utf16_to_utf8 (wdomain, error);
  if (!domain)
    goto out0;

  info->username = dbus_malloc (strlen (domain) + 1 + strlen (name) + 1);
  if (!info->username)
    {
      _DBUS_SET_OOM (error);
      goto out1;
    }

  strcpy (info->username, domain);
  strcat (info->username, "\\");
  strcat (info->username, name);

  info->n_group_ids = 0;
  if (NetUserGetLocalGroups (NULL, wname, 0, LG_INCLUDE_INDIRECT,
			     (LPBYTE *) &local_groups, MAX_PREFERRED_LENGTH,
			     &nread, &ntotal) == NERR_Success)
    {
      DWORD i;
      int n;

      info->group_ids = dbus_new (dbus_gid_t, nread);
      if (!info->group_ids)
	{
	  _DBUS_SET_OOM (error);
	  goto out3;
	}

      for (i = n = 0; i < nread; i++)
	{
	  PSID group_sid;
	  if (_dbus_account_to_win_sid (local_groups[i].lgrui0_name,
					  &group_sid, error))
	    {
	      info->group_ids[n++] = _dbus_win_sid_to_uid_t (group_sid);
	      dbus_free (group_sid);
	    }
	}
      info->n_group_ids = n;
    }

  if (NetUserGetGroups (NULL, wname, 0,
			(LPBYTE *) &global_groups, MAX_PREFERRED_LENGTH,
			&nread, &ntotal) == NERR_Success)
    {
      DWORD i;
      int n = info->n_group_ids;

      info->group_ids = dbus_realloc (info->group_ids, (n + nread) * sizeof (dbus_gid_t));
      if (!info->group_ids)
	{
	  _DBUS_SET_OOM (error);
	  goto out4;
	}

      for (i = 0; i < nread; i++)
	{
	  PSID group_sid;
	  if (_dbus_account_to_win_sid (global_groups[i].grui0_name,
					  &group_sid, error))
	    {
	      info->group_ids[n++] = _dbus_win_sid_to_uid_t (group_sid);
	      dbus_free (group_sid);
	    }
	}
      info->n_group_ids = n;
    }
  
  if (info->n_group_ids > 0)
    {
      /* FIXME: find out actual primary group */
      info->primary_gid = info->group_ids[0];
    }
  else
    {
      info->group_ids = dbus_new (dbus_gid_t, 1);
      info->n_group_ids = 1;
      info->group_ids[0] = DBUS_GID_UNSET;
      info->primary_gid = DBUS_GID_UNSET;
    }

  retval = TRUE;

 out4:
  if (global_groups != NULL)
    NetApiBufferFree (global_groups);
 out3:
  if (local_groups != NULL)
    NetApiBufferFree (local_groups);
 out1:
  dbus_free (domain);
 out0:
  dbus_free (name);

  return retval;
}

dbus_bool_t
fill_win_user_info_homedir (wchar_t  	 *wname,
			      wchar_t  	 *wdomain,
			      DBusUserInfo *info,
			      DBusError    *error)
{
  dbus_bool_t retval = FALSE;
  USER_INFO_1 *user_info = NULL;
  wchar_t wcomputername[MAX_COMPUTERNAME_LENGTH + 1];
  DWORD wcomputername_length = MAX_COMPUTERNAME_LENGTH + 1;
  dbus_bool_t local_computer;
  wchar_t *dc = NULL;
  NET_API_STATUS ret = 0;

  /* If the domain is this computer's name, assume it's a local user.
   * Otherwise look up a DC for the domain, and ask it.
   */

  GetComputerNameW (wcomputername, &wcomputername_length);
  local_computer = (wcsicmp (wcomputername, wdomain) == 0);

  if (!local_computer)
    {
    	ret = NetGetAnyDCName (NULL, wdomain, (LPBYTE *) &dc);
      if (ret != NERR_Success) {
	      info->homedir = _dbus_strdup ("\\");
      	_dbus_warn("NetGetAnyDCName() failed with errorcode %d '%s'\n",ret,_dbus_lm_strerror(ret));
				return TRUE;
			}
    }
      
  /* No way to find out the profile of another user, let's try the
   * "home directory" from NetUserGetInfo's USER_INFO_1.
   */
  ret = NetUserGetInfo (NULL, wname, 1, (LPBYTE *) &user_info); 
  if (ret == NERR_Success )
	  if(user_info->usri1_home_dir != NULL &&
         user_info->usri1_home_dir != (LPWSTR)0xfeeefeee &&  /* freed memory http://www.gamedev.net/community/forums/topic.asp?topic_id=158402 */
         user_info->usri1_home_dir[0] != '\0')
        {
          info->homedir = _dbus_win_utf16_to_utf8 (user_info->usri1_home_dir, error);
          if (!info->homedir)
	        goto out1;
	    }
	  else
        {
          _dbus_warn("NetUserGetInfo() failed: no valid user_info\n");
          /* Not set, so use something random. */
          info->homedir = _dbus_strdup ("\\");
         }
  else
    {
      char *dc_string = _dbus_win_utf16_to_utf8(dc,error);
      _dbus_warn("NetUserGetInfo() failed with errorcode %d '%s', %s\n",ret,_dbus_lm_strerror(ret),dc_string);
      dbus_free(dc_string);
      /* Not set, so use something random. */
      info->homedir = _dbus_strdup ("\\");
    }
  
  retval = TRUE;

 out1:
  if (dc != NULL)
    NetApiBufferFree (dc);
  if (user_info != NULL)
  	NetApiBufferFree (user_info);

  return retval;
}

dbus_bool_t
fill_win_user_info_from_name (wchar_t      *wname,
                              DBusUserInfo *info,
                              DBusError    *error)
{
  dbus_bool_t retval = FALSE;
  PSID sid;
  wchar_t *wdomain;
  DWORD sid_length, wdomain_length;
  SID_NAME_USE use;
		     
  sid_length = 0;
  wdomain_length = 0;
  if (!LookupAccountNameW (NULL, wname, NULL, &sid_length,
			   NULL, &wdomain_length, &use) &&
      GetLastError () != ERROR_INSUFFICIENT_BUFFER)
    {
      _dbus_win_set_error_from_win_error (error, GetLastError ());
      return FALSE;
    }

  sid = dbus_malloc (sid_length);
  if (!sid)
    {
      _DBUS_SET_OOM (error);
      return FALSE;
    }

  wdomain = dbus_new (wchar_t, wdomain_length);
  if (!wdomain)
    {
      _DBUS_SET_OOM (error);
      goto out0;
    }

  if (!LookupAccountNameW (NULL, wname, sid, &sid_length,
			   wdomain, &wdomain_length, &use))
    {
      _dbus_win_set_error_from_win_error (error, GetLastError ());
      goto out1;
    }

  if (!IsValidSid (sid))
    {
      dbus_set_error_const (error, DBUS_ERROR_FAILED, "Invalid SID");
      goto out1;
    }

  info->uid = _dbus_win_sid_to_uid_t (sid);

  if (!fill_win_user_info_name_and_groups (wname, wdomain, info, error))
    goto out1;
    
  if (!fill_win_user_info_homedir (wname, wdomain, info, error))
    goto out1;

  retval = TRUE;

 out1:
  dbus_free (wdomain);
 out0:
  dbus_free (sid);

  return retval;
}

dbus_bool_t
_dbus_win_sid_to_name_and_domain (dbus_uid_t uid,
				    wchar_t  **wname,
				    wchar_t  **wdomain,
				    DBusError *error)
{
  PSID sid;
  DWORD wname_length, wdomain_length;
  SID_NAME_USE use;

  if (!_dbus_uid_t_to_win_sid (uid, &sid))
    {
      _dbus_win_set_error_from_win_error (error, GetLastError ());
      return FALSE;
    }

  wname_length = 0;
  wdomain_length = 0;
  if (!LookupAccountSidW (NULL, sid, NULL, &wname_length,
			  NULL, &wdomain_length, &use) &&
      GetLastError () != ERROR_INSUFFICIENT_BUFFER)
    {
      _dbus_win_set_error_from_win_error (error, GetLastError ());
      goto out0;
    }

  *wname = dbus_new (wchar_t, wname_length);
  if (!*wname)
    {
      _DBUS_SET_OOM (error);
      goto out0;
    }

  *wdomain = dbus_new (wchar_t, wdomain_length);
  if (!*wdomain)
    {
      _DBUS_SET_OOM (error);
      goto out1;
    }

  if (!LookupAccountSidW (NULL, sid, *wname, &wname_length,
			  *wdomain, &wdomain_length, &use))
    {
      _dbus_win_set_error_from_win_error (error, GetLastError ());
      goto out2;
    }

  return TRUE;

 out2:
  dbus_free (*wdomain);
  *wdomain = NULL;
 out1:
  dbus_free (*wname);
  *wname = NULL;
 out0:
  LocalFree (sid);

  return FALSE;
}

dbus_bool_t
fill_win_user_info_from_uid (dbus_uid_t    uid,
			       DBusUserInfo *info,
			       DBusError    *error)
{
  dbus_bool_t retval = FALSE;
  wchar_t *wname, *wdomain;

  info->uid = uid;

  if (!_dbus_win_sid_to_name_and_domain (uid, &wname, &wdomain, error)) {
   	_dbus_verbose("%s after _dbus_win_sid_to_name_and_domain\n",__FUNCTION__);
    return FALSE;
 	 }

  if (!fill_win_user_info_name_and_groups (wname, wdomain, info, error)) {
  	_dbus_verbose("%s after fill_win_user_info_name_and_groups\n",__FUNCTION__);
    goto out0;
 	 }

    
  if (!fill_win_user_info_homedir (wname, wdomain, info, error)) {
  	_dbus_verbose("%s after fill_win_user_info_homedir\n",__FUNCTION__);
     goto out0; 
 	 }
 	 
  retval = TRUE;

 out0:
  dbus_free (wdomain);
  dbus_free (wname);

  return retval;
}




void
_dbus_win_startup_winsock (void)
{
  /* Straight from MSDN, deuglified */

  static dbus_bool_t beenhere = FALSE;

  WORD wVersionRequested;
  WSADATA wsaData;
  int err;
 
  if (beenhere)
    return;

  wVersionRequested = MAKEWORD (2, 0);
 
  err = WSAStartup (wVersionRequested, &wsaData);
  if (err != 0)
    {
      _dbus_assert_not_reached ("Could not initialize WinSock");
      _dbus_abort ();
    }
 
  /* Confirm that the WinSock DLL supports 2.0.  Note that if the DLL
   * supports versions greater than 2.0 in addition to 2.0, it will
   * still return 2.0 in wVersion since that is the version we
   * requested.
   */
  if (LOBYTE (wsaData.wVersion) != 2 ||
      HIBYTE (wsaData.wVersion) != 0)
    {
      _dbus_assert_not_reached ("No usable WinSock found");
      _dbus_abort ();
    }

  beenhere = TRUE;
}









/************************************************************************
 
 UTF / string code

 ************************************************************************/

/**
 * Measure the message length without terminating nul 
 */
int _dbus_printf_string_upper_bound (const char *format,
                         va_list args) 
{
  /* MSVCRT's vsnprintf semantics are a bit different */
  /* The C library source in the Platform SDK indicates that this
   * would work, but alas, it doesn't. At least not on Windows
   * 2000. Presumably those sources correspond to the C library on
   * some newer or even future Windows version.
   *
    len = _vsnprintf (NULL, _DBUS_INT_MAX, format, args);
   */
  char p[1024];
  int len;
  len = vsnprintf (p, sizeof(p)-1, format, args);
  if (len == -1) // try again
    {
      char *p;
      p = malloc (strlen(format)*3);
      len = vsnprintf (p, sizeof(p)-1, format, args);
      free(p);
    }
  return len;
}


/**
 * Returns the UTF-16 form of a UTF-8 string. The result should be
 * freed with dbus_free() when no longer needed.
 *
 * @param str the UTF-8 string
 * @param error return location for error code
 */
wchar_t *
_dbus_win_utf8_to_utf16 (const char *str,
                         DBusError  *error)
{
  DBusString s;
  int n;
  wchar_t *retval;

  _dbus_string_init_const (&s, str);
  
  if (!_dbus_string_validate_utf8 (&s, 0, _dbus_string_get_length (&s)))
    {
      dbus_set_error_const (error, DBUS_ERROR_FAILED, "Invalid UTF-8");
      return NULL;
    }

  n = MultiByteToWideChar (CP_UTF8, 0, str, -1, NULL, 0);

  if (n == 0)
    {
      _dbus_win_set_error_from_win_error (error, GetLastError ());
      return NULL;
    }

  retval = dbus_new (wchar_t, n);

  if (!retval)
    {
      _DBUS_SET_OOM (error);
      return NULL;
    }

  if (MultiByteToWideChar (CP_UTF8, 0, str, -1, retval, n) != n)
    {
      dbus_free (retval);
      dbus_set_error_const (error, DBUS_ERROR_FAILED, "MultiByteToWideChar inconsistency");
      return NULL;
    }

  return retval;
}

/**
 * Returns the UTF-8 form of a UTF-16 string. The result should be
 * freed with dbus_free() when no longer needed.
 *
 * @param str the UTF-16 string
 * @param error return location for error code
 */
char *
_dbus_win_utf16_to_utf8 (const wchar_t *str,
                         DBusError     *error)
{
  int n;
  char *retval;

  n = WideCharToMultiByte (CP_UTF8, 0, str, -1, NULL, 0, NULL, NULL);

  if (n == 0)
    {
      _dbus_win_set_error_from_win_error (error, GetLastError ());
      return NULL;
    }

  retval = dbus_malloc (n);

  if (!retval)
    {
      _DBUS_SET_OOM (error);
      return NULL;
    }

  if (WideCharToMultiByte (CP_UTF8, 0, str, -1, retval, n, NULL, NULL) != n)
    {
      dbus_free (retval);
      dbus_set_error_const (error, DBUS_ERROR_FAILED, "WideCharToMultiByte inconsistency");
      return NULL;
    }

  return retval;
}






/************************************************************************
 
 uid ... <-> win sid functions

 ************************************************************************/

dbus_bool_t
_dbus_win_account_to_sid (const wchar_t *waccount,
			    void      	 **ppsid,
			    DBusError 	  *error)
{
  dbus_bool_t retval = FALSE;
  DWORD sid_length, wdomain_length;
  SID_NAME_USE use;
  wchar_t *wdomain;
		     
  *ppsid = NULL;

  sid_length = 0;
  wdomain_length = 0;
  if (!LookupAccountNameW (NULL, waccount, NULL, &sid_length,
			   NULL, &wdomain_length, &use) &&
      GetLastError () != ERROR_INSUFFICIENT_BUFFER)
    {
      _dbus_win_set_error_from_win_error (error, GetLastError ());
      return FALSE;
    }

  *ppsid = dbus_malloc (sid_length);
  if (!*ppsid)
    {
      _DBUS_SET_OOM (error);
      return FALSE;
    }

  wdomain = dbus_new (wchar_t, wdomain_length);
  if (!wdomain)
    {
      _DBUS_SET_OOM (error);
      goto out1;
    }

  if (!LookupAccountNameW (NULL, waccount, (PSID) *ppsid, &sid_length,
			   wdomain, &wdomain_length, &use))
    {
      _dbus_win_set_error_from_win_error (error, GetLastError ());
      goto out2;
    }

  if (!IsValidSid ((PSID) *ppsid))
    {
      dbus_set_error_const (error, DBUS_ERROR_FAILED, "Invalid SID");
      goto out2;
    }

  retval = TRUE;

 out2:
  dbus_free (wdomain);
 out1:
  if (!retval)
    {
      dbus_free (*ppsid);
      *ppsid = NULL;
    }

  return retval;
}

static void
_sid_atom_cache_shutdown_win (void *unused)
{
 DBusHashIter iter;
 _DBUS_LOCK (sid_atom_cache);
 _dbus_hash_iter_init (sid_atom_cache, &iter);
 while (_dbus_hash_iter_next (&iter))
   {
	 ATOM atom;
     atom = (ATOM) _dbus_hash_iter_get_value (&iter);
     GlobalDeleteAtom(atom);
	 _dbus_hash_iter_remove_entry(&iter);
   }
  _DBUS_UNLOCK (sid_atom_cache);
  _dbus_hash_table_unref (sid_atom_cache);
  sid_atom_cache = NULL;
}
  
/**
 * Returns the 2-way associated dbus_uid_t form a SID.
 *
 * @param psid pointer to the SID
 */
dbus_uid_t
_dbus_win_sid_to_uid_t (PSID psid)
{
  dbus_uid_t uid;
  dbus_uid_t olduid;
  char *string;
  ATOM atom;
  
  if (!IsValidSid (psid)) 
    {
      _dbus_verbose("%s invalid sid\n",__FUNCTION__);
      return DBUS_UID_UNSET;
    }  
  if (!ConvertSidToStringSidA (psid, &string)) 
    {
 	  _dbus_verbose("%s invalid sid\n",__FUNCTION__);
      return DBUS_UID_UNSET;
    }

  atom = GlobalAddAtom(string);

  if (atom == 0)
    {
 	  _dbus_verbose("%s GlobalAddAtom failed\n",__FUNCTION__);
	  LocalFree (string);
      return DBUS_UID_UNSET;
    }

  _DBUS_LOCK (sid_atom_cache);

  if (sid_atom_cache == NULL)
    {
      sid_atom_cache = _dbus_hash_table_new (DBUS_HASH_ULONG, NULL, NULL);
      _dbus_register_shutdown_func (_sid_atom_cache_shutdown_win, NULL);
    }

  uid = atom;
  olduid = (dbus_uid_t) _dbus_hash_table_lookup_ulong (sid_atom_cache, uid);

  if (olduid)
    {
      _dbus_verbose("%s sid with id %i found in cache\n",__FUNCTION__, olduid);
      uid = olduid;
    }
  else
    {
      _dbus_hash_table_insert_ulong (sid_atom_cache, uid, (void*) uid);
      _dbus_verbose("%s sid %s added with uid %i to cache\n",__FUNCTION__, string, uid);
    }

  _DBUS_UNLOCK (sid_atom_cache);

  return uid;
}

dbus_bool_t  _dbus_uid_t_to_win_sid (dbus_uid_t uid, PSID *ppsid)
{
  void* atom;
  char string[255];

  atom = _dbus_hash_table_lookup_ulong (sid_atom_cache, uid);
  if (atom == NULL)
	{
      _dbus_verbose("%s uid %i not found in cache\n",__FUNCTION__,uid);
      return FALSE;
    }
  memset( string, '.', sizeof(string) );
  if (!GlobalGetAtomNameA( (ATOM) atom, string, 255 ))
    {
      _dbus_verbose("%s uid %i not found in cache\n",__FUNCTION__, uid);
      return FALSE;
    }
  if (!ConvertStringSidToSidA(string, ppsid))
    {
      _dbus_verbose("%s could not convert %s into sid \n",__FUNCTION__, string);
      return FALSE;
    }
  _dbus_verbose("%s converted %s into sid \n",__FUNCTION__, string);
  return TRUE;
}


/** @} end of sysdeps-win */



dbus_uid_t
_dbus_getuid_win (void)
{
  dbus_uid_t retval = DBUS_UID_UNSET;
  HANDLE process_token = NULL;
  TOKEN_USER *token_user = NULL;
  DWORD n;

  if (!OpenProcessToken (GetCurrentProcess (), TOKEN_QUERY, &process_token))
    _dbus_win_warn_win_error ("OpenProcessToken failed", GetLastError ());
  else if ((!GetTokenInformation (process_token, TokenUser, NULL, 0, &n) 
	        && GetLastError () != ERROR_INSUFFICIENT_BUFFER) 
			|| (token_user = alloca (n)) == NULL 
			|| !GetTokenInformation (process_token, TokenUser, token_user, n, &n))
    _dbus_win_warn_win_error ("GetTokenInformation failed", GetLastError ());
  else
    retval = _dbus_win_sid_to_uid_t (token_user->User.Sid);

  if (process_token != NULL)
    CloseHandle (process_token);

  _dbus_verbose("_dbus_getuid() returns %d\n",retval);
  return retval;
}

dbus_gid_t
_dbus_getgid_win (void)
{
  dbus_gid_t retval = DBUS_GID_UNSET;
  HANDLE process_token = NULL;
  TOKEN_PRIMARY_GROUP *token_primary_group = NULL;
  DWORD n;

  if (!OpenProcessToken (GetCurrentProcess (), TOKEN_QUERY, &process_token))
    _dbus_win_warn_win_error ("OpenProcessToken failed", GetLastError ());
  else if ((!GetTokenInformation (process_token, TokenPrimaryGroup,
				  NULL, 0, &n) &&
	    GetLastError () != ERROR_INSUFFICIENT_BUFFER) ||
	   (token_primary_group = alloca (n)) == NULL ||
	   !GetTokenInformation (process_token, TokenPrimaryGroup,
				 token_primary_group, n, &n))
    _dbus_win_warn_win_error ("GetTokenInformation failed", GetLastError ());
  else
    retval = _dbus_win_sid_to_uid_t (token_primary_group->PrimaryGroup);

  if (process_token != NULL)
    CloseHandle (process_token);

  return retval;
}


/************************************************************************
 
 pipes

 ************************************************************************/


dbus_bool_t
_dbus_full_duplex_pipe_win (int        *fd1,
                            int        *fd2,
                            dbus_bool_t blocking,
                            DBusError  *error)
{
  SOCKET temp, socket1 = -1, socket2 = -1;
  struct sockaddr_in saddr;
  int len;
  u_long arg;
  fd_set read_set, write_set;
  struct timeval tv;

  _dbus_win_startup_winsock ();

  temp = socket (AF_INET, SOCK_STREAM, 0);
  if (temp == INVALID_SOCKET)
    {
      DBUS_SOCKET_SET_ERRNO ();
      goto out0;
    }
  
  arg = 1;
  if (ioctlsocket (temp, FIONBIO, &arg) == SOCKET_ERROR)
    {
      DBUS_SOCKET_SET_ERRNO ();
      goto out0;
    }

  _DBUS_ZERO (saddr);
  saddr.sin_family = AF_INET;
  saddr.sin_port = 0;
  saddr.sin_addr.s_addr = htonl (INADDR_LOOPBACK);

  if (bind (temp, (struct sockaddr *)&saddr, sizeof (saddr)))
    {
      DBUS_SOCKET_SET_ERRNO ();
      goto out0;
    }

  if (listen (temp, 1) == SOCKET_ERROR)
    {
      DBUS_SOCKET_SET_ERRNO ();
      goto out0;
    }

  len = sizeof (saddr);
  if (getsockname (temp, (struct sockaddr *)&saddr, &len))
    {
      DBUS_SOCKET_SET_ERRNO ();
      goto out0;
    }

  socket1 = socket (AF_INET, SOCK_STREAM, 0);
  if (socket1 == INVALID_SOCKET)
    {
      DBUS_SOCKET_SET_ERRNO ();
      goto out0;
    }
  
  arg = 1;
  if (ioctlsocket (socket1, FIONBIO, &arg) == SOCKET_ERROR)
    {
      DBUS_SOCKET_SET_ERRNO ();
      goto out1;
    }

  if (connect (socket1, (struct sockaddr  *)&saddr, len) != SOCKET_ERROR ||
      WSAGetLastError () != WSAEWOULDBLOCK)
    {
      dbus_set_error_const (error, DBUS_ERROR_FAILED,
			    "_dbus_full_duplex_pipe socketpair() emulation failed");
      goto out1;
    }

  FD_ZERO (&read_set);
  FD_SET (temp, &read_set);
  
  tv.tv_sec = 0;
  tv.tv_usec = 0;

  if (select (0, &read_set, NULL, NULL, NULL) == SOCKET_ERROR)
    {
      DBUS_SOCKET_SET_ERRNO ();
      goto out1;
    }

  _dbus_assert (FD_ISSET (temp, &read_set));

  socket2 = accept (temp, (struct sockaddr *) &saddr, &len);
  if (socket2 == INVALID_SOCKET)
    {
      DBUS_SOCKET_SET_ERRNO ();
      goto out1;
    }

  FD_ZERO (&write_set);
  FD_SET (socket1, &write_set);

  tv.tv_sec = 0;
  tv.tv_usec = 0;

  if (select (0, NULL, &write_set, NULL, NULL) == SOCKET_ERROR)
    {
      DBUS_SOCKET_SET_ERRNO ();
      goto out2;
    }

  _dbus_assert (FD_ISSET (socket1, &write_set));

  if (blocking)
    {
      arg = 0;
      if (ioctlsocket (socket1, FIONBIO, &arg) == SOCKET_ERROR)
	{
	  DBUS_SOCKET_SET_ERRNO ();
	  goto out2;
	}

      arg = 0;
      if (ioctlsocket (socket2, FIONBIO, &arg) == SOCKET_ERROR)
	{
	  DBUS_SOCKET_SET_ERRNO ();
	  goto out2;
	}
    }
  else
    {
      arg = 1;
      if (ioctlsocket (socket2, FIONBIO, &arg) == SOCKET_ERROR)
	{
	  DBUS_SOCKET_SET_ERRNO ();
	  goto out2;
	}
    }
      
  
  *fd1 = _dbus_socket_to_handle (socket1);
  *fd2 = _dbus_socket_to_handle (socket2);

  _dbus_verbose ("full-duplex pipe %d:%d <-> %d:%d\n",
                 *fd1, socket1, *fd2, socket2);
  
  closesocket (temp);

  return TRUE;

 out2:
  closesocket (socket2);
 out1:
  closesocket (socket1);
 out0:
  closesocket (temp);

  dbus_set_error (error, _dbus_error_from_errno (errno),
		  "Could not setup socket pair: %s",
		  _dbus_strerror (errno));
  
  return FALSE;
}

int
_dbus_poll_win (DBusPollFD *fds,
            int         n_fds,
            int         timeout_milliseconds)
{
#ifdef DBUS_WIN
  char msg[200], *msgp;
#endif

  fd_set read_set, write_set, err_set;
  int max_fd = 0;
  int i;
  struct timeval tv;
  int ready;
  
  FD_ZERO (&read_set);
  FD_ZERO (&write_set);
  FD_ZERO (&err_set);

#ifdef DBUS_WIN
  _DBUS_LOCK (win_fds);

  _dbus_assert (win_fds != NULL);

  msgp = msg;
  msgp += sprintf (msgp, "select: to=%d ", timeout_milliseconds);
  for (i = 0; i < n_fds; i++)
    {
      static dbus_bool_t warned = FALSE;
      DBusPollFD *fdp = &fds[i];
      int sock;
      int fd = FROM_HANDLE (fdp->fd);

      _dbus_assert (fd >= 0 && fd < win_n_fds);

      if (!warned &&
          win_fds[fd].type != DBUS_WIN_FD_SOCKET)
      {
        _dbus_warn ("Can poll only sockets on Win32");
        warned = TRUE;
      }
      sock = _dbus_decapsulate_quick (fdp->fd);

      if (fdp->events & _DBUS_POLLIN)
        msgp += sprintf (msgp, "R:%d ", sock);

      if (fdp->events & _DBUS_POLLOUT)
        msgp += sprintf (msgp, "W:%d ", sock);

      msgp += sprintf (msgp, "E:%d ", sock);
    }

  msgp += sprintf (msgp, "\n");
  _dbus_verbose ("%s",msg);
#endif

  for (i = 0; i < n_fds; i++)
    {
      DBusPollFD *fdp = &fds[i];
      int sock = _dbus_decapsulate_quick (fdp->fd);

#ifdef DBUS_WIN
      if (win_fds[FROM_HANDLE (fdp->fd)].type != DBUS_WIN_FD_SOCKET)
        continue;
#endif

      if (fdp->events & _DBUS_POLLIN)
        FD_SET (sock, &read_set);

      if (fdp->events & _DBUS_POLLOUT)
        FD_SET (sock, &write_set);

      FD_SET (sock, &err_set);

      max_fd = MAX (max_fd, sock);
    }
    
#ifdef DBUS_WIN
  _DBUS_UNLOCK (win_fds);
#endif
    
  tv.tv_sec = timeout_milliseconds / 1000;
  tv.tv_usec = (timeout_milliseconds % 1000) * 1000;

  ready = select (max_fd + 1, &read_set, &write_set, &err_set,
                  timeout_milliseconds < 0 ? NULL : &tv);

#ifdef DBUS_WIN
  if (DBUS_SOCKET_API_RETURNS_ERROR (ready))
    {
      DBUS_SOCKET_SET_ERRNO ();
      if (errno != EWOULDBLOCK)
        _dbus_verbose ("select: failed: %s\n", _dbus_strerror (errno));
    }
  else if (ready == 0)
      _dbus_verbose ("select: = 0\n");
  else 
#endif
  if (ready > 0)
    {
#ifdef DBUS_WIN
      msgp = msg;
      msgp += sprintf (msgp, "select: = %d:", ready);
      _DBUS_LOCK (win_fds);
      for (i = 0; i < n_fds; i++)
        {
          DBusPollFD *fdp = &fds[i];
          int sock = _dbus_decapsulate_quick (fdp->fd);

          if (FD_ISSET (sock, &read_set))
            msgp += sprintf (msgp, "R:%d ", sock);

          if (FD_ISSET (sock, &write_set))
            msgp += sprintf (msgp, "W:%d ", sock);

          if (FD_ISSET (sock, &err_set))
            msgp += sprintf (msgp, "E:%d ", sock);
        }
      msgp += sprintf (msgp, "\n");
      _dbus_verbose ("%s",msg);
#endif

      for (i = 0; i < n_fds; i++)
      {
        DBusPollFD *fdp = &fds[i];
        int sock = _dbus_decapsulate_quick (fdp->fd);

        fdp->revents = 0;

        if (FD_ISSET (sock, &read_set))
          fdp->revents |= _DBUS_POLLIN;

        if (FD_ISSET (sock, &write_set))
          fdp->revents |= _DBUS_POLLOUT;

        if (FD_ISSET (sock, &err_set))
          fdp->revents |= _DBUS_POLLERR;
      }
#ifdef DBUS_WIN
      _DBUS_UNLOCK (win_fds);
#endif
    }
  return ready;
}



/************************************************************************
 
 error handling

 ************************************************************************/


/**
 * Assigns an error name and message corresponding to a Win32 error
 * code to a DBusError. Does nothing if error is #NULL.
 *
 * @param error the error.
 * @param code the Win32 error code
 */
void
_dbus_win_set_error_from_win_error (DBusError *error, 
                                    int        code)
{
  char *msg;

  /* As we want the English message, use the A API */
  FormatMessageA (FORMAT_MESSAGE_ALLOCATE_BUFFER |
		  FORMAT_MESSAGE_IGNORE_INSERTS |
		  FORMAT_MESSAGE_FROM_SYSTEM,
		  NULL, code, MAKELANGID (LANG_ENGLISH, SUBLANG_ENGLISH_US),
		  (LPTSTR) &msg, 0, NULL);
  if (msg)
    {
      char *msg_copy;

      msg_copy = dbus_malloc (strlen (msg));
      strcpy (msg_copy, msg);
      LocalFree (msg);
      
      dbus_set_error (error, "Win32 error", "%s", msg_copy);
    }
  else
    dbus_set_error_const (error, "Win32 error", "Unknown error code or FormatMessage failed");
}

void
_dbus_win_warn_win_error (const char *message,
                          int         code)
{
  DBusError error;

  dbus_error_init (&error);
  _dbus_win_set_error_from_win_error (&error, code);
  _dbus_warn ("%s: %s\n", message, error.message);
  dbus_error_free (&error);
}

/**
 * @addtogroup DBusInternalsUtils
 * @{
 */
#ifndef DBUS_DISABLE_ASSERT
/**
 * Aborts the program with SIGABRT (dumping core).
 */
void
_dbus_abort (void)
{
#ifdef DBUS_ENABLE_VERBOSE_MODE
  const char *s;
  s = _dbus_getenv ("DBUS_PRINT_BACKTRACE");
  if (s && *s)
    _dbus_print_backtrace ();
#endif
#if defined (DBUS_WIN) && defined (__GNUC__)
  if (IsDebuggerPresent ())
    __asm__ __volatile__ ("int $03");
#endif
  abort ();
  _exit (1); /* in case someone manages to ignore SIGABRT */
}
#endif

/**
 * A wrapper around strerror() because some platforms
 * may be lame and not have strerror().
 *
 * @param error_number errno.
 * @returns error description.
 */
const char*
_dbus_strerror_win (int error_number)
{
  const char *msg;
  
#ifdef DBUS_WIN
  switch (error_number)
    {
    case WSAEINTR: return "Interrupted function call";
    case WSAEACCES: return "Permission denied";
    case WSAEFAULT: return "Bad address";
    case WSAEINVAL: return "Invalid argument";
    case WSAEMFILE: return "Too many open files";
    case WSAEWOULDBLOCK: return "Resource temporarily unavailable";
    case WSAEINPROGRESS: return "Operation now in progress";
    case WSAEALREADY: return "Operation already in progress";
    case WSAENOTSOCK: return "Socket operation on nonsocket";
    case WSAEDESTADDRREQ: return "Destination address required";
    case WSAEMSGSIZE: return "Message too long";
    case WSAEPROTOTYPE: return "Protocol wrong type for socket";
    case WSAENOPROTOOPT: return "Bad protocol option";
    case WSAEPROTONOSUPPORT: return "Protocol not supported";
    case WSAESOCKTNOSUPPORT: return "Socket type not supported";
    case WSAEOPNOTSUPP: return "Operation not supported";
    case WSAEPFNOSUPPORT: return "Protocol family not supported";
    case WSAEAFNOSUPPORT: return "Address family not supported by protocol family";
    case WSAEADDRINUSE: return "Address already in use";
    case WSAEADDRNOTAVAIL: return "Cannot assign requested address";
    case WSAENETDOWN: return "Network is down";
    case WSAENETUNREACH: return "Network is unreachable";
    case WSAENETRESET: return "Network dropped connection on reset";
    case WSAECONNABORTED: return "Software caused connection abort";
    case WSAECONNRESET: return "Connection reset by peer";
    case WSAENOBUFS: return "No buffer space available";
    case WSAEISCONN: return "Socket is already connected";
    case WSAENOTCONN: return "Socket is not connected";
    case WSAESHUTDOWN: return "Cannot send after socket shutdown";
    case WSAETIMEDOUT: return "Connection timed out";
    case WSAECONNREFUSED: return "Connection refused";
    case WSAEHOSTDOWN: return "Host is down";
    case WSAEHOSTUNREACH: return "No route to host";
    case WSAEPROCLIM: return "Too many processes";
    case WSAEDISCON: return "Graceful shutdown in progress";
    case WSATYPE_NOT_FOUND: return "Class type not found";
    case WSAHOST_NOT_FOUND: return "Host not found";
    case WSATRY_AGAIN: return "Nonauthoritative host not found";
    case WSANO_RECOVERY: return "This is a nonrecoverable error";
    case WSANO_DATA: return "Valid name, no data record of requested type";
    case WSA_INVALID_HANDLE: return "Specified event object handle is invalid";
    case WSA_INVALID_PARAMETER: return "One or more parameters are invalid";
    case WSA_IO_INCOMPLETE: return "Overlapped I/O event object not in signaled state";
    case WSA_IO_PENDING: return "Overlapped operations will complete later";
    case WSA_NOT_ENOUGH_MEMORY: return "Insufficient memory available";
    case WSA_OPERATION_ABORTED: return "Overlapped operation aborted";
#ifdef WSAINVALIDPROCTABLE
    case WSAINVALIDPROCTABLE: return "Invalid procedure table from service provider";
#endif
#ifdef WSAINVALIDPROVIDER
    case WSAINVALIDPROVIDER: return "Invalid service provider version number";
#endif
#ifdef WSAPROVIDERFAILEDINIT
    case WSAPROVIDERFAILEDINIT: return "Unable to initialize a service provider";
#endif
    case WSASYSCALLFAILURE: return "System call failure";
    }
#endif
  msg = strerror (error_number);
  if (msg == NULL)
    msg = "unknown";

  return msg;
}


#include <lmerr.h>
/* lan manager error codes */ 
const char*
_dbus_lm_strerror(int error_number)
{
  const char *msg;
  switch (error_number)
    {
    case NERR_NetNotStarted:                return "The workstation driver is not installed.";
    case NERR_UnknownServer:                return "The server could not be located.";
    case NERR_ShareMem:                     return "An internal error occurred. The network cannot access a shared memory segment.";
    case NERR_NoNetworkResource:            return "A network resource shortage occurred.";
    case NERR_RemoteOnly:                   return "This operation is not supported on workstations.";
    case NERR_DevNotRedirected:             return "The device is not connected.";
    case NERR_ServerNotStarted:             return "The Server service is not started.";
    case NERR_ItemNotFound:                 return "The queue is empty.";
    case NERR_UnknownDevDir:                return "The device or directory does not exist.";
    case NERR_RedirectedPath:               return "The operation is invalid on a redirected resource.";
    case NERR_DuplicateShare:               return "The name has already been shared.";
    case NERR_NoRoom:                       return "The server is currently out of the requested resource.";
    case NERR_TooManyItems:                 return "Requested addition of items exceeds the maximum allowed.";
    case NERR_InvalidMaxUsers:              return "The Peer service supports only two simultaneous users.";
    case NERR_BufTooSmall:                  return "The API return buffer is too small.";
    case NERR_RemoteErr:                    return "A remote API error occurred.";
    case NERR_LanmanIniError:               return "An error occurred when opening or reading the configuration file.";
    case NERR_NetworkError:                 return "A general network error occurred.";
    case NERR_WkstaInconsistentState:       return "The Workstation service is in an inconsistent state. Restart the computer before restarting the Workstation service.";
    case NERR_WkstaNotStarted:              return "The Workstation service has not been started.";
    case NERR_BrowserNotStarted:            return "The requested information is not available.";
    case NERR_InternalError:                return "An internal error occurred.";
    case NERR_BadTransactConfig:            return "The server is not configured for transactions.";
    case NERR_InvalidAPI:                   return "The requested API is not supported on the remote server.";
    case NERR_BadEventName:                 return "The event name is invalid.";
    case NERR_DupNameReboot:                return "The computer name already exists on the network. Change it and restart the computer.";
    case NERR_CfgCompNotFound:              return "The specified component could not be found in the configuration information.";
    case NERR_CfgParamNotFound:             return "The specified parameter could not be found in the configuration information.";
    case NERR_LineTooLong:                  return "A line in the configuration file is too long.";
    case NERR_QNotFound:                    return "The printer does not exist.";
    case NERR_JobNotFound:                  return "The print job does not exist.";
    case NERR_DestNotFound:                 return "The printer destination cannot be found.";
    case NERR_DestExists:                   return "The printer destination already exists.";
    case NERR_QExists:                      return "The printer queue already exists.";
    case NERR_QNoRoom:                      return "No more printers can be added.";
    case NERR_JobNoRoom:                    return "No more print jobs can be added.";
    case NERR_DestNoRoom:                   return "No more printer destinations can be added.";
    case NERR_DestIdle:                     return "This printer destination is idle and cannot accept control operations.";
    case NERR_DestInvalidOp:                return "This printer destination request contains an invalid control function.";
    case NERR_ProcNoRespond:                return "The print processor is not responding.";
    case NERR_SpoolerNotLoaded:             return "The spooler is not running.";
    case NERR_DestInvalidState:             return "This operation cannot be performed on the print destination in its current state.";
    case NERR_QInvalidState:                return "This operation cannot be performed on the printer queue in its current state.";
    case NERR_JobInvalidState:              return "This operation cannot be performed on the print job in its current state.";
    case NERR_SpoolNoMemory:                return "A spooler memory allocation failure occurred.";
    case NERR_DriverNotFound:               return "The device driver does not exist.";
    case NERR_DataTypeInvalid:              return "The data type is not supported by the print processor.";
    case NERR_ProcNotFound:                 return "The print processor is not installed.";
    case NERR_ServiceTableLocked:           return "The service database is locked.";
    case NERR_ServiceTableFull:             return "The service table is full.";
    case NERR_ServiceInstalled:             return "The requested service has already been started.";
    case NERR_ServiceEntryLocked:           return "The service does not respond to control actions.";
    case NERR_ServiceNotInstalled:          return "The service has not been started.";
    case NERR_BadServiceName:               return "The service name is invalid.";
    case NERR_ServiceCtlTimeout:            return "The service is not responding to the control function.";
    case NERR_ServiceCtlBusy:               return "The service control is busy.";
    case NERR_BadServiceProgName:           return "The configuration file contains an invalid service program name.";
    case NERR_ServiceNotCtrl:               return "The service could not be controlled in its present state.";
    case NERR_ServiceKillProc:              return "The service ended abnormally.";
    case NERR_ServiceCtlNotValid:           return "The requested pause or stop is not valid for this service.";
    case NERR_NotInDispatchTbl:             return "The service control dispatcher could not find the service name in the dispatch table.";
    case NERR_BadControlRecv:               return "The service control dispatcher pipe read failed.";
    case NERR_ServiceNotStarting:           return "A thread for the new service could not be created.";
    case NERR_AlreadyLoggedOn:              return "This workstation is already logged on to the local-area network.";
    case NERR_NotLoggedOn:                  return "The workstation is not logged on to the local-area network.";
    case NERR_BadUsername:                  return "The user name or group name parameter is invalid.";
    case NERR_BadPassword:                  return "The password parameter is invalid.";
    case NERR_UnableToAddName_W:            return "@W The logon processor did not add the message alias.";
    case NERR_UnableToAddName_F:            return "The logon processor did not add the message alias.";
    case NERR_UnableToDelName_W:            return "@W The logoff processor did not delete the message alias.";
    case NERR_UnableToDelName_F:            return "The logoff processor did not delete the message alias.";
    case NERR_LogonsPaused:                 return "Network logons are paused.";
    case NERR_LogonServerConflict:          return "A centralized logon-server conflict occurred.";
    case NERR_LogonNoUserPath:              return "The server is configured without a valid user path.";
    case NERR_LogonScriptError:             return "An error occurred while loading or running the logon script.";
    case NERR_StandaloneLogon:              return "The logon server was not specified. Your computer will be logged on as STANDALONE.";
    case NERR_LogonServerNotFound:          return "The logon server could not be found.";
    case NERR_LogonDomainExists:            return "There is already a logon domain for this computer.";
    case NERR_NonValidatedLogon:            return "The logon server could not validate the logon.";
    case NERR_ACFNotFound:                  return "The security database could not be found.";
    case NERR_GroupNotFound:                return "The group name could not be found.";
    case NERR_UserNotFound:                 return "The user name could not be found.";
    case NERR_ResourceNotFound:             return "The resource name could not be found.";
    case NERR_GroupExists:                  return "The group already exists.";
    case NERR_UserExists:                   return "The user account already exists.";
    case NERR_ResourceExists:               return "The resource permission list already exists.";
    case NERR_NotPrimary:                   return "This operation is only allowed on the primary domain controller of the domain.";
    case NERR_ACFNotLoaded:                 return "The security database has not been started.";
    case NERR_ACFNoRoom:                    return "There are too many names in the user accounts database.";
    case NERR_ACFFileIOFail:                return "A disk I/O failure occurred.";
    case NERR_ACFTooManyLists:              return "The limit of 64 entries per resource was exceeded.";
    case NERR_UserLogon:                    return "Deleting a user with a session is not allowed.";
    case NERR_ACFNoParent:                  return "The parent directory could not be located.";
    case NERR_CanNotGrowSegment:            return "Unable to add to the security database session cache segment.";
    case NERR_SpeGroupOp:                   return "This operation is not allowed on this special group.";
    case NERR_NotInCache:                   return "This user is not cached in user accounts database session cache.";
    case NERR_UserInGroup:                  return "The user already belongs to this group.";
    case NERR_UserNotInGroup:               return "The user does not belong to this group.";
    case NERR_AccountUndefined:             return "This user account is undefined.";
    case NERR_AccountExpired:               return "This user account has expired.";
    case NERR_InvalidWorkstation:           return "The user is not allowed to log on from this workstation.";
    case NERR_InvalidLogonHours:            return "The user is not allowed to log on at this time.";
    case NERR_PasswordExpired:              return "The password of this user has expired.";
    case NERR_PasswordCantChange:           return "The password of this user cannot change.";
    case NERR_PasswordHistConflict:         return "This password cannot be used now.";
    case NERR_PasswordTooShort:             return "The password does not meet the password policy requirements. Check the minimum password length, password complexity and password history requirements.";
    case NERR_PasswordTooRecent:            return "The password of this user is too recent to change.";
    case NERR_InvalidDatabase:              return "The security database is corrupted.";
    case NERR_DatabaseUpToDate:             return "No updates are necessary to this replicant network/local security database.";
    case NERR_SyncRequired:                 return "This replicant database is outdated; synchronization is required.";
    case NERR_UseNotFound:                  return "The network connection could not be found.";
    case NERR_BadAsgType:                   return "This asg_type is invalid.";
    case NERR_DeviceIsShared:               return "This device is currently being shared.";
    case NERR_NoComputerName:               return "The computer name could not be added as a message alias. The name may already exist on the network.";
    case NERR_MsgAlreadyStarted:            return "The Messenger service is already started.";
    case NERR_MsgInitFailed:                return "The Messenger service failed to start.";
    case NERR_NameNotFound:                 return "The message alias could not be found on the network.";
    case NERR_AlreadyForwarded:             return "This message alias has already been forwarded.";
    case NERR_AddForwarded:                 return "This message alias has been added but is still forwarded.";
    case NERR_AlreadyExists:                return "This message alias already exists locally.";
    case NERR_TooManyNames:                 return "The maximum number of added message aliases has been exceeded.";
    case NERR_DelComputerName:              return "The computer name could not be deleted.";
    case NERR_LocalForward:                 return "Messages cannot be forwarded back to the same workstation.";
    case NERR_GrpMsgProcessor:              return "An error occurred in the domain message processor.";
    case NERR_PausedRemote:                 return "The message was sent, but the recipient has paused the Messenger service.";
    case NERR_BadReceive:                   return "The message was sent but not received.";
    case NERR_NameInUse:                    return "The message alias is currently in use. Try again later.";
    case NERR_MsgNotStarted:                return "The Messenger service has not been started.";
    case NERR_NotLocalName:                 return "The name is not on the local computer.";
    case NERR_NoForwardName:                return "The forwarded message alias could not be found on the network.";
    case NERR_RemoteFull:                   return "The message alias table on the remote station is full.";
    case NERR_NameNotForwarded:             return "Messages for this alias are not currently being forwarded.";
    case NERR_TruncatedBroadcast:           return "The broadcast message was truncated.";
    case NERR_InvalidDevice:                return "This is an invalid device name.";
    case NERR_WriteFault:                   return "A write fault occurred.";
    case NERR_DuplicateName:                return "A duplicate message alias exists on the network.";
    case NERR_DeleteLater:                  return "@W This message alias will be deleted later.";
    case NERR_IncompleteDel:                return "The message alias was not successfully deleted from all networks.";
    case NERR_MultipleNets:                 return "This operation is not supported on computers with multiple networks.";
    case NERR_NetNameNotFound:              return "This shared resource does not exist.";
    case NERR_DeviceNotShared:              return "This device is not shared.";
    case NERR_ClientNameNotFound:           return "A session does not exist with that computer name.";
    case NERR_FileIdNotFound:               return "There is not an open file with that identification number.";
    case NERR_ExecFailure:                  return "A failure occurred when executing a remote administration command.";
    case NERR_TmpFile:                      return "A failure occurred when opening a remote temporary file.";
    case NERR_TooMuchData:                  return "The data returned from a remote administration command has been truncated to 64K.";
    case NERR_DeviceShareConflict:          return "This device cannot be shared as both a spooled and a non-spooled resource.";
    case NERR_BrowserTableIncomplete:       return "The information in the list of servers may be incorrect.";
    case NERR_NotLocalDomain:               return "The computer is not active in this domain.";
#ifdef NERR_IsDfsShare
    case NERR_IsDfsShare:                   return "The share must be removed from the Distributed File System before it can be deleted.";
#endif
    case NERR_DevInvalidOpCode:             return "The operation is invalid for this device.";
    case NERR_DevNotFound:                  return "This device cannot be shared.";
    case NERR_DevNotOpen:                   return "This device was not open.";
    case NERR_BadQueueDevString:            return "This device name list is invalid.";
    case NERR_BadQueuePriority:             return "The queue priority is invalid.";
    case NERR_NoCommDevs:                   return "There are no shared communication devices.";
    case NERR_QueueNotFound:                return "The queue you specified does not exist.";
    case NERR_BadDevString:                 return "This list of devices is invalid.";
    case NERR_BadDev:                       return "The requested device is invalid.";
    case NERR_InUseBySpooler:               return "This device is already in use by the spooler.";
    case NERR_CommDevInUse:                 return "This device is already in use as a communication device.";
    case NERR_InvalidComputer:              return "This computer name is invalid.";
    case NERR_MaxLenExceeded:               return "The string and prefix specified are too long.";
    case NERR_BadComponent:                 return "This path component is invalid.";
    case NERR_CantType:                     return "Could not determine the type of input.";
    case NERR_TooManyEntries:               return "The buffer for types is not big enough.";
    case NERR_ProfileFileTooBig:            return "Profile files cannot exceed 64K.";
    case NERR_ProfileOffset:                return "The start offset is out of range.";
    case NERR_ProfileCleanup:               return "The system cannot delete current connections to network resources.";
    case NERR_ProfileUnknownCmd:            return "The system was unable to parse the command line in this file.";
    case NERR_ProfileLoadErr:               return "An error occurred while loading the profile file.";
    case NERR_ProfileSaveErr:               return "@W Errors occurred while saving the profile file. The profile was partially saved.";
    case NERR_LogOverflow:                  return "Log file %1 is full.";
    case NERR_LogFileChanged:               return "This log file has changed between reads.";
    case NERR_LogFileCorrupt:               return "Log file %1 is corrupt.";
    case NERR_SourceIsDir:                  return "The source path cannot be a directory.";
    case NERR_BadSource:                    return "The source path is illegal.";
    case NERR_BadDest:                      return "The destination path is illegal.";
    case NERR_DifferentServers:             return "The source and destination paths are on different servers.";
    case NERR_RunSrvPaused:                 return "The Run server you requested is paused.";
    case NERR_ErrCommRunSrv:                return "An error occurred when communicating with a Run server.";
    case NERR_ErrorExecingGhost:            return "An error occurred when starting a background process.";
    case NERR_ShareNotFound:                return "The shared resource you are connected to could not be found.";
    case NERR_InvalidLana:                  return "The LAN adapter number is invalid.";
    case NERR_OpenFiles:                    return "There are open files on the connection.";
    case NERR_ActiveConns:                  return "Active connections still exist.";
    case NERR_BadPasswordCore:              return "This share name or password is invalid.";
    case NERR_DevInUse:                     return "The device is being accessed by an active process.";
    case NERR_LocalDrive:                   return "The drive letter is in use locally.";
    case NERR_AlertExists:                  return "The specified client is already registered for the specified event.";
    case NERR_TooManyAlerts:                return "The alert table is full.";
    case NERR_NoSuchAlert:                  return "An invalid or nonexistent alert name was raised.";
    case NERR_BadRecipient:                 return "The alert recipient is invalid.";
    case NERR_AcctLimitExceeded:            return "A user's session with this server has been deleted.";
    case NERR_InvalidLogSeek:               return "The log file does not contain the requested record number.";
    case NERR_BadUasConfig:                 return "The user accounts database is not configured correctly.";
    case NERR_InvalidUASOp:                 return "This operation is not permitted when the Netlogon service is running.";
    case NERR_LastAdmin:                    return "This operation is not allowed on the last administrative account.";
    case NERR_DCNotFound:                   return "Could not find domain controller for this domain.";
    case NERR_LogonTrackingError:           return "Could not set logon information for this user.";
    case NERR_NetlogonNotStarted:           return "The Netlogon service has not been started.";
    case NERR_CanNotGrowUASFile:            return "Unable to add to the user accounts database.";
    case NERR_TimeDiffAtDC:                 return "This server's clock is not synchronized with the primary domain controller's clock.";
    case NERR_PasswordMismatch:             return "A password mismatch has been detected.";
    case NERR_NoSuchServer:                 return "The server identification does not specify a valid server.";
    case NERR_NoSuchSession:                return "The session identification does not specify a valid session.";
    case NERR_NoSuchConnection:             return "The connection identification does not specify a valid connection.";
    case NERR_TooManyServers:               return "There is no space for another entry in the table of available servers.";
    case NERR_TooManySessions:              return "The server has reached the maximum number of sessions it supports.";
    case NERR_TooManyConnections:           return "The server has reached the maximum number of connections it supports.";
    case NERR_TooManyFiles:                 return "The server cannot open more files because it has reached its maximum number.";
    case NERR_NoAlternateServers:           return "There are no alternate servers registered on this server.";
    case NERR_TryDownLevel:                 return "Try down-level (remote admin protocol) version of API instead.";
    case NERR_UPSDriverNotStarted:          return "The UPS driver could not be accessed by the UPS service.";
    case NERR_UPSInvalidConfig:             return "The UPS service is not configured correctly.";
    case NERR_UPSInvalidCommPort:           return "The UPS service could not access the specified Comm Port.";
    case NERR_UPSSignalAsserted:            return "The UPS indicated a line fail or low battery situation. Service not started.";
    case NERR_UPSShutdownFailed:            return "The UPS service failed to perform a system shut down.";
    case NERR_BadDosRetCode:                return "The program below returned an MS-DOS error code:";
    case NERR_ProgNeedsExtraMem:            return "The program below needs more memory:";                                                      
    case NERR_BadDosFunction:               return "The program below called an unsupported MS-DOS function:";
    case NERR_RemoteBootFailed:             return "The workstation failed to boot.";
    case NERR_BadFileCheckSum:              return "The file below is corrupt.";
    case NERR_NoRplBootSystem:              return "No loader is specified in the boot-block definition file.";
    case NERR_RplLoadrNetBiosErr:           return "NetBIOS returned an error:      The NCB and SMB are dumped above.";
    case NERR_RplLoadrDiskErr:              return "A disk I/O error occurred.";
    case NERR_ImageParamErr:                return "Image parameter substitution failed.";
    case NERR_TooManyImageParams:           return "Too many image parameters cross disk sector boundaries.";
    case NERR_NonDosFloppyUsed:             return "The image was not generated from an MS-DOS diskette formatted with /S.";
    case NERR_RplBootRestart:               return "Remote boot will be restarted later.";
    case NERR_RplSrvrCallFailed:            return "The call to the Remoteboot server failed.";
    case NERR_CantConnectRplSrvr:           return "Cannot connect to the Remoteboot server.";
    case NERR_CantOpenImageFile:            return "Cannot open image file on the Remoteboot server.";
    case NERR_CallingRplSrvr:               return "Connecting to the Remoteboot server...";
    case NERR_StartingRplBoot:              return "Connecting to the Remoteboot server...";
    case NERR_RplBootServiceTerm:           return "Remote boot service was stopped; check the error log for the cause of the problem.";
    case NERR_RplBootStartFailed:           return "Remote boot startup failed; check the error log for the cause of the problem.";
    case NERR_RPL_CONNECTED:                return "A second connection to a Remoteboot resource is not allowed.";
    case NERR_BrowserConfiguredToNotRun:    return "The browser service was configured with MaintainServerList=No.";
    case NERR_RplNoAdaptersStarted:         return "Service failed to start since none of the network adapters started with this service.";
    case NERR_RplBadRegistry:               return "Service failed to start due to bad startup information in the registry.";
    case NERR_RplBadDatabase:               return "Service failed to start because its database is absent or corrupt.";
    case NERR_RplRplfilesShare:             return "Service failed to start because RPLFILES share is absent.";
    case NERR_RplNotRplServer:              return "Service failed to start because RPLUSER group is absent.";
    case NERR_RplCannotEnum:                return "Cannot enumerate service records.";
    case NERR_RplWkstaInfoCorrupted:        return "Workstation record information has been corrupted.";
    case NERR_RplWkstaNotFound:             return "Workstation record was not found.";
    case NERR_RplWkstaNameUnavailable:      return "Workstation name is in use by some other workstation.";
    case NERR_RplProfileInfoCorrupted:      return "Profile record information has been corrupted.";
    case NERR_RplProfileNotFound:           return "Profile record was not found.";
    case NERR_RplProfileNameUnavailable:    return "Profile name is in use by some other profile.";
    case NERR_RplProfileNotEmpty:           return "There are workstations using this profile.";
    case NERR_RplConfigInfoCorrupted:       return "Configuration record information has been corrupted.";
    case NERR_RplConfigNotFound:            return "Configuration record was not found.";
    case NERR_RplAdapterInfoCorrupted:      return "Adapter ID record information has been corrupted.";
    case NERR_RplInternal:                  return "An internal service error has occurred.";
    case NERR_RplVendorInfoCorrupted:       return "Vendor ID record information has been corrupted.";
    case NERR_RplBootInfoCorrupted:         return "Boot block record information has been corrupted.";
    case NERR_RplWkstaNeedsUserAcct:        return "The user account for this workstation record is missing.";
    case NERR_RplNeedsRPLUSERAcct:          return "The RPLUSER local group could not be found.";
    case NERR_RplBootNotFound:              return "Boot block record was not found.";
    case NERR_RplIncompatibleProfile:       return "Chosen profile is incompatible with this workstation.";
    case NERR_RplAdapterNameUnavailable:    return "Chosen network adapter ID is in use by some other workstation.";
    case NERR_RplConfigNotEmpty:            return "There are profiles using this configuration.";
    case NERR_RplBootInUse:                 return "There are workstations, profiles, or configurations using this boot block.";
    case NERR_RplBackupDatabase:            return "Service failed to backup Remoteboot database.";
    case NERR_RplAdapterNotFound:           return "Adapter record was not found.";
    case NERR_RplVendorNotFound:            return "Vendor record was not found.";
    case NERR_RplVendorNameUnavailable:     return "Vendor name is in use by some other vendor record.";
    case NERR_RplBootNameUnavailable:       return "(boot name, vendor ID) is in use by some other boot block record.";
    case NERR_RplConfigNameUnavailable:     return "Configuration name is in use by some other configuration.";
    case NERR_DfsInternalCorruption:        return "The internal database maintained by the Dfs service is corrupt.";
    case NERR_DfsVolumeDataCorrupt:         return "One of the records in the internal Dfs database is corrupt.";
    case NERR_DfsNoSuchVolume:              return "There is no DFS name whose entry path matches the input Entry Path.";
    case NERR_DfsVolumeAlreadyExists:       return "A root or link with the given name already exists.";
    case NERR_DfsAlreadyShared:             return "The server share specified is already shared in the Dfs.";
    case NERR_DfsNoSuchShare:               return "The indicated server share does not support the indicated DFS namespace.";
    case NERR_DfsNotALeafVolume:            return "The operation is not valid on this portion of the namespace.";
    case NERR_DfsLeafVolume:                return "The operation is not valid on this portion of the namespace.";
    case NERR_DfsVolumeHasMultipleServers:  return "The operation is ambiguous because the link has multiple servers.";
    case NERR_DfsCantCreateJunctionPoint:   return "Unable to create a link.";
    case NERR_DfsServerNotDfsAware:         return "The server is not Dfs Aware.";
    case NERR_DfsBadRenamePath:             return "The specified rename target path is invalid.";
    case NERR_DfsVolumeIsOffline:           return "The specified DFS link is offline.";
    case NERR_DfsNoSuchServer:              return "The specified server is not a server for this link.";
    case NERR_DfsCyclicalName:              return "A cycle in the Dfs name was detected.";
    case NERR_DfsNotSupportedInServerDfs:   return "The operation is not supported on a server-based Dfs.";
    case NERR_DfsDuplicateService:          return "This link is already supported by the specified server-share.";
    case NERR_DfsCantRemoveLastServerShare: return "Can't remove the last server-share supporting this root or link.";
    case NERR_DfsVolumeIsInterDfs:          return "The operation is not supported for an Inter-DFS link.";
    case NERR_DfsInconsistent:              return "The internal state of the Dfs Service has become inconsistent.";
    case NERR_DfsServerUpgraded:            return "The Dfs Service has been installed on the specified server.";
    case NERR_DfsDataIsIdentical:           return "The Dfs data being reconciled is identical.";
    case NERR_DfsCantRemoveDfsRoot:         return "The DFS root cannot be deleted. Uninstall DFS if required.";
    case NERR_DfsChildOrParentInDfs:        return "A child or parent directory of the share is already in a Dfs.";
    case NERR_DfsInternalError:             return "Dfs internal error.";
/* the following are not defined in mingw */
#if 0
    case NERR_SetupAlreadyJoined:           return "This machine is already joined to a domain.";
    case NERR_SetupNotJoined:               return "This machine is not currently joined to a domain.";
    case NERR_SetupDomainController:        return "This machine is a domain controller and cannot be unjoined from a domain.";
    case NERR_DefaultJoinRequired:          return "The destination domain controller does not support creating machine accounts in OUs.";
    case NERR_InvalidWorkgroupName:         return "The specified workgroup name is invalid.";
    case NERR_NameUsesIncompatibleCodePage: return "The specified computer name is incompatible with the default language used on the domain controller.";
    case NERR_ComputerAccountNotFound:      return "The specified computer account could not be found.";
    case NERR_PersonalSku:                  return "This version of Windows cannot be joined to a domain.";
    case NERR_PasswordMustChange:           return "The password must change at the next logon.";
    case NERR_AccountLockedOut:             return "The account is locked out.";
    case NERR_PasswordTooLong:              return "The password is too long.";
    case NERR_PasswordNotComplexEnough:     return "The password does not meet the complexity policy.";
    case NERR_PasswordFilterError:          return "The password does not meet the requirements of the password filter DLLs.";
#endif 
		}
  msg = strerror (error_number);
  if (msg == NULL)
    msg = "unknown";

  return msg;
}









/******************************************************************************

Original CVS version of dbus-sysdeps.c

******************************************************************************/
/* -*- mode: C; c-file-style: "gnu" -*- */
/* dbus-sysdeps.c Wrappers around system/libc features (internal to D-Bus implementation)
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

#include "dbus-internals.h"
#include "dbus-sysdeps.h"
#include "dbus-threads.h"
#include "dbus-protocol.h"
#include "dbus-string.h"
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>

#ifdef DBUS_WIN
#include "dbus-sysdeps-win.h"
#include "dbus-hash.h"
#include "dbus-sockets-win.h"
#else
#include <unistd.h>
#include <sys/socket.h>
#include <dirent.h>
#include <sys/un.h>
#include <pwd.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <netdb.h>
#include <grp.h>
#endif

#include <time.h>
#include <locale.h>
#include <sys/stat.h>

#ifdef HAVE_WRITEV
#include <sys/uio.h>
#endif
#ifdef HAVE_POLL
#include <sys/poll.h>
#endif
#ifdef HAVE_BACKTRACE
#include <execinfo.h>
#endif
#ifdef HAVE_GETPEERUCRED
#include <ucred.h>
#endif

#ifndef O_BINARY
#define O_BINARY 0
#endif

#ifndef HAVE_SOCKLEN_T
#define socklen_t int
#endif

_DBUS_DEFINE_GLOBAL_LOCK (win_fds);
_DBUS_DEFINE_GLOBAL_LOCK (sid_atom_cache);

#ifndef DBUS_WIN
#define _dbus_decapsulate_quick(i)       (i)
#define DBUS_SOCKET_IS_INVALID(s)        ((s) < 0)
#define DBUS_SOCKET_API_RETURNS_ERROR(n) ((n) < 0)
#define DBUS_SOCKET_SET_ERRNO()          /* empty */
#define DBUS_CLOSE_SOCKET(s)             close(s)
#endif

/**
 * @addtogroup DBusInternalsUtils
 * @{
 */
#if !defined(DBUS_DISABLE_ASSERT) && !defined(DBUS_WIN)
/**
 * Aborts the program with SIGABRT (dumping core).
 */
void
_dbus_abort (void)
{
#ifdef DBUS_ENABLE_VERBOSE_MODE
  const char *s;
  s = _dbus_getenv ("DBUS_PRINT_BACKTRACE");
  if (s && *s)
    _dbus_print_backtrace ();
#endif
#if defined (DBUS_WIN) && defined (__GNUC__)
  if (IsDebuggerPresent ())
    __asm__ __volatile__ ("int $03");
#endif
  abort ();
  _exit (1); /* in case someone manages to ignore SIGABRT */
}
#endif

int _dbus_mkdir (const char *path, 
	             mode_t mode)
{
#ifdef DBUS_WIN
  return _mkdir(path);
#else
  return mkdir(path, mode);
#endif
}


/**
 * Thin wrapper around the read() system call that appends
 * the data it reads to the DBusString buffer. It appends
 * up to the given count, and returns the same value
 * and same errno as read(). The only exception is that
 * _dbus_read() handles EINTR for you. _dbus_read() can
 * return ENOMEM, even though regular UNIX read doesn't.
 *
 * @param fd the file descriptor to read from
 * @param buffer the buffer to append data to
 * @param count the amount of data to read
 * @returns the number of bytes read or -1
 */
int
_dbus_read (int               fd,
            DBusString       *buffer,
            int               count)
{
#ifdef DBUS_WIN
  return _dbus_read_win (fd, buffer, count);
#else
  int bytes_read;
  int start;
  char *data;

  _dbus_assert (count >= 0);
  
  start = _dbus_string_get_length (buffer);

  if (!_dbus_string_lengthen (buffer, count))
    {
      errno = ENOMEM;
      return -1;
    }

  data = _dbus_string_get_data_len (buffer, start, count);

 again:
  
  bytes_read = read (fd, data, count);

  if (bytes_read < 0)
    {
      if (errno == EINTR)
        goto again;
      else
        {
          /* put length back (note that this doesn't actually realloc anything) */
          _dbus_string_set_length (buffer, start);
          return -1;
        }
    }
  else
    {
      /* put length back (doesn't actually realloc) */
      _dbus_string_set_length (buffer, start + bytes_read);

#if 0
      if (bytes_read > 0)
        _dbus_verbose_bytes_of_string (buffer, start, bytes_read);
#endif
      
      return bytes_read;
    }
#endif
}

/**
 * Thin wrapper around the write() system call that writes a part of a
 * DBusString and handles EINTR for you.
 * 
 * @param fd the file descriptor to write
 * @param buffer the buffer to write data from
 * @param start the first byte in the buffer to write
 * @param len the number of bytes to try to write
 * @returns the number of bytes written or -1 on error
 */
int
_dbus_write (int               fd,
             const DBusString *buffer,
             int               start,
             int               len)
{
#ifdef DBUS_WIN
  return _dbus_write_win (fd, buffer, start, len);
#else
  const char *data;
  int bytes_written;
  
  data = _dbus_string_get_const_data_len (buffer, start, len);
  
 again:

  bytes_written = write (fd, data, len);

  if (bytes_written < 0 && errno == EINTR)
    goto again;

#if 0
  if (bytes_written > 0)
    _dbus_verbose_bytes_of_string (buffer, start, bytes_written);
#endif
  
  return bytes_written;
#endif
}

/**
 * Like _dbus_write() but will use writev() if possible
 * to write both buffers in sequence. The return value
 * is the number of bytes written in the first buffer,
 * plus the number written in the second. If the first
 * buffer is written successfully and an error occurs
 * writing the second, the number of bytes in the first
 * is returned (i.e. the error is ignored), on systems that
 * don't have writev. Handles EINTR for you.
 * The second buffer may be #NULL.
 *
 * @param fd the file descriptor
 * @param buffer1 first buffer
 * @param start1 first byte to write in first buffer
 * @param len1 number of bytes to write from first buffer
 * @param buffer2 second buffer, or #NULL
 * @param start2 first byte to write in second buffer
 * @param len2 number of bytes to write in second buffer
 * @returns total bytes written from both buffers, or -1 on error
 */
int
_dbus_write_two (int               fd,
                 const DBusString *buffer1,
                 int               start1,
                 int               len1,
                 const DBusString *buffer2,
                 int               start2,
                 int               len2)
{
  _dbus_assert (buffer1 != NULL);
  _dbus_assert (start1 >= 0);
  _dbus_assert (start2 >= 0);
  _dbus_assert (len1 >= 0);
  _dbus_assert (len2 >= 0);
  
#ifdef DBUS_WIN
  return _dbus_write_two_win(fd, buffer1, start1, len1, buffer2, start2, len2);
#else

#ifdef HAVE_WRITEV
  {
    struct iovec vectors[2];
    const char *data1;
    const char *data2;
    int bytes_written;

    data1 = _dbus_string_get_const_data_len (buffer1, start1, len1);

    if (buffer2 != NULL)
      data2 = _dbus_string_get_const_data_len (buffer2, start2, len2);
    else
      {
        data2 = NULL;
        start2 = 0;
        len2 = 0;
      }
   
    vectors[0].iov_base = (char*) data1;
    vectors[0].iov_len = len1;
    vectors[1].iov_base = (char*) data2;
    vectors[1].iov_len = len2;

  again:
   
    bytes_written = writev (fd,
                            vectors,
                            data2 ? 2 : 1);

    if (bytes_written < 0 && errno == EINTR)
      goto again;
   
    return bytes_written;
  }
#else /* HAVE_WRITEV */
  {
    int ret1;
    
    ret1 = _dbus_write (fd, buffer1, start1, len1);
    if (ret1 == len1 && buffer2 != NULL)
      {
        ret2 = _dbus_write (fd, buffer2, start2, len2);
        if (ret2 < 0)
          ret2 = 0; /* we can't report an error as the first write was OK */
       
        return ret1 + ret2;
      }
    else
      return ret1;
  }
#endif /* !HAVE_WRITEV */   

#endif
}

#define _DBUS_MAX_SUN_PATH_LENGTH 99

/**
 * @def _DBUS_MAX_SUN_PATH_LENGTH
 *
 * Maximum length of the path to a UNIX domain socket,
 * sockaddr_un::sun_path member. POSIX requires that all systems
 * support at least 100 bytes here, including the nul termination.
 * We use 99 for the max value to allow for the nul.
 *
 * We could probably also do sizeof (addr.sun_path)
 * but this way we are the same on all platforms
 * which is probably a good idea.
 */

/**
 * Creates a socket and connects it to the UNIX domain socket at the
 * given path.  The connection fd is returned, and is set up as
 * nonblocking.
 * 
 * On Windows there are no UNIX domain sockets. Instead, connects to a
 * localhost-bound TCP socket, whose port number is stored in a file
 * at the given path.
 * 
 * Uses abstract sockets instead of filesystem-linked sockets if
 * requested (it's possible only on Linux; see "man 7 unix" on Linux).
 * On non-Linux abstract socket usage always fails.
 *
 * @param path the path to UNIX domain socket
 * @param abstract #TRUE to use abstract namespace
 * @param error return location for error code
 * @returns connection file descriptor or -1 on error
 */
int
_dbus_connect_unix_socket (const char     *path,
                           dbus_bool_t     abstract,
                           DBusError      *error)
{
#ifdef DBUS_WIN
  return _dbus_connect_unix_socket_win(path, abstract, error);
#else

  int fd;
  size_t path_len;
  struct sockaddr_un addr;  

  _DBUS_ASSERT_ERROR_IS_CLEAR (error);

  _dbus_verbose ("connecting to unix socket %s abstract=%d\n",
                 path, abstract);
  
  fd = socket (PF_UNIX, SOCK_STREAM, 0);
  
  if (fd < 0)
    {
      dbus_set_error (error,
                      _dbus_error_from_errno (errno),
                      "Failed to create socket: %s",
                      _dbus_strerror (errno)); 
      
      return -1;
    }

  _DBUS_ZERO (addr);
  addr.sun_family = AF_UNIX;
  path_len = strlen (path);

  if (abstract)
    {
#ifdef HAVE_ABSTRACT_SOCKETS
      addr.sun_path[0] = '\0'; /* this is what says "use abstract" */
      path_len++; /* Account for the extra nul byte added to the start of sun_path */

      if (path_len > _DBUS_MAX_SUN_PATH_LENGTH)
        {
          dbus_set_error (error, DBUS_ERROR_BAD_ADDRESS,
                      "Abstract socket name too long\n");
          close (fd);
          return -1;
	}
	
      strncpy (&addr.sun_path[1], path, path_len);
      /* _dbus_verbose_bytes (addr.sun_path, sizeof (addr.sun_path)); */
#else /* HAVE_ABSTRACT_SOCKETS */
      dbus_set_error (error, DBUS_ERROR_NOT_SUPPORTED,
                      "Operating system does not support abstract socket namespace\n");
      close (fd);
      return -1;
#endif /* ! HAVE_ABSTRACT_SOCKETS */
    }
  else
    {
      if (path_len > _DBUS_MAX_SUN_PATH_LENGTH)
        {
          dbus_set_error (error, DBUS_ERROR_BAD_ADDRESS,
                      "Socket name too long\n");
          close (fd);
          return -1;
	}

      strncpy (addr.sun_path, path, path_len);
    }
  
  if (connect (fd, (struct sockaddr*) &addr, _DBUS_STRUCT_OFFSET (struct sockaddr_un, sun_path) + path_len) < 0)
    {      
      dbus_set_error (error,
                      _dbus_error_from_errno (errno),
                      "Failed to connect to socket %s: %s",
                      path, _dbus_strerror (errno));

      close (fd);
      fd = -1;
      
      return -1;
    }

  if (!_dbus_set_fd_nonblocking (fd, error))
    {
      _DBUS_ASSERT_ERROR_IS_SET (error);
      
      close (fd);
      fd = -1;

      return -1;
    }

  return fd;
#endif
}

/**
 * Creates a socket and binds it to the given path,
 * then listens on the socket. The socket is
 * set to be nonblocking.
 *
 * Uses abstract sockets instead of filesystem-linked
 * sockets if requested (it's possible only on Linux;
 * see "man 7 unix" on Linux).
 * On non-Linux abstract socket usage always fails.
 *
 * @param path the socket name
 * @param abstract #TRUE to use abstract namespace
 * @param error return location for errors
 * @returns the listening file descriptor or -1 on error
 */
int
_dbus_listen_unix_socket (const char     *path,
                          dbus_bool_t     abstract,
                          DBusError      *error)
{
#ifdef DBUS_WIN
  return _dbus_listen_unix_socket_win(path, abstract,error);
#else

  int listen_fd;
  struct sockaddr_un addr;
  size_t path_len;

  _DBUS_ASSERT_ERROR_IS_CLEAR (error);

  _dbus_verbose ("listening on unix socket %s abstract=%d\n",
                 path, abstract);
  
  listen_fd = socket (PF_UNIX, SOCK_STREAM, 0);
  
  if (listen_fd < 0)
    {
      dbus_set_error (error, _dbus_error_from_errno (errno),
                      "Failed to create socket \"%s\": %s",
                      path, _dbus_strerror (errno));
      return -1;
    }

  _DBUS_ZERO (addr);
  addr.sun_family = AF_UNIX;
  path_len = strlen (path);
  
  if (abstract)
    {
#ifdef HAVE_ABSTRACT_SOCKETS
      /* remember that abstract names aren't nul-terminated so we rely
       * on sun_path being filled in with zeroes above.
       */
      addr.sun_path[0] = '\0'; /* this is what says "use abstract" */
      path_len++; /* Account for the extra nul byte added to the start of sun_path */

      if (path_len > _DBUS_MAX_SUN_PATH_LENGTH)
        {
          dbus_set_error (error, DBUS_ERROR_BAD_ADDRESS,
                      "Abstract socket name too long\n");
          close (listen_fd);
          return -1;
	}
      
      strncpy (&addr.sun_path[1], path, path_len);
      /* _dbus_verbose_bytes (addr.sun_path, sizeof (addr.sun_path)); */
#else /* HAVE_ABSTRACT_SOCKETS */
      dbus_set_error (error, DBUS_ERROR_NOT_SUPPORTED,
                      "Operating system does not support abstract socket namespace\n");
      close (listen_fd);
      return -1;
#endif /* ! HAVE_ABSTRACT_SOCKETS */
    }
  else
    {
      /* FIXME discussed security implications of this with Nalin,
       * and we couldn't think of where it would kick our ass, but
       * it still seems a bit sucky. It also has non-security suckage;
       * really we'd prefer to exit if the socket is already in use.
       * But there doesn't seem to be a good way to do this.
       *
       * Just to be extra careful, I threw in the stat() - clearly
       * the stat() can't *fix* any security issue, but it at least
       * avoids inadvertent/accidental data loss.
       */
      {
        struct stat sb;

        if (stat (path, &sb) == 0 &&
            S_ISSOCK (sb.st_mode))
          unlink (path);
      }

      if (path_len > _DBUS_MAX_SUN_PATH_LENGTH)
        {
          dbus_set_error (error, DBUS_ERROR_BAD_ADDRESS,
                      "Abstract socket name too long\n");
          close (listen_fd);
          return -1;
	}
	
      strncpy (addr.sun_path, path, path_len);
    }
  
  if (bind (listen_fd, (struct sockaddr*) &addr, _DBUS_STRUCT_OFFSET (struct sockaddr_un, sun_path) + path_len) < 0)
    {
      dbus_set_error (error, _dbus_error_from_errno (errno),
                      "Failed to bind socket \"%s\": %s",
                      path, _dbus_strerror (errno));
      close (listen_fd);
      return -1;
    }

  if (listen (listen_fd, 30 /* backlog */) < 0)
    {
      dbus_set_error (error, _dbus_error_from_errno (errno),
                      "Failed to listen on socket \"%s\": %s",
                      path, _dbus_strerror (errno));
      close (listen_fd);
      return -1;
    }

  if (!_dbus_set_fd_nonblocking (listen_fd, error))
    {
      _DBUS_ASSERT_ERROR_IS_SET (error);
      close (listen_fd);
      return -1;
    }
  
  /* Try opening up the permissions, but if we can't, just go ahead
   * and continue, maybe it will be good enough.
   */
  if (!abstract && chmod (path, 0777) < 0)
    _dbus_warn ("Could not set mode 0777 on socket %s\n",
                path);
  
  return listen_fd;
#endif
}


/**
 * Creates a socket and connects to a socket at the given host 
 * and port. The connection fd is returned, and is set up as
 * nonblocking.
 *
 * @param host the host name to connect to, NULL for loopback
 * @param port the prot to connect to
 * @param error return location for error code
 * @returns connection file descriptor or -1 on error
 */
int
_dbus_connect_tcp_socket (const char     *host,
                          dbus_uint32_t   port,
                          DBusError      *error)
{
  int fd;
  struct sockaddr_in addr;
  struct hostent *he;
  struct in_addr *haddr;
#ifdef DBUS_WIN
  struct in_addr ina;
#endif

  _DBUS_ASSERT_ERROR_IS_CLEAR (error);
  
#ifdef DBUS_WIN
  _dbus_win_startup_winsock ();
#endif

  fd = socket (AF_INET, SOCK_STREAM, 0);
  
  if (DBUS_SOCKET_IS_INVALID (fd))
    {
      DBUS_SOCKET_SET_ERRNO ();
      dbus_set_error (error,
                      _dbus_error_from_errno (errno),
                      "Failed to create socket: %s",
                      _dbus_strerror (errno)); 
      
      return -1;
    }

  if (host == NULL)
    {
    host = "localhost";
#ifdef DBUS_WIN
      ina.s_addr = htonl (INADDR_LOOPBACK);
      haddr = &ina;
#endif
    }

  he = gethostbyname (host);
  if (he == NULL) 
    {
      DBUS_SOCKET_SET_ERRNO ();
      dbus_set_error (error,
                      _dbus_error_from_errno (errno),
                      "Failed to lookup hostname: %s",
                      host);
      DBUS_CLOSE_SOCKET (fd);
      return -1;
    }
  
  haddr = ((struct in_addr *) (he->h_addr_list)[0]);

  _DBUS_ZERO (addr);
  memcpy (&addr.sin_addr, haddr, sizeof(struct in_addr));
  addr.sin_family = AF_INET;
  addr.sin_port = htons (port);
  
  if (DBUS_SOCKET_API_RETURNS_ERROR
     (connect (fd, (struct sockaddr*) &addr, sizeof (addr)) < 0))
    {      
      DBUS_SOCKET_SET_ERRNO ();
      dbus_set_error (error,
                       _dbus_error_from_errno (errno),
                      "Failed to connect to socket %s:%d %s",
                      host, port, _dbus_strerror (errno));

      DBUS_CLOSE_SOCKET (fd);
      fd = -1;
      
      return -1;
    }

  fd = _dbus_socket_to_handle (fd);

  if (!_dbus_set_fd_nonblocking (fd, error))
    {
      _dbus_close (fd, NULL);
      fd = -1;

      return -1;
    }

  return fd;
}

/**
 * Creates a socket and binds it to the given port,
 * then listens on the socket. The socket is
 * set to be nonblocking. 
 *
 * @param host the interface to listen on, NULL for loopback, empty for any
 * @param port the port to listen on
 * @param error return location for errors
 * @returns the listening file descriptor or -1 on error
 */
int
_dbus_listen_tcp_socket (const char     *host,
                         dbus_uint32_t   port,
                         DBusError      *error)
{
  int listen_fd;
  struct sockaddr_in addr;
  struct hostent *he;
  struct in_addr *haddr;
#ifdef DBUS_WIN
  struct in_addr ina;
#endif


  _DBUS_ASSERT_ERROR_IS_CLEAR (error);
  
#ifdef DBUS_WIN
  _dbus_win_startup_winsock ();
#endif

  listen_fd = socket (AF_INET, SOCK_STREAM, 0);
  
  if (DBUS_SOCKET_IS_INVALID (listen_fd))
    {
      DBUS_SOCKET_SET_ERRNO ();
      dbus_set_error (error, _dbus_error_from_errno (errno),
                      "Failed to create socket \"%s:%d\": %s",
                      host, port, _dbus_strerror (errno));
      return -1;
    }
#ifdef DBUS_WIN
  if (host == NULL)
    {
      host = "localhost";
      ina.s_addr = htonl (INADDR_LOOPBACK);
      haddr = &ina;
    }
  else if (!host[0])
    {
      ina.s_addr = htonl (INADDR_ANY);
      haddr = &ina;
    }
  else
    {
#endif
  he = gethostbyname (host);
  if (he == NULL) 
    {
      DBUS_SOCKET_SET_ERRNO ();
      dbus_set_error (error,
                      _dbus_error_from_errno (errno),
                      "Failed to lookup hostname: %s",
                      host);
      DBUS_CLOSE_SOCKET (listen_fd);
      return -1;
    }
  
  haddr = ((struct in_addr *) (he->h_addr_list)[0]);
#ifdef DBUS_WIN
  }
#endif

  _DBUS_ZERO (addr);
  memcpy (&addr.sin_addr, haddr, sizeof (struct in_addr));
  addr.sin_family = AF_INET;
  addr.sin_port = htons (port);

  if (bind (listen_fd, (struct sockaddr*) &addr, sizeof (struct sockaddr)))
    {
      DBUS_SOCKET_SET_ERRNO ();
      dbus_set_error (error, _dbus_error_from_errno (errno),
                      "Failed to bind socket \"%s:%d\": %s",
                      host, port, _dbus_strerror (errno));
      DBUS_CLOSE_SOCKET (listen_fd);
      return -1;
    }

  if (DBUS_SOCKET_API_RETURNS_ERROR (listen (listen_fd, 30 /* backlog */)))
    {
      DBUS_SOCKET_SET_ERRNO ();
      dbus_set_error (error, _dbus_error_from_errno (errno),  
                      "Failed to listen on socket \"%s:%d\": %s",
                      host, port, _dbus_strerror (errno));
      DBUS_CLOSE_SOCKET (listen_fd);
      return -1;
    }

  listen_fd = _dbus_socket_to_handle (listen_fd);

  if (!_dbus_set_fd_nonblocking (listen_fd, error))
    {
      _dbus_close (listen_fd, NULL);
      return -1;
    }
  
  return listen_fd;
}

/**
 * Accepts a connection on a listening socket.
 * Handles EINTR for you.
 *
 * @param listen_fd the listen file descriptor
 * @returns the connection fd of the client, or -1 on error
 */
int
_dbus_accept  (int listen_fd)
{
  int client_fd;
  struct sockaddr addr;
  socklen_t addrlen;

  addrlen = sizeof (addr);
  
#ifndef DBUS_WIN
 retry:
#endif
  client_fd = accept (listen_fd, &addr, &addrlen);
  
  if (DBUS_SOCKET_IS_INVALID (client_fd))
    {
      DBUS_SOCKET_SET_ERRNO ();
#ifndef DBUS_WIN
      if (errno == EINTR)
        goto retry;
#else
      client_fd = -1;
#endif
    }
  
  return _dbus_socket_to_handle (client_fd);
}



dbus_bool_t
write_credentials_byte (int             server_fd,
                        DBusError      *error)
{
#ifdef DBUS_WIN
  /* FIXME: for the session bus credentials shouldn't matter (?), but
   * for the system bus they are presumably essential. A rough outline
   * of a way to implement the credential transfer would be this:
   *
   * client waits to *read* a byte.
   *
   * server creates a named pipe with a random name, sends a byte
   * contining its length, and its name.
   *
   * client reads the name, connects to it (using Win32 API).
   *
   * server waits for connection to the named pipe, then calls
   * ImpersonateNamedPipeClient(), notes its now-current credentials,
   * calls RevertToSelf(), closes its handles to the named pipe, and
   * is done. (Maybe there is some other way to get the SID of a named
   * pipe client without having to use impersonation?)
   *
   * client closes its handles and is done.
   *
   */

  return TRUE;

#else


  int bytes_written;
  char buf[1] = { '\0' };
#if defined(HAVE_CMSGCRED) && !defined(LOCAL_CREDS)
  struct {
	  struct cmsghdr hdr;
	  struct cmsgcred cred;
  } cmsg;
  struct iovec iov;
  struct msghdr msg;
#endif

#if defined(HAVE_CMSGCRED) && !defined(LOCAL_CREDS)
  iov.iov_base = buf;
  iov.iov_len = 1;

  memset (&msg, 0, sizeof (msg));
  msg.msg_iov = &iov;
  msg.msg_iovlen = 1;

  msg.msg_control = &cmsg;
  msg.msg_controllen = sizeof (cmsg);
  memset (&cmsg, 0, sizeof (cmsg));
  cmsg.hdr.cmsg_len = sizeof (cmsg);
  cmsg.hdr.cmsg_level = SOL_SOCKET;
  cmsg.hdr.cmsg_type = SCM_CREDS;
#endif

  _DBUS_ASSERT_ERROR_IS_CLEAR (error);
  
 again:

#if defined(HAVE_CMSGCRED) && !defined(LOCAL_CREDS)
  bytes_written = sendmsg (server_fd, &msg, 0);
#else
  bytes_written = write (server_fd, buf, 1);
#endif

  if (bytes_written < 0 && errno == EINTR)
    goto again;

  if (bytes_written < 0)
    {
      dbus_set_error (error, _dbus_error_from_errno (errno),
                      "Failed to write credentials byte: %s",
                     _dbus_strerror (errno));
      return FALSE;
    }
  else if (bytes_written == 0)
    {
      dbus_set_error (error, DBUS_ERROR_IO_ERROR,
                      "wrote zero bytes writing credentials byte");
      return FALSE;
    }
  else
    {
      _dbus_assert (bytes_written == 1);
      _dbus_verbose ("wrote credentials byte\n");
      return TRUE;
    }

#endif
}

/**
 * Reads a single byte which must be nul (an error occurs otherwise),
 * and reads unix credentials if available. Fills in pid/uid/gid with
 * -1 if no credentials are available. Return value indicates whether
 * a byte was read, not whether we got valid credentials. On some
 * systems, such as Linux, reading/writing the byte isn't actually
 * required, but we do it anyway just to avoid multiple codepaths.
 * 
 * Fails if no byte is available, so you must select() first.
 *
 * The point of the byte is that on some systems we have to
 * use sendmsg()/recvmsg() to transmit credentials.
 *
 * @param client_fd the client file descriptor
 * @param credentials struct to fill with credentials of client
 * @param error location to store error code
 * @returns #TRUE on success
 */
dbus_bool_t
_dbus_read_credentials_unix_socket  (int              client_fd,
                                     DBusCredentials *credentials,
                                     DBusError       *error)
{
#ifndef DBUS_WIN

  struct msghdr msg;
  struct iovec iov;
  char buf;

#ifdef HAVE_CMSGCRED 
  struct {
	  struct cmsghdr hdr;
	  struct cmsgcred cred;
  } cmsg;
#endif

  _DBUS_ASSERT_ERROR_IS_CLEAR (error);
  
  /* The POSIX spec certainly doesn't promise this, but
   * we need these assertions to fail as soon as we're wrong about
   * it so we can do the porting fixups
   */
  _dbus_assert (sizeof (pid_t) <= sizeof (credentials->pid));
  _dbus_assert (sizeof (uid_t) <= sizeof (credentials->uid));
  _dbus_assert (sizeof (gid_t) <= sizeof (credentials->gid));

  _dbus_credentials_clear (credentials);

#if defined(LOCAL_CREDS) && defined(HAVE_CMSGCRED)
  /* Set the socket to receive credentials on the next message */
  {
    int on = 1;
    if (setsockopt (client_fd, 0, LOCAL_CREDS, &on, sizeof (on)) < 0)
      {
	_dbus_verbose ("Unable to set LOCAL_CREDS socket option\n");
	return FALSE;
      }
  }
#endif

  iov.iov_base = &buf;
  iov.iov_len = 1;

  memset (&msg, 0, sizeof (msg));
  msg.msg_iov = &iov;
  msg.msg_iovlen = 1;

#ifdef HAVE_CMSGCRED
  memset (&cmsg, 0, sizeof (cmsg));
  msg.msg_control = &cmsg;
  msg.msg_controllen = sizeof (cmsg);
#endif

 again:
  if (recvmsg (client_fd, &msg, 0) < 0)
    {
      if (errno == EINTR)
	goto again;

      dbus_set_error (error, _dbus_error_from_errno (errno),
                      "Failed to read credentials byte: %s",
                      _dbus_strerror (errno));
      return FALSE;
    }

  if (buf != '\0')
    {
      dbus_set_error (error, DBUS_ERROR_FAILED,
                      "Credentials byte was not nul");
      return FALSE;
    }

#ifdef HAVE_CMSGCRED
  if (cmsg.hdr.cmsg_len < sizeof (cmsg) || cmsg.hdr.cmsg_type != SCM_CREDS)
    {
      dbus_set_error (error, DBUS_ERROR_FAILED,
                      "Message from recvmsg() was not SCM_CREDS");
      return FALSE;
    }
#endif

  _dbus_verbose ("read credentials byte\n");

  {
#ifdef SO_PEERCRED
    struct ucred cr;   
    int cr_len = sizeof (cr);
   
    if (getsockopt (client_fd, SOL_SOCKET, SO_PEERCRED, &cr, &cr_len) == 0 &&
	cr_len == sizeof (cr))
      {
	credentials->pid = cr.pid;
	credentials->uid = cr.uid;
	credentials->gid = cr.gid;
      }
    else
      {
	_dbus_verbose ("Failed to getsockopt() credentials, returned len %d/%d: %s\n",
		       cr_len, (int) sizeof (cr), _dbus_strerror (errno));
      }
#elif defined(HAVE_CMSGCRED)
    credentials->pid = cmsg.cred.cmcred_pid;
    credentials->uid = cmsg.cred.cmcred_euid;
    credentials->gid = cmsg.cred.cmcred_groups[0];
#elif defined(HAVE_GETPEEREID)
    uid_t euid;
    gid_t egid;
    if (getpeereid (client_fd, &euid, &egid) == 0)
      {
        credentials->uid = euid;
        credentials->gid = egid;
      }
    else
      {
        _dbus_verbose ("Failed to getpeereid() credentials: %s\n", _dbus_strerror (errno));
      }
#elif defined(HAVE_GETPEERUCRED)
    ucred_t * ucred = NULL;
    if (getpeerucred (client_fd, &ucred) == 0)
      {
        credentials->pid = ucred_getpid (ucred);
        credentials->uid = ucred_geteuid (ucred);
        credentials->gid = ucred_getegid (ucred);
      }
    else
      {
        _dbus_verbose ("Failed to getpeerucred() credentials: %s\n", _dbus_strerror (errno));
      }
    if (ucred != NULL)
      ucred_free (ucred);
#else /* !SO_PEERCRED && !HAVE_CMSGCRED && !HAVE_GETPEEREID && !HAVE_GETPEERUCRED */
    _dbus_verbose ("Socket credentials not supported on this OS\n");
#endif
  }

  _dbus_verbose ("Credentials:"
                 "  pid "DBUS_PID_FORMAT
                 "  uid "DBUS_UID_FORMAT
                 "  gid "DBUS_GID_FORMAT"\n",
		 credentials->pid,
		 credentials->uid,
		 credentials->gid);
    
  return TRUE;

#else

  /* FIXME bogus testing credentials */
  _dbus_credentials_from_current_process (credentials);

  return TRUE;

#endif
}

/**
* Checks to make sure the given directory is 
* private to the user 
*
* @param dir the name of the directory
* @param error error return
* @returns #FALSE on failure
**/
dbus_bool_t
_dbus_check_dir_is_private_to_user (DBusString *dir, DBusError *error)
{
  const char *directory;
  struct stat sb;
	
  _DBUS_ASSERT_ERROR_IS_CLEAR (error);
    
#ifndef DBUS_WIN    
  directory = _dbus_string_get_const_data (dir);
	
  if (stat (directory, &sb) < 0)
    {
      dbus_set_error (error, _dbus_error_from_errno (errno),
                      "%s", _dbus_strerror (errno));
   
      return FALSE;
    }
    
  if ((S_IROTH & sb.st_mode) || (S_IWOTH & sb.st_mode) ||
      (S_IRGRP & sb.st_mode) || (S_IWGRP & sb.st_mode))
    {
      dbus_set_error (error, DBUS_ERROR_FAILED,
                     "%s directory is not private to the user", directory);
      return FALSE;
    }
#endif    
  return TRUE;
}



/**
 * @addtogroup DBusInternalsUtils
 * @{
 */

#ifndef DBUS_WIN

static dbus_bool_t
fill_user_info_from_passwd (struct passwd *p,
                            DBusUserInfo  *info,
                            DBusError     *error)
{
  _dbus_assert (p->pw_name != NULL);
  _dbus_assert (p->pw_dir != NULL);
  
  info->uid = p->pw_uid;
  info->primary_gid = p->pw_gid;
  info->username = _dbus_strdup (p->pw_name);
  info->homedir = _dbus_strdup (p->pw_dir);
  
  if (info->username == NULL ||
      info->homedir == NULL)
    {
      dbus_set_error (error, DBUS_ERROR_NO_MEMORY, NULL);
      return FALSE;
    }

  return TRUE;
}
#endif



dbus_bool_t
_dbus_fill_user_info (DBusUserInfo       *info,
                dbus_uid_t          uid,
                const DBusString   *username,
                DBusError          *error)
{
  const char *username_c;
  
  /* exactly one of username/uid provided */
  _dbus_assert (username != NULL || uid != DBUS_UID_UNSET);
  _dbus_assert (username == NULL || uid == DBUS_UID_UNSET);

  info->uid = DBUS_UID_UNSET;
  info->primary_gid = DBUS_GID_UNSET;
  info->group_ids = NULL;
  info->n_group_ids = 0;
  info->username = NULL;
  info->homedir = NULL;
  
  if (username != NULL)
    username_c = _dbus_string_get_const_data (username);
  else
    username_c = NULL;

#ifndef DBUS_WIN
  /* For now assuming that the getpwnam() and getpwuid() flavors
   * are always symmetrical, if not we have to add more configure
   * checks
   */
  
#if defined (HAVE_POSIX_GETPWNAM_R) || defined (HAVE_NONPOSIX_GETPWNAM_R)
  {
    struct passwd *p;
    int result;
    char buf[1024];
    struct passwd p_str;

    p = NULL;
#ifdef HAVE_POSIX_GETPWNAM_R
    if (uid != DBUS_UID_UNSET)
      result = getpwuid_r (uid, &p_str, buf, sizeof (buf),
                           &p);
    else
      result = getpwnam_r (username_c, &p_str, buf, sizeof (buf),
                           &p);
#else
    if (uid != DBUS_UID_UNSET)
      p = getpwuid_r (uid, &p_str, buf, sizeof (buf));
    else
      p = getpwnam_r (username_c, &p_str, buf, sizeof (buf));
    result = 0;
#endif /* !HAVE_POSIX_GETPWNAM_R */
    if (result == 0 && p == &p_str)
      {
        if (!fill_user_info_from_passwd (p, info, error))
          return FALSE;
      }
    else
      {
        dbus_set_error (error, _dbus_error_from_errno (errno),
                        "User \"%s\" unknown or no memory to allocate password entry\n",
                        username_c ? username_c : "???");
        _dbus_verbose ("User %s unknown\n", username_c ? username_c : "???");
        return FALSE;
      }
  }
#else /* ! HAVE_GETPWNAM_R */
  {
    /* I guess we're screwed on thread safety here */
    struct passwd *p;

    if (uid != DBUS_UID_UNSET)
      p = getpwuid (uid);
    else
      p = getpwnam (username_c);

    if (p != NULL)
      {
        if (!fill_user_info_from_passwd (p, info, error))
          return FALSE;
      }
    else
      {
        dbus_set_error (error, _dbus_error_from_errno (errno),
                        "User \"%s\" unknown or no memory to allocate password entry\n",
                        username_c ? username_c : "???");
        _dbus_verbose ("User %s unknown\n", username_c ? username_c : "???");
        return FALSE;
      }
  }
#endif  /* ! HAVE_GETPWNAM_R */

  /* Fill this in so we can use it to get groups */
  username_c = info->username;
  
#ifdef HAVE_GETGROUPLIST
  {
    gid_t *buf;
    int buf_count;
    int i;
    
    buf_count = 17;
    buf = dbus_new (gid_t, buf_count);
    if (buf == NULL)
      {
        dbus_set_error (error, DBUS_ERROR_NO_MEMORY, NULL);
        goto failed;
      }
    
    if (getgrouplist (username_c,
                      info->primary_gid,
                      buf, &buf_count) < 0)
      {
        gid_t *new = dbus_realloc (buf, buf_count * sizeof (buf[0]));
        if (new == NULL)
          {
            dbus_set_error (error, DBUS_ERROR_NO_MEMORY, NULL);
            dbus_free (buf);
            goto failed;
          }
        
        buf = new;

        errno = 0;
        if (getgrouplist (username_c, info->primary_gid, buf, &buf_count) < 0)
          {
            dbus_set_error (error,
                            _dbus_error_from_errno (errno),
                            "Failed to get groups for username \"%s\" primary GID "
                            DBUS_GID_FORMAT ": %s\n",
                            username_c, info->primary_gid,
                            _dbus_strerror (errno));
            dbus_free (buf);
            goto failed;
          }
      }

    info->group_ids = dbus_new (dbus_gid_t, buf_count);
    if (info->group_ids == NULL)
      {
        dbus_set_error (error, DBUS_ERROR_NO_MEMORY, NULL);
        dbus_free (buf);
        goto failed;
      }
    
    for (i = 0; i < buf_count; ++i)
      info->group_ids[i] = buf[i];

    info->n_group_ids = buf_count;
    
    dbus_free (buf);
  }
#else  /* HAVE_GETGROUPLIST */
  {
    /* We just get the one group ID */
    info->group_ids = dbus_new (dbus_gid_t, 1);
    if (info->group_ids == NULL)
      {
        dbus_set_error (error, DBUS_ERROR_NO_MEMORY, NULL);
        goto failed;
      }

    info->n_group_ids = 1;

    (info->group_ids)[0] = info->primary_gid;
  }
#endif /* HAVE_GETGROUPLIST */

  _DBUS_ASSERT_ERROR_IS_CLEAR (error);
  
  return TRUE;
  
 failed:
  _DBUS_ASSERT_ERROR_IS_SET (error);
  return FALSE;

#else  /* DBUS_WIN */

  if (uid != DBUS_UID_UNSET)
    {
      if (!fill_win_user_info_from_uid (uid, info, error)) {
      	_dbus_verbose("%s after fill_win_user_info_from_uid\n",__FUNCTION__);
      return FALSE;
    }
    }
  else
    {
      wchar_t *wname = _dbus_win_utf8_to_utf16 (username_c, error);
      
      if (!wname)
	return FALSE;
      
    if (!fill_win_user_info_from_name (wname, info, error))
	  {
	    dbus_free (wname);
	    return FALSE;
	  }
    dbus_free (wname);
    }

  return TRUE;
#endif  /* DBUS_WIN */
}


/**
 * Appends the given filename to the given directory.
 *
 * @todo it might be cute to collapse multiple '/' such as "foo//"
 * concat "//bar"
 *
 * @param dir the directory name
 * @param next_component the filename
 * @returns #TRUE on success
 */
dbus_bool_t
_dbus_concat_dir_and_file (DBusString       *dir,
                           const DBusString *next_component)
{
  dbus_bool_t dir_ends_in_slash;
  dbus_bool_t file_starts_with_slash;

  if (_dbus_string_get_length (dir) == 0 ||
      _dbus_string_get_length (next_component) == 0)
    return TRUE;
  
#ifndef DBUS_WIN
  dir_ends_in_slash = '/' == _dbus_string_get_byte (dir,
                                                    _dbus_string_get_length (dir) - 1);

  file_starts_with_slash = '/' == _dbus_string_get_byte (next_component, 0);

  if (dir_ends_in_slash && file_starts_with_slash)
    {
      _dbus_string_shorten (dir, 1);
    }
  else if (!(dir_ends_in_slash || file_starts_with_slash))
    {
      if (!_dbus_string_append_byte (dir, '/'))
        return FALSE;
    }
#else
  dir_ends_in_slash =
    ('/' == _dbus_string_get_byte (dir, _dbus_string_get_length (dir) - 1) ||
     '\\' == _dbus_string_get_byte (dir, _dbus_string_get_length (dir) - 1));

  file_starts_with_slash =
     ('/' == _dbus_string_get_byte (next_component, 0) ||
      '\\' == _dbus_string_get_byte (next_component, 0));

  if (dir_ends_in_slash && file_starts_with_slash)
    {
      _dbus_string_shorten (dir, 1);
    }
  else if (!(dir_ends_in_slash || file_starts_with_slash))
    {
      if (!_dbus_string_append_byte (dir, '\\'))
        return FALSE;
    }
#endif

  return _dbus_string_copy (next_component, 0, dir,
                            _dbus_string_get_length (dir));
}




/**
 * Gets our process ID
 * @returns process ID
 */
unsigned long
_dbus_getpid (void)
{
#ifndef DBUS_WIN
  return getpid ();
#else
  return GetCurrentProcessId ();
#endif
}

/** Gets our UID
 * @returns process UID
 */
dbus_uid_t
_dbus_getuid (void)
{
#ifdef DBUS_WIN
  return _dbus_getuid_win ();
#else
  return getuid ();
#endif
}

#ifdef DBUS_BUILD_TESTS
/** Gets our GID
 * @returns process GID
 */
dbus_gid_t
_dbus_getgid (void)
{
#ifdef DBUS_WIN
  return _dbus_getgid_win ();
#else
  return getgid ();
#endif 
}
#endif



/**
 * Wrapper for poll().
 *
 * @param fds the file descriptors to poll
 * @param n_fds number of descriptors in the array
 * @param timeout_milliseconds timeout or -1 for infinite
 * @returns numbers of fds with revents, or <0 on error
 */
int
_dbus_poll (DBusPollFD *fds,
            int         n_fds,
            int         timeout_milliseconds)
{
#ifdef DBUS_WIN
	return _dbus_poll_win (fds, n_fds, timeout_milliseconds);
#else
#ifdef HAVE_POLL
  /* This big thing is a constant expression and should get optimized
   * out of existence. So it's more robust than a configure check at
   * no cost.
   */
  if (_DBUS_POLLIN == POLLIN &&
      _DBUS_POLLPRI == POLLPRI &&
      _DBUS_POLLOUT == POLLOUT &&
      _DBUS_POLLERR == POLLERR &&
      _DBUS_POLLHUP == POLLHUP &&
      _DBUS_POLLNVAL == POLLNVAL &&
      sizeof (DBusPollFD) == sizeof (struct pollfd) &&
      _DBUS_STRUCT_OFFSET (DBusPollFD, fd) ==
      _DBUS_STRUCT_OFFSET (struct pollfd, fd) &&
      _DBUS_STRUCT_OFFSET (DBusPollFD, events) ==
      _DBUS_STRUCT_OFFSET (struct pollfd, events) &&
      _DBUS_STRUCT_OFFSET (DBusPollFD, revents) ==
      _DBUS_STRUCT_OFFSET (struct pollfd, revents))
    {
      return poll ((struct pollfd*) fds,
                   n_fds, 
                   timeout_milliseconds);
    }
  else
    {
      /* We have to convert the DBusPollFD to an array of
       * struct pollfd, poll, and convert back.
       */
      _dbus_warn ("didn't implement poll() properly for this system yet\n");
      return -1;
    }
#else /* ! HAVE_POLL */

  fd_set read_set, write_set, err_set;
  int max_fd = 0;
  int i;
  struct timeval tv;
  int ready;
  
  FD_ZERO (&read_set);
  FD_ZERO (&write_set);
  FD_ZERO (&err_set);

  for (i = 0; i < n_fds; i++)
    {
      DBusPollFD *fdp = &fds[i];

      if (fdp->events & _DBUS_POLLIN)
	FD_SET (fdp->fd, &read_set);

      if (fdp->events & _DBUS_POLLOUT)
	FD_SET (fdp->fd, &write_set);

      FD_SET (fdp->fd, &err_set);

      max_fd = MAX (max_fd, fdp->fd);
    }
    
  tv.tv_sec = timeout_milliseconds / 1000;
  tv.tv_usec = (timeout_milliseconds % 1000) * 1000;

  ready = select (max_fd + 1, &read_set, &write_set, &err_set,
                  timeout_milliseconds < 0 ? NULL : &tv);

  if (ready > 0)
    {
      for (i = 0; i < n_fds; i++)
	{
	  DBusPollFD *fdp = &fds[i];

	  fdp->revents = 0;

	  if (FD_ISSET (fdp->fd, &read_set))
	    fdp->revents |= _DBUS_POLLIN;

	  if (FD_ISSET (fdp->fd, &write_set))
	    fdp->revents |= _DBUS_POLLOUT;

	  if (FD_ISSET (fdp->fd, &err_set))
	    fdp->revents |= _DBUS_POLLERR;
	}
    }

  return ready;
#endif
#endif /* DBUS_WIN */
}


/** nanoseconds in a second */
#define NANOSECONDS_PER_SECOND       1000000000
/** microseconds in a second */
#define MICROSECONDS_PER_SECOND      1000000
/** milliseconds in a second */
#define MILLISECONDS_PER_SECOND      1000
/** nanoseconds in a millisecond */
#define NANOSECONDS_PER_MILLISECOND  1000000
/** microseconds in a millisecond */
#define MICROSECONDS_PER_MILLISECOND 1000

/**
 * Sleeps the given number of milliseconds.
 * @param milliseconds number of milliseconds
 */
void
_dbus_sleep_milliseconds (int milliseconds)
{
#ifndef DBUS_WIN
#ifdef HAVE_NANOSLEEP
  struct timespec req;
  struct timespec rem;

  req.tv_sec = milliseconds / MILLISECONDS_PER_SECOND;
  req.tv_nsec = (milliseconds % MILLISECONDS_PER_SECOND) * NANOSECONDS_PER_MILLISECOND;
  rem.tv_sec = 0;
  rem.tv_nsec = 0;

  while (nanosleep (&req, &rem) < 0 && errno == EINTR)
    req = rem;
#elif defined (HAVE_USLEEP)
  usleep (milliseconds * MICROSECONDS_PER_MILLISECOND);
#else /* ! HAVE_USLEEP */
  sleep (MAX (milliseconds / 1000, 1));
#endif
#else  /* DBUS_WIN */
  Sleep (milliseconds);
#endif /* !DBUS_WIN */
}

/**
 * Get current time, as in gettimeofday().
 *
 * @param tv_sec return location for number of seconds
 * @param tv_usec return location for number of microseconds
 */
void
_dbus_get_current_time (long *tv_sec,
                        long *tv_usec)
{
#ifndef DBUS_WIN
  struct timeval t;

  gettimeofday (&t, NULL);

  if (tv_sec)
    *tv_sec = t.tv_sec;
  if (tv_usec)
    *tv_usec = t.tv_usec;
#else
  FILETIME ft;
  dbus_uint64_t *time64 = (dbus_uint64_t *) &ft;

  GetSystemTimeAsFileTime (&ft);

  /* Convert from 100s of nanoseconds since 1601-01-01
   * to Unix epoch. Yes, this is Y2038 unsafe.
   */
  *time64 -= DBUS_INT64_CONSTANT (116444736000000000);
  *time64 /= 10;

  if (tv_sec)
    *tv_sec = *time64 / 1000000;

  if (tv_usec)
    *tv_usec = *time64 % 1000000;
#endif
}


/**
 * signal (SIGPIPE, SIG_IGN);
 */
void
_dbus_disable_sigpipe (void)
{
#ifndef DBUS_WIN
  signal (SIGPIPE, SIG_IGN);
#endif
}

/**
 * Sets the file descriptor to be close
 * on exec. Should be called for all file
 * descriptors in D-Bus code.
 *
 * @param fd the file descriptor
 */
void
_dbus_fd_set_close_on_exec (int fd)
{
#ifdef DBUS_WIN
  _dbus_fd_set_close_on_exec_win(fd);
#else
  int val;
  
  val = fcntl (fd, F_GETFD, 0);
  
  if (val < 0)
    return;

  val |= FD_CLOEXEC;
  
  fcntl (fd, F_SETFD, val);
#endif
}


/**
 * Closes a file descriptor.
 *
 * @param fd the file descriptor
 * @param error error object
 * @returns #FALSE if error set
 */
dbus_bool_t
_dbus_close (int        fd,
             DBusError *error)
{
#ifdef DBUS_WIN
  return _dbus_close_win (fd, error);
#else

  _DBUS_ASSERT_ERROR_IS_CLEAR (error);
  
 again:
  if (close (fd) < 0)
    {
      if (errno == EINTR)
        goto again;

      dbus_set_error (error, _dbus_error_from_errno (errno),
                      "Could not close fd %d", fd);
      return FALSE;
    }

  return TRUE;
#endif
}

/**
 * Sets a file descriptor to be nonblocking.
 *
 * @param fd the file descriptor.
 * @param error address of error location.
 * @returns #TRUE on success.
 */
dbus_bool_t
_dbus_set_fd_nonblocking (int             fd,
                          DBusError      *error)
{
#ifdef DBUS_WIN
  return _dbus_set_fd_nonblocking_win(fd, error);
#else
  int val;

  _DBUS_ASSERT_ERROR_IS_CLEAR (error);
  
  val = fcntl (fd, F_GETFL, 0);
  if (val < 0)
    {
      dbus_set_error (error, _dbus_error_from_errno (errno),
                      "Failed to get flags from file descriptor %d: %s",
                      fd, _dbus_strerror (errno));
      _dbus_verbose ("Failed to get flags for fd %d: %s\n", fd,
                     _dbus_strerror (errno));
      return FALSE;
    }

  if (fcntl (fd, F_SETFL, val | O_NONBLOCK) < 0)
    {
      dbus_set_error (error, _dbus_error_from_errno (errno),
                      "Failed to set nonblocking flag of file descriptor %d: %s",
                      fd, _dbus_strerror (errno));
      _dbus_verbose ("Failed to set fd %d nonblocking: %s\n",
                     fd, _dbus_strerror (errno));

      return FALSE;
    }

  return TRUE;
#endif
}





/**
 * Creates a full-duplex pipe (as in socketpair()).
 * Sets both ends of the pipe nonblocking.
 *
 * @todo libdbus only uses this for the debug-pipe server, so in
 * principle it could be in dbus-sysdeps-util.c, except that
 * dbus-sysdeps-util.c isn't in libdbus when tests are enabled and the
 * debug-pipe server is used.
 * 
 * @param fd1 return location for one end
 * @param fd2 return location for the other end
 * @param blocking #TRUE if pipe should be blocking
 * @param error error return
 * @returns #FALSE on failure (if error is set)
 */
dbus_bool_t
_dbus_full_duplex_pipe (int        *fd1,
                        int        *fd2,
                        dbus_bool_t blocking,
                        DBusError  *error)
{
#ifdef HAVE_SOCKETPAIR
  int fds[2];

  _DBUS_ASSERT_ERROR_IS_CLEAR (error);
  
  if (socketpair (AF_UNIX, SOCK_STREAM, 0, fds) < 0)
    {
      dbus_set_error (error, _dbus_error_from_errno (errno),
                      "Could not create full-duplex pipe");
      return FALSE;
    }

  if (!blocking &&
      (!_dbus_set_fd_nonblocking (fds[0], NULL) ||
       !_dbus_set_fd_nonblocking (fds[1], NULL)))
    {
      dbus_set_error (error, _dbus_error_from_errno (errno),
                      "Could not set full-duplex pipe nonblocking");
      
      close (fds[0]);
      close (fds[1]);
      
      return FALSE;
    }
  
  *fd1 = fds[0];
  *fd2 = fds[1];

  _dbus_verbose ("full-duplex pipe %d <-> %d\n",
                 *fd1, *fd2);
  
  return TRUE;  

#elif defined (DBUS_WIN)
  return _dbus_full_duplex_pipe_win (fd1, fd2, blocking, error);
#else
  _dbus_warn ("_dbus_full_duplex_pipe() not implemented on this OS\n");
  dbus_set_error (error, DBUS_ERROR_FAILED,
                  "_dbus_full_duplex_pipe() not implemented on this OS");
  return FALSE;
#endif
}

#ifndef DBUS_WIN
/**
 * Measure the length of the given format string and arguments,
 * not including the terminating nul.
 *
 * @param format a printf-style format string
 * @param args arguments for the format string
 * @returns length of the given format string and args
 */
int
_dbus_printf_string_upper_bound (const char *format,
                                 va_list     args)
{
  char c;
  return vsnprintf (&c, 1, format, args);
}
#endif

/**
 * A wrapper around strerror() because some platforms
 * may be lame and not have strerror().
 *
 * @param error_number errno.
 * @returns error description.
 */
const char*
_dbus_strerror (int error_number)
{
#ifndef DBUS_WIN
  const char *msg;
  
  msg = strerror (error_number);
  if (msg == NULL)
    msg = "unknown";

  return msg;
#else
  return _dbus_strerror_win(error_number);
#endif
}

/** @} end of sysdeps */

/* tests in dbus-sysdeps-util.c */
