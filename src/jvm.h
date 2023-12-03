#ifndef JVM_H
#define JVM_H

#include "jni.h"
#include <stdint.h>

/* NOTE: Definitions for OpenJDK 17 (HotSpot JVM) */

typedef unsigned char  u1;
typedef unsigned short u2;
typedef unsigned int   u4;
typedef unsigned long  u8;
typedef void          *address;

struct ConstMethod;
typedef void MethodData;
typedef void MethodCounters;

struct AdapterHandlerEntry {
	void *_fingerprint;
	address _i2c_entry;
	address _c2i_entry;
	address _c2i_unverified_entry;
	address _c2i_no_clinit_check_entry;
};

typedef void CompiledMethod;
typedef int  AccessFlags;

struct Method {
	void **vtable;
	ConstMethod *_constMethod;
	MethodData *_method_data;
	MethodCounters *_method_counters;
	AdapterHandlerEntry *_adapter;
	AccessFlags _access_flags;
	int _vtable_index;
	u2 _intrinsic_id;
	u2 _flags;

	address _i2i_entry;
	address _from_compiled_entry;
	CompiledMethod *_code;
	address _from_interpreted_entry;
};

typedef int BasicType;
typedef void JavaThread;

typedef enum { NORMAL, OVERPASS } MethodType;
typedef void ConstantPool;
template <typename T>
class Array;

struct ConstMethod {
	uint64_t _fingerprint;
	ConstantPool *_constants;
	Array<u1> *_stackmap_data;
	
	int _constMethod_size;
	u2 _flags;
	u1 _result_type;
	
	u2 _code_size;
	u2 _name_index;
	u2 _signature_index;
	u2 _method_idnum;
	
	u2 _max_stack;
	u2 _max_locals;
	u2 _size_of_parameters;
	u2 _orig_method_idnum;
};

#endif
