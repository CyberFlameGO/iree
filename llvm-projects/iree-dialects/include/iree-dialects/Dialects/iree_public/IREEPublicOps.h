// Copyright 2021 The IREE Authors
//
// Licensed under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#ifndef IREE_LLVM_PROJECTS_IREE_DIALECTS_DIALECTS_IREE_PUBLIC_IREE_PUBLIC_OPS_H
#define IREE_LLVM_PROJECTS_IREE_DIALECTS_DIALECTS_IREE_PUBLIC_IREE_PUBLIC_OPS_H

#include "mlir/IR/BuiltinTypes.h"
#include "mlir/IR/Dialect.h"
#include "mlir/IR/OpDefinition.h"
#include "mlir/Interfaces/SideEffectInterfaces.h"

#define GET_OP_CLASSES
#include "iree-dialects/Dialects/iree_public/IREEPublicOps.h.inc"

#endif  // IREE_LLVM_PROJECTS_IREE_DIALECTS_DIALECTS_IREE_PUBLIC_IREE_PUBLIC_OPS_H
