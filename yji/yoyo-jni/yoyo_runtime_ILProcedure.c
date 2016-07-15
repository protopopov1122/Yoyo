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

#include "yoyo-jni/yoyo_runtime_ILProcedure.h"
#include "yoyo-jni/yoyo-jni.h"

ILProcedure* getProcedureFromObj(JNIEnv* env, jobject jproc) {
	jclass yoyo_runtime_ILProcedure_class = (*env)->GetObjectClass(env, jproc);
	jfieldID yoyo_runtime_ILProcedure_proc = (*env)->GetFieldID(env,
			yoyo_runtime_ILProcedure_class, "proc", "J");
	jlong ptr = (*env)->GetLongField(env, jproc, yoyo_runtime_ILProcedure_proc);
	return (ILProcedure*) ptr;
}

JNIEXPORT jint JNICALL Java_yoyo_runtime_ILProcedure_getIdentifier(JNIEnv *env,
		jobject jproc) {
	ILProcedure* proc = getProcedureFromObj(env, jproc);
	return (jint) proc->id;
}

JNIEXPORT void JNICALL Java_yoyo_runtime_ILProcedure_setRegisterCount(
		JNIEnv *env, jobject jproc, jint jregc) {
	ILProcedure* proc = getProcedureFromObj(env, jproc);
	if (jregc >= 0)
		proc->regc = (uint32_t) jregc;
}

JNIEXPORT jint JNICALL Java_yoyo_runtime_ILProcedure_getRegisterCount(
		JNIEnv *env, jobject jproc) {
	ILProcedure* proc = getProcedureFromObj(env, jproc);
	return (jint) proc->regc;
}

JNIEXPORT jint JNICALL Java_yoyo_runtime_ILProcedure_getCodeLength(JNIEnv *env,
		jobject jproc) {
	ILProcedure* proc = getProcedureFromObj(env, jproc);
	return (jint) proc->code_length;
}

JNIEXPORT jbyteArray JNICALL Java_yoyo_runtime_ILProcedure_getCode(JNIEnv *env,
		jobject jproc) {
	ILProcedure* proc = getProcedureFromObj(env, jproc);
	jbyteArray jcode = (*env)->NewByteArray(env, (jsize) proc->code_length);
	jbyte* jarr = (*env)->GetByteArrayElements(env, jcode, JNI_FALSE);
	for (uint32_t i = 0; i < proc->code_length; i++)
		jarr[i] = proc->code[i];
	(*env)->ReleaseByteArrayElements(env, jcode, jarr, 0);
	return jcode;
}

JNIEXPORT void JNICALL Java_yoyo_runtime_ILProcedure_appendCode__BIII(
		JNIEnv *env, jobject jproc, jbyte jopc, jint jarg1, jint jarg2,
		jint jarg3) {
	ILProcedure* proc = getProcedureFromObj(env, jproc);
	union {
		int32_t arg;
		uint8_t raw[4];
	} conv;

	uint8_t opcode[] = { (uint8_t) jopc };
	proc->appendCode(proc, opcode, sizeof(opcode));
	conv.arg = (int32_t) jarg1;
	proc->appendCode(proc, conv.raw, sizeof(conv.raw));
	conv.arg = (int32_t) jarg2;
	proc->appendCode(proc, conv.raw, sizeof(conv.raw));
	conv.arg = (int32_t) jarg3;
	proc->appendCode(proc, conv.raw, sizeof(conv.raw));

}

JNIEXPORT void JNICALL Java_yoyo_runtime_ILProcedure_appendCode___3B(
		JNIEnv *env, jobject jproc, jbyteArray jcode) {
	ILProcedure* proc = getProcedureFromObj(env, jproc);
	uint32_t len = (*env)->GetArrayLength(env, jcode);
	uint8_t* code = malloc(sizeof(uint8_t) * len);

	jbyte* jarr = (*env)->GetByteArrayElements(env, jcode, false);
	for (uint32_t i = 0; i < len; i++)
		code[i] = (uint8_t) jarr[i];
	(*env)->ReleaseByteArrayElements(env, jcode, jarr, JNI_ABORT);

	proc->appendCode(proc, code, len);

	free(code);
}
JNIEXPORT void JNICALL Java_yoyo_runtime_ILProcedure_addCodeTableEntry(
		JNIEnv *env, jobject jproc, jint jline, jint jchp, jint joff, jint jlen,
		jint jfile) {
	ILProcedure* proc = getProcedureFromObj(env, jproc);
	proc->addCodeTableEntry(proc, (uint32_t) jline, (uint32_t) jchp,
			(uint32_t) joff, (uint32_t) jlen, (int32_t) jfile);
}

JNIEXPORT void JNICALL Java_yoyo_runtime_ILProcedure_addLabel(JNIEnv *env,
		jobject jproc, jint jid, jint jv) {
	ILProcedure* proc = getProcedureFromObj(env, jproc);
	proc->addLabel(proc, (int32_t) jid, (uint32_t) jv);
}

JNIEXPORT jint JNICALL Java_yoyo_runtime_ILProcedure_getLabel(JNIEnv *env,
		jobject jproc, jint jid) {
	ILProcedure* proc = getProcedureFromObj(env, jproc);
	LabelEntry* e = proc->getLabel(proc, (int32_t) jid);
	if (e == 0)
		return (jint) -1;
	else
		return (jint) e->value;
}
JNIEXPORT jintArray JNICALL Java_yoyo_runtime_ILProcedure_getLabels(JNIEnv *env,
		jobject jproc) {
	ILProcedure* proc = getProcedureFromObj(env, jproc);
	jintArray out = (*env)->NewIntArray(env, (jsize) proc->labels.length);
	jint* arr = (*env)->GetIntArrayElements(env, out, JNI_FALSE);
	for (uint32_t i = 0; i < proc->labels.length; i++) {
		arr[i] = (jint) proc->labels.table[i].id;
	}
	(*env)->ReleaseIntArrayElements(env, out, arr, 0);
	return out;
}
JNIEXPORT void JNICALL Java_yoyo_runtime_ILProcedure_free(JNIEnv *env,
		jobject jproc) {
	ILProcedure* proc = getProcedureFromObj(env, jproc);
	proc->free(proc, proc->bytecode);
}
