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

void JavaObject_mark(YoyoObject* ptr) {
	ptr->marked = true;
}
void JavaObject_free(YoyoObject* ptr) {
	JavaObject* object = (JavaObject*) ptr;
	JNIDefaultEnvironment* yenv = object->yenv;
	JNIEnv* env;
	yenv->getJNIEnv(yenv, &env);
	(*env)->DeleteGlobalRef(env, object->java_object);
	free(object);
}

YValue* JavaObject_get(YObject* o, int32_t key, YThread* th) {
	JavaObject* object = (JavaObject*) o;
	JNIDefaultEnvironment* yenv = object->yenv;
	JNIEnv* env;
	yenv->getJNIEnv(yenv, &env);
	(*env)->PushLocalFrame(env, 10);
	jclass YJI_class = (*env)->FindClass(env, "yoyo/yji/YJI");
	jmethodID YJI_get = (*env)->GetMethodID(env, YJI_class, "get",
			"(Ljava/lang/Object;I)Lyoyo/yji/YJIValue;");
	jobject jres = (*env)->CallObjectMethod(env, yenv->YJI, YJI_get,
			object->java_object, (jint) key);
	YValue* out = yenv->unwrap(yenv, jres, th);

	(*env)->PopLocalFrame(env, NULL);
	return out;
}
void JavaObject_put(YObject* o, int32_t key, YValue* val, bool newR,
		YThread* th) {
	JavaObject* object = (JavaObject*) o;
	JNIDefaultEnvironment* yenv = object->yenv;
	JNIEnv* env;
	yenv->getJNIEnv(yenv, &env);
	(*env)->PushLocalFrame(env, 10);

	jclass YJI_class = yenv->YJI_class;
	jmethodID YJI_set = (*env)->GetMethodID(env, YJI_class, "set",
			"(Ljava/lang/Object;ILyoyo/yji/YJIValue;)V");
	jobject jobj = yenv->wrap(yenv, val, th);
	(*env)->CallVoidMethod(env, yenv->YJI, YJI_set, object->java_object,
			(jint) key, jobj);
	(*env)->DeleteLocalRef(env, jobj);

	(*env)->PopLocalFrame(env, NULL);
}
bool JavaObject_contains(YObject* o, int32_t key, YThread* th) {
	JavaObject* object = (JavaObject*) o;
	JNIDefaultEnvironment* yenv = object->yenv;
	JNIEnv* env;
	yenv->getJNIEnv(yenv, &env);
	(*env)->PushLocalFrame(env, 10);
	jclass YJI_class = yenv->YJI_class;
	jmethodID YJI_cont = (*env)->GetMethodID(env, YJI_class, "contains",
			"(Ljava/lang/Object;I)Z");
	bool res = (bool) (*env)->CallBooleanMethod(env, yenv->YJI, YJI_cont,
			object->java_object, (jint) key);

	(*env)->PopLocalFrame(env, NULL);
	return res;
}
void JavaObject_remove(YObject* o, int32_t key, YThread* th) {
	return;
}
void JavaObject_setType(YObject* o, int32_t key, YoyoType* type, YThread* th) {
	return;
}
YoyoType* JavaObject_getType(YObject* o, int32_t key, YThread* th) {
	return th->runtime->NullType.TypeConstant;
}

YObject* newJavaObject(jobject jobj, JNIDefaultEnvironment* yenv, YThread* th) {
	JNIEnv* env;
	yenv->getJNIEnv(yenv, &env);
	JavaObject* object = malloc(sizeof(JavaObject));
	initYoyoObject((YoyoObject*) object, JavaObject_mark, JavaObject_free);
	th->runtime->gc->registrate(th->runtime->gc, (YoyoObject*) object);

	object->yenv = yenv;
	object->java_object = (*env)->NewGlobalRef(env, jobj);

	object->object.iterator = false;

	object->object.parent.type = &th->runtime->ObjectType;

	object->object.get = JavaObject_get;
	object->object.contains = JavaObject_contains;
	object->object.put = JavaObject_put;
	object->object.remove = JavaObject_remove;
	object->object.getType = JavaObject_getType;
	object->object.setType = JavaObject_setType;

	return (YObject*) object;
}
