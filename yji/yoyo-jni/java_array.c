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

#include "yoyo-jni/yoyo-jni.h"

void JavaArray_mark(YoyoObject* ptr) {
	ptr->marked = true;
}

size_t JavaArray_size(YArray* a, YThread* th) {
	JavaArray* array = (JavaArray*) a;
	JNIDefaultEnvironment* yenv = array->yenv;
	JNIEnv* env;
	yenv->getJNIEnv(yenv, &env);
	(*env)->PushLocalFrame(env, 10);

	jclass YJIArray_class = (*env)->FindClass(env, "yoyo/yji/YJIArray");
	jmethodID YJIArray_size = (*env)->GetMethodID(env, YJIArray_class, "size",
			"()I");
	uint32_t value = (uint32_t) (*env)->CallIntMethod(env, array->java_array,
			YJIArray_size);

	(*env)->PopLocalFrame(env, NULL);

	return value;
}

YValue* JavaArray_get(YArray* a, size_t index, YThread* th) {
	JavaArray* array = (JavaArray*) a;
	JNIDefaultEnvironment* yenv = array->yenv;
	JNIEnv* env;
	yenv->getJNIEnv(yenv, &env);
	(*env)->PushLocalFrame(env, 10);

	jclass YJIArray_class = (*env)->FindClass(env, "yoyo/yji/YJIArray");
	jmethodID YJIArray_get = (*env)->GetMethodID(env, YJIArray_class, "getYJI",
			"(I)Lyoyo/yji/YJIValue;");
	jobject jvalue = (*env)->CallObjectMethod(env, array->java_array,
			YJIArray_get, (jint) index);
	YValue* value = yenv->unwrap(yenv, jvalue, th);

	(*env)->PopLocalFrame(env, NULL);

	return value;
}

void JavaArray_set(YArray* a, size_t index, YValue* v, YThread* th) {
	JavaArray* array = (JavaArray*) a;
	JNIDefaultEnvironment* yenv = array->yenv;
	JNIEnv* env;
	yenv->getJNIEnv(yenv, &env);
	(*env)->PushLocalFrame(env, 10);

	jclass YJIArray_class = (*env)->FindClass(env, "yoyo/yji/YJIArray");
	jmethodID YJIArray_set = (*env)->GetMethodID(env, YJIArray_class, "setYJI",
			"(ILyoyo/yji/YJIValue;)V");
	jobject jvalue = yenv->wrap(yenv, v, th);
	(*env)->CallObjectMethod(env, array->java_array, YJIArray_set, (jint) index,
			jvalue);
	(*env)->DeleteLocalRef(env, jvalue);

	(*env)->PopLocalFrame(env, NULL);
}

void JavaArray_add(YArray* a, YValue* v, YThread* th) {
	return;
}
void JavaArray_remove(YArray* a, size_t i, YThread* th) {
	return;
}
void JavaArray_insert(YArray* a, size_t i, YValue* v, YThread* th) {
	return;
}

YArray* newJavaArray(jobject java_array, JNIDefaultEnvironment* yenv,
		YThread* th) {
	JNIEnv* env;
	yenv->getJNIEnv(yenv, &env);
	(*env)->PushLocalFrame(env, 10);
	JavaArray* array = malloc(sizeof(JavaArray));
	initYoyoObject((YoyoObject*) array, JavaArray_mark, JavaArray_free);
	th->runtime->gc->registrate(th->runtime->gc, (YoyoObject*) array);
	array->array.parent.type = &th->runtime->ArrayType;
	array->yenv = yenv;
	array->java_array = (*env)->NewGlobalRef(env, java_array);

	array->array.size = JavaArray_size;
	array->array.get = JavaArray_get;
	array->array.set = JavaArray_set;
	array->array.add = JavaArray_add;
	array->array.remove = JavaArray_remove;
	array->array.insert = JavaArray_insert;
	array->array.toString = NULL;

	(*env)->PopLocalFrame(env, NULL);
	return (YArray*) array;
}
void JavaArray_free(YoyoObject* ptr) {
	JavaArray* array = (JavaArray*) ptr;
	JNIDefaultEnvironment* yenv = array->yenv;
	JNIEnv* env;
	yenv->getJNIEnv(yenv, &env);
	(*env)->PushLocalFrame(env, 10);
	(*env)->DeleteGlobalRef(env, array->java_array);
	free(array);
	(*env)->PopLocalFrame(env, NULL);
}
