#pragma once

#include <cstdint>

#include <string>

#include "Offsets.h"
#include "../memory/Memory.h"

typedef uint16_t u2;
typedef uint8_t u1;
typedef uint32_t u4;
typedef uint64_t u8;

typedef const void* jobject;
typedef const void* jclass;
typedef const void* jmethodID;

typedef int jint;
typedef double jdouble;
typedef float jfloat;
typedef unsigned char jboolean;
typedef unsigned char jbyte;
typedef unsigned char jchar;
typedef long jlong;
typedef unsigned short jfieldID;

typedef jobject jobjectArray;


class AccessFlags {
public:
    uint16_t mFlags;
    AccessFlags(uint16_t flags) : mFlags(flags) {}

    AccessFlags() : mFlags(0) {}
};



// FIELDS
#define FIELDINFO_TAG_SIZE             2
#define FIELDINFO_TAG_OFFSET           1 << 0
#define FIELDINFO_TAG_CONTENDED        1 << 1

class FieldInfo17
{
public:
    FieldInfo17() = default;

    inline int buildIntFromShorts(uint16_t low, uint16_t high)
    {
        return ((int)((unsigned int)high << 16) | (unsigned int)low);
    }

    enum FieldOffset
    {
        access_flags_offset = 0,
        name_index_offset = 1,
        signature_index_offset = 2,
        initval_index_offset = 3,
        low_packed_offset = 4,
        high_packed_offset = 5,
        field_slots = 6
    };

    uint16_t nameIndex() const { return _shorts[name_index_offset]; }
    uint16_t signatureIndex() const { return _shorts[signature_index_offset]; }
    uint16_t accessFlags() const { return _shorts[access_flags_offset]; }
    uint16_t initvalIndex() const {return _shorts[initval_index_offset];
    }

    // static FieldInfo17* fromFieldArray(Array<uint16_t>* fields, int index) {
    //     return reinterpret_cast<FieldInfo17 *>(fields->adr_at(index * field_slots));
    // }

    uint16_t offset()
    {
        return buildIntFromShorts(_shorts[low_packed_offset], _shorts[high_packed_offset]) >> FIELDINFO_TAG_SIZE;
    }



private:
    uint16_t _shorts[field_slots];
};


// specify 20 JVM
static constexpr uint32_t flag_mask(int pos) {
    return static_cast<uint32_t>(1) << pos;
};

class FieldFlags {
public:

    enum FieldFlagBitPosition {
        _ff_initialized,  // has ConstantValue initializer attribute
        _ff_injected,     // internal field injected by the JVM
        _ff_generic,      // has a generic signature
        _ff_stable,       // trust as stable b/c declared as @Stable
        _ff_contended,    // is contended, may have contention-group
      };

    uint32_t _flags;

    FieldFlags(uint32_t flags) {
        _flags = flags;
    }
    FieldFlags() : _flags(0) {}

    static const uint32_t _optional_item_bit_mask =
    flag_mask((int)_ff_initialized) |
    flag_mask((int)_ff_generic)     |
    flag_mask((int)_ff_contended);

    [[nodiscard]] bool test_flag(FieldFlagBitPosition pos) const {
        return (_flags & flag_mask(pos)) != 0;
    }

    void update_flag(FieldFlagBitPosition pos, bool z) {
        if (z)    _flags |=  flag_mask(pos);
        else      _flags &= ~flag_mask(pos);
    }

    [[nodiscard]] bool is_initialized() const     { return test_flag(_ff_initialized); }
    [[nodiscard]] bool is_injected() const        { return test_flag(_ff_injected); }
    [[nodiscard]] bool is_generic() const         { return test_flag(_ff_generic); }
    [[nodiscard]] bool is_stable() const          { return test_flag(_ff_stable); }
    [[nodiscard]] bool is_contended() const       { return test_flag(_ff_contended); }

    void update_initialized(bool z) { update_flag(_ff_initialized, z); }
    void update_injected(bool z)    { update_flag(_ff_injected, z); }
    void update_generic(bool z)     { update_flag(_ff_generic, z); }
    void update_stable(bool z)      { update_flag(_ff_stable, z); }
    void update_contended(bool z)   { update_flag(_ff_contended, z); }

};

class FieldInfo20 {
public:
    uint32_t _index = 0;
    uint16_t _name_index = 0;
    uint16_t _signature_index = 0;
    uint32_t _offset = 0;
    AccessFlags _access_flags;
    FieldFlags _field_flags;
    uint16_t _initializer_index = 0;
    uint16_t _generic_signature_index = 0;
    uint16_t _contention_group = 0;

    FieldInfo20() = default;

    [[nodiscard]] uint16_t name_index() const { return _name_index; }
    [[nodiscard]] uint16_t signature_index() const { return _signature_index; }
    [[nodiscard]] int offset() const { return _offset; }
};

enum MethodCompilation {
    InvocationEntryBci   = -1,     // i.e., not a on-stack replacement compilation
    BeforeBci            = InvocationEntryBci,
    AfterBci             = -2,
    UnwindBci            = -3,
    AfterExceptionBci    = -4,
    UnknownBci           = -5,
    InvalidFrameStateBci = -6
  };

// Enumeration to distinguish tiers of compilation
enum CompLevel : char {
    CompLevel_any               = -1,        // Used for querying the state
    CompLevel_all               = -1,        // Used for changing the state
    CompLevel_none              = 0,         // Interpreter
    CompLevel_simple            = 1,         // C1
    CompLevel_limited_profile   = 2,         // C1, invocation & backedge counters
    CompLevel_full_profile      = 3,         // C1, invocation & backedge counters + mdo
    CompLevel_full_optimization = 4          // C2 or JVMCI
};

enum {
    not_installed = -1, // in construction, only the owner doing the construction is
                           // allowed to advance state
    in_use        = 0,  // executable nmethod
    not_used      = 1,  // not entrant, but revivable
    not_entrant   = 2,  // marked for deoptimization but activations may still exist
};