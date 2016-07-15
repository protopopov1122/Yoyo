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

#include "yoyo-jni/yoyo_yji_YBindings.h"
#include "yoyo-jni/yoyo-jni.h"

JNIDefaultEnvironment* getEnvFromYBindings(JNIEnv* env, jobject ybindings) {
	jclass cl = (*env)->GetObjectClass(env, ybindings);
	jfieldID fid = (*env)->GetFieldID(env, cl, "defenv", "J");
	return (JNIDefaultEnvironment*) (*env)->GetLongField(env, ybindings, fid);
}

YObject* getObjectFromYBindings(JNIEnv* env, jobject ybindings) {
	jclass cl = (*env)->GetObjectClass(env, ybindings);
	jfieldID fid = (*env)->GetFieldID(env, cl, "ptr", "J");
	return (YObject*) (*env)->GetLongField(env, ybindings, fid);
}

YRuntime* getRuntimeFromYBindings(JNIEnv* env, jobject ybindings) {
	jclass cl = (*env)->GetObjectClass(env, ybindings);
	jfieldID fid = (*env)->GetFieldID(env, cl, "runtime",
			"Lyoyo/runtime/YRuntime;");
	return getRuntime(env, (*env)->GetObjectField(env, ybindings, fid));
}

JNIEXPORT jobject JNICALL Java_yoyo_yji_YBindings__1get(JNIEnv *env,
		jobject ybindings, jint jkey) {
	JNIDefaultEnvironment* yenv = getEnvFromYBindings(env, ybindings);
	YRuntime* runtime = getRuntimeFromYBindings(env, ybindings);
	YObject* obj = getObjectFromYBindings(env, ybindings);
	YThread* th = runtime->CoreThread;
	YValue* val = obj->get(obj, (int32_t) jkey, th);
	jobject jval = yenv->wrap(yenv, val, th);
	return jval;
}
JNIEXPORT void JNICALL Java_yoyo_yji_YBindings__1put(JNIEnv *env,
		jobject ybindings, jint key, jboolean newF, jobject jval) {
	JNIDefaultEnvironment* yenv = getEnvFromYBindings(env, ybindings);
	YRuntime* runtime = getRuntimeFromYBindings(env, ybindings);
	YObject* obj = getObjectFromYBindings(env, ybindings);
	YThread* th = runtime->CoreThread;
	YValue* val = yenv->unwrap(yenv, jval, th);
	obj->put(obj, (int32_t) key, val, (bool) newF, th);
}

JNIEXPORT void JNICALL Java_yoyo_yji_YBindings_init(JNIEnv *env,
		jobject ybindings) {
	YObject* obj = getObjectFromYBindings(env, ybindings);
	((HeapObject*) obj)->linkc++;
}

JNIEXPORT void JNICALL Java_yoyo_yji_YBindings_destroy(JNIEnv *env,
		jobject ybindings) {
	YObject* obj = getObjectFromYBindings(env, ybindings);
	((HeapObject*) obj)->linkc--;
}
