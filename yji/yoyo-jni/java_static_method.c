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

void JavaStaticMethod_mark(HeapObject* ptr) {
	ptr->marked = true;
}
YValue* JavaStaticMethod_exec(YLambda* l, YValue** args, size_t argc,
		YThread* th) {
	JavaStaticMethod* method = (JavaStaticMethod*) l;
	JNIDefaultEnvironment* yenv = method->yenv;
	JNIEnv* env;
	yenv->getJNIEnv(yenv, &env);
	(*env)->PushLocalFrame(env, 50 + argc);

	jclass Object_class = (*env)->FindClass(env, "yoyo/yji/YJIValue");
	jobjectArray jargs = (*env)->NewObjectArray(env, (jsize) argc, Object_class,
	NULL);
	for (jsize i = 0; i < argc; i++)
		(*env)->SetObjectArrayElement(env, jargs, i,
				yenv->wrap(yenv, args[i], th));

	jclass YJI_class = yenv->YJI_class;
	jmethodID YJI_call =
			(*env)->GetMethodID(env, YJI_class, "callStatic",
					"(Lyoyo/yji/YJIStaticMethod;[Lyoyo/yji/YJIValue;)Lyoyo/yji/YJIValue;");
	jobject jout = (*env)->CallObjectMethod(env, yenv->YJI, YJI_call,
			method->java_static_method, jargs);
	YValue* out = yenv->unwrap(yenv, jout, th);
	jthrowable jexc = (*env)->ExceptionOccurred(env);
	if (jexc != NULL) {
		th->exception = newException((YValue*) newJavaObject(jexc, yenv, th),
				th);
		(*env)->ExceptionClear(env);
	}

	(*env)->PopLocalFrame(env, NULL);
	return out;
}
YoyoType* JavaStaticMethod_signature(YLambda* l, YThread* th) {
	return (YoyoType*) l->sig;
}
YLambda* newJavaStaticMethod(jobject java_static_method,
		JNIDefaultEnvironment* yenv, YThread* th) {
	JNIEnv* env;
	yenv->getJNIEnv(yenv, &env);
	(*env)->PushLocalFrame(env, 10);
	JavaStaticMethod* method = malloc(sizeof(JavaStaticMethod));
	initHeapObject((HeapObject*) method, JavaStaticMethod_mark,
			JavaStaticMethod_free);
	th->runtime->gc->registrate(th->runtime->gc, (HeapObject*) method);
	method->lambda.parent.type = &th->runtime->LambdaType;
	method->yenv = yenv;
	method->java_static_method = (*env)->NewGlobalRef(env, java_static_method);
	method->lambda.execute = JavaStaticMethod_exec;
	method->lambda.sig = newLambdaSignature(-1, false, NULL, NULL, th);
	method->lambda.signature = JavaStaticMethod_signature;
	(*env)->PopLocalFrame(env, NULL);
	return (YLambda*) method;
}
void JavaStaticMethod_free(HeapObject* ptr) {
	JavaStaticMethod* method = (JavaStaticMethod*) ptr;
	JNIDefaultEnvironment* yenv = method->yenv;
	JNIEnv* env;
	yenv->getJNIEnv(yenv, &env);
	(*env)->PushLocalFrame(env, 10);
	(*env)->DeleteGlobalRef(env, method->java_static_method);
	(*env)->PopLocalFrame(env, NULL);
	free(method);
}
