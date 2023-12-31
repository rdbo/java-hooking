#include "jvm.h"
#include <libmem/libmem.hpp>
#include <iostream>

address orig_i2c_entry = NULL;
address orig_c2i_entry = NULL;
lm_address_t hkInterpStub = LM_ADDRESS_BAD;
lm_address_t hkCompStub = LM_ADDRESS_BAD;

jstring custom_string = NULL;

address get_method_arg(Method *method, void *senderSP, size_t argno)
{
	size_t nparams = method->_constMethod->_size_of_parameters;
	void **args = (void **)senderSP;
	
	if (argno >= nparams)
		return NULL;

	return (address)(&args[nparams - 1 - argno]);
}

void hkHookMe(JavaVM *jvm, Method *method, void *senderSP)
{
	JNIEnv *jni;
	
	std::cout << "[*] hkHookMe called!" << std::endl;
	std::cout << "[*] hkInterpStub: " << (void *)hkInterpStub << std::endl;
	std::cout << "[*] JavaVM: " << jvm << std::endl;

	jvm->GetEnv((void **)&jni, JNI_VERSION_1_6);
	std::cout << "[*] JNIEnv: " << jni << std::endl;
		
	std::cout << "[*] Stub info: " << std::endl;
	std::cout << "      method: " << method << std::endl;
	std::cout << "      senderSP: " << senderSP << std::endl;

	jmethodID mID = (jmethodID)&method;

	std::cout << "[*] Method (stub): " << std::endl;
	std::cout << "      _constMethod: " << method->_constMethod << std::endl;
	std::cout << "          _size_of_parameters: " << method->_constMethod->_size_of_parameters << std::endl;
	std::cout << "      _from_interpreted_entry: " << method->_from_interpreted_entry << std::endl;

	jint *number = (jint *)get_method_arg(method, senderSP, 0);
	std::cout << "      arg0 (number): " << *number << std::endl;

	jstring message = (jstring)get_method_arg(method, senderSP, 1);
	std::cout << "      arg1 (message): " << *(void **)message << std::endl;

	jsize len = jni->GetStringUTFLength((jstring)message);
	std::cout << "      message size: " << len << std::endl;

	//// For some reason, this crashes...
	// const jchar *messageStr = jni->GetStringChars((jstring)message, NULL);
	// std::cout << "      message: " << messageStr << std::endl;

	// *(void **)message = *(void **)custom_string;

	jint new_number = 1337;
	std::cout << "[*] Modifying number to: " << new_number << std::endl;
	*number = new_number;
}

lm_address_t create_hook_stub(JavaVM *jvm, void *hookfn, void *jmp_back)
{
	char buf[1000];
	lm_bytearr_t code;
	lm_size_t codesize;
	lm_address_t hookaddr;

	hookaddr = LM_AllocMemory(0x1000, LM_PROT_XRW);
	if (hookaddr == LM_ADDRESS_BAD)
		return hookaddr;

	snprintf(buf, sizeof(buf),
		/// Back up registers and alloc bytes on stack
		"push rbx\n"
		"push rcx\n"
		"push rdx\n"
		"push rsi\n"
		"push rdi\n"
		"push r8\n"
		"push r9\n"
		"push r10\n"
		"push r11\n"
		"push rbp\n"
		"push rsp\n"

		/// arg 0
		"mov rdi, %p\n"

		/// arg 1
		// "mov rsi, [rbp - 48]\n"
		// "mov rsi, [rbp - 24]\n"
		"mov rsi, rbx\n"

		/// arg 2
		// "mov rdx, [rbp - 40]\n"
		"mov rdx, r13\n"

		/// arg 3
		// "mov rcx, [rbp - 32]\n"

		/// arg 4
		// "mov r8, [rbp - 24]\n"

		/// arg 5
		// "mov r9, [rbp - 16]\n"

		/// arg 8
		// "mov rax, [rbp + 24]\n"
		// "push rax\n"

		/// arg 7
		// "mov rax, [rbp + 16]\n"
		// "push rax\n"

		/// arg 6
		// "mov rax, [rbp - 8]\n"
		// "push rax\n"

		"mov rax, %p\n"
		"call rax\n"
		// "add rsp, 24\n"

		/// Clean up
		"pop rsp\n"
		"pop rbp\n"
		"pop r11\n"
		"pop r10\n"
		"pop r9\n"
		"pop r8\n"
		"pop rdi\n"
		"pop rsi\n"
		"pop rdx\n"
		"pop rcx\n"
		"pop rbx\n"

		/// Restore function
		"mov rax, %p\n"
		"jmp rax",

		jvm, hookfn, jmp_back
	);

	codesize = LM_AssembleEx(buf, LM_BITS, hookaddr, &code);
	if (!codesize) {
		LM_FreeMemory(hookaddr, 0x1000);
		return LM_ADDRESS_BAD;
	}

	if (LM_WriteMemory(hookaddr, code, codesize) != codesize) {
		LM_FreeMemory(hookaddr, 0x1000);
		return LM_ADDRESS_BAD;
	}

	return hookaddr;
}

struct threadargs {
	Method *method;
} thargs;

void *mythread(void *arg)
{
	for (;;) {
		std::cout << "MyThread!" << std::endl;
		std::cout << "_code: " << thargs.method->_code << std::endl;
		if (thargs.method->_code) {
			std::cout << "[*] Is Compiled? " << thargs.method->_code->is_compiled() << std::endl;
			std::cout << "[*] Compile ID: " << thargs.method->_code->compile_id() << std::endl;
			std::cout << "[*] Compile Level: " << thargs.method->_code->comp_level() << std::endl;
			std::cout << "[*] Entry Point: " << thargs.method->_code->entry_point() << std::endl;
			nmethod *nm = (nmethod *)thargs.method->_code;
			std::cout << "[*] NMethod: " << nm << std::endl;
			std::cout << "      - compile_id: " << nm->_compile_id << std::endl;
			std::cout << "      - comp_level: " << nm->_comp_level << std::endl;
		}
		sleep(1);
	}
	return NULL;
}

// TODO: Make sure method is not inlined
// TODO: Give support for compiled methods
int dl_main(JavaVM *jvm, JNIEnv *jni)
{
	lm_module_t libjvm;

	LM_FindModule((lm_string_t)"libjvm.so", &libjvm);
	int *_should_compile_new_jobs = (int *)(libjvm.base + 0x11172a8);
	std::cout << "[*] ptr _should_compile_new_jobs: " << _should_compile_new_jobs << std::endl;
	std::cout << "[*] _should_compile_new_jobs: " << *_should_compile_new_jobs << std::endl;
	*_should_compile_new_jobs = 2;
	
	jclass main = jni->FindClass("main/Main");
	if (!main) {
		std::cout << "[!] Failed to find main class" << std::endl;
		return -1;
	}
	std::cout << "[*] Main class: " << main << std::endl;

	jmethodID hookMeID = jni->GetStaticMethodID(main, "hookMe", "(ILjava/lang/String;)I");
	if (!hookMeID) {
		std::cout << "[*] Failed to find hookMe method ID" << std::endl;
		return -1;
	}
	std::cout << "[*] hookMe method ID: " << hookMeID << std::endl;

	Method *hookMe = *(Method **)hookMeID;
	std::cout << "[*] hookMe Method: " << hookMe << std::endl;
	std::cout << "[*] _i2i_entry: " << hookMe->_i2i_entry << std::endl;
	std::cout << "[*] _from_interpreted_entry (i2c_entry): " << hookMe->_from_interpreted_entry << std::endl;
	std::cout << "[*] _code: " << hookMe->_code << std::endl;
	std::cout << "[*] _from_compiled_entry (c2i_entry): " << hookMe->_from_compiled_entry << std::endl;
	std::cout << "[*] _flags: " << hookMe->_flags << std::endl;
	if (!hookMe->_i2i_entry || !hookMe->_from_interpreted_entry) {
		std::cout << "[!] Method is not interpreted!" << std::endl;
		return -1;
	}

	hookMe->_flags &= 0b11111101; // remove '_force_inline'
	hookMe->_flags |= 0b100; // add '_dont_inline'
	std::cout << "[*] _flags (after disabling inline): " << hookMe->_flags << std::endl;
	
	orig_i2c_entry = hookMe->_from_interpreted_entry;

	if ((hkInterpStub = create_hook_stub(jvm, (void *)hkHookMe, (void *)orig_i2c_entry)) == LM_ADDRESS_BAD) {
		std::cout << "[*] Failed to create hook stub" << std::endl;
		return -1;
	}
	
	hookMe->_from_interpreted_entry = (address)hkInterpStub;

	std::cout << "[*] Adapter: " << std::endl;
	std::cout << "  _i2c_entry: " << hookMe->_adapter->_i2c_entry << std::endl;
	std::cout << "  _c2i_entry: " << hookMe->_adapter->_c2i_entry << std::endl;
	std::cout << "  _c2i_entry (unverified): " << hookMe->_adapter->_c2i_unverified_entry << std::endl;
	std::cout << "  _c2i_entry (no clinit): " << hookMe->_adapter->_c2i_no_clinit_check_entry << std::endl;

	orig_c2i_entry = hookMe->_adapter->_c2i_unverified_entry;
	if ((hkCompStub = create_hook_stub(jvm, (void *)hkHookMe, (void *)orig_c2i_entry)) == LM_ADDRESS_BAD) {
		std::cout << "[*] Failed to create hook stub" << std::endl;
		return -1;
	}
	hookMe->_adapter->_c2i_entry = (address)hkCompStub;
	hookMe->_adapter->_c2i_unverified_entry = (address)hkCompStub;
	hookMe->_adapter->_c2i_no_clinit_check_entry = (address)hkCompStub;
	hookMe->_from_compiled_entry = (address)hkCompStub;
	// pthread_t th;
	// thargs.method = hookMe;
	// pthread_create(&th, NULL, mythread, NULL);

	jclass MyClass = jni->FindClass("main/MyClass");
	if (!MyClass) {
		std::cout << "[!] Failed to find MyClass" << std::endl;
		return -1;
	}
	std::cout << "[*] MyClass: " << MyClass << std::endl;

	/*
	jmethodID getUsernameID = jni->GetMethodID(MyClass, "getUsername", "()Ljava/lang/String;");
	std::cout << "[*] getUsername ID: " << getUsernameID << std::endl;

	Method *getUsername = *(Method **)getUsernameID;
	std::cout << "[*] getUsername method: " << getUsername << std::endl;

	std::cout << "[*] _i2i_entry: " << getUsername->_i2i_entry << std::endl;
	std::cout << "[*] _from_interpreted_entry (i2c_entry): " << getUsername->_from_interpreted_entry << std::endl;
	std::cout << "[*] _code: " << getUsername->_code << std::endl;
	std::cout << "[*] _from_compiled_entry (c2i_entry): " << getUsername->_from_compiled_entry << std::endl;
	std::cout << "[*] _flags: " << getUsername->_flags << std::endl;
	if (!getUsername->_i2i_entry || !getUsername->_from_interpreted_entry) {
		std::cout << "[!] Method is not interpreted!" << std::endl;
		return -1;
	}

	getUsername->_flags &= 0b11111101; // remove '_force_inline'
	getUsername->_flags |= 0b100; // add '_dont_inline'
	std::cout << "[*] _flags (after disabling inline): " << getUsername->_flags << std::endl;
	*/

	custom_string = jni->NewStringUTF("Hooked!");

	return 0;
}

void __attribute__((constructor))
dl_entry()
{
	JavaVM *jvm;
	JNIEnv *jni;

	std::cout << "[*] Library Loaded!" << std::endl;

	if (JNI_GetCreatedJavaVMs(&jvm, 1, NULL) != JNI_OK) {
		std::cout << "[!] Failed to retrieve JavaVM pointer" << std::endl;
		return;
	}

	std::cout << "[*] JavaVM: " << jvm << std::endl;

	if (jvm->AttachCurrentThread((void **)&jni, NULL) != JNI_OK) {
		std::cout << "[!] Failed to retrieve JNI environment" << std::endl;
		return;
	}

	std::cout << "[*] JNIEnv: " << jni << std::endl;

	dl_main(jvm, jni);
}
