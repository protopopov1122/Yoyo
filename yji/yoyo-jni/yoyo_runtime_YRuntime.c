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

#include "yoyo-jni/yoyo_runtime_YRuntime.h"

#include "runtime.h"
#include "interpreter.h"
#include "yoyo-jni/yoyo-jni.h"

YRuntime* getRuntime(JNIEnv* env, jobject jruntime) {
	jclass yoyo_runtime_YRuntime_class = (*env)->GetObjectClass(env, jruntime);
	jfieldID yoyo_runtime_YRuntime_runtime = (*env)->GetFieldID(env,
			yoyo_runtime_YRuntime_class, "runtime", "J");
	jlong ptr = (*env)->GetLongField(env, jruntime,
			yoyo_runtime_YRuntime_runtime);
	return (YRuntime*) ptr;
}

JNIEXPORT jobject JNICALL Java_yoyo_runtime_YRuntime__1interpret(JNIEnv *env,
		jobject jruntime, jint jpid) {
	YRuntime* runtime = getRuntime(env, jruntime);
	JNIDefaultEnvironment* yenv = (JNIDefaultEnvironment*) runtime->env;
	YValue* val = runtime->interpret((int32_t) jpid, runtime);
	YThread* th = runtime->CoreThread;
	jobject jout = yenv->wrap(yenv, val, th);
	return jout;
}

JNIEXPORT void JNICALL Java_yoyo_runtime_YRuntime_init(JNIEnv *env,
		jobject jruntime) {
}

JNIEXPORT void JNICALL Java_yoyo_runtime_YRuntime_destroy(JNIEnv *env,
		jobject jruntime) {
	YRuntime* runtime = getRuntime(env, jruntime);
	runtime->free(runtime);
}
