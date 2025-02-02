// RUN: iree-translate -iree-vm-ir-to-c-module %s | IreeFileCheck %s

// CHECK: #include "iree/vm/ops.h"
vm.module @calling_convention_test {
  // CHECK: static iree_status_t calling_convention_test_no_in_no_return(iree_vm_stack_t* v1, calling_convention_test_t* v2, calling_convention_test_state_t* v3) {
  vm.func @no_in_no_return() -> () {
    // CHECK-NEXT: iree_status_t v4;
    // CHECK-NEXT: v4 = iree_ok_status();
    // CHECK-NEXT: return v4;
    vm.return
  }

  // CHECK: static iree_status_t calling_convention_test_i32_in_no_return(iree_vm_stack_t* v1, calling_convention_test_t* v2, calling_convention_test_state_t* v3, int32_t v4) {
  vm.func @i32_in_no_return(%arg0 : i32) -> () {
    // CHECK-NEXT: iree_status_t v5;
    // CHECK-NEXT: v5 = iree_ok_status();
    // CHECK-NEXT: return v5;
    vm.return
  }

  // CHECK: static iree_status_t calling_convention_test_no_in_i32_return(iree_vm_stack_t* v1, calling_convention_test_t* v2, calling_convention_test_state_t* v3, int32_t* v4) {
  vm.func @no_in_i32_return() -> (i32) {
    // CHECK-NEXT: int32_t v5;
    // CHECK-NEXT: iree_status_t v6;
    // CHECK-NEXT: v5 = 32;
    %0 = vm.const.i32 32 : i32
    // CHECK-NEXT: EMITC_DEREF_ASSIGN(v4, v5);
    // CHECK-NEXT: v6 = iree_ok_status();
    // CHECK-NEXT: return v6;
    vm.return %0 : i32
  }

  // CHECK: static iree_status_t calling_convention_test_i32_in_i32_return(iree_vm_stack_t* v1, calling_convention_test_t* v2, calling_convention_test_state_t* v3, int32_t v4, int32_t* v5) {
  vm.func @i32_in_i32_return(%arg0 : i32) -> (i32) {
    // CHECK-NEXT: int32_t v6;
    // CHECK-NEXT: iree_status_t v7;
    // CHECK-NEXT: v6 = 32;
    %0 = vm.const.i32 32 : i32
    // CHECK-NEXT: EMITC_DEREF_ASSIGN(v5, v6);
    // CHECK-NEXT: v7 = iree_ok_status();
    // CHECK-NEXT: return v7;
    vm.return %0 : i32
  }
}
