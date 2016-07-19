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

void JavaClass_mark(YoyoObject* ptr) {
	ptr->marked = true;
}
void JavaClass_remove(YObject* o, int32_t key, YThread* th) {
	return;
}

YValue* JavaClass_get(YObject* o, int32_t key, YThread* th) {
	JavaClass* object = (JavaClass*) o;
	JNIDefaultEnvironment* yenv = object->yenv;
	JNIEnv* env;
	yenv->getJNIEnv(yenv, &env);
	(*env)->PushLocalFrame(env, 10);
	jclass YJI_class = (*env)->FindClass(env, "yoyo/yji/YJI");
	jmethodID YJI_get = (*env)->GetMethodID(env, YJI_class, "loadStatic",
			"(Ljava/lang/Class;I)Lyoyo/yji/YJIValue;");
	jobject jres = (*env)->CallObjectMethod(env, yenv->YJI, YJI_get,
			object->java_class, (jint) key);
	YValue* out = yenv->unwrap(yenv, jres, th);

	(*env)->PopLocalFrame(env, NULL);
	return out;
}
void JavaClass_put(YObject* o, int32_t key, YValue* val, bool newR, YThread* th) {
}
bool JavaClass_contains(YObject* o, int32_t key, YThread* th) {
	JavaClass* object = (JavaClass*) o;
	JNIDefaultEnvironment* yenv = object->yenv;
	JNIEnv* env;
	yenv->getJNIEnv(yenv, &env);
	(*env)->PushLocalFrame(env, 10);
	jclass YJI_class = yenv->YJI_class;
	jmethodID YJI_cont = (*env)->GetMethodID(env, YJI_class, "containsStatic",
			"(Ljava/lang/Class;I)Z");
	bool res = (bool) (*env)->CallBooleanMethod(env, yenv->YJI, YJI_cont,
			object->java_class, (jint) key);

	(*env)->PopLocalFrame(env, NULL);
	return res;
}
void JavaClass_setType(YObject* o, int32_t key, YoyoType* type, YThread* th) {
	return;
}
YoyoType* JavaClass_getType(YObject* o, int32_t key, YThread* th) {
	return th->runtime->NullType.TypeConstant;
}

YObject* newJavaClass(jobject java_class, JNIDefaultEnvironment* yenv,
		YThread* th) {
	JNIEnv* env;
	yenv->getJNIEnv(yenv, &env);
	JavaClass* object = malloc(sizeof(JavaClass));
	initYoyoObject((YoyoObject*) object, JavaClass_mark, JavaClass_free);
	th->runtime->gc->registrate(th->runtime->gc, (YoyoObject*) object);

	object->yenv = yenv;
	object->java_class = (*env)->NewGlobalRef(env, java_class);

	object->object.parent.type = &th->runtime->ObjectType;

	object->object.iterator = false;

	object->object.get = JavaClass_get;
	object->object.contains = JavaClass_contains;
	object->object.put = JavaClass_put;
	object->object.remove = JavaClass_remove;
	object->object.getType = JavaClass_getType;
	object->object.setType = JavaClass_setType;

	return (YObject*) object;
}
void JavaClass_free(YoyoObject* ptr) {
	JavaClass* object = (JavaClass*) ptr;
	JNIDefaultEnvironment* yenv = object->yenv;
	JNIEnv* env;
	yenv->getJNIEnv(yenv, &env);
	(*env)->DeleteGlobalRef(env, object->java_class);
	free(object);
}
