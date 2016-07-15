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

void JNIDefaultEnvironment_free(Environment* _env, YRuntime* runtime) {
	JNIDefaultEnvironment* yenv = (JNIDefaultEnvironment*) _env;

	JNIEnv* env;
	yenv->getJNIEnv(yenv, &env);
	(*env)->PushLocalFrame(env, 10);

	(*env)->DeleteGlobalRef(env, yenv->jruntime);
	(*env)->DeleteGlobalRef(env, yenv->YJI);
	(*env)->DeleteGlobalRef(env, yenv->YJI_class);

	(*env)->PopLocalFrame(env, NULL);
	yenv->realEnv->free(yenv->realEnv, runtime);
	free(yenv);
}
YObject* JNIDefaultEnvironment_system(Environment* _env, YRuntime* runtime) {
	JNIDefaultEnvironment* yenv = (JNIDefaultEnvironment*) _env;
	JNIEnv* env;
	yenv->getJNIEnv(yenv, &env);
	(*env)->PushLocalFrame(env, 10);

	jclass YJISystem_class = (*env)->FindClass(env, "yoyo/yji/YJISystem");
	jmethodID YJISystem_init = (*env)->GetMethodID(env, YJISystem_class,
			"<init>", "(Lyoyo/yji/YJI;)V");
	jobject YJISystem = (*env)->NewObject(env, YJISystem_class, YJISystem_init,
			yenv->YJI);

	YThread* th = runtime->CoreThread;
	YObject* obj = newJavaObject(YJISystem, yenv, th);

	(*env)->PopLocalFrame(env, NULL);
	return obj;
}

void JNIDefaultEnvironment_getJNIEnv(JNIDefaultEnvironment* yenv, JNIEnv** env) {
	JavaVM* jvm = yenv->jvm;
	jint getEnvSuccess = (*jvm)->GetEnv(jvm, (void**) env, JNI_VERSION_1_6);

	if (getEnvSuccess == JNI_EDETACHED) {
		(*jvm)->AttachCurrentThread(jvm, (void**) env, NULL);
	}
}

YValue* JNIDefaultEnvironment_unwrap(JNIDefaultEnvironment* yenv, jobject obj,
		YThread* th) {
	if (obj == NULL)
		return getNull(th);
	JNIEnv* env;
	yenv->getJNIEnv(yenv, &env);
	(*env)->PushLocalFrame(env, 50);
	YValue* out = getNull(th);

	jclass objcl = (*env)->GetObjectClass(env, obj);
	jmethodID isInt_method = (*env)->GetMethodID(env, objcl, "isInt", "()Z");
	jmethodID isFloat_method = (*env)->GetMethodID(env, objcl, "isFloat",
			"()Z");
	jmethodID isBool_method = (*env)->GetMethodID(env, objcl, "isBoolean",
			"()Z");
	jmethodID isString_method = (*env)->GetMethodID(env, objcl, "isString",
			"()Z");
	jmethodID isObject_method = (*env)->GetMethodID(env, objcl, "isObject",
			"()Z");
	jmethodID isMethod_method = (*env)->GetMethodID(env, objcl, "isMethod",
			"()Z");
	jmethodID isArray_method = (*env)->GetMethodID(env, objcl, "isArray",
			"()Z");
	jmethodID isClass_method = (*env)->GetMethodID(env, objcl, "isClass",
			"()Z");
	jmethodID isStaticMethod_method = (*env)->GetMethodID(env, objcl,
			"isStaticMethod", "()Z");

	if ((bool) (*env)->CallBooleanMethod(env, obj, isInt_method)) {
		jfieldID field = (*env)->GetFieldID(env, objcl, "value", "J");
		int64_t value = (int64_t) (*env)->GetLongField(env, obj, field);
		out = newInteger(value, th);
	}

	else if ((bool) (*env)->CallBooleanMethod(env, obj, isFloat_method)) {
		jfieldID field = (*env)->GetFieldID(env, objcl, "value", "D");
		double value = (double) (*env)->GetDoubleField(env, obj, field);
		out = newFloat(value, th);
	}

	else if ((bool) (*env)->CallBooleanMethod(env, obj, isBool_method)) {
		jfieldID field = (*env)->GetFieldID(env, objcl, "value", "Z");
		bool value = (bool) (*env)->GetBooleanField(env, obj, field);
		out = newBoolean(value, th);
	}

	else if ((bool) (*env)->CallBooleanMethod(env, obj, isString_method)) {
		jfieldID field = (*env)->GetFieldID(env, objcl, "id", "I");
		int32_t value = (int32_t) (*env)->GetIntField(env, obj, field);
		out = newString(
				th->runtime->bytecode->getSymbolById(th->runtime->bytecode,
						value), th);
	}

	else if ((bool) (*env)->CallBooleanMethod(env, obj, isObject_method)) {
		jfieldID field = (*env)->GetFieldID(env, objcl, "value",
				"Ljava/lang/Object;");
		jobject value = (*env)->GetObjectField(env, obj, field);
		out = (YValue*) newJavaObject(value, yenv, th);
	}

	else if ((bool) (*env)->CallBooleanMethod(env, obj, isClass_method)) {
		jfieldID field = (*env)->GetFieldID(env, objcl, "value",
				"Ljava/lang/Class;");
		jobject value = (*env)->GetObjectField(env, obj, field);
		out = (YValue*) newJavaClass(value, yenv, th);
	}

	else if ((bool) (*env)->CallBooleanMethod(env, obj, isMethod_method)) {
		out = (YValue*) newJavaMethod(obj, yenv, th);
	}

	else if ((bool) (*env)->CallBooleanMethod(env, obj, isArray_method)) {
		out = (YValue*) newJavaArray(obj, yenv, th);
	}

	else if ((bool) (*env)->CallBooleanMethod(env, obj,
			isStaticMethod_method)) {
		out = (YValue*) newJavaStaticMethod(obj, yenv, th);
	}
	(*env)->PopLocalFrame(env, NULL);
	return out;
}

jobject JNIDefaultEnvironment_wrap(JNIDefaultEnvironment* yenv, YValue* val,
		YThread* th) {
	JNIEnv* env;
	yenv->getJNIEnv(yenv, &env);
	(*env)->PushLocalFrame(env, 50);

	jobject out = NULL;

	switch (val->type->type) {
	case IntegerT: {
		jlong jvalue = (jlong) ((YInteger*) val)->value;
		jclass YJIInteger_class = (*env)->FindClass(env, "yoyo/yji/YJIInteger");
		jmethodID YJIInteger_init = (*env)->GetMethodID(env, YJIInteger_class,
				"<init>", "(J)V");
		out = (*env)->NewObject(env, YJIInteger_class, YJIInteger_init, jvalue);
	}
		break;
	case FloatT: {
		jdouble jvalue = (jdouble) ((YFloat*) val)->value;
		jclass YJIFloat_class = (*env)->FindClass(env, "yoyo/yji/YJIFloat");
		jmethodID YJIFloat_init = (*env)->GetMethodID(env, YJIFloat_class,
				"<init>", "(D)V");
		out = (*env)->NewObject(env, YJIFloat_class, YJIFloat_init, jvalue);
	}
		break;
	case BooleanT: {
		jboolean jvalue = (jboolean) ((YBoolean*) val)->value;
		jclass YJIBoolean_class = (*env)->FindClass(env, "yoyo/yji/YJIBoolean");
		jmethodID YJIBoolean_init = (*env)->GetMethodID(env, YJIBoolean_class,
				"<init>", "(Z)V");
		out = (*env)->NewObject(env, YJIBoolean_class, YJIBoolean_init, jvalue);
	}
		break;
	case StringT: {
		jint jvalue = (jint) th->runtime->bytecode->getSymbolId(
				th->runtime->bytecode, ((YString*) val)->value);
		jclass YJIString_class = (*env)->FindClass(env, "yoyo/yji/YJIString");
		jmethodID YJIString_init = (*env)->GetMethodID(env, YJIString_class,
				"<init>", "(I)V");
		out = (*env)->NewObject(env, YJIString_class, YJIString_init, jvalue);
	}
		break;
	case ObjectT: {
		if (val->o.free == JavaObject_free) {
			JavaObject* jobject = (JavaObject*) val;
			jclass YJIObject_class = (*env)->FindClass(env,
					"yoyo/yji/YJIObject");
			jmethodID YJIObject_init = (*env)->GetMethodID(env, YJIObject_class,
					"<init>", "(Ljava/lang/Object;)V");
			out = (*env)->NewObject(env, YJIObject_class, YJIObject_init,
					jobject->java_object);
		} else {
			jclass cl = (*env)->FindClass(env, "yoyo/yji/YBindings");
			jmethodID init = (*env)->GetMethodID(env, cl, "<init>",
					"(Lyoyo/runtime/YRuntime;JJ)V");
			jclass YJIObject_class = (*env)->FindClass(env,
					"yoyo/yji/YJIObject");
			jmethodID YJIObject_init = (*env)->GetMethodID(env, YJIObject_class,
					"<init>", "(Ljava/lang/Object;)V");
			jobject ybind = (*env)->NewObject(env, cl, init, yenv->jruntime,
					(jlong) yenv, (jlong) val);
			out = (*env)->NewObject(env, YJIObject_class, YJIObject_init,
					ybind);
		}
	}
		break;
	case ArrayT: {
		if (val->o.free == JavaArray_free) {
			JavaArray* jarr = (JavaArray*) val;
			out = jarr->java_array;
		} else {
			YArray* arr = (YArray*) val;
			jclass YJIValue_class = (*env)->FindClass(env, "yoyo/yji/YJIValue");
			jobjectArray jarr = (*env)->NewObjectArray(env,
					(jsize) arr->size(arr, th), YJIValue_class, NULL);
			for (jsize i = 0; i < arr->size(arr, th); i++) {
				jobject obj = yenv->wrap(yenv, arr->get(arr, (uint32_t) i, th),
						th);
				(*env)->SetObjectArrayElement(env, jarr, i, obj);
				(*env)->DeleteLocalRef(env, obj);
			}
			jclass YJIArray_class = (*env)->FindClass(env, "yoyo/yji/YJIArray");
			jmethodID YJIArray_init = (*env)->GetMethodID(env, YJIArray_class,
					"<init>", "(Lyoyo/yji/YJI;[Lyoyo/yji/YJIValue;)V");
			out = (*env)->NewObject(env, YJIArray_class, YJIArray_init,
					yenv->YJI, jarr);
		}
	}
		break;
	case LambdaT: {
		jclass cl = (*env)->FindClass(env, "yoyo/yji/YRunnable");
		jmethodID init = (*env)->GetMethodID(env, cl, "<init>",
				"(Lyoyo/runtime/YRuntime;JJ)V");
		jclass YJIObject_class = (*env)->FindClass(env, "yoyo/yji/YJIObject");
		jmethodID YJIObject_init = (*env)->GetMethodID(env, YJIObject_class,
				"<init>", "(Ljava/lang/Object;)V");
		jobject ybind = (*env)->NewObject(env, cl, init, yenv->jruntime,
				(jlong) yenv, (jlong) val);
		out = (*env)->NewObject(env, YJIObject_class, YJIObject_init, ybind);
	}
		break;
	default:
		break;
	}

	return (*env)->PopLocalFrame(env, out);
}

CompilationResult JNIDefaultEnvironment_parse(Environment* env,
		YRuntime* runtime, wchar_t* wstr) {
	JNIDefaultEnvironment* yenv = (JNIDefaultEnvironment*) env;

	return yenv->realEnv->parse(yenv->realEnv, runtime, wstr);
}
CompilationResult JNIDefaultEnvironment_eval(Environment* env,
		YRuntime* runtime, wchar_t* wstr) {
	JNIDefaultEnvironment* yenv = (JNIDefaultEnvironment*) env;

	return yenv->realEnv->eval(yenv->realEnv, runtime, wstr);
}
wchar_t* JNIDefaultEnvironment_getenv(Environment* env, wchar_t* wkey) {
	JNIDefaultEnvironment* yenv = (JNIDefaultEnvironment*) env;
	return yenv->realEnv->getDefined(yenv->realEnv, wkey);
}
void JNIDefaultEnvironment_putenv(Environment* env, wchar_t* wkey,
		wchar_t* wvalue) {
	JNIDefaultEnvironment* yenv = (JNIDefaultEnvironment*) env;
	yenv->realEnv->define(yenv->realEnv, wkey, wvalue);
}
FILE* JNIDefaultEnvironment_getfile(Environment* env, wchar_t* wkey) {
	JNIDefaultEnvironment* yenv = (JNIDefaultEnvironment*) env;
	return yenv->realEnv->getFile(yenv->realEnv, wkey);
}
void JNIDefaultEnvironment_addpath(Environment* env, wchar_t* wkey) {
	JNIDefaultEnvironment* yenv = (JNIDefaultEnvironment*) env;
	yenv->realEnv->addPath(yenv->realEnv, wkey);
}
wchar_t** JNIDefaultEnvironment_getLoadedFiles(Environment* env) {
	JNIDefaultEnvironment* yenv = (JNIDefaultEnvironment*) env;
	return yenv->realEnv->getLoadedFiles(yenv->realEnv);
}

Environment* newJNIDefaultEnvironment(Environment* realEnv, JNIEnv* env,
		jobject jruntime) {
	JNIDefaultEnvironment* yenv = malloc(sizeof(JNIDefaultEnvironment));
	yenv->realEnv = realEnv;
	(*env)->PushLocalFrame(env, 10);

	(*env)->GetJavaVM(env, &yenv->jvm);
	yenv->jruntime = (*env)->NewGlobalRef(env, jruntime);
	yenv->getJNIEnv = JNIDefaultEnvironment_getJNIEnv;

	yenv->env.out_stream = realEnv->out_stream;
	yenv->env.in_stream = realEnv->in_stream;
	yenv->env.err_stream = realEnv->err_stream;
	yenv->env.free = JNIDefaultEnvironment_free;
	yenv->env.system = JNIDefaultEnvironment_system;
	yenv->env.parse = JNIDefaultEnvironment_parse;
	yenv->env.eval = JNIDefaultEnvironment_eval;
	yenv->env.getDefined = JNIDefaultEnvironment_getenv;
	yenv->env.define = JNIDefaultEnvironment_putenv;
	yenv->env.getFile = JNIDefaultEnvironment_getfile;
	yenv->env.addPath = JNIDefaultEnvironment_addpath;
	yenv->env.getLoadedFiles = JNIDefaultEnvironment_getLoadedFiles;

	jclass YJI_class = (*env)->FindClass(env, "yoyo/yji/YJI");
	jmethodID YJI_init = (*env)->GetMethodID(env, YJI_class, "<init>",
			"(Lyoyo/runtime/YRuntime;)V");
	jobject YJI = (*env)->NewObject(env, YJI_class, YJI_init, yenv->jruntime);

	yenv->YJI_class = (*env)->NewGlobalRef(env, YJI_class);
	yenv->YJI = (*env)->NewGlobalRef(env, YJI);
	yenv->wrap = JNIDefaultEnvironment_wrap;
	yenv->unwrap = JNIDefaultEnvironment_unwrap;

	(*env)->PopLocalFrame(env, NULL);

	return (Environment*) yenv;
}
