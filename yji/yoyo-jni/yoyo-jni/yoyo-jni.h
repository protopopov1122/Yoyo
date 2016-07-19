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

#ifndef JNI_ENV_H
#define JNI_ENV_H

#include "../../../headers/yoyo/value.h"
#include "jni.h"

YObject* JNIDefaultEnvironment_system(Environment*, YRuntime*);
YValue* Yoyo_initYJI(YRuntime*);

YRuntime* getRuntime(JNIEnv*, jobject);
typedef struct JNIDefaultEnvironment {
	Environment env;
	Environment* realEnv;

	void (*getJNIEnv)(struct JNIDefaultEnvironment*, JNIEnv**);
	jobject (*wrap)(struct JNIDefaultEnvironment*, YValue*, YThread*);
	YValue* (*unwrap)(struct JNIDefaultEnvironment*, jobject, YThread*);

	JavaVM* jvm;
	jobject jruntime;
	jobject YJI;
	jobject YJI_class;
} JNIDefaultEnvironment;

Environment* newJNIDefaultEnvironment(Environment*, JNIEnv*, jobject);

typedef struct JavaObject {
	YObject object;

	jobject java_object;
	JNIDefaultEnvironment* yenv;
} JavaObject;

typedef struct JavaMethod {
	YLambda lambda;

	jobject java_method;
	JNIDefaultEnvironment* yenv;
} JavaMethod;

typedef struct JavaArray {
	YArray array;

	jobject java_array;
	JNIDefaultEnvironment* yenv;
} JavaArray;

typedef struct JavaClass {
	YObject object;

	jobject java_class;
	JNIDefaultEnvironment* yenv;
} JavaClass;

typedef struct JavaStaticMethod {
	YLambda lambda;

	jobject java_static_method;
	JNIDefaultEnvironment* yenv;
} JavaStaticMethod;

YObject* newJavaObject(jobject, JNIDefaultEnvironment*, YThread*);
void JavaObject_free(YoyoObject* ptr);

YLambda* newJavaMethod(jobject, JNIDefaultEnvironment*, YThread*);
void JavaMethod_free(YoyoObject* ptr);

YArray* newJavaArray(jobject, JNIDefaultEnvironment*, YThread*);
void JavaArray_free(YoyoObject* ptr);

YObject* newJavaClass(jobject, JNIDefaultEnvironment*, YThread*);
void JavaClass_free(YoyoObject* ptr);

YLambda* newJavaStaticMethod(jobject, JNIDefaultEnvironment*, YThread*);
void JavaStaticMethod_free(YoyoObject* ptr);

#endif
