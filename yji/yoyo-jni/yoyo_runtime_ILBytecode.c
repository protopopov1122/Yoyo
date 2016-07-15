/*This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.*/

#include "yoyo-jni/yoyo_runtime_ILBytecode.h"
#include "yoyo-jni/yoyo-jni.h"

YRuntime* getRuntimeFromBytecode(JNIEnv* env, jobject jbytecode) {
	jclass yoyo_runtime_ILBytecode_class = (*env)->GetObjectClass(env,
			jbytecode);
	jfieldID yoyo_runtime_ILBytecode_runtime = (*env)->GetFieldID(env,
			yoyo_runtime_ILBytecode_class, "runtime",
			"Lyoyo/runtime/YRuntime;");
	jobject jruntime = (*env)->GetObjectField(env, jbytecode,
			yoyo_runtime_ILBytecode_runtime);
	return getRuntime(env, jruntime);
}

JNIEXPORT jobjectArray JNICALL Java_yoyo_runtime_ILBytecode_getProcedures(
		JNIEnv *env, jobject jbytecode) {
	YRuntime* runtime = getRuntimeFromBytecode(env, jbytecode);
	ILBytecode* bc = runtime->bytecode;

	jclass yoyo_runtime_ILProcedure_class = (*env)->FindClass(env,
			"yoyo/runtime/ILProcedure");
	jmethodID yoyo_runtime_ILProcedure_init = (*env)->GetMethodID(env,
			yoyo_runtime_ILProcedure_class, "<init>", "()V");
	jfieldID yoyo_runtime_ILProcedure_proc = (*env)->GetFieldID(env,
			yoyo_runtime_ILProcedure_class, "proc", "J");

	jobjectArray outArr = (*env)->NewObjectArray(env,
			(jsize) bc->procedure_count, yoyo_runtime_ILProcedure_class, NULL);

	for (jsize i = 0; i < bc->procedure_count; i++) {
		if (bc->procedures[i] == NULL)
			continue;
		jobject jproc = (*env)->NewObject(env, yoyo_runtime_ILProcedure_class,
				yoyo_runtime_ILProcedure_init);
		(*env)->SetLongField(env, jproc, yoyo_runtime_ILProcedure_proc,
				(jlong) bc->procedures[i]);
		(*env)->SetObjectArrayElement(env, outArr, i, jproc);
	}

	return outArr;
}

JNIEXPORT jobject JNICALL Java_yoyo_runtime_ILBytecode_newProcedure(JNIEnv *env,
		jobject jbytecode) {
	YRuntime* runtime = getRuntimeFromBytecode(env, jbytecode);
	ILBytecode* bc = runtime->bytecode;

	ILProcedure* proc = bc->newProcedure(bc);

	jclass yoyo_runtime_ILProcedure_class = (*env)->FindClass(env,
			"yoyo/runtime/ILProcedure");
	jmethodID yoyo_runtime_ILProcedure_init = (*env)->GetMethodID(env,
			yoyo_runtime_ILProcedure_class, "<init>", "()V");
	jfieldID yoyo_runtime_ILProcedure_proc = (*env)->GetFieldID(env,
			yoyo_runtime_ILProcedure_class, "proc", "J");
	jobject jproc = (*env)->NewObject(env, yoyo_runtime_ILProcedure_class,
			yoyo_runtime_ILProcedure_init);
	(*env)->SetLongField(env, jproc, yoyo_runtime_ILProcedure_proc,
			(jlong) proc);

	return jproc;
}

JNIEXPORT jint JNICALL Java_yoyo_runtime_ILBytecode_getSymbolId(JNIEnv *env,
		jobject jbytecode, jstring jstr) {
	YRuntime* runtime = getRuntimeFromBytecode(env, jbytecode);
	ILBytecode* bc = runtime->bytecode;

	const char* cstr = (*env)->GetStringUTFChars(env, jstr, false);
	wchar_t* wstr = malloc(sizeof(wchar_t) * (strlen(cstr) + 1));
	mbstowcs(wstr, cstr, sizeof(wchar_t) * strlen(cstr));
	wstr[strlen(cstr)] = L'\0';
	int32_t id = bc->getSymbolId(bc, wstr);
	free(wstr);
	(*env)->ReleaseStringUTFChars(env, jstr, cstr);
	return (jint) id;
}

JNIEXPORT jstring JNICALL Java_yoyo_runtime_ILBytecode_getSymbolById(
		JNIEnv *env, jobject jbytecode, jint jid) {
	YRuntime* runtime = getRuntimeFromBytecode(env, jbytecode);
	ILBytecode* bc = runtime->bytecode;

	wchar_t* wstr = bc->getSymbolById(bc, (int32_t) jid);
	char* cstr = malloc(sizeof(char) * (wcslen(wstr) + 1));
	wcstombs(cstr, wstr, sizeof(char) * wcslen(wstr));
	cstr[wcslen(wstr)] = '\0';
	jstring jstr = (*env)->NewStringUTF(env, cstr);
	free(cstr);
	return jstr;
}

JNIEXPORT jintArray JNICALL Java_yoyo_runtime_ILBytecode_getSymbolIds(
		JNIEnv *env, jobject jbytecode) {
	YRuntime* runtime = getRuntimeFromBytecode(env, jbytecode);
	ILBytecode* bc = runtime->bytecode;

	jintArray out = (*env)->NewIntArray(env, (jsize) bc->symbols->size);
	jint* arr = (*env)->GetIntArrayElements(env, out, JNI_FALSE);
	for (uint32_t i = 0; i < bc->symbols->size; i++) {
		arr[i] = (jint) bc->symbols->map[i].id;
	}
	(*env)->ReleaseIntArrayElements(env, out, arr, 0);
	return out;
}

JNIEXPORT jint JNICALL Java_yoyo_runtime_ILBytecode_addIntegerConstant(
		JNIEnv *env, jobject jbytecode, jlong jcnst) {
	YRuntime* runtime = getRuntimeFromBytecode(env, jbytecode);
	ILBytecode* bc = runtime->bytecode;
	int32_t id = bc->addIntegerConstant(bc, (int64_t) jcnst);
	return (jint) id;
}

JNIEXPORT jint JNICALL Java_yoyo_runtime_ILBytecode_addFloatConstant(
		JNIEnv *env, jobject jbytecode, jdouble jcnst) {
	YRuntime* runtime = getRuntimeFromBytecode(env, jbytecode);
	ILBytecode* bc = runtime->bytecode;
	int32_t id = bc->addFloatConstant(bc, (double) jcnst);
	return (jint) id;
}

JNIEXPORT jint JNICALL Java_yoyo_runtime_ILBytecode_addBooleanConstant(
		JNIEnv *env, jobject jbytecode, jboolean jcnst) {
	YRuntime* runtime = getRuntimeFromBytecode(env, jbytecode);
	ILBytecode* bc = runtime->bytecode;
	int32_t id = bc->addBooleanConstant(bc, (bool) jcnst);
	return (jint) id;
}

JNIEXPORT jint JNICALL Java_yoyo_runtime_ILBytecode_addStringConstant(
		JNIEnv *env, jobject jbytecode, jstring jstr) {
	YRuntime* runtime = getRuntimeFromBytecode(env, jbytecode);
	ILBytecode* bc = runtime->bytecode;

	const char* cstr = (*env)->GetStringUTFChars(env, jstr, false);
	wchar_t* wstr = malloc(sizeof(wchar_t) * (strlen(cstr) + 1));
	mbstowcs(wstr, cstr, sizeof(wchar_t) * strlen(cstr));
	wstr[strlen(cstr)] = L'\0';
	int32_t id = bc->addStringConstant(bc, wstr);
	free(wstr);
	(*env)->ReleaseStringUTFChars(env, jstr, cstr);

	return (jint) id;
}

JNIEXPORT jint JNICALL Java_yoyo_runtime_ILBytecode_addNullConstant(JNIEnv *env,
		jobject jbytecode) {
	YRuntime* runtime = getRuntimeFromBytecode(env, jbytecode);
	ILBytecode* bc = runtime->bytecode;
	return (jint) bc->getNullConstant(bc);
}
JNIEXPORT jobject JNICALL Java_yoyo_runtime_ILBytecode_getProcedure(JNIEnv *env,
		jobject jbytecode, jint jpid) {
	YRuntime* runtime = getRuntimeFromBytecode(env, jbytecode);
	ILBytecode* bc = runtime->bytecode;
	jclass yoyo_runtime_ILProcedure_class = (*env)->FindClass(env,
			"yoyo/runtime/ILProcedure");
	jmethodID yoyo_runtime_ILProcedure_init = (*env)->GetMethodID(env,
			yoyo_runtime_ILProcedure_class, "<init>", "()V");
	jfieldID yoyo_runtime_ILProcedure_proc = (*env)->GetFieldID(env,
			yoyo_runtime_ILProcedure_class, "proc", "J");
	jobject jproc = (*env)->NewObject(env, yoyo_runtime_ILProcedure_class,
			yoyo_runtime_ILProcedure_init);
	(*env)->SetLongField(env, jproc, yoyo_runtime_ILProcedure_proc,
			(jlong) bc->procedures[(uint32_t) jpid]);

	return jproc;
}
JNIEXPORT jintArray JNICALL Java_yoyo_runtime_ILBytecode_getConstantIds(
		JNIEnv *env, jobject jbytecode) {
	YRuntime* runtime = getRuntimeFromBytecode(env, jbytecode);
	ILBytecode* bc = runtime->bytecode;

	jintArray out = (*env)->NewIntArray(env, (jsize) bc->constants.size);
	jint* arr = (*env)->GetIntArrayElements(env, out, JNI_FALSE);
	for (uint32_t i = 0; i < bc->constants.size; i++) {
		arr[i] = (jint) bc->constants.pool[i]->id;
	}
	(*env)->ReleaseIntArrayElements(env, out, arr, 0);
	return out;
}
JNIEXPORT jobject JNICALL Java_yoyo_runtime_ILBytecode_getConstant(JNIEnv *env,
		jobject jbytecode, jint cid) {
	YRuntime* runtime = getRuntimeFromBytecode(env, jbytecode);
	ILBytecode* bc = runtime->bytecode;
	Constant* cnst = bc->getConstant(bc, (int32_t) cid);
	if (cnst != NULL)
		switch (cnst->type) {
		case IntegerC: {
			jclass cls = (*env)->FindClass(env, "java/lang/Integer");
			jmethodID methodID = (*env)->GetMethodID(env, cls, "<init>",
					"(I)V");
			return (*env)->NewObject(env, cls, methodID,
					(jint) ((IntConstant*) cnst)->value);
		}
		case FloatC: {
			jclass cls = (*env)->FindClass(env, "java/lang/Double");
			jmethodID methodID = (*env)->GetMethodID(env, cls, "<init>",
					"(D)V");
			return (*env)->NewObject(env, cls, methodID,
					(jdouble) ((FloatConstant*) cnst)->value);
		}
		case StringC: {
			wchar_t* wstr = bc->getSymbolById(bc,
					((StringConstant*) cnst)->value);
			size_t size = (sizeof(wchar_t) / sizeof(char)) * (wcslen(wstr) + 1);
			char* cstr = malloc(size);
			wcstombs(cstr, wstr, size);
			cstr[wcslen(wstr)] = '\0';
			jstring jstr = (*env)->NewStringUTF(env, cstr);
			free(cstr);
			return jstr;
		}
		case BooleanC: {
			jclass cls = (*env)->FindClass(env, "java/lang/Boolean");
			jmethodID methodID = (*env)->GetMethodID(env, cls, "<init>",
					"(Z)V");
			return (*env)->NewObject(env, cls, methodID,
					(jboolean) ((BooleanConstant*) cnst)->value);
		}
		default:
			return NULL;
		}
	return NULL;
}
JNIEXPORT void JNICALL Java_yoyo_runtime_ILBytecode_export(JNIEnv *env,
		jobject jbytecode, jstring jpath) {
	YRuntime* runtime = getRuntimeFromBytecode(env, jbytecode);
	ILBytecode* bc = runtime->bytecode;

	const char* path = (*env)->GetStringUTFChars(env, jpath, JNI_FALSE);
	FILE* file = fopen(path, "w");
	bc->exportBytecode(bc, file);
	fclose(file);
}
