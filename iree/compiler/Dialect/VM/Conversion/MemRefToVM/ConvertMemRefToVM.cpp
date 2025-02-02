// Copyright 2021 The IREE Authors
//
// Licensed under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include "iree/compiler/Dialect/VM/Conversion/MemRefToVM/ConvertMemRefToVM.h"

#include "iree/compiler/Dialect/Util/IR/UtilTypes.h"
#include "iree/compiler/Dialect/VM/Conversion/TargetOptions.h"
#include "iree/compiler/Dialect/VM/Conversion/TypeConverter.h"
#include "iree/compiler/Dialect/VM/IR/VMOps.h"
#include "mlir/Dialect/Affine/IR/AffineOps.h"
#include "mlir/Dialect/MemRef/IR/MemRef.h"
#include "mlir/Dialect/StandardOps/IR/Ops.h"
#include "mlir/IR/Attributes.h"
#include "mlir/IR/Builders.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/IR/Matchers.h"
#include "mlir/IR/OperationSupport.h"
#include "mlir/Transforms/DialectConversion.h"

namespace mlir {
namespace iree_compiler {

namespace {

/// Pattern to lower operations that become a no-ops at this level.
template <typename OpTy>
struct FoldAsNoOp final : public OpConversionPattern<OpTy> {
  using OpConversionPattern<OpTy>::OpConversionPattern;
  LogicalResult matchAndRewrite(
      OpTy op, ArrayRef<Value> operands,
      ConversionPatternRewriter &rewriter) const override {
    rewriter.replaceOp(op, operands);
    return success();
  }
};

/// Returns true if the given `type` is a MemRef of rank 0 or 1.
static bool isRankZeroOrOneMemRef(Type type) {
  if (auto memrefType = type.dyn_cast<MemRefType>()) {
    return memrefType.hasRank() && memrefType.getRank() <= 1;
  }
  return false;
}

// Returns the number of bytes an element of the given type occupies
// post-conversion. For example, the size of i1 would be '1 byte'.
static int32_t getRoundedElementByteWidth(Type type) {
  return (type.getIntOrFloatBitWidth() + 8 - 1) / 8;
}

// Returns the offset, in bytes, of an index within a linearized dense buffer.
// Expects that the |memrefValue| has been linearized already.
static Value getBufferOffset(Location loc, Value memrefValue,
                             ValueRange indices,
                             ConversionPatternRewriter &rewriter) {
  auto memrefType = memrefValue.getType().cast<ShapedType>();
  if (memrefType.getRank() == 0) {
    // Rank 0 buffers (like memref<i32>) have only a single valid offset at 0.
    return rewriter.createOrFold<ConstantIndexOp>(loc, 0);
  }
  assert(memrefType.getRank() == 1 && "memrefs should have been flattened");

  // Element type byte length as the base.
  auto elementType = memrefType.getElementType();
  auto scalingExpr = getAffineBinaryOpExpr(
      AffineExprKind::Mul, getAffineSymbolExpr(0, rewriter.getContext()),
      getAffineConstantExpr(getRoundedElementByteWidth(elementType),
                            rewriter.getContext()));

  // Rank 1 memrefs are just offset by their element width by the offset.
  return rewriter.createOrFold<AffineApplyOp>(loc, scalingExpr,
                                              ArrayRef<Value>{indices.front()});
}

class ConvertMemRefGlobalOp : public OpConversionPattern<memref::GlobalOp> {
 public:
  using OpConversionPattern::OpConversionPattern;

  LogicalResult matchAndRewrite(
      memref::GlobalOp globalOp, ArrayRef<Value> rawOperands,
      ConversionPatternRewriter &rewriter) const override {
    memref::GlobalOpAdaptor operands(rawOperands);
    if (!isRankZeroOrOneMemRef(globalOp.type())) {
      return rewriter.notifyMatchFailure(
          globalOp,
          "only rank-0 and rank-1 memrefs are supported; flatten first");
    }

    // For mutable values we'd want to either have a RwdataOp or a global
    // !vm.buffer that we initialized with rodata.
    if (!globalOp.constant()) {
      return rewriter.notifyMatchFailure(
          globalOp, "mutable global memrefs not yet implemented");
    }

    auto rodataOp = rewriter.replaceOpWithNewOp<IREE::VM::RodataOp>(
        globalOp, globalOp.sym_name(),
        globalOp.initial_valueAttr().cast<ElementsAttr>());
    rodataOp.setPrivate();
    return success();
  }
};

class ConvertMemRefGetGlobalOp
    : public OpConversionPattern<memref::GetGlobalOp> {
 public:
  using OpConversionPattern::OpConversionPattern;

  LogicalResult matchAndRewrite(
      memref::GetGlobalOp getOp, ArrayRef<Value> rawOperands,
      ConversionPatternRewriter &rewriter) const override {
    memref::GetGlobalOpAdaptor operands(rawOperands);
    if (!isRankZeroOrOneMemRef(getOp.result().getType())) {
      return rewriter.notifyMatchFailure(
          getOp, "only rank-0 and rank-1 memrefs are supported; flatten first");
    }
    rewriter.replaceOpWithNewOp<IREE::VM::ConstRefRodataOp>(getOp,
                                                            getOp.name());
    return success();
  }
};

class ConvertMemRefLoadOp : public OpConversionPattern<memref::LoadOp> {
 public:
  using OpConversionPattern::OpConversionPattern;

  LogicalResult matchAndRewrite(
      memref::LoadOp loadOp, ArrayRef<Value> rawOperands,
      ConversionPatternRewriter &rewriter) const override {
    memref::LoadOpAdaptor operands(rawOperands);
    if (!isRankZeroOrOneMemRef(loadOp.memref().getType())) {
      return rewriter.notifyMatchFailure(
          loadOp,
          "only rank-0 and rank-1 memrefs are supported; flatten first");
    }
    auto oldType = loadOp.result().getType();
    auto newType = getTypeConverter()->convertType(oldType);
    auto byteOffset = getBufferOffset(loadOp.getLoc(), loadOp.memref(),
                                      operands.indices(), rewriter);
    if (auto integerType = oldType.dyn_cast<IntegerType>()) {
      if (integerType.isInteger(1) || integerType.isInteger(8)) {
        if (integerType.isSigned() || integerType.isSignless()) {
          rewriter.replaceOpWithNewOp<IREE::VM::BufferLoadI8SOp>(
              loadOp, newType, operands.memref(), byteOffset);
        } else {
          rewriter.replaceOpWithNewOp<IREE::VM::BufferLoadI8UOp>(
              loadOp, newType, operands.memref(), byteOffset);
        }
      } else if (integerType.isInteger(16)) {
        if (integerType.isSigned() || integerType.isSignless()) {
          rewriter.replaceOpWithNewOp<IREE::VM::BufferLoadI16SOp>(
              loadOp, newType, operands.memref(), byteOffset);
        } else {
          rewriter.replaceOpWithNewOp<IREE::VM::BufferLoadI16UOp>(
              loadOp, newType, operands.memref(), byteOffset);
        }
      } else if (integerType.isInteger(32)) {
        rewriter.replaceOpWithNewOp<IREE::VM::BufferLoadI32Op>(
            loadOp, newType, operands.memref(), byteOffset);
      } else if (integerType.isInteger(64)) {
        rewriter.replaceOpWithNewOp<IREE::VM::BufferLoadI64Op>(
            loadOp, newType, operands.memref(), byteOffset);
      } else {
        return rewriter.notifyMatchFailure(
            loadOp, "invalid integer buffer element type");
      }
    } else if (oldType.isF32()) {
      rewriter.replaceOpWithNewOp<IREE::VM::BufferLoadF32Op>(
          loadOp, newType, operands.memref(), byteOffset);
    } else if (oldType.isF64()) {
      rewriter.replaceOpWithNewOp<IREE::VM::BufferLoadF64Op>(
          loadOp, newType, operands.memref(), byteOffset);
    } else {
      return rewriter.notifyMatchFailure(loadOp,
                                         "invalid float buffer element type");
    }
    return success();
  }
};

class ConvertMemRefStoreOp : public OpConversionPattern<memref::StoreOp> {
 public:
  using OpConversionPattern::OpConversionPattern;

  LogicalResult matchAndRewrite(
      memref::StoreOp storeOp, ArrayRef<Value> rawOperands,
      ConversionPatternRewriter &rewriter) const override {
    memref::StoreOpAdaptor operands(rawOperands);
    if (!isRankZeroOrOneMemRef(storeOp.memref().getType())) {
      return rewriter.notifyMatchFailure(
          storeOp,
          "only rank-0 and rank-1 memrefs are supported; flatten first");
    }
    auto oldType = storeOp.value().getType();
    auto byteOffset = getBufferOffset(storeOp.getLoc(), storeOp.memref(),
                                      operands.indices(), rewriter);
    if (oldType.isInteger(1) || oldType.isInteger(8)) {
      rewriter.replaceOpWithNewOp<IREE::VM::BufferStoreI8Op>(
          storeOp, operands.memref(), byteOffset, operands.value());
    } else if (oldType.isInteger(16)) {
      rewriter.replaceOpWithNewOp<IREE::VM::BufferStoreI16Op>(
          storeOp, operands.memref(), byteOffset, operands.value());
    } else if (oldType.isInteger(32)) {
      rewriter.replaceOpWithNewOp<IREE::VM::BufferStoreI32Op>(
          storeOp, operands.memref(), byteOffset, operands.value());
    } else if (oldType.isInteger(64)) {
      rewriter.replaceOpWithNewOp<IREE::VM::BufferStoreI64Op>(
          storeOp, operands.memref(), byteOffset, operands.value());
    } else if (oldType.isF32()) {
      rewriter.replaceOpWithNewOp<IREE::VM::BufferStoreF32Op>(
          storeOp, operands.memref(), byteOffset, operands.value());
    } else if (oldType.isF64()) {
      rewriter.replaceOpWithNewOp<IREE::VM::BufferStoreF64Op>(
          storeOp, operands.memref(), byteOffset, operands.value());
    } else {
      return rewriter.notifyMatchFailure(storeOp,
                                         "invalid buffer element type");
    }
    return success();
  }
};

}  // namespace

void populateMemRefToVMPatterns(MLIRContext *context,
                                ConversionTarget &conversionTarget,
                                TypeConverter &typeConverter,
                                OwningRewritePatternList &patterns) {
  conversionTarget.addIllegalDialect<memref::MemRefDialect>();

  typeConverter.addConversion([&](MemRefType type) -> llvm::Optional<Type> {
    if (isRankZeroOrOneMemRef(type)) {
      return IREE::VM::RefType::get(
          IREE::VM::BufferType::get(type.getContext()));
    }
    return llvm::None;
  });

  patterns.insert<FoldAsNoOp<memref::BufferCastOp>>(typeConverter, context);
  patterns.insert<ConvertMemRefGlobalOp, ConvertMemRefGetGlobalOp,
                  ConvertMemRefLoadOp, ConvertMemRefStoreOp>(typeConverter,
                                                             context);
}

}  // namespace iree_compiler
}  // namespace mlir
