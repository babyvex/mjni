//
// Created by natty on 14.07.2026.
//

#ifndef MICOTIANEXTERNAL_MJNI_H
#define MICOTIANEXTERNAL_MJNI_H
#include "jvm/Structs.hpp"

#include <vector>

#include "jvm/Offsets.h"
#include "memory/Memory.h"

// utility functions
namespace mJNIUtilityFunctions {

    jclass decodeKlass(jclass klass);
    jclass encodeKlass(jclass klass);

    jobject decodeOop(jobject object);
    jobject encodeOop(jobject object);

    uint32_t readU5( const uint8_t* data, size_t& pos);
    void readFieldInfoStream(FieldInfo20& out, const uint8_t* buffer, size_t& pos);
    int copyFieldInfo(void* address, std::vector<FieldInfo20>& outFields);
    bool writeBytecode(void* address,  void* buffer, size_t size);

    std::string readSymbol(void* address);

    jobject getClassOOP(jclass klass);


    // utility for method deopt
    void clearCode(jmethodID);
    void makeNoEntrant(jmethodID );
    void removeOSRNmethod(void* poolholder, jmethodID);

    void patchEntryPoint(void* entry,void* verEntry, void* dst);

}

// main  function
namespace mJNI {

    jclass findClass(const char* name);

    jfieldID findFieldID(jclass klass, const char* name, const char* sig);
    jmethodID findMethodID(jclass klass, const char* name, const char* sig);

    jobject getStaticObjectField(jclass klass, jfieldID fieldID);
    void setStaticObjectField(jclass klass, jfieldID fieldID, jobject var);

    jobject getObjectField(jobject object, jfieldID fieldID);
    void setObjectField(jobject object, jfieldID fieldID, jobject var);

    jclass getObjectClass(jobject object);

    jboolean instanceOfKlass(jclass klass1, jclass klass2);
    jboolean instanceOf(jobject object, jclass klass);
    jboolean isSameObject(jobject object1, jobject object2);

    jint getArrayLength(jobject array);

    std::string getClassName(jclass klass);

    void getObjectArrayElement(jobject oop, jobject* array, int start, int end);

    std::vector<uint8_t> getBytecode(jmethodID method);

    void deoptimization(jmethodID method);

    bool retransformMethod(jmethodID method,  std::vector<uint8_t>& newBytecode);

    // defl
    template<typename type>
    type getField(jobject object, jfieldID fieldID) {
        type var{};
        if (!object) return type {};
        mReadMemory(&var, (void*)((uintptr_t)object + (uintptr_t)fieldID), sizeof(var));
        return var;
    }

    template<typename type>
    void setField(jobject object, jfieldID fieldID, type var) {
        if (!object) return;
        mWriteMemory(&var, (void*)((uintptr_t)object + fieldID), sizeof(var));
    }

    // static
    template<typename type>
    type getStaticField(jclass klass, jfieldID fieldID) {
        type var{};
        if (!klass) return type {};
        mReadMemory(&var, (void*)((uintptr_t) mJNIUtilityFunctions::getClassOOP(klass) + fieldID), sizeof(var));
        return var;
    }

    template<typename type>
    void setStaticField(jclass klass, jfieldID fieldID, type var) {
        if (!klass) return;
        mWriteMemory(&var, (void*)((uintptr_t) mJNIUtilityFunctions::getClassOOP(klass) + fieldID), sizeof(var));
    }

    // for arrays
     template<typename type>
     void getArrayElement(jobject object, type* array, int start, int end) {

         bool isCompressedUse;
         mReadMemory(&isCompressedUse, (void*)Offsets::CompressedOops::useCompressedOops, sizeof(isCompressedUse));

         static auto base = isCompressedUse ? 0x10 : 0x18;
         auto length = getArrayLength(object);

         if ( start < 0 && start >= length ) {
             return;
         }

         if (end + start > length) {
             end = length - start;
         }

         mReadMemory(array, (void*)((uintptr_t)object + base + start * sizeof(*array)), end * sizeof(*array));
     }


    template<typename type>
    type setArrayElement(jobject object, type* array, int start, int end) {

        bool isCompressedUse;
        mReadMemory(&isCompressedUse, (void*)Offsets::CompressedOops::useCompressedOops, sizeof(isCompressedUse));

        static auto base = isCompressedUse ? 0x10 : 0x18;
        auto length = getArrayLength(object);

        if ( start < 0 && start >= length ) {
            return type{};
        }

        if (end + start > length) {
            end = length - start;
        }

        mWriteMemory(array, (void*)((uintptr_t)object + base + start * sizeof(*array)), end * sizeof(*array));
    }

    bool mJNIAttachToJVM(int PID);
}

#endif //MICOTIANEXTERNAL_MJNI_H