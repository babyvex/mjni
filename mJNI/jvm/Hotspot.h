//
// Created by natty on 14.07.2026.
//

#ifndef MICOTIANEXTERNAL_HOTSPOT_H
#define MICOTIANEXTERNAL_HOTSPOT_H
#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>
#include <functional>

struct VMStructEntry {
    const char* typeName;
    const char* fieldName;
    const char* typeString;
    int32_t  isStatic;
    uint64_t offset;
    void* address;
};

struct VMTypeEntry {
    const char* type_name;
    const char* superclass_name;
    int32_t is_oop_type;
    int32_t is_integer_type;
    int32_t is_unsigned;
    uint64_t size;
};


struct HotspotVM {
private:
    void* findExport(const char* name);
    bool parseStructs();

    using structEntry = std::unordered_map<std::string, VMStructEntry*>;
    using structMap = std::unordered_map<std::string, structEntry>;

    using typeMap = std::unordered_map<std::string, VMTypeEntry*>;

    static structMap mStructMap;
    static typeMap mTypeMap;

public:

    static inline HotspotVM* instance = nullptr;

    HotspotVM();

    void Initialize();

    static std::optional<std::reference_wrapper<structEntry>> findTypeFields(const std::string& typeName);
    static std::optional<std::reference_wrapper<VMTypeEntry>> findType(const std::string& typeName);

    static int JvmVersion();
};




#endif //MICOTIANEXTERNAL_HOTSPOT_H
