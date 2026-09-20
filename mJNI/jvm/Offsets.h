//
// Created by natty on 14.07.2026.
//

#ifndef MICOTIANEXTERNAL_OFFSETS_H
#define MICOTIANEXTERNAL_OFFSETS_H
#include "Hotspot.h"


namespace Offsets {
    namespace InstanceKlass {
        inline uint64_t name ;
        inline uint64_t nextLink ;
        inline uint64_t constants;
        inline uint64_t methods ;
        inline uint64_t fields;
        inline uint64_t fieldsInfoStream ;
        inline uint64_t fieldsCount;
        inline uint64_t javaMirror;
        inline uint64_t superKlass;
        inline uint64_t subKlass;
        inline uint64_t localInterfaces;
        inline uint64_t osrNmethodsHead;
    }

    namespace Method {
        inline uint64_t code ;
        inline uint64_t i2iEntry;
        inline uint64_t fromCompiledEntry;
        inline uint64_t fromInterpretedEntry;
        inline uint64_t constMethod;
        inline uint64_t adapter; // shit n1
    }

    namespace AdapterHandlerEntry {
        inline uint64_t c2iEntry; // shit n2
    }

    namespace ConstMethod {
        inline uint64_t nameIndex;
        inline uint64_t signatureIndex;
        inline uint64_t codeSize;
        inline uint64_t constMethodSize;
        inline uint64_t constants;

    }


    namespace CompileMethod {
        inline uint64_t method;
    }

    namespace nmethod {
        inline uint64_t entryPoint;
        inline uint64_t verifiedEntryPoint;
        inline uint64_t osrEntryPoint;
        inline uint64_t state;
        inline uint64_t osrLink;
        inline uint64_t compileID;
        inline uint64_t compileLVL;
        inline uint64_t entryBCI;

    }

    namespace ClassLoaderData {

        inline uint64_t klasses;
        inline uint64_t head;
        inline uint64_t next;

    }
    namespace CompressedOops {
        inline uint64_t narrowOopBase;
        inline uint64_t narrowOopShift;
        inline uint64_t useCompressedOops;

    }
    namespace ConstantPool {
        inline uint64_t length;
        inline uint64_t base;
        inline uint64_t constPoolSize;
        inline uint64_t poolHolder;

    }
    namespace CompressedKlassPointers {
        inline uint64_t narrowKlassBase;
        inline uint64_t narrowKlassShift;
    }

    namespace Symbol {
        inline uint64_t body;
        inline uint64_t length;
    }

    namespace CodeBlob {
        inline int64_t codeBegin;
        inline int64_t codeEnd;
    }

    namespace SharedRuntime {
        inline uint64_t wrongMethodBlob;

    }


    void InitializeOffsets();
}

#endif //MICOTIANEXTERNAL_OFFSETS_H
