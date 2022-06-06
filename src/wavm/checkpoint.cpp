#include <faabric/util/func.h>
#include <faabric/util/logging.h>
#include <wavm/WAVMWasmModule.h>

#include <WAVM/Runtime/Intrinsics.h>
#include <WAVM/Runtime/Runtime.h>

namespace wasm {

void checkpointLink() {}

WAVM_DEFINE_INTRINSIC_FUNCTION(env,
                               "__faasm_checkpoint",
                               void,
                               __faasm_checkpoint)
{
    SPDLOG_INFO("Checkpointing");

    auto moduleClone = *getExecutingWAVMModule();

    // Create a restore snapshot for this checkpoint

    // Will chain here for testing purposes and to avoid having to implement
    // file serialisation

    throw faabric::util::FunctionMigratedException("For the lack of a better exception");
}

WAVM_DEFINE_INTRINSIC_FUNCTION(env,
                               "__faasm_restore",
                               void,
                               __faasm_restore)
{
    SPDLOG_INFO("Restored :)");
}
}
