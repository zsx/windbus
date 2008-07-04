LIBRARY ${DEF_LIBRARY_NAME}
EXPORTS
_dbus_abort
_dbus_accept
_dbus_address_append_escaped
_dbus_address_test
_dbus_append_keyring_directory_for_credentials
_dbus_append_session_config_file
_dbus_append_system_config_file
_dbus_append_user_from_current_process
_dbus_atomic_dec
_dbus_atomic_inc
_dbus_auth_bytes_sent
_dbus_auth_client_new
_dbus_auth_decode_data
_dbus_auth_delete_unused_bytes
_dbus_auth_do_work
_dbus_auth_encode_data
_dbus_auth_get_buffer
_dbus_auth_get_bytes_to_send
_dbus_auth_get_guid_from_server
_dbus_auth_get_identity
_dbus_auth_get_unused_bytes
_dbus_auth_needs_decoding
_dbus_auth_needs_encoding
_dbus_auth_ref
_dbus_auth_return_buffer
_dbus_auth_script_run
_dbus_auth_server_new
_dbus_auth_set_context
_dbus_auth_set_credentials
_dbus_auth_set_mechanisms
_dbus_auth_test
_dbus_auth_unref
_dbus_babysitter_get_child_exited
_dbus_babysitter_get_child_exit_status
_dbus_babysitter_kill_child
_dbus_babysitter_ref
_dbus_babysitter_set_child_exit_error
_dbus_babysitter_set_watch_functions
_dbus_babysitter_unref
_dbus_become_daemon
_dbus_bus_notify_shared_connection_disconnected_unlocked
_dbus_change_identity
_dbus_change_to_daemon_user
_dbus_check_dir_is_private_to_user
_dbus_check_is_valid_bus_name
_dbus_check_is_valid_error_name
_dbus_check_is_valid_interface
_dbus_check_is_valid_member
_dbus_check_is_valid_path
_dbus_check_is_valid_signature
_dbus_close_socket
_dbus_concat_dir_and_file
_dbus_condvar_free
_dbus_condvar_free_at_location
_dbus_condvar_new
_dbus_condvar_new_at_location
_dbus_condvar_wait
_dbus_condvar_wait_timeout
_dbus_condvar_wake_all
_dbus_condvar_wake_one
_dbus_connect_tcp_socket
_dbus_connection_add_timeout_unlocked
_dbus_connection_add_watch_unlocked
_dbus_connection_block_pending_call
_dbus_connection_close_if_only_one_ref
_dbus_connection_close_possibly_shared
_dbus_connection_do_iteration_unlocked
_dbus_connection_get_message_to_send
_dbus_connection_handle_watch
_dbus_connection_has_messages_to_send_unlocked
_dbus_connection_lock
_dbus_connection_message_sent
_dbus_connection_new_for_transport
_dbus_connection_queue_received_message
_dbus_connection_queue_received_message_link
_dbus_connection_queue_synthesized_message_link
_dbus_connection_ref_unlocked
_dbus_connection_remove_pending_call
_dbus_connection_remove_timeout_unlocked
_dbus_connection_remove_watch_unlocked
_dbus_connection_send_and_unlock
_dbus_connection_test_get_locks
_dbus_connection_toggle_timeout_unlocked
_dbus_connection_toggle_watch_unlocked
_dbus_connection_unlock
_dbus_connection_unref_unlocked
_dbus_counter_adjust
_dbus_counter_get_value
_dbus_counter_new
_dbus_counter_ref
_dbus_counter_set_notify
_dbus_counter_unref
_dbus_create_directory
_dbus_create_file_exclusively
_dbus_credentials_add_credential
_dbus_credentials_add_credentials
_dbus_credentials_add_from_current_process
_dbus_credentials_add_from_user
_dbus_credentials_add_unix_pid
_dbus_credentials_add_unix_uid
_dbus_credentials_add_windows_sid
_dbus_credentials_are_anonymous
_dbus_credentials_are_empty
_dbus_credentials_are_superset
_dbus_credentials_clear
_dbus_credentials_copy
_dbus_credentials_get_unix_pid
_dbus_credentials_get_unix_uid
_dbus_credentials_get_windows_sid
_dbus_credentials_include
_dbus_credentials_new
_dbus_credentials_new_from_current_process
_dbus_credentials_ref
_dbus_credentials_same_user
_dbus_credentials_test
_dbus_credentials_unref
_dbus_current_generation DATA
_dbus_daemon_init
_dbus_daemon_release
_dbus_data_slot_allocator_alloc
_dbus_data_slot_allocator_free
_dbus_data_slot_allocator_init
_dbus_data_slot_list_clear
_dbus_data_slot_list_free
_dbus_data_slot_list_get
_dbus_data_slot_list_init
_dbus_data_slot_list_set
_dbus_data_slot_test
_dbus_decompose_path
_dbus_decrement_fail_alloc_counter
_dbus_delete_directory
_dbus_delete_file
_dbus_directory_close
_dbus_directory_get_next_file
_dbus_directory_open
_dbus_disable_mem_pools
_dbus_disable_sigpipe
_dbus_dup_string_array
_dbus_error_from_errno
_dbus_exit
_dbus_fd_set_close_on_exec
_dbus_file_close
_dbus_file_exists
_dbus_file_get_contents
_dbus_file_open
_dbus_file_read
_dbus_file_write
_dbus_first_type_in_signature
_dbus_first_type_in_signature_c_str
_dbus_flush_caches
_dbus_fstat
_dbus_full_duplex_pipe
_dbus_generate_pseudorandom_bytes_buffer
_dbus_generate_random_ascii
_dbus_generate_random_bytes
_dbus_generate_random_bytes_buffer
_dbus_generate_uuid
_dbus_get_autolaunch_address
_dbus_get_config_file_name
_dbus_get_current_time
_dbus_get_fail_alloc_counter
_dbus_get_fail_alloc_failures
_dbus_get_install_root
_dbus_get_is_errno_eagain_or_ewouldblock
_dbus_get_is_errno_eintr
_dbus_get_is_errno_enomem
_dbus_get_is_errno_nonzero
_dbus_get_local_machine_uuid_encoded
_dbus_get_malloc_blocks_outstanding
_dbus_get_oom_wait
_dbus_get_standard_session_servicedirs
_dbus_get_standard_system_servicedirs
_dbus_get_tmpdir
_dbus_getenv
_dbus_getgid
_dbus_getpid
_dbus_getsid
_dbus_getuid
_dbus_hash_iter_get_int_key
_dbus_hash_iter_get_string_key
_dbus_hash_iter_get_two_strings_key
_dbus_hash_iter_get_ulong_key
_dbus_hash_iter_get_value
_dbus_hash_iter_init
_dbus_hash_iter_lookup
_dbus_hash_iter_next
_dbus_hash_iter_remove_entry
_dbus_hash_iter_set_value
_dbus_hash_table_free_preallocated_entry
_dbus_hash_table_get_n_entries
_dbus_hash_table_insert_int
_dbus_hash_table_insert_pointer
_dbus_hash_table_insert_string
_dbus_hash_table_insert_string_preallocated
_dbus_hash_table_insert_two_strings
_dbus_hash_table_insert_ulong
_dbus_hash_table_lookup_int
_dbus_hash_table_lookup_pointer
_dbus_hash_table_lookup_string
_dbus_hash_table_lookup_two_strings
_dbus_hash_table_lookup_ulong
_dbus_hash_table_new
_dbus_hash_table_preallocate_entry
_dbus_hash_table_ref
_dbus_hash_table_remove_all
_dbus_hash_table_remove_int
_dbus_hash_table_remove_pointer
_dbus_hash_table_remove_string
_dbus_hash_table_remove_two_strings
_dbus_hash_table_remove_ulong
_dbus_hash_table_unref
_dbus_hash_test
_dbus_header_byteswap
_dbus_header_copy
_dbus_header_create
_dbus_header_delete_field
_dbus_header_field_to_string
_dbus_header_free
_dbus_header_get_field_basic
_dbus_header_get_field_raw
_dbus_header_get_flag
_dbus_header_get_message_type
_dbus_header_get_serial
_dbus_header_have_message_untrusted
_dbus_header_init
_dbus_header_load
_dbus_header_reinit
_dbus_header_set_field_basic
_dbus_header_set_serial
_dbus_header_toggle_flag
_dbus_header_update_lengths
_dbus_is_valid_file
_dbus_is_verbose_real
_dbus_keyring_get_best_key
_dbus_keyring_get_hex_key
_dbus_keyring_is_for_credentials
_dbus_keyring_new_for_credentials
_dbus_keyring_ref
_dbus_keyring_test
_dbus_keyring_unref
_dbus_keyring_validate_context
_dbus_list_alloc_link
_dbus_list_append
_dbus_list_append_link
_dbus_list_clear
_dbus_list_copy
_dbus_list_find_last
_dbus_list_foreach
_dbus_list_free_link
_dbus_list_get_first
_dbus_list_get_first_link
_dbus_list_get_last
_dbus_list_get_last_link
_dbus_list_get_length
_dbus_list_insert_after
_dbus_list_insert_after_link
_dbus_list_insert_before
_dbus_list_insert_before_link
_dbus_list_length_is_one
_dbus_list_pop_first
_dbus_list_pop_first_link
_dbus_list_pop_last
_dbus_list_pop_last_link
_dbus_list_prepend
_dbus_list_prepend_link
_dbus_list_remove
_dbus_list_remove_last
_dbus_list_remove_link
_dbus_list_test
_dbus_list_unlink
_dbus_listen_tcp_socket
_dbus_lm_strerror
_dbus_lock_atomic DATA
_dbus_lock_bus DATA
_dbus_lock_bus_datas DATA
_dbus_lock_connection_slots DATA
_dbus_lock_list DATA
_dbus_lock_machine_uuid DATA
_dbus_lock_message_cache DATA
_dbus_lock_message_slots DATA
_dbus_lock_pending_call_slots DATA
_dbus_lock_server_slots DATA
_dbus_lock_shared_connections DATA
_dbus_lock_shutdown_funcs DATA
_dbus_lock_sid_atom_cache DATA
_dbus_lock_system_users DATA
_dbus_lock_win_fds DATA
_dbus_loop_add_timeout
_dbus_loop_add_watch
_dbus_loop_dispatch
_dbus_loop_iterate
_dbus_loop_new
_dbus_loop_queue_dispatch
_dbus_loop_quit
_dbus_loop_ref
_dbus_loop_remove_timeout
_dbus_loop_remove_watch
_dbus_loop_run
_dbus_loop_unref
_dbus_make_file_world_readable
_dbus_marshal_byteswap
_dbus_marshal_byteswap_test
_dbus_marshal_header_test
_dbus_marshal_read_basic
_dbus_marshal_read_fixed_multi
_dbus_marshal_read_uint32
_dbus_marshal_recursive_test
_dbus_marshal_set_basic
_dbus_marshal_set_uint32
_dbus_marshal_skip_array
_dbus_marshal_skip_basic
_dbus_marshal_test
_dbus_marshal_validate_test
_dbus_marshal_write_basic
_dbus_marshal_write_fixed_multi
_dbus_mem_pool_alloc
_dbus_mem_pool_dealloc
_dbus_mem_pool_free
_dbus_mem_pool_new
_dbus_mem_pool_test
_dbus_memdup
_dbus_memory_test
_dbus_message_add_size_counter
_dbus_message_add_size_counter_link
_dbus_message_data_free
_dbus_message_data_iter_get_and_next
_dbus_message_data_iter_init
_dbus_message_get_network_data
_dbus_message_iter_get_args_valist
_dbus_message_loader_get_buffer
_dbus_message_loader_get_is_corrupted
_dbus_message_loader_get_max_message_size
_dbus_message_loader_new
_dbus_message_loader_peek_message
_dbus_message_loader_pop_message
_dbus_message_loader_pop_message_link
_dbus_message_loader_putback_message_link
_dbus_message_loader_queue_messages
_dbus_message_loader_ref
_dbus_message_loader_return_buffer
_dbus_message_loader_set_max_message_size
_dbus_message_loader_unref
_dbus_message_lock
_dbus_message_remove_size_counter
_dbus_message_set_serial
_dbus_message_test
_dbus_misc_test
_dbus_mkdir
_dbus_mutex_free
_dbus_mutex_free_at_location
_dbus_mutex_lock
_dbus_mutex_new
_dbus_mutex_new_at_location
_dbus_mutex_unlock
_dbus_no_memory_message
_dbus_object_tree_dispatch_and_unlock
_dbus_object_tree_free_all_unlocked
_dbus_object_tree_get_user_data_unlocked
_dbus_object_tree_list_registered_and_unlock
_dbus_object_tree_new
_dbus_object_tree_ref
_dbus_object_tree_register
_dbus_object_tree_test
_dbus_object_tree_unref
_dbus_object_tree_unregister_and_unlock
_dbus_pack_uint32
_dbus_parse_unix_group_from_config
_dbus_parse_unix_user_from_config
_dbus_path_is_absolute
_dbus_pending_call_complete
_dbus_pending_call_get_completed_unlocked
_dbus_pending_call_get_connection_and_lock
_dbus_pending_call_get_connection_unlocked
_dbus_pending_call_get_reply_serial_unlocked
_dbus_pending_call_get_timeout_unlocked
_dbus_pending_call_is_timeout_added_unlocked
_dbus_pending_call_new_unlocked
_dbus_pending_call_queue_timeout_error_unlocked
_dbus_pending_call_ref_unlocked
_dbus_pending_call_set_data_unlocked
_dbus_pending_call_set_reply_serial_unlocked
_dbus_pending_call_set_reply_unlocked
_dbus_pending_call_set_timeout_added_unlocked
_dbus_pending_call_set_timeout_error_unlocked
_dbus_pending_call_test
_dbus_pending_call_unref_and_unlock
_dbus_pid_for_log
_dbus_pipe_close
_dbus_pipe_init
_dbus_pipe_init_stdout
_dbus_pipe_invalidate
_dbus_pipe_is_stdout_or_stderr
_dbus_pipe_is_valid
_dbus_pipe_write
_dbus_poll
_dbus_print_backtrace
_dbus_printf_string_upper_bound
_dbus_read_credentials_socket
_dbus_read_local_machine_uuid
_dbus_read_socket
_dbus_read_uuid_file
_dbus_real_assert
_dbus_real_assert_not_reached
_dbus_register_shutdown_func
_dbus_return_if_fail_warning_format DATA
_dbus_send_credentials_socket
_dbus_server_add_timeout
_dbus_server_add_watch
_dbus_server_debug_pipe_new
_dbus_server_finalize_base
_dbus_server_init_base
_dbus_server_listen_debug_pipe
_dbus_server_listen_platform_specific
_dbus_server_listen_socket
_dbus_server_new_for_socket
_dbus_server_new_for_tcp_socket
_dbus_server_ref_unlocked
_dbus_server_remove_timeout
_dbus_server_remove_watch
_dbus_server_socket_own_filename
_dbus_server_test
_dbus_server_toggle_timeout
_dbus_server_toggle_watch
_dbus_server_unref_unlocked
_dbus_set_bad_address
_dbus_set_errno_to_zero
_dbus_set_fail_alloc_counter
_dbus_set_fail_alloc_failures
_dbus_set_fd_nonblocking
_dbus_set_signal_handler
_dbus_setenv
_dbus_sha_compute
_dbus_sha_final
_dbus_sha_init
_dbus_sha_test
_dbus_sha_update
_dbus_shell_parse_argv
_dbus_shell_unquote
_dbus_signature_test
_dbus_sleep_milliseconds
_dbus_spawn_async_with_babysitter
_dbus_spawn_test
_dbus_split_paths_and_append
_dbus_stat
_dbus_strdup
_dbus_strerror
_dbus_strerror_from_errno
_dbus_string_align_length
_dbus_string_alloc_space
_dbus_string_append
_dbus_string_append_4_aligned
_dbus_string_append_8_aligned
_dbus_string_append_byte
_dbus_string_append_byte_as_hex
_dbus_string_append_double
_dbus_string_append_int
_dbus_string_append_len
_dbus_string_append_printf
_dbus_string_append_printf_valist
_dbus_string_append_uint
_dbus_string_append_unichar
_dbus_string_array_contains
_dbus_string_chop_white
_dbus_string_copy
_dbus_string_copy_data
_dbus_string_copy_data_len
_dbus_string_copy_len
_dbus_string_copy_to_buffer
_dbus_string_delete
_dbus_string_delete_first_word
_dbus_string_delete_leading_blanks
_dbus_string_ends_with_c_str
_dbus_string_equal
_dbus_string_equal_c_str
_dbus_string_equal_len
_dbus_string_equal_substring
_dbus_string_find
_dbus_string_find_blank
_dbus_string_find_byte_backward
_dbus_string_find_eol
_dbus_string_find_to
_dbus_string_free
_dbus_string_get_byte
_dbus_string_get_const_data
_dbus_string_get_const_data_len
_dbus_string_get_data
_dbus_string_get_data_len
_dbus_string_get_dirname
_dbus_string_get_length
_dbus_string_get_unichar
_dbus_string_hex_decode
_dbus_string_hex_encode
_dbus_string_init
_dbus_string_init_const
_dbus_string_init_const_len
_dbus_string_init_preallocated
_dbus_string_insert_2_aligned
_dbus_string_insert_4_aligned
_dbus_string_insert_8_aligned
_dbus_string_insert_alignment
_dbus_string_insert_byte
_dbus_string_insert_bytes
_dbus_string_lengthen
_dbus_string_lock
_dbus_string_move
_dbus_string_move_len
_dbus_string_parse_double
_dbus_string_parse_int
_dbus_string_parse_uint
_dbus_string_pop_line
_dbus_string_replace_len
_dbus_string_save_to_file
_dbus_string_set_byte
_dbus_string_set_length
_dbus_string_shorten
_dbus_string_skip_blank
_dbus_string_skip_white
_dbus_string_skip_white_reverse
_dbus_string_starts_with_c_str
_dbus_string_steal_data
_dbus_string_steal_data_len
_dbus_string_test
_dbus_string_validate_ascii
_dbus_string_validate_nul
_dbus_string_validate_utf8
_dbus_string_zero
_dbus_swap_array
_dbus_sysdeps_test
_dbus_test_oom_handling
_dbus_threads_init_debug
_dbus_threads_init_platform_specific
_dbus_timeout_list_add_timeout
_dbus_timeout_list_free
_dbus_timeout_list_new
_dbus_timeout_list_remove_timeout
_dbus_timeout_list_set_functions
_dbus_timeout_list_toggle_timeout
_dbus_timeout_new
_dbus_timeout_ref
_dbus_timeout_set_enabled
_dbus_timeout_set_interval
_dbus_timeout_unref
_dbus_transport_debug_pipe_new
_dbus_transport_disconnect
_dbus_transport_do_iteration
_dbus_transport_finalize_base
_dbus_transport_get_address
_dbus_transport_get_dispatch_status
_dbus_transport_get_is_anonymous
_dbus_transport_get_is_authenticated
_dbus_transport_get_is_connected
_dbus_transport_get_max_message_size
_dbus_transport_get_max_received_size
_dbus_transport_get_server_id
_dbus_transport_get_socket_fd
_dbus_transport_get_unix_process_id
_dbus_transport_get_unix_user
_dbus_transport_get_windows_user
_dbus_transport_handle_watch
_dbus_transport_init_base
_dbus_transport_new_for_socket
_dbus_transport_new_for_tcp_socket
_dbus_transport_open
_dbus_transport_open_debug_pipe
_dbus_transport_open_platform_specific
_dbus_transport_open_socket
_dbus_transport_queue_messages
_dbus_transport_ref
_dbus_transport_set_allow_anonymous
_dbus_transport_set_auth_mechanisms
_dbus_transport_set_connection
_dbus_transport_set_max_message_size
_dbus_transport_set_max_received_size
_dbus_transport_set_unix_user_function
_dbus_transport_set_windows_user_function
_dbus_transport_unref
_dbus_type_get_alignment
_dbus_type_is_valid
_dbus_type_reader_delete
_dbus_type_reader_equal_values
_dbus_type_reader_get_array_length
_dbus_type_reader_get_current_type
_dbus_type_reader_get_element_type
_dbus_type_reader_get_signature
_dbus_type_reader_get_value_pos
_dbus_type_reader_greater_than
_dbus_type_reader_has_next
_dbus_type_reader_init
_dbus_type_reader_init_types_only
_dbus_type_reader_next
_dbus_type_reader_read_basic
_dbus_type_reader_read_fixed_multi
_dbus_type_reader_read_raw
_dbus_type_reader_recurse
_dbus_type_reader_set_basic
_dbus_type_signature_next
_dbus_type_to_string
_dbus_type_writer_add_types
_dbus_type_writer_append_array
_dbus_type_writer_init
_dbus_type_writer_init_types_delayed
_dbus_type_writer_init_values_only
_dbus_type_writer_recurse
_dbus_type_writer_remove_types
_dbus_type_writer_set_enabled
_dbus_type_writer_unrecurse
_dbus_type_writer_write_basic
_dbus_type_writer_write_fixed_multi
_dbus_type_writer_write_reader
_dbus_type_writer_write_reader_partial
_dbus_unix_groups_from_uid
_dbus_unix_user_is_at_console
_dbus_unix_user_is_process_owner
_dbus_unpack_uint16
_dbus_unpack_uint32
_dbus_user_at_console
_dbus_uuid_encode
_dbus_validate_body_with_reason
_dbus_validate_bus_name
_dbus_validate_error_name
_dbus_validate_interface
_dbus_validate_member
_dbus_validate_path
_dbus_validate_signature
_dbus_validate_signature_with_reason
_dbus_verbose_bytes
_dbus_verbose_bytes_of_string
_dbus_verbose_real
_dbus_verbose_reset_real
_dbus_verify_daemon_user
_dbus_wait_for_memory
_dbus_warn
_dbus_warn_check_failed
_dbus_watch_invalidate
_dbus_watch_list_add_watch
_dbus_watch_list_free
_dbus_watch_list_new
_dbus_watch_list_remove_watch
_dbus_watch_list_set_functions
_dbus_watch_list_toggle_watch
_dbus_watch_new
_dbus_watch_ref
_dbus_watch_sanitize_condition
_dbus_watch_set_handler
_dbus_watch_unref
_dbus_win_account_to_sid
_dbus_win_set_error_from_win_error
_dbus_win_startup_winsock
_dbus_win_utf16_to_utf8
_dbus_win_utf8_to_utf16
_dbus_win_warn_win_error
_dbus_windows_user_is_process_owner
_dbus_write_pid_to_file_and_pipe
_dbus_write_socket
_dbus_write_socket_two
dbus_address_entries_free
dbus_address_entry_get_method
dbus_address_entry_get_value
dbus_address_escape_value
dbus_address_unescape_value
dbus_bus_add_match
dbus_bus_get
dbus_bus_get_id
dbus_bus_get_private
dbus_bus_get_unique_name
dbus_bus_get_unix_user
dbus_bus_name_has_owner
dbus_bus_register
dbus_bus_release_name
dbus_bus_remove_match
dbus_bus_request_name
dbus_bus_set_unique_name
dbus_bus_start_service_by_name
dbus_connection_add_filter
dbus_connection_allocate_data_slot
dbus_connection_borrow_message
dbus_connection_close
dbus_connection_dispatch
dbus_connection_flush
dbus_connection_free_data_slot
dbus_connection_free_preallocated_send
dbus_connection_get_data
dbus_connection_get_dispatch_status
dbus_connection_get_is_anonymous
dbus_connection_get_is_authenticated
dbus_connection_get_is_connected
dbus_connection_get_max_message_size
dbus_connection_get_max_received_size
dbus_connection_get_object_path_data
dbus_connection_get_outgoing_size
dbus_connection_get_server_id
dbus_connection_get_socket
dbus_connection_get_unix_fd
dbus_connection_get_unix_process_id
dbus_connection_get_unix_user
dbus_connection_get_windows_user
dbus_connection_has_messages_to_send
dbus_connection_list_registered
dbus_connection_open
dbus_connection_open_private
dbus_connection_pop_message
dbus_connection_preallocate_send
dbus_connection_read_write
dbus_connection_read_write_dispatch
dbus_connection_ref
dbus_connection_register_fallback
dbus_connection_register_object_path
dbus_connection_remove_filter
dbus_connection_return_message
dbus_connection_send
dbus_connection_send_preallocated
dbus_connection_send_with_reply
dbus_connection_send_with_reply_and_block
dbus_connection_set_allow_anonymous
dbus_connection_set_change_sigpipe
dbus_connection_set_data
dbus_connection_set_dispatch_status_function
dbus_connection_set_exit_on_disconnect
dbus_connection_set_max_message_size
dbus_connection_set_max_received_size
dbus_connection_set_route_peer_messages
dbus_connection_set_timeout_functions
dbus_connection_set_unix_user_function
dbus_connection_set_wakeup_main_function
dbus_connection_set_watch_functions
dbus_connection_set_windows_user_function
dbus_connection_steal_borrowed_message
dbus_connection_unref
dbus_connection_unregister_object_path
dbus_error_free
dbus_error_has_name
dbus_error_init
dbus_error_is_set
dbus_free
dbus_free_string_array
dbus_get_local_machine_id
dbus_internal_do_not_use_foreach_message_file
dbus_internal_do_not_use_generate_bodies
dbus_internal_do_not_use_load_message_file
dbus_internal_do_not_use_run_tests
dbus_internal_do_not_use_try_message_data
dbus_internal_do_not_use_try_message_file
dbus_malloc
dbus_malloc0
dbus_message_allocate_data_slot
dbus_message_append_args
dbus_message_append_args_valist
dbus_message_copy
dbus_message_demarshal
dbus_message_free_data_slot
dbus_message_get_args
dbus_message_get_args_valist
dbus_message_get_auto_start
dbus_message_get_data
dbus_message_get_destination
dbus_message_get_error_name
dbus_message_get_interface
dbus_message_get_member
dbus_message_get_no_reply
dbus_message_get_path
dbus_message_get_path_decomposed
dbus_message_get_reply_serial
dbus_message_get_sender
dbus_message_get_serial
dbus_message_get_signature
dbus_message_get_type
dbus_message_has_destination
dbus_message_has_interface
dbus_message_has_member
dbus_message_has_path
dbus_message_has_sender
dbus_message_has_signature
dbus_message_is_error
dbus_message_is_method_call
dbus_message_is_signal
dbus_message_iter_append_basic
dbus_message_iter_append_fixed_array
dbus_message_iter_close_container
dbus_message_iter_get_arg_type
dbus_message_iter_get_array_len
dbus_message_iter_get_basic
dbus_message_iter_get_element_type
dbus_message_iter_get_fixed_array
dbus_message_iter_get_signature
dbus_message_iter_has_next
dbus_message_iter_init
dbus_message_iter_init_append
dbus_message_iter_next
dbus_message_iter_open_container
dbus_message_iter_recurse
dbus_message_marshal
dbus_message_new
dbus_message_new_error
dbus_message_new_error_printf
dbus_message_new_method_call
dbus_message_new_method_return
dbus_message_new_signal
dbus_message_ref
dbus_message_set_auto_start
dbus_message_set_data
dbus_message_set_destination
dbus_message_set_error_name
dbus_message_set_interface
dbus_message_set_member
dbus_message_set_no_reply
dbus_message_set_path
dbus_message_set_reply_serial
dbus_message_set_sender
dbus_message_type_from_string
dbus_message_type_to_string
dbus_message_unref
dbus_move_error
dbus_parse_address
dbus_pending_call_allocate_data_slot
dbus_pending_call_block
dbus_pending_call_cancel
dbus_pending_call_free_data_slot
dbus_pending_call_get_completed
dbus_pending_call_get_data
dbus_pending_call_ref
dbus_pending_call_set_data
dbus_pending_call_set_notify
dbus_pending_call_steal_reply
dbus_pending_call_unref
dbus_realloc
dbus_server_allocate_data_slot
dbus_server_disconnect
dbus_server_free_data_slot
dbus_server_get_address
dbus_server_get_data
dbus_server_get_id
dbus_server_get_is_connected
dbus_server_listen
dbus_server_ref
dbus_server_set_auth_mechanisms
dbus_server_set_data
dbus_server_set_new_connection_function
dbus_server_set_timeout_functions
dbus_server_set_watch_functions
dbus_server_unref
dbus_set_error
dbus_set_error_const
dbus_set_error_from_message
dbus_shutdown
dbus_signature_iter_get_current_type
dbus_signature_iter_get_element_type
dbus_signature_iter_get_signature
dbus_signature_iter_init
dbus_signature_iter_next
dbus_signature_iter_recurse
dbus_signature_validate
dbus_signature_validate_single
dbus_threads_init
dbus_threads_init_default
dbus_timeout_get_data
dbus_timeout_get_enabled
dbus_timeout_get_interval
dbus_timeout_handle
dbus_timeout_set_data
dbus_type_is_basic
dbus_type_is_container
dbus_type_is_fixed
dbus_watch_get_data
dbus_watch_get_enabled
dbus_watch_get_fd
dbus_watch_get_flags
dbus_watch_get_socket
dbus_watch_get_unix_fd
dbus_watch_handle
dbus_watch_set_data
dbus_kde_patch
