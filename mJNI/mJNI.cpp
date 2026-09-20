//
// Created by natty on 14.07.2026.
//

#include "mJNI.h"

#include "memory/Memory.h"

#include <unordered_map>
#include <string>
#include <iostream>
#include <mutex>


static bool compressedCached = false;
static bool isCompressedUse = false;
static uintptr_t narrowOopBase = 0;
static int narrowOopShift = 0;
static uintptr_t narrowKlassBase = 0;
static int narrowKlassShift = 0;

static void ensureCompressedConstants() {
    if (compressedCached) return;
    if (compressedCached) return;

    bool isUse = false;
    if (Offsets::CompressedOops::useCompressedOops) {
        mReadMemory(&isUse, (void*)Offsets::CompressedOops::useCompressedOops, sizeof(isUse));
    }
    isCompressedUse = isUse;

    if (isCompressedUse) {
        void* oopBase = nullptr;
        int oopShift = 0;
        if (Offsets::CompressedOops::narrowOopBase)
            mReadMemory(&oopBase, (void*)Offsets::CompressedOops::narrowOopBase, sizeof(oopBase));
        if (Offsets::CompressedOops::narrowOopShift)
            mReadMemory(&oopShift, (void*)Offsets::CompressedOops::narrowOopShift, sizeof(oopShift));
        narrowOopBase = (uintptr_t)oopBase;
        narrowOopShift = oopShift;

        void* kBase = nullptr;

        int kShift = 0;
        if (Offsets::CompressedKlassPointers::narrowKlassBase)
            mReadMemory(&kBase, (void*)((uintptr_t)Offsets::CompressedKlassPointers::narrowKlassBase), sizeof(kBase));
        if (Offsets::CompressedKlassPointers::narrowKlassShift)
            mReadMemory(&kShift, (void*)((uintptr_t)Offsets::CompressedKlassPointers::narrowKlassShift), sizeof(kShift));
        narrowKlassBase = (uintptr_t)kBase;
        narrowKlassShift = kShift;
    }
    compressedCached = true;
}

jclass mJNI::findClass(const char * klassName)
{
    void* head;
    if (!mReadMemory(&head, (void*)Offsets::ClassLoaderData::head, sizeof(head)))
        return nullptr;

    while (head != nullptr) {
        void* klasses;
        if (mReadMemory(&klasses, (void*)(uintptr_t(head) + Offsets::ClassLoaderData::klasses), sizeof(klasses))) {
            while (klasses != nullptr) {
                std::string name = mJNIUtilityFunctions::readSymbol( (void*)((uintptr_t)klasses + Offsets::InstanceKlass::name));

                if (name == klassName) {
                    return (jclass)klasses;
                }

                klasses = readPointer((void*)((uintptr_t)klasses + Offsets::InstanceKlass::nextLink));
            }
        }
        head = readPointer((void*)((uintptr_t)head + Offsets::ClassLoaderData::next));
    }
    return nullptr;
}

jfieldID mJNI::findFieldID(jclass klass, const char *name, const char *sig) {
    if (!klass || !name) return 0;

    void* constants = nullptr;
    mReadMemory(&constants,
                (void*)((uintptr_t)klass + Offsets::InstanceKlass::constants),
                sizeof(constants));
    if (!constants) return 0;

    uintptr_t baseAddr = (uintptr_t)constants + Offsets::ConstantPool::constPoolSize;
    jfieldID fallbackOffset = 0;

    // 21
    if (Offsets::InstanceKlass::fieldsInfoStream != 0) {
        std::vector<FieldInfo20> fields;
        void* fieldInfo = readPointer(
            (void*)((uintptr_t)klass + Offsets::InstanceKlass::fieldsInfoStream));

        if (fieldInfo && mJNIUtilityFunctions::copyFieldInfo(fieldInfo, fields)) {
            for (size_t i = 0; i < fields.size(); i++) {
                FieldInfo20 field = fields.at(i);

                std::string fName = mJNIUtilityFunctions::readSymbol(
                    (void*)(baseAddr + field.name_index() * sizeof(intptr_t)));

                if (fName == name) {
                    if (!sig || sig[0] == '\0') {
                        return field.offset();
                    }

                    std::string fDesc = mJNIUtilityFunctions::readSymbol(
                        (void*)(baseAddr + field.signature_index() * sizeof(intptr_t)));

                    if (fDesc == sig) {
                        return field.offset();
                    }
                    fallbackOffset = field.offset();
                }
            }
        }
    }
    // 17
    else {
        FieldInfo17 fieldInfo {};
        uint16_t fieldCount = 0, i = 0;
        void* fields = nullptr;

        mReadMemory(&fields,(void*)((uintptr_t)klass + Offsets::InstanceKlass::fields),sizeof(fields));
        mReadMemory(&fieldCount,(void*)((uintptr_t)klass + Offsets::InstanceKlass::fieldsCount),sizeof(fieldCount));

        if (fields) {
            fields = (void*)((uintptr_t)fields + sizeof(jint));
            static unsigned long long usLen = sizeof(unsigned short);

            for (i = 0; i < fieldCount; i++) {
                mReadMemory(&fieldInfo,(void*)((uintptr_t)fields + i * FieldInfo17::field_slots * usLen),sizeof(fieldInfo));

                std::string fName = mJNIUtilityFunctions::readSymbol((void*)(baseAddr + fieldInfo.nameIndex() * 8));

                if (fName == name) {
                    if (!sig || sig[0] == '\0') {
                        return fieldInfo.offset();
                    }

                    std::string fDesc = mJNIUtilityFunctions::readSymbol( (void*)(baseAddr + fieldInfo.signatureIndex() * 8));

                    if (fDesc == sig) {
                        return fieldInfo.offset();
                    }
                    fallbackOffset = fieldInfo.offset();
                }
            }
        }
    }
    return fallbackOffset;
}
jmethodID mJNI::findMethodID(jclass klass, const char *name, const char *sig) {
    if (!klass) return nullptr;

    void* constants = nullptr;
    mReadMemory(&constants,(void*)((uintptr_t)klass + Offsets::InstanceKlass::constants),sizeof(constants));
    if (!constants) return nullptr;

    uintptr_t baseAddr = (uintptr_t)constants + Offsets::ConstantPool::constPoolSize;

    void* pMethods = nullptr;
    mReadMemory(&pMethods,(void*)((uintptr_t)klass + Offsets::InstanceKlass::methods),sizeof(pMethods));
    if (!pMethods) return nullptr;

    int methodsCount = 0;
    mReadMemory(&methodsCount, pMethods, sizeof(methodsCount));

    void* methodsArray = (void*)((uintptr_t)pMethods + sizeof(void*));

    for (int i = 0; i < methodsCount; i++) {
        void* methodPtr = nullptr;
        mReadMemory(&methodPtr,(void*)((uintptr_t)methodsArray + i * sizeof(void*)),sizeof(methodPtr));

        void* constMethod = nullptr;
        mReadMemory(&constMethod,(void*)((uintptr_t)methodPtr + Offsets::Method::constMethod),sizeof(constMethod));

        uint16_t nameIndex = 0, sigIndex = 0;
        mReadMemory(&nameIndex,(void*)((uintptr_t)constMethod + Offsets::ConstMethod::nameIndex),sizeof(nameIndex));
        mReadMemory(&sigIndex, (void*)((uintptr_t)constMethod + Offsets::ConstMethod::signatureIndex), sizeof(sigIndex));

        std::string nameSymbol = mJNIUtilityFunctions::readSymbol((void*)(baseAddr + nameIndex * sizeof(intptr_t)));
        std::string sigSymbol = mJNIUtilityFunctions::readSymbol( (void*)(baseAddr + sigIndex * sizeof(intptr_t)));

        if (nameSymbol == name && sigSymbol == sig) {
            return methodPtr;
        }
    }
    return nullptr;
}


jobject mJNI::getStaticObjectField(jclass klass, jfieldID fieldID) {
    if (!klass) return nullptr;
    jobject mirrorKlass = mJNIUtilityFunctions::getClassOOP(klass);

    uint32_t compressed = 0;
    mReadMemory(&compressed, (void*)((uintptr_t)mirrorKlass + (uintptr_t)fieldID), sizeof(uint32_t));

    auto pCompressed = (void*)compressed;
    return mJNIUtilityFunctions::decodeOop((jobject)pCompressed);
}

void mJNI::setStaticObjectField(jclass klass, jfieldID fieldID, jobject var) {
    if (!klass || !fieldID) return;
    jobject mirrorKlass = mJNIUtilityFunctions::getClassOOP(klass);
    if (!mirrorKlass) return;

    uint32_t compressed = static_cast<uint32_t>((uintptr_t)mJNIUtilityFunctions::encodeOop(var));

    void* pField = (void*)((uintptr_t)mirrorKlass + (uintptr_t)fieldID);

    mWriteMemory(pField, &compressed, sizeof(uint32_t));
}

jobject mJNI::getObjectField(jobject object, jfieldID fieldID) {

    jobject value = nullptr;

    mReadMemory(&value, (void*)((uintptr_t)object + fieldID), sizeof(uint32_t));
    value = mJNIUtilityFunctions::decodeOop(value);
    return value;
}

void mJNI::setObjectField(jobject object, jfieldID fieldID, jobject var) {

    var = mJNIUtilityFunctions::encodeOop(var);
    mWriteMemory(&var, (void*)((uintptr_t)object + fieldID), sizeof(uint32_t));
}

jclass mJNI::getObjectClass(jobject object) {
    if (!object) return nullptr;

    ensureCompressedConstants();

    jclass rKlass = nullptr;
    if (isCompressedUse) {
        uint32_t compressed = 0;
        mReadMemory(&compressed, (void*)((uintptr_t)object + 0x8), sizeof(uint32_t));

        uintptr_t kAddress = ((uintptr_t) compressed << narrowKlassShift) + narrowKlassBase;
        return (jclass)kAddress;
    }
    else {
        mReadMemory(&rKlass, (void*)((uintptr_t)object + 0x8), sizeof(jclass));
        return rKlass;
    }
}

jboolean mJNI::instanceOfKlass(jclass klass1, jclass klass2) {

    if (klass1 == klass2) return true;

    void* superKlass;
    mReadMemory(&superKlass, (void*)((uintptr_t)klass1 + Offsets::InstanceKlass::superKlass), sizeof(superKlass));

    if (superKlass == klass2) return true;

    return false;

}

jboolean mJNI::instanceOf(jobject object, jclass klass) {

    jclass objKlass = getObjectClass(object);

    if (!objKlass) return false;

    if (objKlass == klass) return true;
    jclass current = objKlass;

    int maxDepth = 50;

    while (current && maxDepth-- > 0) {

        auto name = mJNI::getClassName(current);

        jclass superKlass = nullptr;
        if (!mReadMemory(&superKlass, (void*)((uintptr_t)current + Offsets::InstanceKlass::superKlass), sizeof(superKlass))) {
            break;
        }
        if (!superKlass) {
            return false;
        }

        if (superKlass == klass) {
            return true;
        }
        if (superKlass == current) {
            break;
        }
        current = superKlass;
    }
    return false;

}

jboolean mJNI::isSameObject(jobject object1, jobject object2) {
    if (object1 == object2) return true;
    return false;
}

jint mJNI::getArrayLength(jobject array) {
    bool isCompressedUse;
    mReadMemory(&isCompressedUse, (void*)Offsets::CompressedOops::useCompressedOops, sizeof(isCompressedUse));

    static auto f = isCompressedUse ? 0xC : 0x10;

    int length;
    mReadMemory(&length, (void*)((uintptr_t)array + f), sizeof(length));

    return length;
}

std::string mJNI::getClassName(jclass klass) {
    if (!klass) return "";

    return mJNIUtilityFunctions::readSymbol(
        (void*)((uintptr_t)klass + Offsets::InstanceKlass::name));
}

void mJNI::getObjectArrayElement(jobject oop, jobject*array, int start, int end) {
    bool isCompressedUse;
    mReadMemory(&isCompressedUse, (void*)Offsets::CompressedOops::useCompressedOops, sizeof(isCompressedUse));

    int base = isCompressedUse ? 0x10 : 0x18;
    jint length = getArrayLength(oop);
    end += start;

    for ( int i = start; i < end && i < length; i++ ) {
        uint32_t val = 0;
        mReadMemory(&val, (void*)(uintptr_t(oop) + base + i * sizeof(unsigned int)), sizeof(uint32_t));
        array[i - start] = mJNIUtilityFunctions::decodeOop((jobject)(uintptr_t)val);
    }
}

std::vector<uint8_t> mJNI::getBytecode(jmethodID method) {

    std::vector<uint8_t> bytecode;

    if (!method) return bytecode;

    void* constMethod = nullptr ;
    mReadMemory(&constMethod, (void*)((uintptr_t)method + Offsets::Method::constMethod), sizeof(constMethod));

    uint16_t bytecodeSize = 0;
    mReadMemory(&bytecodeSize, (void*)((uintptr_t)constMethod + Offsets::ConstMethod::codeSize), sizeof(bytecodeSize));

    size_t sizeConstMethod = Offsets::ConstMethod::constMethodSize;
    void* bytecodeAddress = (void*)((uintptr_t)constMethod + sizeConstMethod);

    bytecode.resize(bytecodeSize);
    if (!mReadMemory(bytecode.data(), bytecodeAddress, bytecodeSize)) {
        bytecode.clear();
    }

    return bytecode;
}

void mJNI::deoptimization(jmethodID method) {
    void* compileCode = nullptr ;
    mReadMemory(&compileCode, (void*)((uintptr_t)method + Offsets::Method::code), sizeof(compileCode));

    if (compileCode == nullptr) {
        return;
    }

    mJNIUtilityFunctions::makeNoEntrant(method);
    mJNIUtilityFunctions::clearCode(method);

}

bool mJNI::retransformMethod(jmethodID method, std::vector<uint8_t> &newBytecode) {
    if (!method) return false;

    void* constMethod = nullptr;
    mReadMemory(&constMethod, (void*)((uintptr_t)method + Offsets::Method::constMethod), sizeof(constMethod));

    uint16_t codeSize = 0;
    mReadMemory(&codeSize, (void*)((uintptr_t)constMethod + Offsets::ConstMethod::codeSize), sizeof(codeSize));

    if (newBytecode.size() > codeSize) {
        return false;
    }

    size_t sizeConstMethod = Offsets::ConstMethod::constMethodSize;

    void* bytecodeAddress = (void*)((uintptr_t)constMethod + sizeConstMethod);

    if (!mJNIUtilityFunctions::writeBytecode(bytecodeAddress, newBytecode.data(), newBytecode.size())) {
        return false;
    }

    if (newBytecode.size() < codeSize) {
        std::vector<uint8_t> nops(codeSize - newBytecode.size(), 0x00);
        auto* const address = reinterpret_cast<uint8_t*>(bytecodeAddress) + newBytecode.size();
        mJNIUtilityFunctions::writeBytecode(address, nops.data(), nops.size());
    }

    deoptimization(method);

    return true;
}


bool mJNI::mJNIAttachToJVM(int PID) {
    DWORD pid = (PID != 0) ? PID : Utils::getPidFromProcessName("javaw.exe");
    if (!pid) {
        pid = Utils::getPidFromProcessName("java.exe");
    }

    auto handle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
    if (!handle) {
        return false;
    }

    mAttachToJvm(handle);

    HotspotVM::instance->Initialize();
    Offsets::InitializeOffsets();

    return true;
}

jclass mJNIUtilityFunctions::decodeKlass(jclass klass) {
    void* base;
    int shift;
    bool isCompressedUse;
    mReadMemory(&isCompressedUse, (void*)Offsets::CompressedOops::useCompressedOops, sizeof(isCompressedUse));

    auto dKlass = (uintptr_t)klass;

    if (isCompressedUse) {
        mReadMemory(&base, (void*)Offsets::CompressedKlassPointers::narrowKlassBase, sizeof(base));
        mReadMemory(&shift, (void*)Offsets::CompressedKlassPointers::narrowKlassShift, sizeof(shift));

        dKlass <<= shift;
        dKlass += (uintptr_t)base;
    }

    return (jclass)dKlass;
}

jobject mJNIUtilityFunctions::decodeOop(jobject object) {
    auto dObj = (uintptr_t)object;
    if (dObj == 0) return nullptr;

    ensureCompressedConstants();

    if (isCompressedUse) {
        dObj <<= narrowOopShift;
        dObj += narrowOopBase;
    }
    return (jobject)dObj;
}

jobject mJNIUtilityFunctions::encodeOop(jobject object) {
    auto dObj = (uintptr_t)object;
    if (dObj == 0) return nullptr;

    ensureCompressedConstants();

    if (isCompressedUse) {
        if (dObj >= narrowOopBase) {
            dObj -= narrowOopBase;
            dObj >>= narrowOopShift;
        } else {
            return nullptr;
        }
    }
    return (jobject)dObj;
}


uint32_t mJNIUtilityFunctions::readU5( const uint8_t *data, size_t &pos) {

    uint32_t sum = data[pos++] - 1;
    if (sum < 191) return sum;

    int shift = 6;
    while (true) {
        uint8_t b = data[pos++];
        sum += (b - 1) << shift;
        if (b < (1 + 191)) break;
        shift += 6;
    }
    return sum;
}

void mJNIUtilityFunctions::readFieldInfoStream(FieldInfo20& out, const uint8_t *buffer, size_t &pos) {
    out._index = 0;
    out._name_index = static_cast<uint16_t>(readU5(buffer, pos));
    out._signature_index = static_cast<uint16_t>(readU5(buffer, pos));
    out._offset = readU5(buffer, pos);
    out._access_flags = AccessFlags(static_cast<uint16_t>(readU5(buffer, pos)));
    out._field_flags = FieldFlags(readU5(buffer, pos));
    if (out._field_flags.is_initialized()) {
        out._initializer_index = static_cast<uint16_t>(readU5(buffer, pos));
    }
    else {
        out._initializer_index = 0;
    }
    if (out._field_flags.is_generic()) {
        out._generic_signature_index = static_cast<uint16_t>(readU5(buffer, pos));
    }
    else {
        out._generic_signature_index = 0;
    }
    if (out._field_flags.is_contended()) {
        out._contention_group = static_cast<uint16_t>(readU5(buffer, pos));
    }
    else {
        out._contention_group = 0;
    }
}

int mJNIUtilityFunctions::copyFieldInfo(void *address, std::vector<FieldInfo20> &outFields) {
    int length;
    if (!mReadMemory(&length, (LPCVOID)address, sizeof(length)))
        return false;

    std::vector<uint8_t> buffer(length);
    if (!mReadMemory(buffer.data(), (LPCVOID)((uintptr_t)address + sizeof(int)), length))
        return false;

    size_t pos = 0;
    uint32_t java_count = readU5(buffer.data(), pos);
    uint32_t injected_count = readU5(buffer.data(), pos);
    uint32_t total = java_count + injected_count;

    outFields.clear();
    for (uint32_t i = 0; i < total; ++i) {
        FieldInfo20 field {};
        readFieldInfoStream(field, buffer.data(), pos);
        outFields.push_back(field);
    }

    return true;
}

bool mJNIUtilityFunctions::writeBytecode(void *address,  void *buffer, size_t size) {
    SIZE_T bytes_written = 0;
    DWORD old_protect = 0;

    VirtualProtectEx(gJvmHandle, (LPVOID)address, size, PAGE_EXECUTE_READWRITE, &old_protect);

    bool res = WriteMem(gJvmHandle, buffer, address, size, &bytes_written)
               && bytes_written == size;

    VirtualProtectEx(gJvmHandle, (LPVOID)address, size, old_protect, &old_protect);

    return res;
}


std::string mJNIUtilityFunctions::readSymbol(void *address) {
    if (!address) return "";

    void* symbolPtr = readPointer(address);
    if (!symbolPtr) return "";

    uint16_t length = 0;
    if (!mReadMemory(&length,(void*)((uintptr_t)symbolPtr + Offsets::Symbol::length),sizeof(length))) {
        return "";
    }

    if (length == 0 || length > 0x1000) return "";

    std::vector<char> buf(length);
    if (!mReadMemory(buf.data(), (void*)((uintptr_t)symbolPtr + Offsets::Symbol::body), length)) {
        return "";
    }

    return std::string(buf.data(), length);
}

jobject mJNIUtilityFunctions::getClassOOP(jclass klass) {
    if (!klass) return nullptr;
    void* mirror = (void*)((uintptr_t)klass + Offsets::InstanceKlass::javaMirror);

    uintptr_t jMirrorPtr = 0;
    mReadMemory(&jMirrorPtr, mirror, sizeof(uintptr_t));
    if (!jMirrorPtr) return nullptr;

    uintptr_t jObject = 0;
    mReadMemory(&jObject, (void*)jMirrorPtr, sizeof(uintptr_t));

    return (jobject)jObject;
}

void mJNIUtilityFunctions::clearCode(jmethodID method) {

    void* adapter = nullptr;
    mReadMemory(&adapter, (void*)((uintptr_t)method + Offsets::Method::adapter), sizeof(adapter));

    void* c2iEntry = nullptr;
    mReadMemory(&c2iEntry, (void*)((uintptr_t)adapter + Offsets::AdapterHandlerEntry::c2iEntry), sizeof(c2iEntry));

    if (c2iEntry != nullptr) {
        mWriteMemory(&c2iEntry, (void*)((uintptr_t)method + Offsets::Method::fromCompiledEntry), sizeof(c2iEntry));
    }

    void* i2i = nullptr;
    mReadMemory(&i2i, (void*)((uintptr_t)method + Offsets::Method::i2iEntry), sizeof(i2i));

    if (i2i != nullptr) {
        mWriteMemory(&i2i, (void*)((uintptr_t)method + Offsets::Method::i2iEntry), sizeof(i2i));
    }

    uintptr_t zero = 0;
    mWriteMemory(&zero, (void*)((uintptr_t)method + Offsets::Method::code), sizeof(zero));

}

void mJNIUtilityFunctions::removeOSRNmethod(void *poolholder, jmethodID method) {
    void* prev = nullptr;
    void* current = nullptr;
    mReadMemory(&current, (void*)((uintptr_t)poolholder + Offsets::InstanceKlass::osrNmethodsHead), sizeof(current));

    while (current != nullptr) {
        if (current == method) {
            void* next = nullptr;

            mReadMemory(&next, (void*)((uintptr_t)current + Offsets::nmethod::osrLink), sizeof(next));

            if (prev == nullptr) {
                mWriteMemory(&next,(void*)((uintptr_t)poolholder + Offsets::InstanceKlass::osrNmethodsHead),sizeof(next));
            }
            else {
                mWriteMemory(&next,(void*)((uintptr_t)prev + Offsets::nmethod::osrLink),sizeof(next));
            }
            break;

        }
        prev = current;
        mReadMemory(&current, (void*)((uintptr_t)current + Offsets::nmethod::osrLink), sizeof(current));
    }

}

void mJNIUtilityFunctions::patchEntryPoint(void* entry, void* verEntry, void* dst) {
    if (!verEntry || !dst) return;

    const uintptr_t verAddr = (uintptr_t)verEntry;
    const uintptr_t dstAddr = (uintptr_t)dst;
    const int32_t rel32 = static_cast<int32_t>(dstAddr - (verAddr + 5));

    uint8_t patch[5];
    patch[0] = 0xE9;
    std::memcpy(&patch[1], &rel32, sizeof(rel32));

    if (!mWriteMemory(patch, (void*)verAddr, 5)) {
        return;
    }

    FlushInstructionCache(gJvmHandle, (LPVOID)verAddr, 5);
}

void mJNIUtilityFunctions::makeNoEntrant(jmethodID method) {

    if (!method) return;

    void* compiledCode = nullptr;
    mReadMemory(&compiledCode, (void*)((uintptr_t)method + Offsets::Method::code), sizeof(compiledCode));

    uint8_t state = 0;
    mReadMemory(&state, (void*)((uintptr_t)compiledCode + Offsets::nmethod::state), sizeof(state));

    int entryBCI  = 0 ;
    mReadMemory(&entryBCI, (void*)((uintptr_t)compiledCode + Offsets::nmethod::entryBCI), sizeof(entryBCI));

    bool isOsrMethod = (entryBCI != InvocationEntryBci);
    bool isInUse = (state <= in_use);
    bool isNoEntrant = (state == not_entrant);


    if (isOsrMethod && isInUse) {

        void* constMethod = nullptr;
        mReadMemory(&constMethod, (void*)((uintptr_t)method + Offsets::Method::constMethod), sizeof(constMethod));

        void* constants = nullptr;
        mReadMemory(&constants, (void*)((uintptr_t)constMethod + Offsets::ConstMethod::constants), sizeof(constants));

        void* poolHolder = nullptr;
        mReadMemory(&poolHolder, (void*)((uintptr_t)constants + Offsets::ConstantPool::poolHolder), sizeof(poolHolder));

        removeOSRNmethod(poolHolder, compiledCode);
    }
    if (!isOsrMethod && !isNoEntrant) {
        //* nmethod = nullptr;
        ///mReadMemory(&nmethod, (void*)((uintptr_t)method + Offsets::Method::code), sizeof(nmethod));

        void* entryPoint = nullptr;
        void* verifiedEntry = nullptr;

        mReadMemory(&entryPoint, (void*)((uintptr_t)compiledCode + Offsets::nmethod::entryPoint), sizeof(entryPoint));
        mReadMemory(&verifiedEntry, (void*)((uintptr_t)compiledCode + Offsets::nmethod::verifiedEntryPoint), sizeof(verifiedEntry));

        void* stub = nullptr;
        mReadMemory(&stub, (void*)(Offsets::SharedRuntime::wrongMethodBlob), sizeof(stub));

        void* dst = nullptr;
        mReadMemory(&dst, (void*)((uintptr_t)stub + Offsets::CodeBlob::codeBegin), sizeof(dst));

        patchEntryPoint(entryPoint, verifiedEntry, dst);

    }

}
