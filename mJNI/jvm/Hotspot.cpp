//
// Created by natty on 14.07.2026.
//

#include "Hotspot.h"

#include <vector>
#include <Windows.h>

#include "../memory/Memory.h"
#include "../utils/Utils.h"

static std::vector<ExportFunction> mExports; // g

HotspotVM::structMap HotspotVM::mStructMap;
HotspotVM::typeMap HotspotVM::mTypeMap;

HotspotVM::HotspotVM() {
    instance = this;
}

void HotspotVM::Initialize() {

    if (!parseStructs()) {
        return;
    }
}

std::optional<std::reference_wrapper<HotspotVM::structEntry>> HotspotVM::findTypeFields(const std::string &typeName) {
    auto it = mStructMap.find(typeName);
    if (it != mStructMap.end()) {
        return it->second;
    }
    return std::nullopt;
}

std::optional<std::reference_wrapper<VMTypeEntry>>
HotspotVM::findType(const std::string& typeName) {
    auto it = mTypeMap.find(typeName);
    if (it != mTypeMap.end()) {
        return std::ref(*it->second);
    }
    return std::nullopt;
}

int HotspotVM::JvmVersion() {

    auto strct = HotspotVM::findTypeFields("JDK_Version");
    if (!strct.has_value()) {
        static auto offset = findTypeFields("Abstract_VM_Version").value().get()["_vm_major_version"]->address;
        int version;
        mReadMemory(&version, offset, sizeof(version));

        return version;
    }

    auto current = strct.value().get()["_current"]->address;
    unsigned int majorOffset =  HotspotVM::findTypeFields("JDK_Version").value().get()["_major"]->offset;
    unsigned char version;
    mReadMemory(&version, (void*)((uintptr_t)current + majorOffset), sizeof(version));

    return version;
}


void * HotspotVM::findExport(const char *name) {
    for (auto& e : mExports) {
        if (e.name == name) {
            return (void*)((uintptr_t)gJvmDllAddress + e.address);
        }
    }
    return nullptr;
}

bool HotspotVM::parseStructs() {

    mExports = mJNI::Utils::copyExports(gJvmDllAddress);

    void* gHotSpotVMStructs = findExport("gHotSpotVMStructs");
    void* gHotSpotVMTypes = findExport("gHotSpotVMTypes");

    VMStructEntry entry;
    uintptr_t oRes;

    VMTypeEntry typeEntry;
    uintptr_t tRes;


    mReadMemory(&oRes, gHotSpotVMStructs, sizeof(oRes));
    mReadMemory(&tRes, gHotSpotVMTypes, sizeof(tRes));

    for (int i = 0; i < 788; i++) {

        uintptr_t structAddr = oRes + i * sizeof(VMStructEntry);
        mReadMemory(&entry, reinterpret_cast<void *>(structAddr), sizeof(VMStructEntry));

        if (entry.typeName == nullptr)
            break;

        char typeName[256] = { 0 };
        char fieldName[256] = { 0 };
        char typeString[256] = { 0 };

        mReadMemory(typeName, entry.typeName, sizeof(typeName) - 1);
        mReadMemory(fieldName, entry.fieldName, sizeof(fieldName) - 1);
        mReadMemory(typeString, entry.typeString, sizeof(typeString) - 1);

        VMStructEntry* out = new VMStructEntry();
        out->typeName = _strdup(typeName);
        out->fieldName = _strdup(fieldName);
        out->typeString = _strdup(typeString);
        out->isStatic = entry.isStatic;
        out->offset = entry.offset;
        out->address = entry.address;

        mStructMap[typeName][fieldName] = out;
    }

    for (int i = 0 ; i < 750; i++) {
        uintptr_t typeAddr = tRes + i * sizeof(VMTypeEntry);

        VMTypeEntry entry;
        mReadMemory(&entry, reinterpret_cast<void*>(typeAddr), sizeof(VMTypeEntry));

        char typeName[256] = { 0 };
        char superclassName[256] = { 0 };

        mReadMemory(typeName, entry.type_name, sizeof(typeName) - 1);
        if (entry.superclass_name) {
            mReadMemory(superclassName, entry.superclass_name, sizeof(superclassName) - 1);
        }

        VMTypeEntry* out = new VMTypeEntry();
        out->type_name = _strdup(typeName);
        out->superclass_name = entry.superclass_name ? _strdup(superclassName) : nullptr;
        out->is_oop_type = entry.is_oop_type;
        out->is_integer_type = entry.is_integer_type;
        out->is_unsigned = entry.is_unsigned;
        out->size = entry.size;

        mTypeMap[typeName] = out;
    }

    return true;
}