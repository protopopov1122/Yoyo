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

YValue* Yoyo_initYJI(YRuntime* runtime) {
	if (runtime->env->system == JNIDefaultEnvironment_system)
		return (YValue*) runtime->env->system(runtime->env, runtime);
	wchar_t* wjar_path = runtime->env->getDefined(runtime->env, L"yjijar");
	wchar_t* wlib_path = runtime->env->getDefined(runtime->env, L"libyji");
	if (wjar_path == NULL || wlib_path == NULL)
		return false;
	char* jar_path = calloc(1,
			sizeof(wchar_t) / sizeof(char) * (wcslen(wjar_path) + 1));
	wcstombs(jar_path, wjar_path, wcslen(wjar_path));
	char* lib_path = calloc(1,
			sizeof(wchar_t) / sizeof(char) * (wcslen(wlib_path) + 1));
	wcstombs(lib_path, wlib_path, wcslen(wlib_path));

	const char* CLASSPATH = "-Djava.class.path=";

	char yji_dir[PATH_MAX + 1];
	char* res = realpath(lib_path, yji_dir);

	char* arg1 = malloc(
			sizeof(char) * (strlen(jar_path) + strlen(CLASSPATH) + 1));
	strcpy(arg1, CLASSPATH);
	strcat(arg1, jar_path);

	JavaVMOption args[1];
	args[0].optionString = arg1;

	JavaVMInitArgs vmArgs;
	vmArgs.version = JNI_VERSION_1_8;
	vmArgs.nOptions = 1;
	vmArgs.options = args;
	vmArgs.ignoreUnrecognized = JNI_FALSE;

	JavaVM *jvm;
	JNIEnv *env;
	long flag = JNI_CreateJavaVM(&jvm, (void**) &env, &vmArgs);
	free(arg1);
	bool result = true;
	if (flag != JNI_ERR && res != NULL) {
		jclass cl = (*env)->FindClass(env, "yoyo/runtime/YRuntime");
		jmethodID mid =
				cl == NULL ?
						NULL :
						(*env)->GetMethodID(env, cl, "<init>",
								"(JLjava/lang/String;)V");
		jstring jpath = (*env)->NewStringUTF(env, yji_dir);
		jobject jruntime =
				mid == NULL ?
						NULL :
						(*env)->NewObject(env, cl, mid, (jlong) runtime, jpath);
		if (jruntime == NULL)
			result = false;
		else
			runtime->env = newJNIDefaultEnvironment(runtime->env, env,
					jruntime);
	} else
		result = false;
	if (!result)
		(*jvm)->DestroyJavaVM(jvm);
	free(jar_path);
	free(lib_path);

	if (result)
		return (YValue*) runtime->env->system(runtime->env, runtime);
	else
		return getNull(runtime->CoreThread);

}
