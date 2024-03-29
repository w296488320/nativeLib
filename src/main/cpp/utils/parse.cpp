//
// Created by Administrator on 2020-09-14.
//

#include "parse.h"
#include "fileUtils.h"
#include "logging.h"


#define BUF_SIZE 1024

using namespace std;
std::string parse::jbyteArrayToStdString(JNIEnv* env, jbyteArray byteArray) {
    jsize len = env->GetArrayLength(byteArray);
    jbyte* bytes = env->GetByteArrayElements(byteArray, nullptr);
    std::string str(reinterpret_cast<char*>(bytes), len);
    env->ReleaseByteArrayElements(byteArray, bytes, JNI_ABORT);
    return str;
}
jbyteArray parse::stringToJByteArray(JNIEnv *env, const std::string &str) {
    jbyteArray byteArray = env->NewByteArray(static_cast<jsize>(str.size()));
    if (byteArray != nullptr) {
        env->SetByteArrayRegion(byteArray, 0,
                                static_cast<jsize>(str.size()),
                                reinterpret_cast<const jbyte *>(str.data()));
    }
    return byteArray;
}
[[maybe_unused]] jstring parse::char2jstring(JNIEnv *env, const char *pat) {

    //定义java String类 strClass
    jclass strClass = (env)->FindClass("java/lang/String");
    //获取String(byte[],String)的构造器,用于将本地byte[]数组转换为一个新String
    jmethodID ctorID = (env)->GetMethodID(strClass, "<init>", "([BLjava/lang/String;)V");
    //建立byte数组
    jbyteArray bytes = (env)->NewByteArray((jsize) strlen(pat));
    //将char* 转换为byte数组
    (env)->SetByteArrayRegion(bytes, 0, (jsize) strlen(pat), (jbyte *) pat);
    // 设置String, 保存语言类型,用于byte数组转换至String时的参数
    jstring encoding = (env)->NewStringUTF("GB2312");
    //将byte数组转换为java String,并输出
    return (jstring) (env)->NewObject(strClass, ctorID, bytes, encoding);
}

string parse::get_process_name() {
    //调用系统原生api
    return getprogname();
}

string parse::get_process_name_pid(pid_t pid) {
    //优先尝试读取cmdline
    auto pidStr = fileUtils::readText(
            string("/proc/").append(to_string(pid)).append("/comm"));
    if (pidStr.empty()) {
        pidStr = fileUtils::readText(
                string("/proc/").append(to_string(pid)).append("/cmdline"));
    }
    if(pidStr.empty()){
        return {};
    }
    return pidStr;
}

string parse::jstring2str(JNIEnv *env, jstring jstr) {
    return {env->GetStringUTFChars(jstr, nullptr)};
}

map<string, string> parse::jmap2cmap(JNIEnv *env, jobject jmap) {
    std::map<std::string, std::string> cmap;
    jclass jMapClass = env->FindClass("java/util/HashMap");
    jmethodID jKeySetMethodId = env->GetMethodID(jMapClass, "keySet", "()Ljava/util/Set;");
    jmethodID jGetMethodId = env->GetMethodID(jMapClass, "get",
                                              "(Ljava/lang/Object;)Ljava/lang/Object;");
    jobject jSetKey = env->CallObjectMethod(jmap, jKeySetMethodId);
    jclass jSetClass = env->FindClass("java/util/Set");
    jmethodID jToArrayMethodId = env->GetMethodID(jSetClass, "toArray", "()[Ljava/lang/Object;");
    auto jObjArray = (jobjectArray) env->CallObjectMethod(jSetKey, jToArrayMethodId);
    if (jObjArray == nullptr) {
        return cmap;
    }
    jsize size = env->GetArrayLength(jObjArray);
    int i;
    for (i = 0; i < size; i++) {
        auto jkey = (jstring) env->GetObjectArrayElement(jObjArray, i);
        auto jvalue = (jstring) env->CallObjectMethod(jmap, jGetMethodId, jkey);
        if (jvalue == nullptr) {
            continue;
        }
        char *key = (char *) env->GetStringUTFChars(jkey, nullptr);
        char *value = (char *) env->GetStringUTFChars(jvalue, nullptr);
        cmap[std::string(key)] = std::string(value);
    }
    return cmap;
}

jobject parse::cmap2jmap(JNIEnv *env,std::map<std::string, std::string> map){
    jclass hashMapClass = env->FindClass("java/util/HashMap");
    if (hashMapClass == nullptr) return nullptr;
    jmethodID hashMapInit = env->GetMethodID(hashMapClass, "<init>", "()V");
    if (hashMapInit == nullptr) return nullptr;
    jobject hashMap = env->NewObject(hashMapClass, hashMapInit);
    if (hashMap == nullptr) return nullptr;
    jmethodID hashMapPut = env->GetMethodID(hashMapClass, "put", "(Ljava/lang/Object;Ljava/lang/Object;)Ljava/lang/Object;");
    if (hashMapPut == nullptr) return nullptr;
    for (const auto& entry : map) {
        jstring key = env->NewStringUTF(entry.first.c_str());
        jstring value = env->NewStringUTF(entry.second.c_str());

        // 调用put方法，添加键值对到HashMap
        env->CallObjectMethod(hashMap, hashMapPut, key, value);

        // 释放局部引用，避免引用表溢出
        env->DeleteLocalRef(key);
        env->DeleteLocalRef(value);
    }
    return hashMap;
}

std::list<string> parse::jlist2clist(JNIEnv *env, jobject jlist) {
    std::list<std::string> clist;
    jclass listClazz = env->FindClass("java/util/ArrayList");
    jmethodID sizeMid = env->GetMethodID(listClazz, "size", "()I");
    jint size = env->CallIntMethod(jlist, sizeMid);
    jmethodID list_get = env->GetMethodID(listClazz, "get", "(I)Ljava/lang/Object;");
    for (int i = 0; i < size; i++) {
        jobject item = env->CallObjectMethod(jlist, list_get, i);
        //末尾添加
        clist.push_back(parse::jstring2str(env, (jstring) item));
    }
    return clist;
}

jobject parse::clist2jlist(JNIEnv *env, const list<string> &myList) {
    jclass arrayListClass = env->FindClass("java/util/ArrayList");
    jmethodID arrayListConstructor = env->GetMethodID(arrayListClass, "<init>", "()V");
    jobject arrayList = env->NewObject(arrayListClass, arrayListConstructor);
    jmethodID arrayListAddMethod = env->GetMethodID(arrayListClass, "add", "(Ljava/lang/Object;)Z");
    for (const std::string &value: myList) {
        jstring javaString = env->NewStringUTF(value.c_str());
        env->CallBooleanMethod(arrayList, arrayListAddMethod, javaString);
        env->DeleteLocalRef(javaString);
    }
    return arrayList;
}

[[maybe_unused]] bool parse::jboolean2bool(jboolean value) {
    return value == JNI_TRUE;
}


