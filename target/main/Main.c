#include "jni.h"
#include <stdio.h>

JNIEXPORT jlong JNICALL Java_main_Main_getAddress(JNIEnv *env, jclass klass, jobject obj) {
	return (jlong)obj;
}

void __attribute__((constructor))
dl_entry()
{
	printf("libMain loaded!\n");
}
