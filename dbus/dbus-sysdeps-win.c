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
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <locale.h>
#include <sys/stat.h>

#ifdef HAVE_TIME_H
#include <time.h>
#endif
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#ifdef HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif
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

#include "dbus-sysdeps-win.h"

int win32_n_fds = 0;
DBusWin32FD *win_fds = NULL;


static int  win32_encap_randomizer;
static DBusHashTable *sid_atom_cache = NULL;



_DBUS_DEFINE_GLOBAL_LOCK (win_fds);
_DBUS_DEFINE_GLOBAL_LOCK (sid_atom_cache);

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

  fd = UNRANDOMIZE (fd);

  _dbus_assert (fd >= 0 && fd < win32_n_fds);
  _dbus_assert (win_fds != NULL);

  type = win_fds[fd].type;
  fd = win_fds[fd].fd;

  _DBUS_UNLOCK (win_fds);
    
  switch (type)
    {
    case DBUS_win_FD_SOCKET:
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

  fd = UNRANDOMIZE (fd);

  _dbus_assert (fd >= 0 && fd < win32_n_fds);
  _dbus_assert (win_fds != NULL);

  type = win_fds[fd].type;
  fd = win_fds[fd].fd;

  _DBUS_UNLOCK (win_fds);

  switch (type)
    {
    case DBUS_win_FD_SOCKET:
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

  fd = UNRANDOMIZE (fd);

  _dbus_assert (fd >= 0 && fd < win32_n_fds);
  _dbus_assert (win_fds != NULL);

  switch (win_fds[fd].type)
    {
    case DBUS_win_FD_SOCKET:
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

  fd2 = UNRANDOMIZE (fd);
  _dbus_verbose("fd %d %d %d\n",fd,fd2,win32_n_fds);
  _dbus_assert (fd2 >= 0 && fd2 < win32_n_fds);
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

  fd = UNRANDOMIZE (fd);

  _dbus_assert (fd >= 0 && fd < win32_n_fds);
  _dbus_assert (win_fds != NULL);

  switch (win_fds[fd].type)
    {
    case DBUS_win_FD_SOCKET:
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

  fd = UNRANDOMIZE (fd);

  _dbus_assert (fd >= 0 && fd < win32_n_fds);
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
    case DBUS_win_FD_SOCKET:
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

  sock = win_fds[UNRANDOMIZE (listen_fd)].fd;

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

  win_fds[UNRANDOMIZE (listen_fd)].port_file_fd = filefd;

  /* Use strdup() to avoid memory leak in dbus-test */
  path = strdup (path);
  if (!path)
    {
      _DBUS_SET_OOM (error);
      _dbus_close (listen_fd, NULL);
      return -1;
    }

  _dbus_string_init_const (&win_fds[UNRANDOMIZE (listen_fd)].port_file, path);

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
int
_dbus_win_allocate_fd (void)
{
  int i;

  _DBUS_LOCK (win_fds);

  if (win_fds == NULL)
    {
      DBusString random;

      win32_n_fds = 16;
      /* Use malloc to avoid memory leak failure in dbus-test */
      win_fds = malloc (win32_n_fds * sizeof (*win_fds));

      _dbus_assert (win_fds != NULL);

      for (i = 0; i < win32_n_fds; i++)
	win_fds[i].type = DBUS_win_FD_UNUSED;

      _dbus_string_init (&random);
      _dbus_generate_random_bytes (&random, sizeof (int));
      memmove (&win32_encap_randomizer, _dbus_string_get_const_data (&random), sizeof (int));
      win32_encap_randomizer &= 0xFF;
      _dbus_string_free (&random);
    }

  for (i = 0; i < win32_n_fds && win_fds[i].type != DBUS_win_FD_UNUSED; i++)
    ;

  if (i == win32_n_fds)
    {
      int oldn = win32_n_fds;
      int j;

      win32_n_fds += 16;
      win_fds = realloc (win_fds, win32_n_fds * sizeof (*win_fds));

      _dbus_assert (win_fds != NULL);

      for (j = oldn; j < win32_n_fds; j++)
	win_fds[i].type = DBUS_win_FD_UNUSED;
    }

  win_fds[i].type = DBUS_win_FD_BEING_OPENED;
  win_fds[i].fd = -1;
  win_fds[i].port_file_fd = -1;
  win_fds[i].close_on_exec = FALSE;
  win_fds[i].non_blocking = FALSE;

  _DBUS_UNLOCK (win_fds);

  return i;
}

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
         user_info->usri1_home_dir != 0xfeeefeee &&  /* freed memory http://www.gamedev.net/community/forums/topic.asp?topic_id=158402 */
         user_info->usri1_home_dir[0] != '\0')
        {
          info->homedir = _dbus_win_utf16_to_utf8 (user_info->usri1_home_dir, error);
          if (!info->homedir)
	        goto out1;
	    }
	  else
        {
			_dbus_warn("NetUserGetInfo() failed: now valid  user_info\n");
          /* Not set, so use something random. */
          info->homedir = _dbus_strdup ("\\");
         }
  else
    {
      _dbus_warn("NetUserGetInfo() failed with errorcode %d '%s', %s\n",ret,_dbus_lm_strerror(ret),_dbus_win_utf16_to_utf8(dc,error));
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
_dbus_win_deallocate_fd (int fd)
{
  _DBUS_LOCK (win_fds);
  win_fds[UNRANDOMIZE (fd)].type = DBUS_win_FD_UNUSED;
  _DBUS_UNLOCK (win_fds);
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

int
_dbus_handle_from_socket (int socket)
{
  int i = _dbus_win_allocate_fd ();
  int retval;

  win_fds[i].fd = socket;
  win_fds[i].type = DBUS_win_FD_SOCKET;

  retval = RANDOMIZE (i);

  _dbus_verbose ("encapsulated socket %d:%d:%d\n", retval, i, socket);

  return retval;
}

int
_dbus_handle_to_socket (int socket)
{
  int i;
  int retval = -1;

  _DBUS_LOCK (win_fds);

  _dbus_assert (win_fds != NULL);

  for (i = 0; i < win32_n_fds; i++)
    if (win_fds[i].type == DBUS_win_FD_SOCKET &&
	win_fds[i].fd == socket)
      {
	retval = RANDOMIZE (i);
	break;
      }
  
  _DBUS_UNLOCK (win_fds);

  return retval;
}

int
_dbus_handle_from_fd (int fd)
{
  int i = _dbus_win_allocate_fd ();
  int retval;

  win_fds[i].fd = fd;
  win_fds[i].type = DBUS_WIN_FD_C_LIB;

  retval = RANDOMIZE (i);

  _dbus_verbose ("encapsulated C file descriptor %d:%d:%d\n", retval, i, fd);

  return retval;
}

int
_dbus_handle_to_fd (int fd)
{
  int i;
  int retval = -1;

  _DBUS_LOCK (win_fds);

  _dbus_assert (win_fds != NULL);

  for (i = 0; i < win32_n_fds; i++)
    if (win_fds[i].type == DBUS_WIN_FD_C_LIB &&
	win_fds[i].fd == fd)
      {
	retval = RANDOMIZE (i);
	break;
      }
  
  _DBUS_UNLOCK (win_fds);

  return retval;
}

int
_dbus_decapsulate (int fd)
{
  if (fd == -1)
    return -1;

  _DBUS_LOCK (win_fds);

  fd = UNRANDOMIZE (fd);

  _dbus_assert (fd >= 0 && fd < win32_n_fds);
  _dbus_assert (win_fds != NULL);

  fd = win_fds[fd].fd;
  
  _DBUS_UNLOCK (win_fds);

  return fd;
}

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
      
  
  *fd1 = _dbus_handle_from_socket (socket1);
  *fd2 = _dbus_handle_from_socket (socket2);

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
#ifdef HAVE_POLL
#else /* ! HAVE_POLL */

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
      DBusPollFD *f = fds+i;
      int fd = UNRANDOMIZE (f->fd);

      _dbus_assert (fd >= 0 && fd < win32_n_fds);

      if (!warned &&
	  win_fds[fd].type != DBUS_win_FD_SOCKET)
	{
	  _dbus_warn ("Can poll only sockets on Win32");
	  warned = TRUE;
	}

      if (f->events & _DBUS_POLLIN)
	msgp += sprintf (msgp, "R:%d ", _dbus_handle_to_fd_quick (f->fd));

      if (f->events & _DBUS_POLLOUT)
	msgp += sprintf (msgp, "W:%d ", _dbus_handle_to_fd_quick (f->fd));

      msgp += sprintf (msgp, "E:%d ", _dbus_handle_to_fd_quick (f->fd));
    }

  msgp += sprintf (msgp, "\n");
  _dbus_verbose ("%s",msg);
#endif

  for (i = 0; i < n_fds; i++)
    {
      DBusPollFD *fdp = &fds[i];

#ifdef DBUS_WIN
      if (win_fds[UNRANDOMIZE (fdp->fd)].type != DBUS_win_FD_SOCKET)
	continue;
#endif

      if (fdp->events & _DBUS_POLLIN)
	FD_SET (_dbus_handle_to_fd_quick (fdp->fd), &read_set);

      if (fdp->events & _DBUS_POLLOUT)
	FD_SET (_dbus_handle_to_fd_quick (fdp->fd), &write_set);

      FD_SET (_dbus_handle_to_fd_quick (fdp->fd), &err_set);

      max_fd = MAX (max_fd, _dbus_handle_to_fd_quick (fdp->fd));
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
	      DBusPollFD *f = fds+i;

	      if (FD_ISSET (_dbus_handle_to_fd_quick (f->fd), &read_set))
	        msgp += sprintf (msgp, "R:%d ", _dbus_handle_to_fd_quick (f->fd));

	      if (FD_ISSET (_dbus_handle_to_fd_quick (f->fd), &write_set))
	        msgp += sprintf (msgp, "W:%d ", _dbus_handle_to_fd_quick (f->fd));

	      if (FD_ISSET (_dbus_handle_to_fd_quick (f->fd), &err_set))
	        msgp += sprintf (msgp, "E:%d ", _dbus_handle_to_fd_quick (f->fd));
	    }
      msgp += sprintf (msgp, "\n");
      _dbus_verbose ("%s",msg);
#endif

    for (i = 0; i < n_fds; i++)
	{
	  DBusPollFD *fdp = &fds[i];

	  fdp->revents = 0;

	  if (FD_ISSET (_dbus_handle_to_fd_quick (fdp->fd), &read_set))
	    fdp->revents |= _DBUS_POLLIN;

	  if (FD_ISSET (_dbus_handle_to_fd_quick (fdp->fd), &write_set))
	    fdp->revents |= _DBUS_POLLOUT;

	  if (FD_ISSET (_dbus_handle_to_fd_quick (fdp->fd), &err_set))
	    fdp->revents |= _DBUS_POLLERR;
	}
#ifdef DBUS_WIN
      _DBUS_UNLOCK (win_fds);
#endif
    }
  return ready;
#endif /* ! HAVE_POLL */
}

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