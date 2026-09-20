#include "Offsets.h"

void Offsets::InitializeOffsets() {

     InstanceKlass::name = HotspotVM::findTypeFields("Klass").value().get()["_name"]->offset;
     InstanceKlass::nextLink = HotspotVM::findTypeFields("Klass").value().get()["_next_link"]->offset;
     InstanceKlass::constants = HotspotVM::findTypeFields("InstanceKlass").value().get()["_constants"]->offset;
     InstanceKlass::methods = HotspotVM::findTypeFields("InstanceKlass").value().get()["_methods"]->offset;
     InstanceKlass::fields = (HotspotVM::JvmVersion() <= 17 ? HotspotVM::findTypeFields("InstanceKlass").value().get()["_fields"]->offset : 0);
     InstanceKlass::fieldsInfoStream = (HotspotVM::JvmVersion() >= 21 ? HotspotVM::findTypeFields("InstanceKlass").value().get()["_fieldinfo_stream"]->offset : 0);
     InstanceKlass::fieldsCount = (HotspotVM::JvmVersion() <= 17 ? HotspotVM::findTypeFields("InstanceKlass").value().get()["_java_fields_count"]->offset : 0);
     InstanceKlass::javaMirror = HotspotVM::findTypeFields("Klass").value().get()["_java_mirror"]->offset;
     InstanceKlass::superKlass = HotspotVM::findTypeFields("Klass").value().get()["_super"]->offset;
     InstanceKlass::subKlass = HotspotVM::findTypeFields("Klass").value().get()["_subklass"]->offset;
     InstanceKlass::localInterfaces = HotspotVM::findTypeFields("InstanceKlass").value().get()["_local_interfaces"]->offset;
     InstanceKlass::osrNmethodsHead = HotspotVM::findTypeFields("InstanceKlass").value().get()["_osr_nmethods_head"]->offset;

     Method::code = HotspotVM::findTypeFields("Method").value().get()["_code"]->offset;
     Method::i2iEntry = HotspotVM::findTypeFields("Method").value().get()["_i2i_entry"]->offset;
     Method::fromCompiledEntry = HotspotVM::findTypeFields("Method").value().get()["_from_compiled_entry"]->offset;
     Method::fromInterpretedEntry = HotspotVM::findTypeFields("Method").value().get()["_from_interpreted_entry"]->offset;
     Method::constMethod = HotspotVM::findTypeFields("Method").value().get()["_constMethod"]->offset;
     Method::adapter = HotspotVM::findTypeFields("Method").value().get()["_method_counters"]->offset + 0x8;

     AdapterHandlerEntry::c2iEntry = (HotspotVM::JvmVersion() > 17 ? 0x10 : 0x20);

     ConstMethod::nameIndex = HotspotVM::findTypeFields("ConstMethod").value().get()["_name_index"]->offset;
     ConstMethod::signatureIndex = HotspotVM::findTypeFields("ConstMethod").value().get()["_signature_index"]->offset;
     ConstMethod::codeSize = HotspotVM::findTypeFields("ConstMethod").value().get()["_code_size"]->offset;
     ConstMethod::constants = HotspotVM::findTypeFields("ConstMethod").value().get()["_constants"]->offset;
     ConstMethod::constMethodSize = HotspotVM::findType("ConstMethod").value().get().size;

     ClassLoaderData::head = (uint64_t)HotspotVM::findTypeFields("ClassLoaderDataGraph").value().get()["_head"]->address;
     ClassLoaderData::klasses = HotspotVM::findTypeFields("ClassLoaderData").value().get()["_klasses"]->offset;
     ClassLoaderData::next = HotspotVM::findTypeFields("ClassLoaderData").value().get()["_next"]->offset;

     //CompileMethod::method = HotspotVM::findTypeFields("CompileMethod").value().get()["_method"]->offset;

     nmethod::verifiedEntryPoint = HotspotVM::findTypeFields("nmethod").value().get()["_verified_entry_point"]->offset;
     nmethod::osrEntryPoint = HotspotVM::findTypeFields("nmethod").value().get()["_osr_entry_point"]->offset;
     nmethod::entryPoint = HotspotVM::findTypeFields("nmethod").value().get()["_entry_point"]->offset;
     nmethod::entryBCI = HotspotVM::findTypeFields("nmethod").value().get()["_entry_bci"]->offset;
     nmethod::osrLink = HotspotVM::findTypeFields("nmethod").value().get()["_osr_link"]->offset;
     nmethod::state = HotspotVM::findTypeFields("nmethod").value().get()["_state"]->offset;
     nmethod::compileID = HotspotVM::findTypeFields("nmethod").value().get()["_compile_id"]->offset;
     nmethod::compileLVL = HotspotVM::findTypeFields("nmethod").value().get()["_comp_level"]->offset;

     CodeBlob::codeBegin = HotspotVM::findTypeFields("CodeBlob").value().get()["_code_begin"]->offset;
     CodeBlob::codeEnd = HotspotVM::findTypeFields("CodeBlob").value().get()["_code_end"]->offset;

     SharedRuntime::wrongMethodBlob = (uint64_t)HotspotVM::findTypeFields("SharedRuntime").value().get()["_wrong_method_blob"]->address;

     CompressedOops::narrowOopBase = [&]() -> uint64_t {
          auto opt = HotspotVM::findTypeFields("CompressedOops");
          if (!opt.has_value()) return 0;
          auto& fields = opt.value().get();
          int ver = HotspotVM::JvmVersion();
          if (ver == 25) {
               return (uint64_t)fields["_base"]->address;
          } else if (ver == 8) {
               return 0;
          } else {
               return (uint64_t)fields["_narrow_oop._base"]->address;
          }
     }();

     CompressedOops::narrowOopShift = [&]() -> uint64_t {
          auto opt = HotspotVM::findTypeFields("CompressedOops");
          if (!opt.has_value()) return 0;
          auto& fields = opt.value().get();
          int ver = HotspotVM::JvmVersion();
          if (ver == 25) {
               return (uint64_t)fields["_shift"]->address;
          } else if (ver == 8) {
               return 0;
          } else {
               return (uint64_t)fields["_narrow_oop._shift"]->address;
          }
     }();

     CompressedOops::useCompressedOops = [&]() -> uint64_t {
          auto opt = HotspotVM::findTypeFields("CompressedOops");
          if (!opt.has_value()) return 0;
          auto& fields = opt.value().get();
          int ver = HotspotVM::JvmVersion();
          if (ver == 25) {
               return (uint64_t)fields["_use_implicit_null_checks"]->address;
          } else if (ver == 8) {
               return 0;
          } else {
               return (uint64_t)fields["_narrow_oop._use_implicit_null_checks"]->address;
          }
     }();


     CompressedKlassPointers::narrowKlassBase = [&]() -> uint64_t {
          auto opt = HotspotVM::findTypeFields("CompressedKlassPointers");
          if (!opt.has_value()) return 0;
          auto& fields = opt.value().get();
          int ver = HotspotVM::JvmVersion();
          if (ver == 25) {
               return (uint64_t)fields["_base"]->address;
          } else if (ver == 8) {
               return 0;
          } else {
               return (uint64_t)fields["_narrow_klass._base"]->address;
          }
     }();

     CompressedKlassPointers::narrowKlassShift = [&]() -> uint64_t {
          auto opt = HotspotVM::findTypeFields("CompressedKlassPointers");
          if (!opt.has_value()) return 0;
          auto& fields = opt.value().get();
          int ver = HotspotVM::JvmVersion();
          if (ver == 25) {
               return (uint64_t)fields["_shift"]->address;
          } else if (ver == 8) {
               return 0;
          } else {
               return (uint64_t)fields["_narrow_klass._shift"]->address;
          }
     }();

     ConstantPool::length = HotspotVM::findTypeFields("ConstantPool").value().get()["_length"]->offset;
     ConstantPool::poolHolder = HotspotVM::findTypeFields("ConstantPool").value().get()["_pool_holder"]->offset;
     ConstantPool::constPoolSize = (HotspotVM::JvmVersion() <= 17 ? HotspotVM::findType("ConstantPool").value().get().size : ConstantPool::length + sizeof(int) + sizeof(void*));

     Symbol::length = HotspotVM::findTypeFields("Symbol").value().get()["_length"]->offset;
     Symbol::body = HotspotVM::findTypeFields("Symbol").value().get()["_body"]->offset;

}

