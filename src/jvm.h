#ifndef JVM_H
#define JVM_H

#include "jni.h"

/* NOTE: Definitions for OpenJDK 17 (HotSpot JVM) */

typedef unsigned char  u1;
typedef unsigned short u2;
typedef unsigned int   u4;
typedef unsigned long  u8;
typedef void          *address;

typedef void ConstMethod;
typedef void MethodData;
typedef void MethodCounters;
typedef void AdapterHandlerEntry;
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

#endif
