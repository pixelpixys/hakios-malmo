#ifndef _LX_LINUX_ACPI_H_
#define _LX_LINUX_ACPI_H_
#include <linux/types.h>
#define ACPI_TYPE_INTEGER	1
#define ACPI_TYPE_PACKAGE	4
#define ACPI_ALLOCATE_BUFFER	((size_t)-1)
#define AE_OK			0
#define AE_NOT_FOUND		5
#define ACPI_FAILURE(s)		((s) != AE_OK)
typedef u32 acpi_status;
typedef char *acpi_string;
typedef void *acpi_handle;
struct acpi_buffer {
	size_t length;
	void *pointer;
};
union acpi_object {
	u32 type;
	struct {
		u32 type;
		u64 value;
	} integer;
	struct {
		u32 type;
		u32 count;
		union acpi_object *elements;
	} package;
	struct {
		u32 type;
		u32 length;
		u8 *pointer;
	} buffer;
};
#define ACPI_HANDLE(dev) NULL
static inline acpi_status acpi_get_handle(acpi_handle parent, acpi_string pathname, acpi_handle *ret_handle) { return AE_NOT_FOUND; }
static inline acpi_status acpi_evaluate_object(acpi_handle obj, acpi_string pathname, void *ext_params, struct acpi_buffer *ret_buf) { return AE_NOT_FOUND; }
static inline union acpi_object *acpi_evaluate_dsm(acpi_handle handle, const void *guid, u64 rev, u64 func, union acpi_object *argv4) { return NULL; }
static inline void acpi_free(void *ptr) {}
#endif
