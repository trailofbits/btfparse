#include "llvmbridge.h"

#include <unordered_map>

#include <btfparse/illvmbridge.h>

namespace btfparse {

namespace {

const std::unordered_map<LLVMBridgeErrorCode, std::string>
    kErrorTranslationMap = {
        {LLVMBridgeErrorCode::Unknown, "Unknown"},

        {LLVMBridgeErrorCode::MemoryAllocationFailure,
         "MemoryAllocationFailure"},

        {LLVMBridgeErrorCode::UnsupportedBTFType, "UnsupportedBTFType"},
        {LLVMBridgeErrorCode::InvalidBTFTypeID, "InvalidBTFTypeID"},
        {LLVMBridgeErrorCode::MissingDependency, "MissingDependency"},
        {LLVMBridgeErrorCode::NotFound, "NotFound"},
};

}

std::string LLVMBridgeErrorCodePrinter::operator()(
    const LLVMBridgeErrorCode &error_code) const {
  auto error_it = kErrorTranslationMap.find(error_code);
  if (error_it == kErrorTranslationMap.end()) {
    return "UnknownErrorCode:" + std::to_string(static_cast<int>(error_code));
  }

  return error_it->second;
}

Result<ILLVMBridge::Ptr, LLVMBridgeError>
ILLVMBridge::create(llvm::Module &module, const IBTF &btf) {
  try {
    return Ptr(new LLVMBridge(module, btf));

  } catch (const std::bad_alloc &) {
    return LLVMBridgeError(LLVMBridgeErrorCode::MemoryAllocationFailure);

  } catch (const LLVMBridgeError &e) {
    return e;
  }
}

} // namespace btfparse
