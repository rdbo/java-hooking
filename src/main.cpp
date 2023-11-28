#include "jvm.h"
#include <libmem/libmem.hpp>
#include <iostream>

address orig_i2c_entry = NULL;
lm_address_t hkInterpStub = LM_ADDRESS_BAD;

void hkCallStub(JavaVM *jvm, address *link, intptr_t *result, BasicType *result_type, Method *method, address entry_point, intptr_t *parameters, int *size_of_parameters, JavaThread *__the_thread__)
{
	JNIEnv *jni;
	
	std::cout << "[*] hkCallStub called!" << std::endl;
	std::cout << "[*] hkInterpStub: " << (void *)hkInterpStub << std::endl;
	std::cout << "[*] JavaVM: " << jvm << std::endl;

	jvm->GetEnv((void **)&jni, JNI_VERSION_1_6);
	std::cout << "[*] JNIEnv: " << jni << std::endl;
		
	std::cout << "[*] Stub info: " << std::endl;
	std::cout << "      link: " << link << std::endl;
	std::cout << "      result: " << result << std::endl;
	std::cout << "      result_type: " << result_type << std::endl;
	std::cout << "      method: " << method << std::endl;
	std::cout << "      entry_point: " << entry_point << std::endl;
	std::cout << "      parameters: " << parameters << std::endl;
	std::cout << "      size_of_parameters: " << size_of_parameters << std::endl;
	std::cout << "      __the_thread__: " << __the_thread__ << std::endl;

	jmethodID mID = (jmethodID)&method;

	std::cout << "[*] Method (stub): " << std::endl;
	std::cout << "      _from_interpreted_entry: " << method->_from_interpreted_entry << std::endl;
}

int create_hook_stub(JavaVM *jvm)
{
	char buf[1000];
	lm_bytearr_t code;
	lm_size_t codesize;

	hkInterpStub = LM_AllocMemory(0x1000, LM_PROT_XRW);
	if (hkInterpStub == LM_ADDRESS_BAD)
		return -1;

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
		"mov rsi, [rbp - 48]\n"

		/// arg 2
		"mov rdx, [rbp - 40]\n"

		/// arg 3
		"mov rcx, [rbp - 32]\n"

		/// arg 4
		"mov r8, [rbp - 24]\n"

		/// arg 5
		"mov r9, [rbp - 16]\n"

		/// arg 8
		"mov rax, [rbp + 24]\n"
		"push rax\n"

		/// arg 7
		"mov rax, [rbp + 16]\n"
		"push rax\n"

		/// arg 6
		"mov rax, [rbp - 8]\n"
		"push rax\n"

		"mov rax, %p\n"
		"call rax\n"
		"add rsp, 24\n"

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

		jvm, hkCallStub, orig_i2c_entry
	);

	codesize = LM_AssembleEx(buf, LM_BITS, hkInterpStub, &code);
	if (!codesize)
		return -1;

	return LM_WriteMemory(hkInterpStub, code, codesize) == codesize;
}

// TODO: Make sure method is not inlined
// TODO: Give support for compiled methods
int dl_main(JavaVM *jvm, JNIEnv *jni)
{
	jclass main = jni->FindClass("main/Main");
	if (!main) {
		std::cout << "[!] Failed to find main class" << std::endl;
		return -1;
	}
	std::cout << "[*] Main class: " << main << std::endl;

	jmethodID hookMeID = jni->GetStaticMethodID(main, "hookMe", "(I)I");
	if (!hookMeID) {
		std::cout << "[*] Failed to find hookMe method ID" << std::endl;
		return -1;
	}
	std::cout << "[*] hookMe method ID: " << hookMeID << std::endl;

	Method *hookMe = *(Method **)hookMeID;
	std::cout << "[*] hookMe Method: " << hookMe << std::endl;
	std::cout << "[*] _i2i_entry: " << hookMe->_i2i_entry << std::endl;
	std::cout << "[*] _from_interpreted_entry (i2c_entry): " << hookMe->_from_interpreted_entry << std::endl;
	std::cout << "[*] _flags: " << hookMe->_flags << std::endl;
	if (!hookMe->_i2i_entry || !hookMe->_from_interpreted_entry) {
		std::cout << "[!] Method is not interpreted!" << std::endl;
		return -1;
	}

	hookMe->_flags &= 0b11111101; // remove '_force_inline'
	hookMe->_flags |= 0b100; // add '_dont_inline'
	std::cout << "[*] _flags (after disabling inline): " << hookMe->_flags << std::endl;
	
	orig_i2c_entry = hookMe->_from_interpreted_entry;

	if (!create_hook_stub(jvm)) {
		std::cout << "[*] Failed to create hook stub" << std::endl;
		return -1;
	}
	
	hookMe->_from_interpreted_entry = (address)hkInterpStub;

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
