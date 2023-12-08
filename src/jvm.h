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

struct CompiledMethod;
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

typedef int MarkForDeoptimizationStatus;
typedef void ExceptionCache;

typedef void PcDesc;
typedef PcDesc *PcDescPtr;

struct PcDescCache {
	enum { cache_size = 4 };
	PcDescPtr _pc_descs[cache_size];
};

struct PcDescContainer {
	PcDescCache _pc_desc_cache;
};

typedef int CompilerType; // TODO: Check this
typedef void ImmutableOopMapSet;

struct CodeBlob {
	CompilerType _type;
	int _size;
	int _header_size;
	int _frame_complete_offset;

	int _data_offset;
	int _frame_size;

	address _code_begin;
	address _code_end;
	address _content_begin;

	address _data_end;
	address _relocation_begin;
	address _relocation_end;

	ImmutableOopMapSet *__oop_maps;
	bool _caller_must_gc_arguments;

	const char *_name;
	// int _ctable_offset;
	
	virtual void flush();
	virtual bool is_buffer_blob();
	virtual bool is_nmethod();
	virtual bool is_runtime_stub();
  virtual bool is_deoptimization_stub() const         { return false; }
  virtual bool is_uncommon_trap_stub() const          { return false; }
  virtual bool is_exception_stub() const              { return false; }
  virtual bool is_safepoint_stub() const              { return false; }
  virtual bool is_adapter_blob() const                { return false; }
  virtual bool is_vtable_blob() const                 { return false; }
  virtual bool is_method_handles_adapter_blob() const { return false; }
  virtual bool is_compiled() const                    { return false; }
  virtual bool is_optimized_entry_blob() const                  { return false; }

  virtual bool is_zombie() const                 { return false; }
  virtual bool is_locked_by_vm() const           { return false; }

  virtual bool is_unloaded() const               { return false; }
  virtual bool is_not_entrant() const            { return false; }

  // GC support
  virtual bool is_alive() const                  = 0;


  virtual void preserve_callee_argument_oops(/* args */) = 0;

	typedef void outputStream;
  virtual void verify() = 0;
  virtual void print() const;
  virtual void print_on(outputStream* st) const;
  virtual void print_value_on(outputStream* st) const;

  virtual void print_block_comment(outputStream* stream, address block_begin) const;
};

struct CompiledMethod : CodeBlob {
	MarkForDeoptimizationStatus _mark_for_deoptimization_status;
	
	unsigned int _has_unsafe_access;
	unsigned int _has_method_handle_invokes;
	unsigned int _has_wide_vector;
	
	Method *_method;
	address _scopes_data_begin;

	address _deopt_handler_begin;

	address _deopt_mh_handler_begin;

	PcDescContainer _pc_desc_container;
	ExceptionCache *_exception_cache;

	void *_gc_data;

	// define vtable
	virtual void flush();
	
	virtual bool is_compiled();

	enum { not_installed = -1, // in construction, only the owner doing the construction is
		                     // allowed to advance state
		 in_use        = 0,  // executable nmethod
		 not_used      = 1,  // not entrant, but revivable
		 not_entrant   = 2,  // marked for deoptimization but activations may still exist,
		                     // will be transformed to zombie when all activations are gone
		 unloaded      = 3,  // there should be no activations, should not be called, will be
		                     // transformed to zombie by the sweeper, when not "locked in vm".
		 zombie        = 4   // no activations exist, nmethod is ready for purge
	};

	virtual bool is_in_use();
	virtual int comp_level();
	virtual int compile_id();

	virtual address verified_entry_point() const = 0;
  virtual void log_identity(void* log) const = 0;
  virtual void log_state_change() const = 0;
  virtual bool make_not_used() = 0;
  virtual bool make_not_entrant() = 0;
  virtual bool make_entrant() = 0;
  virtual address entry_point() const = 0;
  virtual bool make_zombie() = 0;
  virtual bool is_osr_method() const = 0;
  virtual int osr_entry_bci() const = 0;

	/* ... */
};

typedef void oops_do_mark_link;

struct nmethod : CompiledMethod {
	int _entry_bci;
	nmethod *_osr_link;

	oops_do_mark_link* _oops_do_mark_link;
	
	address _entry_point;
	address _verified_entry_point;
	address _osr_entry_point;
	
	int _exception_offset;
	int _unwind_handler_offset;
	
	int _consts_offset;
	int _stub_offset;
	int _oops_offset;
	int _metadata_offset;
	int _scopes_data_offset;
	int _scopes_pcs_offset;
	int _dependencies_offset;
	int _native_invokers_offset;
	int _handler_table_offset;
	int _nul_chk_table_offset;

	int _speculations_offset;
	int _jvmci_data_offset;
	
	int _nmethod_end_offset;
	
	int _orig_pc_offset;
	
	int _compile_id;
	int _comp_level;
	
	bool _has_flushed_dependencies;
	bool _unload_reported;
	bool _load_reported;
	signed char _state;
	// ...
};

#endif
