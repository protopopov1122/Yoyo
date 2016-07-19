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

#include "yoyo-jni/yoyo_yji_YRunnable.h"
#include "yoyo-jni/yoyo-jni.h"

JNIDefaultEnvironment* getEnvFromYRunnable(JNIEnv* env, jobject yrunnable) {
	jclass cl = (*env)->GetObjectClass(env, yrunnable);
	jfieldID fid = (*env)->GetFieldID(env, cl, "defenv", "J");
	return (JNIDefaultEnvironment*) (*env)->GetLongField(env, yrunnable, fid);
}

YLambda* getLambdaFromYRunnable(JNIEnv* env, jobject yrunnable) {
	jclass cl = (*env)->GetObjectClass(env, yrunnable);
	jfieldID fid = (*env)->GetFieldID(env, cl, "ptr", "J");
	return (YLambda*) (*env)->GetLongField(env, yrunnable, fid);
}

YRuntime* getRuntimeFromYRunnable(JNIEnv* env, jobject yrunnable) {
	jclass cl = (*env)->GetObjectClass(env, yrunnable);
	jfieldID fid = (*env)->GetFieldID(env, cl, "runtime",
			"Lyoyo/runtime/YRuntime;");
	return getRuntime(env, (*env)->GetObjectField(env, yrunnable, fid));
}

JNIEXPORT jobject JNICALL Java_yoyo_yji_YRunnable__1invoke(JNIEnv *env,
		jobject yrunnable, jobjectArray jargs) {
	YLambda* lambda = getLambdaFromYRunnable(env, yrunnable);
	JNIDefaultEnvironment* yenv = getEnvFromYRunnable(env, yrunnable);
	YRuntime* runtime = getRuntimeFromYRunnable(env, yrunnable);
	YThread* th = newThread(runtime);
	uint32_t argc = (uint32_t) (*env)->GetArrayLength(env, jargs);
	YValue** args = malloc(sizeof(YValue*) * argc);
	for (uint32_t i = 0; i < argc; i++)
		args[i] = yenv->unwrap(yenv,
				(*env)->GetObjectArrayElement(env, jargs, (jsize) i), th);

	YValue* out = invokeLambda(lambda, args, argc, th);
	jobject jout = yenv->wrap(yenv, out, th);
	free(args);
	th->free(th);
	return jout;
}

JNIEXPORT void JNICALL Java_yoyo_yji_YRunnable_init(JNIEnv *env,
		jobject yrunnable) {
	YoyoObject* ptr = (YoyoObject*) getLambdaFromYRunnable(env, yrunnable);
	ptr->linkc++;
}

JNIEXPORT void JNICALL Java_yoyo_yji_YRunnable_destroy(JNIEnv *env,
		jobject yrunnable) {
	YoyoObject* ptr = (YoyoObject*) getLambdaFromYRunnable(env, yrunnable);
	ptr->linkc--;
}
