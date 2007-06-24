/* -*- mode: C; c-file-style: "gnu" -*- */
/* dbus-launch.c  dbus-launch utility
 *
 * Copyright (C) 2007 Ralf Habacker <ralf.habacker@freenet.de>
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
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#if defined __MINGW32__ || (defined _MSC_VER && _MSC_VER <= 1310)
/* save string functions version
*/ 
#define errno_t int

errno_t strcat_s(char *dest, size_t size, char *src) 
{
  assert(strlen(dest) + strlen(src) +1 <= size);
  strcat(dest,src);
  return 0;
}

errno_t strcpy_s(char *dest, size_t size, char *src)
{
  assert(strlen(src) +1 <= size);
  strcpy(dest,src);  
  return 0;
}
#endif

/* TODO: use unicode version as suggested by Tor Lillqvist */

#define AUTO_ACTIVATE_CONSOLE_WHEN_VERBOSE_MODE 1

int main(int argc,char **argv)
{
  char dbusDaemonPath[MAX_PATH*2+1];
  char command[MAX_PATH*2+1];
  char *p;
  char *daemon_name;
  int result;
  int showConsole = 0;
  char *s = getenv("DBUS_VERBOSE");
  int verbose = s && *s != '\0' ? 1 : 0;
  PROCESS_INFORMATION pi;
  STARTUPINFO si;
  HANDLE h; 
  
#ifdef AUTO_ACTIVATE_CONSOLE_WHEN_VERBOSE_MODE
  if (verbose)
      showConsole = 1; 
#endif
  GetModuleFileName(NULL,dbusDaemonPath,sizeof(dbusDaemonPath));
  /* check for debug version */
  if (strstr(dbusDaemonPath,"dbus-launchd.exe"))
      daemon_name = "dbus-daemond.exe";
  else
      daemon_name = "dbus-daemon.exe";
  if ((p = strrchr(dbusDaemonPath,'\\'))) 
    {
      *(p+1)= '\0';
      strcat_s(dbusDaemonPath,sizeof(dbusDaemonPath),daemon_name);
    }
  else 
    {
      if (verbose)
          fprintf(stderr,"error: could not extract path from current applications module filename\n");
      return 1;
    } 
  
  strcpy_s(command,sizeof(command),dbusDaemonPath);
  strcat_s(command,sizeof(command)," --session");
  if (verbose)
      fprintf(stderr,"%s\n",command);
  
  memset(&si, 0, sizeof(si));
  memset(&pi, 0, sizeof(pi));
  si.cb = sizeof(si);

  result = CreateProcess(NULL, 
                  command,
                  0,
                  0,
                  TRUE,
                  (showConsole ? CREATE_NEW_CONSOLE : 0) | NORMAL_PRIORITY_CLASS,
                  0,
                  0,
                  &si,
                  &pi);

  CloseHandle(pi.hProcess);
  CloseHandle(pi.hThread);

  if (result == 0) 
    {
      if (verbose)
          fprintf(stderr,"could not start dbus-daemon error=%d",GetLastError());
      return 4;
    }
   
  return 0;
}
