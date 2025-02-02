# Use CMake to build the sample until bazel can support lld.
# TODO(llvm-bazel/#228): re-enable bazel.

iree_add_all_subdirs()

if(${IREE_HAL_DRIVER_VMVX} AND (${IREE_TARGET_BACKEND_VMVX} OR DEFINED IREE_HOST_BINARY_ROOT))

iree_cc_binary(
  NAME
    simple_embedding_vmvx_sync
  SRCS
    "device_vmvx_sync.c"
    "simple_embedding.c"
  DEPS
    ::simple_embedding_test_bytecode_module_vmvx_c
    iree::base
    iree::hal
    iree::hal::local
    iree::hal::local::loaders::vmvx_module_loader
    iree::hal::local::sync_driver
    iree::modules::hal
    iree::vm
    iree::vm::bytecode_module
)

iree_bytecode_module(
  NAME
    simple_embedding_test_bytecode_module_vmvx
  SRC
    "simple_embedding_test.mlir"
  C_IDENTIFIER
    "iree_samples_simple_embedding_test_module_vmvx"
  FLAGS
    "-iree-input-type=mhlo"
    "-iree-mlir-to-vm-bytecode-module"
    "-iree-hal-target-backends=vmvx"
  PUBLIC
)

iree_run_binary_test(
  NAME
    "simple_embedding_vmvx_sync_test"
  TEST_BINARY
    ::simple_embedding_vmvx_sync
)

endif()

if(${IREE_HAL_DRIVER_DYLIB} AND (${IREE_TARGET_BACKEND_DYLIB-LLVM-AOT} OR DEFINED IREE_HOST_BINARY_ROOT))

iree_cc_binary(
  NAME
    simple_embedding_embedded_sync
  SRCS
    "device_embedded_sync.c"
    "simple_embedding.c"
  DEPS
    ::simple_embedding_test_bytecode_module_dylib_arm_32_c
    ::simple_embedding_test_bytecode_module_dylib_arm_64_c
    ::simple_embedding_test_bytecode_module_dylib_riscv_32_c
    ::simple_embedding_test_bytecode_module_dylib_riscv_64_c
    ::simple_embedding_test_bytecode_module_dylib_x86_64_c
    iree::base
    iree::hal
    iree::hal::local
    iree::hal::local::loaders::embedded_library_loader
    iree::hal::local::sync_driver
    iree::modules::hal
    iree::vm
    iree::vm::bytecode_module
)

iree_bytecode_module(
  NAME
    simple_embedding_test_bytecode_module_dylib_x86_64
  SRC
    "simple_embedding_test.mlir"
  C_IDENTIFIER
    "iree_samples_simple_embedding_test_module_dylib_x86_64"
  FLAGS
    "-iree-input-type=mhlo"
    "-iree-mlir-to-vm-bytecode-module"
    "-iree-hal-target-backends=dylib-llvm-aot"
    "-iree-llvm-target-triple=x86_64-pc-linux-elf"
    "-iree-llvm-link-embedded=true"
    "-iree-llvm-debug-symbols=false"
  PUBLIC
)

iree_bytecode_module(
  NAME
    simple_embedding_test_bytecode_module_dylib_riscv_32
  SRC
    "simple_embedding_test.mlir"
  C_IDENTIFIER
    "iree_samples_simple_embedding_test_module_dylib_riscv_32"
  FLAGS
    "-iree-input-type=mhlo"
    "-iree-mlir-to-vm-bytecode-module"
    "-iree-hal-target-backends=dylib-llvm-aot"
    "-iree-llvm-target-triple=riscv32-pc-linux-elf"
    "-iree-llvm-target-cpu=generic-rv32"
    "-iree-llvm-target-cpu-features=+m,+f"
    "-iree-llvm-target-abi=ilp32"
    "-iree-llvm-link-embedded=true"
    "-iree-llvm-debug-symbols=false"
  PUBLIC
)

iree_bytecode_module(
  NAME
    simple_embedding_test_bytecode_module_dylib_riscv_64
  SRC
    "simple_embedding_test.mlir"
  C_IDENTIFIER
    "iree_samples_simple_embedding_test_module_dylib_riscv_64"
  FLAGS
    "-iree-input-type=mhlo"
    "-iree-mlir-to-vm-bytecode-module"
    "-iree-hal-target-backends=dylib-llvm-aot"
    "-iree-llvm-target-triple=riscv64-pc-linux-elf"
    "-iree-llvm-target-cpu=generic-rv64"
    "-iree-llvm-target-cpu-features=+m,+a,+f,+d,+c"
    "-iree-llvm-target-abi=lp64d"
    "-iree-llvm-link-embedded=true"
    "-iree-llvm-debug-symbols=false"
  PUBLIC
)

iree_bytecode_module(
    NAME
      simple_embedding_test_bytecode_module_dylib_arm_32
    SRC
      "simple_embedding_test.mlir"
    C_IDENTIFIER
      "iree_samples_simple_embedding_test_module_dylib_arm_32"
    FLAGS
        "-iree-input-type=mhlo"
        "-iree-mlir-to-vm-bytecode-module"
        "-iree-hal-target-backends=dylib-llvm-aot"
        "-iree-llvm-target-triple=armv7a-pc-linux-elf"
        "-iree-llvm-target-float-abi=hard"
        "-iree-llvm-link-embedded=true"
        "-iree-llvm-debug-symbols=false"
    PUBLIC
)

iree_bytecode_module(
  NAME
    simple_embedding_test_bytecode_module_dylib_arm_64
  SRC
    "simple_embedding_test.mlir"
  C_IDENTIFIER
    "iree_samples_simple_embedding_test_module_dylib_arm_64"
  FLAGS
    "-iree-input-type=mhlo"
    "-iree-mlir-to-vm-bytecode-module"
    "-iree-hal-target-backends=dylib-llvm-aot"
    "-iree-llvm-target-triple=aarch64-pc-linux-elf"
    "-iree-llvm-link-embedded=true"
    "-iree-llvm-debug-symbols=false"
  PUBLIC
)

iree_run_binary_test(
  NAME
    "simple_embedding_embedded_sync_test"
  TEST_BINARY
    ::simple_embedding_embedded_sync
)

if(${IREE_ENABLE_THREADING})

iree_cc_binary(
  NAME
    simple_embedding_dylib
  SRCS
    "device_dylib.c"
    "simple_embedding.c"
  DEPS
    ::simple_embedding_test_bytecode_module_dylib_arm_64_c
    ::simple_embedding_test_bytecode_module_dylib_riscv_64_c
    ::simple_embedding_test_bytecode_module_dylib_x86_64_c
    iree::base
    iree::hal
    iree::hal::local
    iree::hal::local::loaders::embedded_library_loader
    iree::hal::local::task_driver
    iree::modules::hal
    iree::task::api
    iree::vm
    iree::vm::bytecode_module
)

iree_run_binary_test(
  NAME
    "simple_embedding_dylib_test"
  TEST_BINARY
    ::simple_embedding_dylib
)

endif()

endif()

if(${IREE_HAL_DRIVER_VULKAN} AND (${IREE_TARGET_BACKEND_VULKAN-SPIRV} OR DEFINED IREE_HOST_BINARY_ROOT))

iree_cc_binary(
  NAME
    simple_embedding_vulkan
  SRCS
    "device_vulkan.c"
    "simple_embedding.c"
  DEPS
    ::simple_embedding_test_bytecode_module_vulkan_c
    iree::base
    iree::hal
    iree::hal::vulkan::registration
    iree::modules::hal
    iree::vm
    iree::vm::bytecode_module
)

iree_bytecode_module(
  NAME
    simple_embedding_test_bytecode_module_vulkan
  SRC
    "simple_embedding_test.mlir"
  C_IDENTIFIER
    "iree_samples_simple_embedding_test_module_vulkan"
  FLAGS
    "-iree-input-type=mhlo"
    "-iree-mlir-to-vm-bytecode-module"
    "-iree-hal-target-backends=vulkan-spirv"
    "-iree-llvm-debug-symbols=false"
  PUBLIC
)

iree_run_binary_test(
  NAME
    "simple_embedding_vulkan_test"
  TEST_BINARY
    ::simple_embedding_vulkan
)

endif()

if(${IREE_HAL_DRIVER_CUDA} AND (${IREE_TARGET_BACKEND_CUDA} OR DEFINED IREE_HOST_BINARY_ROOT))

iree_cc_binary(
  NAME
    simple_embedding_cuda
  SRCS
    "device_cuda.c"
    "simple_embedding.c"
  DEPS
    ::simple_embedding_test_bytecode_module_cuda_c
    iree::base
    iree::hal
    iree::hal::cuda::registration
    iree::modules::hal
    iree::vm
    iree::vm::bytecode_module
)

iree_bytecode_module(
  NAME
    simple_embedding_test_bytecode_module_cuda
  SRC
    "simple_embedding_test.mlir"
  C_IDENTIFIER
    "iree_samples_simple_embedding_test_module_cuda"
  FLAGS
    "-iree-input-type=mhlo"
    "-iree-mlir-to-vm-bytecode-module"
    "-iree-hal-target-backends=cuda"
    "-iree-llvm-debug-symbols=false"
  PUBLIC
)

iree_run_binary_test(
  NAME
    "simple_embedding_cuda_test"
  LABELS
    "driver=cuda"
  TEST_BINARY
    ::simple_embedding_cuda
)

endif()

### BAZEL_TO_CMAKE_PRESERVES_ALL_CONTENT_BELOW_THIS_LINE ###
