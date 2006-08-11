/* -*- mode: C; c-file-style: "gnu" -*- */
/* activation.c  Activation of services
 *
 * Copyright (C) 2003  CodeFactory AB
 * Copyright (C) 2003  Red Hat, Inc.
 * Copyright (C) 2004  Imendio HB
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
#include "activation.h"
#include "desktop-file.h"
#include "services.h"
#include "test.h"
#include "utils.h"
#include <dbus/dbus-internals.h>
#include <dbus/dbus-hash.h>
#include <dbus/dbus-list.h>
#include <dbus/dbus-shell.h>
#include <dbus/dbus-spawn.h>
#include <dbus/dbus-timeout.h>
#include <dbus/dbus-sysdeps.h>
#include <dirent.h>
#include <errno.h>

#define DBUS_SERVICE_SECTION "D-BUS Service"
#define DBUS_SERVICE_NAME "Name"
#define DBUS_SERVICE_EXEC "Exec"

struct BusActivation
{
  int refcount;
  DBusHashTable *entries;
  DBusHashTable *pending_activations;
  char *server_address;
  BusContext *context;
  int n_pending_activations; /**< This is in fact the number of BusPendingActivationEntry,
                              * i.e. number of pending activation requests, not pending
                              * activations per se
                              */
  DBusHashTable *directories;
};

typedef struct
{
  int refcount;
  char *dir_c;
  DBusHashTable *entries;
} BusServiceDirectory;

typedef struct
{
  int refcount;
  char *name;
  char *exec;
  unsigned long mtime;
  BusServiceDirectory *s_dir;
  char *filename;
} BusActivationEntry;

typedef struct BusPendingActivationEntry BusPendingActivationEntry;

struct BusPendingActivationEntry
{
  DBusMessage *activation_message;
  DBusConnection *connection;

  dbus_bool_t auto_activation;
};

typedef struct
{
  int refcount;
  BusActivation *activation;
  char *service_name;
  char *exec;
  DBusList *entries;
  int n_entries;
  DBusBabysitter *babysitter;
  DBusTimeout *timeout;
  unsigned int timeout_added : 1;
} BusPendingActivation;

#if 0
static BusServiceDirectory *
bus_service_directory_ref (BusServiceDirectory *dir)
{
  _dbus_assert (dir->refcount);
  
  dir->refcount++;

  return dir;
}
#endif

static void
bus_service_directory_unref (BusServiceDirectory *dir)
{
  if (dir == NULL) 
    return; 

  _dbus_assert (dir->refcount > 0);
  dir->refcount--;

  if (dir->refcount > 0)
    return;

  if (dir->entries)
    _dbus_hash_table_unref (dir->entries);

  dbus_free (dir->dir_c);
  dbus_free (dir);
}

static void
bus_pending_activation_entry_free (BusPendingActivationEntry *entry)
{
  if (entry->activation_message)
    dbus_message_unref (entry->activation_message);
  
  if (entry->connection)
    dbus_connection_unref (entry->connection);
  
  dbus_free (entry);
}

static void
handle_timeout_callback (DBusTimeout   *timeout,
                         void          *data)
{
  BusPendingActivation *pending_activation = data;

  while (!dbus_timeout_handle (pending_activation->timeout))
    _dbus_wait_for_memory ();
}

static BusPendingActivation * 
bus_pending_activation_ref (BusPendingActivation *pending_activation)
{
  _dbus_assert (pending_activation->refcount > 0);
  pending_activation->refcount += 1;

  return pending_activation;
}

static void
bus_pending_activation_unref (BusPendingActivation *pending_activation)
{
  DBusList *link;
  
  if (pending_activation == NULL) /* hash table requires this */
    return;

  _dbus_assert (pending_activation->refcount > 0);
  pending_activation->refcount -= 1;

  if (pending_activation->refcount > 0)
    return;
  
  if (pending_activation->timeout_added)
    {
      _dbus_loop_remove_timeout (bus_context_get_loop (pending_activation->activation->context),
                                 pending_activation->timeout,
                                 handle_timeout_callback, pending_activation);
      pending_activation->timeout_added = FALSE;
    }

  if (pending_activation->timeout)
    _dbus_timeout_unref (pending_activation->timeout);
  
  if (pending_activation->babysitter)
    {
      if (!_dbus_babysitter_set_watch_functions (pending_activation->babysitter,
                                                 NULL, NULL, NULL,
                                                 pending_activation->babysitter,
                                                 NULL))
        _dbus_assert_not_reached ("setting watch functions to NULL failed");
      
      _dbus_babysitter_unref (pending_activation->babysitter);
    }
  
  dbus_free (pending_activation->service_name);
  dbus_free (pending_activation->exec);

  link = _dbus_list_get_first_link (&pending_activation->entries);

  while (link != NULL)
    {
      BusPendingActivationEntry *entry = link->data;

      bus_pending_activation_entry_free (entry);

      link = _dbus_list_get_next_link (&pending_activation->entries, link);
    }
  _dbus_list_clear (&pending_activation->entries);

  pending_activation->activation->n_pending_activations -=
    pending_activation->n_entries;

  _dbus_assert (pending_activation->activation->n_pending_activations >= 0);
  
  dbus_free (pending_activation);
}

static BusActivationEntry *
bus_activation_entry_ref (BusActivationEntry *entry)
{
  _dbus_assert (entry->refcount > 0);
  entry->refcount++;

  return entry;
}

static void
bus_activation_entry_unref (BusActivationEntry *entry)
{
  if (entry == NULL) /* hash table requires this */
    return;
  
  _dbus_assert (entry->refcount > 0);
  entry->refcount--;
  
  if (entry->refcount > 0) 
    return;
  
  dbus_free (entry->name);
  dbus_free (entry->exec);
  dbus_free (entry->filename);

  dbus_free (entry);
}

static dbus_bool_t
update_desktop_file_entry (BusActivation       *activation,
                           BusServiceDirectory *s_dir,
                           DBusString          *filename,
                           BusDesktopFile      *desktop_file,
                           DBusError           *error)
{
  char *name, *exec;
  BusActivationEntry *entry;
  DBusStat stat_buf;
  DBusString file_path;

  _DBUS_ASSERT_ERROR_IS_CLEAR (error);
  
  name = NULL;
  exec = NULL;
  entry = NULL;
  
  if (!_dbus_string_init (&file_path))
    {
      BUS_SET_OOM (error);
      return FALSE;
    }
 
  if (!_dbus_string_append (&file_path, s_dir->dir_c) ||
      !_dbus_concat_dir_and_file (&file_path, filename))
    {
      BUS_SET_OOM (error);
      goto failed;
    }
 
  if (!_dbus_stat (&file_path, &stat_buf, NULL)) 
    {
      dbus_set_error (error, DBUS_ERROR_FAILED,
                      "Can't stat the service file\n");
      goto failed;
    }
 
  if (!bus_desktop_file_get_string (desktop_file,
                                    DBUS_SERVICE_SECTION,
                                    DBUS_SERVICE_NAME,
                                    &name))
    {
      dbus_set_error (error, DBUS_ERROR_FAILED,
                      "No \""DBUS_SERVICE_NAME"\" key in .service file\n");
      goto failed;
    }

  if (!bus_desktop_file_get_string (desktop_file,
                                    DBUS_SERVICE_SECTION,
                                    DBUS_SERVICE_EXEC,
                                    &exec))
    {
      dbus_set_error (error, DBUS_ERROR_FAILED,
                      "No \""DBUS_SERVICE_EXEC"\" key in .service file\n");
      goto failed;
    }

  entry = _dbus_hash_table_lookup_string (s_dir->entries, 
                                          _dbus_string_get_const_data (filename));
  if (entry == NULL) /* New file */
    { 
      /* FIXME we need a better-defined algorithm for which service file to
       * pick than "whichever one is first in the directory listing"
       */
      if (_dbus_hash_table_lookup_string (activation->entries, name))
        {
          dbus_set_error (error, DBUS_ERROR_FAILED,
                          "Service %s already exists in activation entry list\n", name);
          goto failed;
        }
      
      entry = dbus_new0 (BusActivationEntry, 1);
      if (entry == NULL)
        {
          BUS_SET_OOM (error);
          goto failed;
        }
     
      entry->name = name;
      entry->exec = exec;
      entry->refcount = 1;
    
      entry->s_dir = s_dir;
      entry->filename = _dbus_strdup (_dbus_string_get_const_data (filename));
      if (!entry->filename)
        {
          BUS_SET_OOM (error);
          goto failed;
        }

      if (!_dbus_hash_table_insert_string (activation->entries, entry->name, bus_activation_entry_ref (entry)))
        {
          BUS_SET_OOM (error);
          goto failed;
        }
     
      if (!_dbus_hash_table_insert_string (s_dir->entries, entry->filename, bus_activation_entry_ref (entry)))
        {
          /* Revert the insertion in the entries table */
          _dbus_hash_table_remove_string (activation->entries, entry->name);
          BUS_SET_OOM (error);
          goto failed;
        }

      _dbus_verbose ("Added \"%s\" to list of services\n", entry->name);
    }
  else /* Just update the entry */
    {
      bus_activation_entry_ref (entry);
      _dbus_hash_table_remove_string (activation->entries, entry->name);

      if (_dbus_hash_table_lookup_string (activation->entries, name))
        {
          _dbus_verbose ("The new service name \"%s\" of service file \"%s\" already in cache, ignoring\n",
                         name, _dbus_string_get_const_data (&file_path));
          goto failed;
        }
 
      dbus_free (entry->name);
      dbus_free (entry->exec);
      entry->name = name;
      entry->exec = exec;
      if (!_dbus_hash_table_insert_string (activation->entries,
                                           entry->name, bus_activation_entry_ref(entry)))
        {
          BUS_SET_OOM (error);
          /* Also remove path to entries hash since we want this in sync with
           * the entries hash table */
          _dbus_hash_table_remove_string (entry->s_dir->entries, 
                                          entry->filename);
          bus_activation_entry_unref (entry);
          return FALSE;
        }
    }
  
  entry->mtime = stat_buf.mtime;
  
  _dbus_string_free (&file_path);
  bus_activation_entry_unref (entry);

  return TRUE;

failed:
  dbus_free (name);
  dbus_free (exec);
  _dbus_string_free (&file_path);

  if (entry)
    bus_activation_entry_unref (entry);
  
  return FALSE;
}

static dbus_bool_t
check_service_file (BusActivation       *activation,
                    BusActivationEntry  *entry,
                    BusActivationEntry **updated_entry,
                    DBusError           *error)
{
  DBusStat stat_buf;
  dbus_bool_t retval;
  BusActivationEntry *tmp_entry;
  DBusString file_path;
  DBusString filename;

  retval = TRUE;
  tmp_entry = entry;
  
  _dbus_string_init_const (&filename, entry->filename);
  
  if (!_dbus_string_init (&file_path))
    {
      BUS_SET_OOM (error);
      return FALSE;
    }
 
  if (!_dbus_string_append (&file_path, entry->s_dir->dir_c) ||
      !_dbus_concat_dir_and_file (&file_path, &filename))
    {
      BUS_SET_OOM (error);
      retval = FALSE;
      goto out;
    }
  
  if (!_dbus_stat (&file_path, &stat_buf, NULL))
    {
      _dbus_verbose ("****** Can't stat file \"%s\", removing from cache\n",
                     _dbus_string_get_const_data (&file_path));

      _dbus_hash_table_remove_string (activation->entries, entry->name);
      _dbus_hash_table_remove_string (entry->s_dir->entries, entry->filename);

      tmp_entry = NULL;
      retval = TRUE;
      goto out;
    }
  else 
    {
      if (stat_buf.mtime > entry->mtime) 
        {
          BusDesktopFile *desktop_file;
          DBusError tmp_error;
          
          dbus_error_init (&tmp_error);
          
          desktop_file = bus_desktop_file_load (&file_path, &tmp_error);
          if (desktop_file == NULL)
            {
              _dbus_verbose ("Could not load %s: %s\n",
                             _dbus_string_get_const_data (&file_path), 
                             tmp_error.message);
              if (dbus_error_has_name (&tmp_error, DBUS_ERROR_NO_MEMORY))
                {
                  dbus_move_error (&tmp_error, error);
                  retval = FALSE;
                  goto out;
                }
              dbus_error_free (&tmp_error);
              retval = TRUE;
              goto out;
            }
          
          if (!update_desktop_file_entry (activation, entry->s_dir, &filename, desktop_file, &tmp_error))
            {
              bus_desktop_file_free (desktop_file);
              if (dbus_error_has_name (&tmp_error, DBUS_ERROR_NO_MEMORY))
                {
                  dbus_move_error (&tmp_error, error);
                  retval = FALSE;
                  goto out;
                }
              dbus_error_free (&tmp_error);
              retval = TRUE;
              goto out;
            }
         
          bus_desktop_file_free (desktop_file);
          retval = TRUE;
        }
    }
  
out:
  _dbus_string_free (&file_path);

  if (updated_entry != NULL)
    *updated_entry = tmp_entry;
  return retval;
}


/* warning: this doesn't fully "undo" itself on failure, i.e. doesn't strip
 * hash entries it already added.
 */
static dbus_bool_t
update_directory (BusActivation       *activation,
                  BusServiceDirectory *s_dir,
                  DBusError           *error)
{
  DBusDirIter *iter;
  DBusString dir, filename;
  BusDesktopFile *desktop_file;
  DBusError tmp_error;
  dbus_bool_t retval;
  BusActivationEntry *entry;
  DBusString full_path;
  
  _DBUS_ASSERT_ERROR_IS_CLEAR (error);
  
  iter = NULL;
  desktop_file = NULL;
  
  _dbus_string_init_const (&dir, s_dir->dir_c);
  
  if (!_dbus_string_init (&filename))
    {
      BUS_SET_OOM (error);
      return FALSE;
    }

  if (!_dbus_string_init (&full_path))
    {
      BUS_SET_OOM (error);
      _dbus_string_free (&filename);
      return FALSE;
    }

  retval = FALSE;

  /* from this point it's safe to "goto out" */
  
  iter = _dbus_directory_open (&dir, error);
  if (iter == NULL)
    {
      _dbus_verbose ("Failed to open directory %s: %s\n",
                     s_dir->dir_c, 
                     error ? error->message : "unknown");
      goto out;
    }
  
  /* Now read the files */
  dbus_error_init (&tmp_error);
  while (_dbus_directory_get_next_file (iter, &filename, &tmp_error))
    {
      _dbus_assert (!dbus_error_is_set (&tmp_error));
      
      _dbus_string_set_length (&full_path, 0);
      
      if (!_dbus_string_ends_with_c_str (&filename, ".service"))
        {
          _dbus_verbose ("Skipping non-.service file %s\n",
                         _dbus_string_get_const_data (&filename));
          continue;
        }

      entry = _dbus_hash_table_lookup_string (s_dir->entries, _dbus_string_get_const_data (&filename));
      if (entry) /* Already has this service file in the cache */ 
        {
          if (!check_service_file (activation, entry, NULL, error))
            goto out;

          continue;
        }
      
      if (!_dbus_string_append (&full_path, s_dir->dir_c) ||
          !_dbus_concat_dir_and_file (&full_path, &filename))
        {
          BUS_SET_OOM (error);
          goto out;
        }
          
      /* New file */
      desktop_file = bus_desktop_file_load (&full_path, &tmp_error);
      if (desktop_file == NULL)
        {
          _dbus_verbose ("Could not load %s: %s\n",
                         _dbus_string_get_const_data (&full_path),
                         tmp_error.message);

          if (dbus_error_has_name (&tmp_error, DBUS_ERROR_NO_MEMORY))
            {
              dbus_move_error (&tmp_error, error);
              goto out;
            }
          
          dbus_error_free (&tmp_error);
          continue;
        }

      if (!update_desktop_file_entry (activation, s_dir, &filename, desktop_file, &tmp_error))
        {
          bus_desktop_file_free (desktop_file);
          desktop_file = NULL;
          
          _dbus_verbose ("Could not add %s to activation entry list: %s\n",
                         _dbus_string_get_const_data (&full_path), tmp_error.message);

          if (dbus_error_has_name (&tmp_error, DBUS_ERROR_NO_MEMORY))
            {
              dbus_move_error (&tmp_error, error);
              goto out;
            }

          dbus_error_free (&tmp_error);
          continue;
        }
      else
        {
          bus_desktop_file_free (desktop_file);
          desktop_file = NULL;
          continue;
        }
    }

  if (dbus_error_is_set (&tmp_error))
    {
      dbus_move_error (&tmp_error, error);
      goto out;
    }
  
  retval = TRUE;

 out:
  if (!retval)
    _DBUS_ASSERT_ERROR_IS_SET (error);
  else
    _DBUS_ASSERT_ERROR_IS_CLEAR (error);
  
  if (iter != NULL)
    _dbus_directory_close (iter);
  _dbus_string_free (&filename);
  _dbus_string_free (&full_path);
  
  return retval;
}

BusActivation*
bus_activation_new (BusContext        *context,
                    const DBusString  *address,
                    DBusList         **directories,
                    DBusError         *error)
{
  BusActivation *activation;
  DBusList      *link;
  char          *dir;
  
  _DBUS_ASSERT_ERROR_IS_CLEAR (error);
  
  activation = dbus_new0 (BusActivation, 1);
  if (activation == NULL)
    {
      BUS_SET_OOM (error);
      return NULL;
    }
  
  activation->refcount = 1;
  activation->context = context;
  activation->n_pending_activations = 0;
  
  if (!_dbus_string_copy_data (address, &activation->server_address))
    {
      BUS_SET_OOM (error);
      goto failed;
    }
  
  activation->entries = _dbus_hash_table_new (DBUS_HASH_STRING, NULL,
                                             (DBusFreeFunction)bus_activation_entry_unref);
  if (activation->entries == NULL)
    {      
      BUS_SET_OOM (error);
      goto failed;
    }

  activation->pending_activations = _dbus_hash_table_new (DBUS_HASH_STRING, NULL,
                                                          (DBusFreeFunction)bus_pending_activation_unref);

  if (activation->pending_activations == NULL)
    {
      BUS_SET_OOM (error);
      goto failed;
    }

  activation->directories = _dbus_hash_table_new (DBUS_HASH_STRING, NULL,
                                                  (DBusFreeFunction)bus_service_directory_unref);
  
  if (activation->directories == NULL) 
    {
      BUS_SET_OOM (error);
      goto failed;
    }
 
  /* Load service files */
  link = _dbus_list_get_first_link (directories);
  while (link != NULL)
    {
      BusServiceDirectory *s_dir;
      
      dir = _dbus_strdup ((const char *) link->data);
      if (!dir)
        {
          BUS_SET_OOM (error);
          goto failed;
        }
      
      s_dir = dbus_new0 (BusServiceDirectory, 1);
      if (!s_dir)
        {
          dbus_free (dir);
          BUS_SET_OOM (error);
          goto failed;
        }

      s_dir->refcount = 1;
      s_dir->dir_c = dir;
      
      s_dir->entries = _dbus_hash_table_new (DBUS_HASH_STRING, NULL,
                                             (DBusFreeFunction)bus_activation_entry_unref);

      if (!s_dir->entries)
        {
          bus_service_directory_unref (s_dir);
          BUS_SET_OOM (error);
          goto failed;
        }

      if (!_dbus_hash_table_insert_string (activation->directories, s_dir->dir_c, s_dir))
        {
          bus_service_directory_unref (s_dir);
          BUS_SET_OOM (error);
          goto failed;
        }

      if (!update_directory (activation, s_dir, error))
        goto failed;
      
      link = _dbus_list_get_next_link (directories, link);
    }

  return activation;
  
 failed:
  bus_activation_unref (activation);  
  return NULL;
}

BusActivation *
bus_activation_ref (BusActivation *activation)
{
  _dbus_assert (activation->refcount > 0);
  
  activation->refcount += 1;

  return activation;
}

void
bus_activation_unref (BusActivation *activation)
{
  _dbus_assert (activation->refcount > 0);

  activation->refcount -= 1;

  if (activation->refcount > 0)
    return;
  
  dbus_free (activation->server_address);
  if (activation->entries)
    _dbus_hash_table_unref (activation->entries);
  if (activation->pending_activations)
    _dbus_hash_table_unref (activation->pending_activations);
  if (activation->directories)  
    _dbus_hash_table_unref (activation->directories);
  
  dbus_free (activation);
}

static void
child_setup (void *data)
{
  BusActivation *activation = data;
  const char *type;
  
  /* If no memory, we simply have the child exit, so it won't try
   * to connect to the wrong thing.
   */
  if (!_dbus_setenv ("DBUS_STARTER_ADDRESS", activation->server_address))
    _dbus_exit (1);
  
  type = bus_context_get_type (activation->context);
  if (type != NULL)
    {
      if (!_dbus_setenv ("DBUS_STARTER_BUS_TYPE", type))
        _dbus_exit (1);

      if (strcmp (type, "session") == 0)
        {
          if (!_dbus_setenv ("DBUS_SESSION_BUS_ADDRESS",
                             activation->server_address))
            _dbus_exit (1);
        }
      else if (strcmp (type, "system") == 0)
        {
          if (!_dbus_setenv ("DBUS_SYSTEM_BUS_ADDRESS",
                             activation->server_address))
            _dbus_exit (1);
        }
    }
}

typedef struct
{
  BusPendingActivation *pending_activation;
  DBusPreallocatedHash *hash_entry;
} RestorePendingData;

static void
restore_pending (void *data)
{
  RestorePendingData *d = data;

  _dbus_assert (d->pending_activation != NULL);
  _dbus_assert (d->hash_entry != NULL);

  _dbus_verbose ("Restoring pending activation for service %s, has timeout = %d\n",
                 d->pending_activation->service_name,
                 d->pending_activation->timeout_added);
  
  _dbus_hash_table_insert_string_preallocated (d->pending_activation->activation->pending_activations,
                                               d->hash_entry,
                                               d->pending_activation->service_name, d->pending_activation);

  bus_pending_activation_ref (d->pending_activation);
  
  d->hash_entry = NULL;
}

static void
free_pending_restore_data (void *data)
{
  RestorePendingData *d = data;

  if (d->hash_entry)
    _dbus_hash_table_free_preallocated_entry (d->pending_activation->activation->pending_activations,
                                              d->hash_entry);

  bus_pending_activation_unref (d->pending_activation);
  
  dbus_free (d);
}

static dbus_bool_t
add_restore_pending_to_transaction (BusTransaction       *transaction,
                                    BusPendingActivation *pending_activation)
{
  RestorePendingData *d;

  d = dbus_new (RestorePendingData, 1);
  if (d == NULL)
    return FALSE;
  
  d->pending_activation = pending_activation;
  d->hash_entry = _dbus_hash_table_preallocate_entry (d->pending_activation->activation->pending_activations);
  
  bus_pending_activation_ref (d->pending_activation);
  
  if (d->hash_entry == NULL ||
      !bus_transaction_add_cancel_hook (transaction, restore_pending, d,
                                        free_pending_restore_data))
    {
      free_pending_restore_data (d);
      return FALSE;
    }

  _dbus_verbose ("Saved pending activation to be restored if the transaction fails\n");
  
  return TRUE;
}

dbus_bool_t
bus_activation_service_created (BusActivation  *activation,
                                const char     *service_name,
                                BusTransaction *transaction,
                                DBusError      *error)
{
  BusPendingActivation *pending_activation;
  DBusMessage *message;
  DBusList *link;

  _DBUS_ASSERT_ERROR_IS_CLEAR (error);
  
  /* Check if it's a pending activation */
  pending_activation = _dbus_hash_table_lookup_string (activation->pending_activations, service_name);

  if (!pending_activation)
    return TRUE;

  link = _dbus_list_get_first_link (&pending_activation->entries);
  while (link != NULL)
    {
      BusPendingActivationEntry *entry = link->data;
      DBusList *next = _dbus_list_get_next_link (&pending_activation->entries, link);
      
      if (dbus_connection_get_is_connected (entry->connection))
        {
          /* Only send activation replies to regular activation requests. */
          if (!entry->auto_activation)
            {
              dbus_uint32_t result;
              
              message = dbus_message_new_method_return (entry->activation_message);
              if (!message)
                {
                  BUS_SET_OOM (error);
                  goto error;
                }

              result = DBUS_START_REPLY_SUCCESS;
              
              if (!dbus_message_append_args (message,
                                             DBUS_TYPE_UINT32, &result,
                                             DBUS_TYPE_INVALID))
                {
                  dbus_message_unref (message);
                  BUS_SET_OOM (error);
                  goto error;
                }
              
              if (!bus_transaction_send_from_driver (transaction, entry->connection, message))
                {
                  dbus_message_unref (message);
                  BUS_SET_OOM (error);
                  goto error;
                }
              
              dbus_message_unref (message);
            }
        }
      
      link = next;
    }

  return TRUE;

 error:
  return FALSE;
}

dbus_bool_t
bus_activation_send_pending_auto_activation_messages (BusActivation  *activation,
                                                      BusService     *service,
                                                      BusTransaction *transaction,
                                                      DBusError      *error)
{
  BusPendingActivation *pending_activation;
  DBusList *link;

  _DBUS_ASSERT_ERROR_IS_CLEAR (error);
  
  /* Check if it's a pending activation */
  pending_activation = _dbus_hash_table_lookup_string (activation->pending_activations,
                                                       bus_service_get_name (service));

  if (!pending_activation)
    return TRUE;

  link = _dbus_list_get_first_link (&pending_activation->entries);
  while (link != NULL)
    {
      BusPendingActivationEntry *entry = link->data;
      DBusList *next = _dbus_list_get_next_link (&pending_activation->entries, link);

      if (entry->auto_activation && dbus_connection_get_is_connected (entry->connection))
        {
          DBusConnection *addressed_recipient;
          
          addressed_recipient = bus_service_get_primary_owners_connection (service);

          /* Check the security policy, which has the side-effect of adding an
           * expected pending reply.
           */
          if (!bus_context_check_security_policy (activation->context, transaction,
                                                  entry->connection,
                                                  addressed_recipient,
                                                  addressed_recipient,
                                                  entry->activation_message, error))
            goto error;

          if (!bus_transaction_send (transaction, addressed_recipient, entry->activation_message))
            {
              BUS_SET_OOM (error);
              goto error;
            }
        }

      link = next;
    }

  if (!add_restore_pending_to_transaction (transaction, pending_activation))
    {
      _dbus_verbose ("Could not add cancel hook to transaction to revert removing pending activation\n");
      BUS_SET_OOM (error);
      goto error;
    }
  
  _dbus_hash_table_remove_string (activation->pending_activations, bus_service_get_name (service));

  return TRUE;

 error:
  return FALSE;
}

/**
 * FIXME @todo the error messages here would ideally be preallocated
 * so we don't need to allocate memory to send them.
 * Using the usual tactic, prealloc an OOM message, then
 * if we can't alloc the real error send the OOM error instead.
 */
static dbus_bool_t
try_send_activation_failure (BusPendingActivation *pending_activation,
                             const DBusError      *how)
{
  BusActivation *activation;
  DBusList *link;
  BusTransaction *transaction;
  
  activation = pending_activation->activation;

  transaction = bus_transaction_new (activation->context);
  if (transaction == NULL)
    return FALSE;
  
  link = _dbus_list_get_first_link (&pending_activation->entries);
  while (link != NULL)
    {
      BusPendingActivationEntry *entry = link->data;
      DBusList *next = _dbus_list_get_next_link (&pending_activation->entries, link);
      
      if (dbus_connection_get_is_connected (entry->connection))
        {
          if (!bus_transaction_send_error_reply (transaction,
                                                 entry->connection,
                                                 how,
                                                 entry->activation_message))
            goto error;
        }
      
      link = next;
    }

  bus_transaction_execute_and_free (transaction);
  
  return TRUE;

 error:
  if (transaction)
    bus_transaction_cancel_and_free (transaction);
  return FALSE;
}

/**
 * Free the pending activation and send an error message to all the
 * connections that were waiting for it.
 */
static void
pending_activation_failed (BusPendingActivation *pending_activation,
                           const DBusError      *how)
{
  /* FIXME use preallocated OOM messages instead of bus_wait_for_memory() */
  while (!try_send_activation_failure (pending_activation, how))
    _dbus_wait_for_memory ();

  /* Destroy this pending activation */
  _dbus_hash_table_remove_string (pending_activation->activation->pending_activations,
                                  pending_activation->service_name);
}

static dbus_bool_t
babysitter_watch_callback (DBusWatch     *watch,
                           unsigned int   condition,
                           void          *data)
{
  BusPendingActivation *pending_activation = data;
  dbus_bool_t retval;
  DBusBabysitter *babysitter;

  babysitter = pending_activation->babysitter;
  
  _dbus_babysitter_ref (babysitter);
  
  retval = dbus_watch_handle (watch, condition);

  /* FIXME this is broken in the same way that
   * connection watches used to be; there should be
   * a separate callback for status change, instead
   * of doing "if we handled a watch status might
   * have changed"
   *
   * Fixing this lets us move dbus_watch_handle
   * calls into dbus-mainloop.c
   */
  
  if (_dbus_babysitter_get_child_exited (babysitter))
    {
      DBusError error;
      DBusHashIter iter;
      
      dbus_error_init (&error);
      _dbus_babysitter_set_child_exit_error (babysitter, &error);

      /* Destroy all pending activations with the same exec */
      _dbus_hash_iter_init (pending_activation->activation->pending_activations,
                            &iter);
      while (_dbus_hash_iter_next (&iter))
        {
          BusPendingActivation *p = _dbus_hash_iter_get_value (&iter);
         
          if (p != pending_activation && strcmp (p->exec, pending_activation->exec) == 0)
            pending_activation_failed (p, &error);
        }
      
      /* Destroys the pending activation */
      pending_activation_failed (pending_activation, &error);

      dbus_error_free (&error);
    }
  
  _dbus_babysitter_unref (babysitter);

  return retval;
}

static dbus_bool_t
add_babysitter_watch (DBusWatch      *watch,
                      void           *data)
{
  BusPendingActivation *pending_activation = data;

  return _dbus_loop_add_watch (bus_context_get_loop (pending_activation->activation->context),
                               watch, babysitter_watch_callback, pending_activation,
                               NULL);
}

static void
remove_babysitter_watch (DBusWatch      *watch,
                         void           *data)
{
  BusPendingActivation *pending_activation = data;
  
  _dbus_loop_remove_watch (bus_context_get_loop (pending_activation->activation->context),
                           watch, babysitter_watch_callback, pending_activation);
}

static dbus_bool_t
pending_activation_timed_out (void *data)
{
  BusPendingActivation *pending_activation = data;
  DBusError error;
  
  /* Kill the spawned process, since it sucks
   * (not sure this is what we want to do, but
   * may as well try it for now)
   */
  if (pending_activation->babysitter) 
    _dbus_babysitter_kill_child (pending_activation->babysitter);

  dbus_error_init (&error);

  dbus_set_error (&error, DBUS_ERROR_TIMED_OUT,
                  "Activation of %s timed out",
                  pending_activation->service_name);

  pending_activation_failed (pending_activation, &error);

  dbus_error_free (&error);

  return TRUE;
}

static void
cancel_pending (void *data)
{
  BusPendingActivation *pending_activation = data;

  _dbus_verbose ("Canceling pending activation of %s\n",
                 pending_activation->service_name);

  if (pending_activation->babysitter)
    _dbus_babysitter_kill_child (pending_activation->babysitter);
  
  _dbus_hash_table_remove_string (pending_activation->activation->pending_activations,
                                  pending_activation->service_name);
}

static void
free_pending_cancel_data (void *data)
{
  BusPendingActivation *pending_activation = data;
  
  bus_pending_activation_unref (pending_activation);
}

static dbus_bool_t
add_cancel_pending_to_transaction (BusTransaction       *transaction,
                                   BusPendingActivation *pending_activation)
{  
  if (!bus_transaction_add_cancel_hook (transaction, cancel_pending,
                                        pending_activation,
                                        free_pending_cancel_data))
    return FALSE;

  bus_pending_activation_ref (pending_activation); 
  
  _dbus_verbose ("Saved pending activation to be canceled if the transaction fails\n");
  
  return TRUE;
}

static dbus_bool_t 
update_service_cache (BusActivation *activation, DBusError *error)
{
  DBusHashIter iter;
 
  _dbus_hash_iter_init (activation->directories, &iter);
  while (_dbus_hash_iter_next (&iter))
    {
      DBusError tmp_error;
      BusServiceDirectory *s_dir;

      s_dir = _dbus_hash_iter_get_value (&iter);

      dbus_error_init (&tmp_error);
      if (!update_directory (activation, s_dir, &tmp_error))
        {
          if (dbus_error_has_name (&tmp_error, DBUS_ERROR_NO_MEMORY))
            {
              dbus_move_error (&tmp_error, error);
              return FALSE;
            }

          dbus_error_free (&tmp_error);
          continue;
        }
    }
  
  return TRUE;
}

static BusActivationEntry *
activation_find_entry (BusActivation *activation, 
                       const char    *service_name,
                       DBusError     *error)
{
  BusActivationEntry *entry;
  
  entry = _dbus_hash_table_lookup_string (activation->entries, service_name);
  if (!entry)
    { 
      if (!update_service_cache (activation, error)) 
        return NULL;

      entry = _dbus_hash_table_lookup_string (activation->entries,
                                              service_name);
    }
  else 
    {
      BusActivationEntry *updated_entry;

      if (!check_service_file (activation, entry, &updated_entry, error)) 
        return NULL;

      entry = updated_entry;
    }

  if (!entry) 
    {
      dbus_set_error (error, DBUS_ERROR_SERVICE_UNKNOWN,
                      "The name %s was not provided by any .service files",
                      service_name);
      return NULL;
    }

  return entry;
}

dbus_bool_t
bus_activation_activate_service (BusActivation  *activation,
                                 DBusConnection *connection,
                                 BusTransaction *transaction,
                                 dbus_bool_t     auto_activation,
                                 DBusMessage    *activation_message,
                                 const char     *service_name,
                                 DBusError      *error)
{
  BusActivationEntry *entry;
  BusPendingActivation *pending_activation;
  BusPendingActivationEntry *pending_activation_entry;
  DBusMessage *message;
  DBusString service_str;
  char **argv;
  int argc;
  dbus_bool_t retval;
  DBusHashIter iter;
  dbus_bool_t activated;
  
  activated = TRUE;

  _DBUS_ASSERT_ERROR_IS_CLEAR (error);

  if (activation->n_pending_activations >=
      bus_context_get_max_pending_activations (activation->context))
    {
      dbus_set_error (error, DBUS_ERROR_LIMITS_EXCEEDED,
                      "The maximum number of pending activations has been reached, activation of %s failed",
                      service_name);
      return FALSE;
    }

  entry = activation_find_entry (activation, service_name, error);
  if (!entry) 
    return FALSE;

  /* Bypass the registry lookup if we're auto-activating, bus_dispatch would not
   * call us if the service is already active.
   */
  if (!auto_activation)
    {
      /* Check if the service is active */
      _dbus_string_init_const (&service_str, service_name);
      if (bus_registry_lookup (bus_context_get_registry (activation->context), &service_str) != NULL)
        {
          dbus_uint32_t result;
          
          _dbus_verbose ("Service \"%s\" is already active\n", service_name);
      
          message = dbus_message_new_method_return (activation_message);

          if (!message)
            {
              _dbus_verbose ("No memory to create reply to activate message\n");
              BUS_SET_OOM (error);
              return FALSE;
            }

          result = DBUS_START_REPLY_ALREADY_RUNNING;
          
          if (!dbus_message_append_args (message,
                                         DBUS_TYPE_UINT32, &result,
                                         DBUS_TYPE_INVALID))
            {
              _dbus_verbose ("No memory to set args of reply to activate message\n");
              BUS_SET_OOM (error);
              dbus_message_unref (message);
              return FALSE;
            }

          retval = bus_transaction_send_from_driver (transaction, connection, message);
          dbus_message_unref (message);
          if (!retval)
            {
              _dbus_verbose ("Failed to send reply\n");
              BUS_SET_OOM (error);
            }

          return retval;
        }
    }
  
  pending_activation_entry = dbus_new0 (BusPendingActivationEntry, 1);
  if (!pending_activation_entry)
    {
      _dbus_verbose ("Failed to create pending activation entry\n");
      BUS_SET_OOM (error);
      return FALSE;
    }

  pending_activation_entry->auto_activation = auto_activation;

  pending_activation_entry->activation_message = activation_message;
  dbus_message_ref (activation_message);
  pending_activation_entry->connection = connection;
  dbus_connection_ref (connection);
  
  /* Check if the service is being activated */
  pending_activation = _dbus_hash_table_lookup_string (activation->pending_activations, service_name);
  if (pending_activation)
    {
      if (!_dbus_list_append (&pending_activation->entries, pending_activation_entry))
        {
          _dbus_verbose ("Failed to append a new entry to pending activation\n");
          
          BUS_SET_OOM (error);
          bus_pending_activation_entry_free (pending_activation_entry);
          return FALSE;
        }

      pending_activation->n_entries += 1;
      pending_activation->activation->n_pending_activations += 1;
    }
  else
    {
      pending_activation = dbus_new0 (BusPendingActivation, 1);
      if (!pending_activation)
        {
          _dbus_verbose ("Failed to create pending activation\n");
          
          BUS_SET_OOM (error);
          bus_pending_activation_entry_free (pending_activation_entry);          
          return FALSE;
        }

      pending_activation->activation = activation;
      pending_activation->refcount = 1;
      
      pending_activation->service_name = _dbus_strdup (service_name);
      if (!pending_activation->service_name)
        {
          _dbus_verbose ("Failed to copy service name for pending activation\n");
          
          BUS_SET_OOM (error);
          bus_pending_activation_unref (pending_activation);
          bus_pending_activation_entry_free (pending_activation_entry);          
          return FALSE;
        }

      pending_activation->exec = _dbus_strdup (entry->exec);
      if (!pending_activation->exec)
        {
          _dbus_verbose ("Failed to copy service exec for pending activation\n");
          BUS_SET_OOM (error);
          bus_pending_activation_unref (pending_activation);
          bus_pending_activation_entry_free (pending_activation_entry);
          return FALSE;
        }

      pending_activation->timeout =
        _dbus_timeout_new (bus_context_get_activation_timeout (activation->context),
                           pending_activation_timed_out,
                           pending_activation,
                           NULL);
      if (!pending_activation->timeout)
        {
          _dbus_verbose ("Failed to create timeout for pending activation\n");
          
          BUS_SET_OOM (error);
          bus_pending_activation_unref (pending_activation);
          bus_pending_activation_entry_free (pending_activation_entry);
          return FALSE;
        }

      if (!_dbus_loop_add_timeout (bus_context_get_loop (activation->context),
                                   pending_activation->timeout,
                                   handle_timeout_callback,
                                   pending_activation,
                                   NULL))
        {
          _dbus_verbose ("Failed to add timeout for pending activation\n");
          
          BUS_SET_OOM (error);
          bus_pending_activation_unref (pending_activation);
          bus_pending_activation_entry_free (pending_activation_entry);          
          return FALSE;
        }

      pending_activation->timeout_added = TRUE;
      
      if (!_dbus_list_append (&pending_activation->entries, pending_activation_entry))
        {
          _dbus_verbose ("Failed to add entry to just-created pending activation\n");
          
          BUS_SET_OOM (error);
          bus_pending_activation_unref (pending_activation);
          bus_pending_activation_entry_free (pending_activation_entry);          
          return FALSE;
        }

      pending_activation->n_entries += 1;
      pending_activation->activation->n_pending_activations += 1;
    
      activated = FALSE;
      _dbus_hash_iter_init (activation->pending_activations, &iter);
      while (_dbus_hash_iter_next (&iter))
        {
          BusPendingActivation *p = _dbus_hash_iter_get_value (&iter);
          
          if (strcmp (p->exec, entry->exec) == 0) 
            {
              activated = TRUE;
              break;
            }
        }
     
      if (!_dbus_hash_table_insert_string (activation->pending_activations,
                                           pending_activation->service_name,
                                           pending_activation))
        {
          _dbus_verbose ("Failed to put pending activation in hash table\n");
          
          BUS_SET_OOM (error);
          bus_pending_activation_unref (pending_activation);
          return FALSE;
        }
    }
  
  if (!add_cancel_pending_to_transaction (transaction, pending_activation))
    {
      _dbus_verbose ("Failed to add pending activation cancel hook to transaction\n");
      BUS_SET_OOM (error);
      _dbus_hash_table_remove_string (activation->pending_activations,
                                      pending_activation->service_name);

      return FALSE;
    }
  
  if (activated)
    return TRUE;

  /* Now try to spawn the process */
  if (!_dbus_shell_parse_argv (entry->exec, &argc, &argv, error))
    {
      _dbus_verbose ("Failed to parse command line: %s\n", entry->exec);
      _DBUS_ASSERT_ERROR_IS_SET (error);
      
      _dbus_hash_table_remove_string (activation->pending_activations,
                                      pending_activation->service_name);

      return FALSE;
    }

  _dbus_verbose ("Spawning %s ...\n", argv[0]);
  if (!_dbus_spawn_async_with_babysitter (&pending_activation->babysitter, argv,
                                          child_setup, activation, 
                                          error))
    {
      _dbus_verbose ("Failed to spawn child\n");
      _DBUS_ASSERT_ERROR_IS_SET (error);
      dbus_free_string_array (argv);

      return FALSE;
    }

  dbus_free_string_array (argv);

  _dbus_assert (pending_activation->babysitter != NULL);
  
  if (!_dbus_babysitter_set_watch_functions (pending_activation->babysitter,
                                             add_babysitter_watch,
                                             remove_babysitter_watch,
                                             NULL,
                                             pending_activation,
                                             NULL))
    {
      BUS_SET_OOM (error);
      _dbus_verbose ("Failed to set babysitter watch functions\n");
      return FALSE;
    }
  
  return TRUE;
}

dbus_bool_t
bus_activation_list_services (BusActivation *activation,
			      char        ***listp,
			      int           *array_len)
{
  int i, j, len;
  char **retval;
  DBusHashIter iter;

  len = _dbus_hash_table_get_n_entries (activation->entries);
  retval = dbus_new (char *, len + 1);

  if (retval == NULL)
    return FALSE;

  _dbus_hash_iter_init (activation->entries, &iter);
  i = 0;
  while (_dbus_hash_iter_next (&iter))
    {
      BusActivationEntry *entry = _dbus_hash_iter_get_value (&iter);

      retval[i] = _dbus_strdup (entry->name);
      if (retval[i] == NULL)
	goto error;

      i++;
    }

  retval[i] = NULL;

  if (array_len)
    *array_len = len;

  *listp = retval;
  return TRUE;

 error:
  for (j = 0; j < i; j++)
    dbus_free (retval[i]);
  dbus_free (retval);

  return FALSE;
}
  

#ifdef DBUS_BUILD_TESTS

#include <stdio.h>

#define SERVICE_NAME_1 "MyService1"
#define SERVICE_NAME_2 "MyService2"
#define SERVICE_NAME_3 "MyService3"

#define SERVICE_FILE_1 "service-1.service"
#define SERVICE_FILE_2 "service-2.service"
#define SERVICE_FILE_3 "service-3.service"

static dbus_bool_t
test_create_service_file (DBusString *dir,
                          const char *filename, 
                          const char *name, 
                          const char *exec)
{
  DBusString  file_name, full_path;
  FILE        *file;
  dbus_bool_t  ret_val;

  ret_val = TRUE;
  _dbus_string_init_const (&file_name, filename);

  if (!_dbus_string_init (&full_path))
    return FALSE;

  if (!_dbus_string_append (&full_path, _dbus_string_get_const_data (dir)) ||
      !_dbus_concat_dir_and_file (&full_path, &file_name))
    {
      ret_val = FALSE;
      goto out;
    }
  
  file = fopen (_dbus_string_get_const_data (&full_path), "w");
  if (!file)
    {
      ret_val = FALSE;
      goto out;
    }

  fprintf (file, "[D-BUS Service]\nName=%s\nExec=%s\n", name, exec);
  fclose (file);

out:
  _dbus_string_free (&full_path);
  return ret_val;
}

static dbus_bool_t
test_remove_service_file (DBusString *dir, const char *filename)
{
  DBusString  file_name, full_path;
  dbus_bool_t ret_val;
  
  ret_val = TRUE;
 
  _dbus_string_init_const (&file_name, filename);

  if (!_dbus_string_init (&full_path))
    return FALSE;

  if (!_dbus_string_append (&full_path, _dbus_string_get_const_data (dir)) ||
      !_dbus_concat_dir_and_file (&full_path, &file_name))
    {
      ret_val = FALSE;
      goto out;
    }

  if (!_dbus_delete_file (&full_path, NULL))
    {
      ret_val = FALSE;
      goto out;
    }

out:
  _dbus_string_free (&full_path);
  return ret_val;
}

static dbus_bool_t
test_remove_directory (DBusString *dir)
{
  DBusDirIter *iter;
  DBusString   filename, full_path;
  dbus_bool_t  ret_val;
  
  ret_val = TRUE;
  
  if (!_dbus_string_init (&filename))
    return FALSE;

  if (!_dbus_string_init (&full_path))
    {
      _dbus_string_free (&filename);
      return FALSE;
    }
    
  iter = _dbus_directory_open (dir, NULL);
  if (iter == NULL)
    {
      ret_val = FALSE;
      goto out;
    }
  
  while (_dbus_directory_get_next_file (iter, &filename, NULL)) 
    {
      if (!test_remove_service_file (dir, _dbus_string_get_const_data (&filename)))
        {
          ret_val = FALSE;
          goto out;
        }
    }
  _dbus_directory_close (iter);

  if (!_dbus_delete_directory (dir, NULL))
    {
      ret_val = FALSE;
      goto out;
    }

out:
  _dbus_string_free (&filename);
  _dbus_string_free (&full_path);

  return ret_val;
}

static dbus_bool_t
init_service_reload_test (DBusString *dir)
{
  DBusStat stat_buf;
 
  if (!_dbus_stat (dir, &stat_buf, NULL))
    {
      if (!_dbus_create_directory (dir, NULL))
        return FALSE;
    }
  else 
    {
      if (!test_remove_directory (dir))
        return FALSE;

      if (!_dbus_create_directory (dir, NULL))
        return FALSE;
    }

  /* Create one initial file */
  if (!test_create_service_file (dir, SERVICE_FILE_1, SERVICE_NAME_1, "exec-1"))
    return FALSE;

  return TRUE;
}

static dbus_bool_t
cleanup_service_reload_test (DBusString *dir)
{
  if (!test_remove_directory (dir))
    return FALSE;

  return TRUE;
}

typedef struct 
{
  BusActivation *activation;
  const char    *service_name;
  dbus_bool_t    expecting_find;
} CheckData;

static dbus_bool_t
check_func (void *data)
{
  CheckData          *d;
  BusActivationEntry *entry;
  DBusError           error;
  dbus_bool_t         ret_val;
  
  ret_val = TRUE;
  d = data;
  
  dbus_error_init (&error);
 
  entry = activation_find_entry (d->activation, d->service_name, &error);
  if (entry == NULL)
    {
      if (dbus_error_has_name (&error, DBUS_ERROR_NO_MEMORY)) 
        {
          ret_val = TRUE;
        }
      else
        {
          if (d->expecting_find)
            ret_val = FALSE;
        }
      
      dbus_error_free (&error);
    }
  else 
    {
      if (!d->expecting_find)
        ret_val = FALSE;
    }

  return ret_val;
}

static dbus_bool_t
do_test (const char *description, dbus_bool_t oom_test, CheckData *data)
{
  dbus_bool_t err;

  if (oom_test)
    err = !_dbus_test_oom_handling (description, check_func, data);
  else
    err = !check_func (data);

  if (err) 
    _dbus_assert_not_reached ("Test failed");

  return TRUE;
}

static dbus_bool_t
do_service_reload_test (DBusString *dir, dbus_bool_t oom_test)
{
  BusActivation *activation;
  DBusString     address;
  DBusList      *directories;
  CheckData      d;
  
  directories = NULL;
  _dbus_string_init_const (&address, "");
 
  if (!_dbus_list_append (&directories, _dbus_string_get_data (dir)))
    return FALSE; 

  activation = bus_activation_new (NULL, &address, &directories, NULL);
  if (!activation)
    return FALSE;

  d.activation = activation;
  
  /* Check for existing service file */
  d.expecting_find = TRUE;
  d.service_name = SERVICE_NAME_1;

  if (!do_test ("Existing service file", oom_test, &d))
    return FALSE;

  /* Check for non-existing service file */
  d.expecting_find = FALSE;
  d.service_name = SERVICE_NAME_3;

  if (!do_test ("Nonexisting service file", oom_test, &d))
    return FALSE;

  /* Check for added service file */
  if (!test_create_service_file (dir, SERVICE_FILE_2, SERVICE_NAME_2, "exec-2"))
    return FALSE;

  d.expecting_find = TRUE;
  d.service_name = SERVICE_NAME_2;
  
  if (!do_test ("Added service file", oom_test, &d))
    return FALSE;
  
  /* Check for removed service file */
  if (!test_remove_service_file (dir, SERVICE_FILE_2))
    return FALSE;

  d.expecting_find = FALSE;
  d.service_name = SERVICE_FILE_2;

  if (!do_test ("Removed service file", oom_test, &d))
    return FALSE;
  
  /* Check for updated service file */
  
  _dbus_sleep_milliseconds (1000); /* Sleep a second to make sure the mtime is updated */

  if (!test_create_service_file (dir, SERVICE_FILE_1, SERVICE_NAME_3, "exec-3"))
    return FALSE;

  d.expecting_find = TRUE;
  d.service_name = SERVICE_NAME_3;

  if (!do_test ("Updated service file, part 1", oom_test, &d))
    return FALSE;

  d.expecting_find = FALSE;
  d.service_name = SERVICE_NAME_1;

  if (!do_test ("Updated service file, part 2", oom_test, &d))
    return FALSE; 

  bus_activation_unref (activation);
  _dbus_list_clear (&directories);

  return TRUE;
}

dbus_bool_t
bus_activation_service_reload_test (const DBusString *test_data_dir)
{
  DBusString directory;

  if (!_dbus_string_init (&directory))
    return FALSE;
  
  if (!_dbus_string_append (&directory, _dbus_get_tmpdir()))
    return FALSE;
  
  if (!_dbus_string_append (&directory, "/dbus-reload-test-") ||
      !_dbus_generate_random_ascii (&directory, 6))
     {
       return FALSE;
     }
   
  /* Do normal tests */
  if (!init_service_reload_test (&directory))
    _dbus_assert_not_reached ("could not initiate service reload test");
 
  if (!do_service_reload_test (&directory, FALSE))
    ; /* Do nothing? */
  
  /* Do OOM tests */
  if (!init_service_reload_test (&directory))
    _dbus_assert_not_reached ("could not initiate service reload test");
 
  if (!do_service_reload_test (&directory, TRUE))
    ; /* Do nothing? */
 
  /* Cleanup test directory */
  if (!cleanup_service_reload_test (&directory))
    return FALSE;
  
  _dbus_string_free (&directory);
  
  return TRUE;
}

#endif /* DBUS_BUILD_TESTS */
