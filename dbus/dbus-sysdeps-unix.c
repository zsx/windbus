/* -*- mode: C; c-file-style: "gnu" -*- */
/* dbus-sysdeps.c Wrappers around system/libc features (internal to D-Bus implementation)
 * 
 * Copyright (C) 2002, 2003  Red Hat, Inc.
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

#include "dbus-internals.h"
#include "dbus-sysdeps.h"
#include "dbus-threads.h"
#include "dbus-protocol.h"
#include "dbus-string.h"
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <dirent.h>
#include <sys/un.h>
#include <pwd.h>
#include <time.h>
#include <locale.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <netdb.h>
#include <grp.h>

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

int _dbus_socket_to_handle (int socket){ return socket; }
int _dbus_handle_to_socket (int handle){ return handle; }
int _dbus_fd_to_handle     (int fd)    { return fd; }
int _dbus_handle_to_fd     (int handle){ return handle; }


int _dbus_mkdir (const char *path, 
	             mode_t mode)
{
  return mkdir(path, mode);
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
  abort ();
  _exit (1); /* in case someone manages to ignore SIGABRT */
}
#endif

 
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
          _dbus_close (fd, NULL);
          return -1;
	}
	
      strncpy (&addr.sun_path[1], path, path_len);
      /* _dbus_verbose_bytes (addr.sun_path, sizeof (addr.sun_path)); */
#else /* HAVE_ABSTRACT_SOCKETS */
      dbus_set_error (error, DBUS_ERROR_NOT_SUPPORTED,
                      "Operating system does not support abstract socket namespace\n");
      _dbus_close (fd, NULL);
      return -1;
#endif /* ! HAVE_ABSTRACT_SOCKETS */
    }
  else
    {
      if (path_len > _DBUS_MAX_SUN_PATH_LENGTH)
        {
          dbus_set_error (error, DBUS_ERROR_BAD_ADDRESS,
                      "Socket name too long\n");
          _dbus_close (fd, NULL);
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

      _dbus_close (fd, NULL);
      fd = -1;
      
      return -1;
    }

  if (!_dbus_set_fd_nonblocking (fd, error))
    {
      _DBUS_ASSERT_ERROR_IS_SET (error);
      
      _dbus_close (fd, NULL);
      fd = -1;

      return -1;
    }

  return fd;
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
          _dbus_close (listen_fd, NULL);
          return -1;
	}
      
      strncpy (&addr.sun_path[1], path, path_len);
      /* _dbus_verbose_bytes (addr.sun_path, sizeof (addr.sun_path)); */
#else /* HAVE_ABSTRACT_SOCKETS */
      dbus_set_error (error, DBUS_ERROR_NOT_SUPPORTED,
                      "Operating system does not support abstract socket namespace\n");
      _dbus_close (listen_fd, NULL);
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
          _dbus_close (listen_fd, NULL);
          return -1;
	}
	
      strncpy (addr.sun_path, path, path_len);
    }
  
  if (bind (listen_fd, (struct sockaddr*) &addr, _DBUS_STRUCT_OFFSET (struct sockaddr_un, sun_path) + path_len) < 0)
    {
      dbus_set_error (error, _dbus_error_from_errno (errno),
                      "Failed to bind socket \"%s\": %s",
                      path, _dbus_strerror (errno));
      _dbus_close (listen_fd, NULL);
      return -1;
    }

  if (listen (listen_fd, 30 /* backlog */) < 0)
    {
      dbus_set_error (error, _dbus_error_from_errno (errno),
                      "Failed to listen on socket \"%s\": %s",
                      path, _dbus_strerror (errno));
      _dbus_close (listen_fd, NULL);
      return -1;
    }

  if (!_dbus_set_fd_nonblocking (listen_fd, error))
    {
      _DBUS_ASSERT_ERROR_IS_SET (error);
      _dbus_close (listen_fd, NULL);
      return -1;
    }
  
  /* Try opening up the permissions, but if we can't, just go ahead
   * and continue, maybe it will be good enough.
   */
  if (!abstract && chmod (path, 0777) < 0)
    _dbus_warn ("Could not set mode 0777 on socket %s\n",
                path);
  
  return listen_fd;
}

/**
 * Creates a socket and connects to a socket at the given host 
 * and port. The connection fd is returned, and is set up as
 * nonblocking.
 *
 * @param host the host name to connect to
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

  _DBUS_ASSERT_ERROR_IS_CLEAR (error);
  
  fd = socket (AF_INET, SOCK_STREAM, 0);
  
  if (fd < 0)
    {
      dbus_set_error (error,
                      _dbus_error_from_errno (errno),
                      "Failed to create socket: %s",
                      _dbus_strerror (errno)); 
      
      return -1;
    }

  if (host == NULL)
    host = "localhost";

  he = gethostbyname (host);
  if (he == NULL) 
    {
      dbus_set_error (error,
                      _dbus_error_from_errno (errno),
                      "Failed to lookup hostname: %s",
                      host);
      _dbus_close (fd, NULL);
      return -1;
    }
  
  haddr = ((struct in_addr *) (he->h_addr_list)[0]);

  _DBUS_ZERO (addr);
  memcpy (&addr.sin_addr, haddr, sizeof(struct in_addr));
  addr.sin_family = AF_INET;
  addr.sin_port = htons (port);
  
  if (connect (fd, (struct sockaddr*) &addr, sizeof (addr)) < 0)
    {      
      dbus_set_error (error,
                       _dbus_error_from_errno (errno),
                      "Failed to connect to socket %s:%d %s",
                      host, port, _dbus_strerror (errno));

      _dbus_close (fd, NULL);
      fd = -1;
      
      return -1;
    }

  if (!_dbus_set_fd_nonblocking (fd, error))
    {
      _dbus_close (fd, NULL);
      fd = -1;

      return -1;
    }

  return fd;
}

/**
 * Creates a socket and binds it to the given path,
 * then listens on the socket. The socket is
 * set to be nonblocking. 
 *
 * @param host the host name to listen on
 * @param port the prot to listen on
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

  _DBUS_ASSERT_ERROR_IS_CLEAR (error);
  
  listen_fd = socket (AF_INET, SOCK_STREAM, 0);
  
  if (listen_fd < 0)
    {
      dbus_set_error (error, _dbus_error_from_errno (errno),
                      "Failed to create socket \"%s:%d\": %s",
                      host, port, _dbus_strerror (errno));
      return -1;
    }

  he = gethostbyname (host);
  if (he == NULL) 
    {
      dbus_set_error (error,
                      _dbus_error_from_errno (errno),
                      "Failed to lookup hostname: %s",
                      host);
      _dbus_close (listen_fd, NULL);
      return -1;
    }
  
  haddr = ((struct in_addr *) (he->h_addr_list)[0]);

  _DBUS_ZERO (addr);
  memcpy (&addr.sin_addr, haddr, sizeof (struct in_addr));
  addr.sin_family = AF_INET;
  addr.sin_port = htons (port);

  if (bind (listen_fd, (struct sockaddr*) &addr, sizeof (struct sockaddr)))
    {
      dbus_set_error (error, _dbus_error_from_errno (errno),
                      "Failed to bind socket \"%s:%d\": %s",
                      host, port, _dbus_strerror (errno));
      _dbus_close (listen_fd, NULL);
      return -1;
    }

  if (listen (listen_fd, 30 /* backlog */) < 0)
    {
      dbus_set_error (error, _dbus_error_from_errno (errno),  
                      "Failed to listen on socket \"%s:%d\": %s",
                      host, port, _dbus_strerror (errno));
      _dbus_close (listen_fd, NULL);
      return -1;
    }

  if (!_dbus_set_fd_nonblocking (listen_fd, error))
    {
      _dbus_close (listen_fd, NULL);
      return -1;
    }
  
  return listen_fd;
}

dbus_bool_t
write_credentials_byte (int             server_fd,
                        DBusError      *error)
{
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
}

/**
 * Sends a single nul byte with our UNIX credentials as ancillary
 * data.  Returns #TRUE if the data was successfully written.  On
 * systems that don't support sending credentials, just writes a byte,
 * doesn't send any credentials.  On some systems, such as Linux,
 * reading/writing the byte isn't actually required, but we do it
 * anyway just to avoid multiple codepaths.
 *
 * Fails if no byte can be written, so you must select() first.
 *
 * The point of the byte is that on some systems we have to
 * use sendmsg()/recvmsg() to transmit credentials.
 *
 * @param server_fd file descriptor for connection to server
 * @param error return location for error code
 * @returns #TRUE if the byte was sent
 */
dbus_bool_t
_dbus_send_credentials_unix_socket  (int              server_fd,
                                     DBusError       *error)
{
  _DBUS_ASSERT_ERROR_IS_CLEAR (error);
  
  if (write_credentials_byte (server_fd, error))
    return TRUE;
  else
    return FALSE;
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
  
 retry:
  client_fd = accept (listen_fd, &addr, &addrlen);
  
  if (client_fd < 0)
    {
      if (errno == EINTR)
        goto retry;
    }
  
  return client_fd;
}

/** @} */

/**
 * @addtogroup DBusString
 *
 * @{
 */
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
    
  return TRUE;
}

/** @} */ /* DBusString group */

 /**
 * @addtogroup DBusInternalsUtils
 * @{
 */
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

dbus_bool_t
fill_user_info (DBusUserInfo       *info,
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
}


/**
 * Gets our process ID
 * @returns process ID
 */
unsigned long
_dbus_getpid (void)
{
  return getpid ();
}
 
/** Gets our UID
 * @returns process UID
 */
dbus_uid_t
_dbus_getuid (void)
{
  return getuid ();
}

#ifdef DBUS_BUILD_TESTS
/** Gets our GID
 * @returns process GID
 */
dbus_gid_t
_dbus_getgid (void)
{
  return getgid ();
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
}

/**
 * Get current time, as in gettimeofday().
 *
 * @param tv_sec return location for number of seconds
 * @param tv_usec return location for number of microseconds (thousandths)
 */
void
_dbus_get_current_time (long *tv_sec,
                        long *tv_usec)
{
  struct timeval t;

  gettimeofday (&t, NULL);

  if (tv_sec)
    *tv_sec = t.tv_sec;
  if (tv_usec)
    *tv_usec = t.tv_usec;
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

  return _dbus_string_copy (next_component, 0, dir,
                            _dbus_string_get_length (dir));
}


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
  const char *msg;
  
  msg = strerror (error_number);
  if (msg == NULL)
    msg = "unknown";
 
  return msg;
}
 
/**
 * signal (SIGPIPE, SIG_IGN);
 */
void
_dbus_disable_sigpipe (void)
{
  signal (SIGPIPE, SIG_IGN);
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
  int val;
  
  val = fcntl (fd, F_GETFD, 0);
  
  if (val < 0)
    return;
 
  val |= FD_CLOEXEC;
  
  fcntl (fd, F_SETFD, val);
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
  _DBUS_ASSERT_ERROR_IS_CLEAR (error);
  
 again:
  if (_dbus_close (fd, NULL) < 0)
    {
      if (errno == EINTR)
        goto again;
 
      dbus_set_error (error, _dbus_error_from_errno (errno),
                      "Could not close fd %d", fd);
      return FALSE;
    }

  return TRUE;
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
#else
  _dbus_warn ("_dbus_full_duplex_pipe() not implemented on this OS\n");
  dbus_set_error (error, DBUS_ERROR_FAILED,
                  "_dbus_full_duplex_pipe() not implemented on this OS");
  return FALSE;
#endif
}


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

/** @} end of sysdeps-unix */

/* tests in dbus-sysdeps-util.c and tests in dbus-sysdeps-util-unix.c */

