#include <faabric/util/logging.h>
#include <wavm/WAVMWasmModule.h>

#include <WAVM/Runtime/Intrinsics.h>
#include <WAVM/Runtime/Runtime.h>

namespace wasm {

WAVM_DEFINE_INTRINSIC_FUNCTION(env,
                               "__faasm_checkpoint",
                               void,
                               __faasm_checkpoint)
{
    SPDLOG_INFO("Checkpointingg");

    WAVMWasmModule* thisModule = getExecutingWAVMModule();
}
}
