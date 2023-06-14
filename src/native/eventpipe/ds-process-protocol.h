#ifndef __DIAGNOSTICS_PROCESS_PROTOCOL_H__
#define __DIAGNOSTICS_PROCESS_PROTOCOL_H__

#include "ds-rt-config.h"

#ifdef ENABLE_PERFTRACING
#include "ds-types.h"
#include "ds-ipc.h"

#undef DS_IMPL_GETTER_SETTER
#ifdef DS_IMPL_PROCESS_PROTOCOL_GETTER_SETTER
#define DS_IMPL_GETTER_SETTER
#endif
#include "ds-getter-setter.h"

/*
* DiagnosticsProcessInfoPayload
*/

#if defined(DS_INLINE_GETTER_SETTER) || defined(DS_IMPL_PROCESS_PROTOCOL_GETTER_SETTER)
struct _DiagnosticsProcessInfoPayload {
#else
struct _DiagnosticsProcessInfoPayload_Internal {
#endif
	uint64_t process_id;
	const ep_char16_t *command_line;
	const ep_char16_t *os;
	const ep_char16_t *arch;
	uint8_t runtime_cookie [EP_GUID_SIZE];
};

#if !defined(DS_INLINE_GETTER_SETTER) && !defined(DS_IMPL_PROCESS_PROTOCOL_GETTER_SETTER)
struct _DiagnosticsProcessInfoPayload {
	uint8_t _internal [sizeof (struct _DiagnosticsProcessInfoPayload_Internal)];
};
#endif

DiagnosticsProcessInfoPayload *
ds_process_info_payload_init (
	DiagnosticsProcessInfoPayload *payload,
	const ep_char16_t *command_line,
	const ep_char16_t *os,
	const ep_char16_t *arch,
	uint32_t process_id,
	const uint8_t *runtime_cookie);

/*
* DiagnosticsProcessInfo2Payload
*/

#if defined(DS_INLINE_GETTER_SETTER) || defined(DS_IMPL_PROCESS_PROTOCOL_GETTER_SETTER)
struct _DiagnosticsProcessInfo2Payload {
#else
struct _DiagnosticsProcessInfo2Payload_Internal {
#endif
	uint64_t process_id;
	const ep_char16_t *command_line;
	const ep_char16_t *os;
	const ep_char16_t *arch;
	uint8_t runtime_cookie [EP_GUID_SIZE];
	const ep_char16_t *managed_entrypoint_assembly_name;
	const ep_char16_t *clr_product_version;

};

#if !defined(DS_INLINE_GETTER_SETTER) && !defined(DS_IMPL_PROCESS_PROTOCOL_GETTER_SETTER)
struct _DiagnosticsProcessInfo2Payload {
	uint8_t _internal [sizeof (struct _DiagnosticsProcessInfo2Payload_Internal)];
};
#endif

DiagnosticsProcessInfo2Payload *
ds_process_info_2_payload_init (
	DiagnosticsProcessInfo2Payload *payload,
	const ep_char16_t *command_line,
	const ep_char16_t *os,
	const ep_char16_t *arch,
	uint32_t process_id,
	const uint8_t *runtime_cookie,
	const ep_char16_t *managed_entrypoint_assembly_name,
	const ep_char16_t *clr_product_version);

/*
* DiagnosticsProcessInfo3Payload
*/

#if defined(DS_INLINE_GETTER_SETTER) || defined(DS_IMPL_PROCESS_PROTOCOL_GETTER_SETTER)
struct _DiagnosticsProcessInfo3Payload {
#else
struct _DiagnosticsProcessInfo3Payload_Internal {
#endif
	uint64_t process_id;
	const ep_char16_t *command_line;
	const ep_char16_t *os;
	const ep_char16_t *arch;
	uint8_t runtime_cookie [EP_GUID_SIZE];
	const ep_char16_t *managed_entrypoint_assembly_name;
	const ep_char16_t *clr_product_version;
	const ep_char16_t *runtime_rid;

};

#if !defined(DS_INLINE_GETTER_SETTER) && !defined(DS_IMPL_PROCESS_PROTOCOL_GETTER_SETTER)
struct _DiagnosticsProcessInfo3Payload {
	uint8_t _internal [sizeof (struct _DiagnosticsProcessInfo3Payload_Internal)];
};
#endif

DiagnosticsProcessInfo3Payload *
ds_process_info_3_payload_init (
	DiagnosticsProcessInfo3Payload *payload,
	const ep_char16_t *command_line,
	const ep_char16_t *os,
	const ep_char16_t *arch,
	uint32_t process_id,
	const uint8_t *runtime_cookie,
	const ep_char16_t *managed_entrypoint_assembly_name,
	const ep_char16_t *clr_product_version,
	const ep_char16_t *runtime_rid);

/*
* DiagnosticsEnvironmentInfoPayload
*/

#if defined(DS_INLINE_GETTER_SETTER) || defined(DS_IMPL_PROCESS_PROTOCOL_GETTER_SETTER)
struct _DiagnosticsEnvironmentInfoPayload {
#else
struct _DiagnosticsEnvironmentInfoPayload_Internal {
#endif
	// The environment is sent back as an optional continuation stream of data.
	// It is encoded in the typical length-prefixed array format as defined in
	// the Diagnostics IPC Spec: https://github.com/dotnet/diagnostics/blob/main/documentation/design-docs/ipc-protocol.md
	uint32_t incoming_bytes;
	uint16_t future;
	dn_vector_ptr_t *env_array;
};

#if !defined(DS_INLINE_GETTER_SETTER) && !defined(DS_IMPL_PROCESS_PROTOCOL_GETTER_SETTER)
struct _DiagnosticsEnvironmentInfoPayload {
	uint8_t _internal [sizeof (struct _DiagnosticsEnvironmentInfoPayload_Internal)];
};
#endif

DiagnosticsEnvironmentInfoPayload *
ds_env_info_payload_init (DiagnosticsEnvironmentInfoPayload *payload);

void
ds_env_info_payload_fini (DiagnosticsEnvironmentInfoPayload *payload);

/*
* DiagnosticsSetEnvironmentVariablePayload
*/

#if defined(DS_INLINE_GETTER_SETTER) || defined(DS_IMPL_PROCESS_PROTOCOL_GETTER_SETTER)
struct _DiagnosticsSetEnvironmentVariablePayload {
#else
struct _DiagnosticsSetEnvironmentVariablePayload_Internal {
#endif
	uint8_t * incoming_buffer;

	const ep_char16_t *name;
	const ep_char16_t *value;
};

#if !defined(DS_INLINE_GETTER_SETTER) && !defined(DS_IMPL_PROCESS_PROTOCOL_GETTER_SETTER)
struct _DiagnosticsSetEnvironmentVariablePayload {
	uint8_t _internal [sizeof (struct _DiagnosticsSetEnvironmentVariablePayload_Internal)];
};
#endif

DiagnosticsSetEnvironmentVariablePayload *
ds_set_environment_variable_payload_alloc (void);

void
ds_set_environment_variable_payload_free (DiagnosticsSetEnvironmentVariablePayload *payload);

/*
* DiagnosticsEnablePerfmapPayload
*/

#if defined(DS_INLINE_GETTER_SETTER) || defined(DS_IMPL_PROCESS_PROTOCOL_GETTER_SETTER)
struct _DiagnosticsEnablePerfmapPayload {
#else
struct _DiagnosticsEnablePerfmapPayload_Internal {
#endif
	uint8_t * incoming_buffer;

	uint32_t perfMapType;
};

#if !defined(DS_INLINE_GETTER_SETTER) && !defined(DS_IMPL_PROCESS_PROTOCOL_GETTER_SETTER)
struct _DiagnosticsEnablePerfmapPayload {
	uint8_t _internal [sizeof (struct _DiagnosticsEnablePerfmapPayload_Internal)];
};
#endif

DiagnosticsEnablePerfmapPayload *
ds_enable_perfmap_payload_alloc (void);

void
ds_enable_perfmap_payload_free (DiagnosticsEnablePerfmapPayload *payload);

/*
* DiagnosticsApplyStartupHookPayload
*/

#if defined(DS_INLINE_GETTER_SETTER) || defined(DS_IMPL_PROCESS_PROTOCOL_GETTER_SETTER)
struct _DiagnosticsApplyStartupHookPayload {
#else
struct _DiagnosticsApplyStartupHookPayload_Internal {
#endif
	uint8_t * incoming_buffer;

	const ep_char16_t *startup_hook_path;
};

#if !defined(DS_INLINE_GETTER_SETTER) && !defined(DS_IMPL_PROCESS_PROTOCOL_GETTER_SETTER)
struct _DiagnosticsApplyStartupHookPayload {
	uint8_t _internal [sizeof (struct _DiagnosticsApplyStartupHookPayload_Internal)];
};
#endif

DiagnosticsApplyStartupHookPayload *
ds_apply_startup_hook_payload_alloc (void);

void
ds_apply_startup_hook_payload_free (DiagnosticsApplyStartupHookPayload *payload);

/*
 * DiagnosticsProcessProtocolHelper.
 */

bool
ds_process_protocol_helper_handle_ipc_message (
	DiagnosticsIpcMessage *message,
	DiagnosticsIpcStream *stream);

#endif /* ENABLE_PERFTRACING */
#endif /* __DIAGNOSTICS_PROCESS_PROTOCOL_H__ */
