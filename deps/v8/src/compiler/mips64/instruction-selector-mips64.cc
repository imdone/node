// Copyright 2014 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/base/adapters.h"
#include "src/base/bits.h"
#include "src/compiler/instruction-selector-impl.h"
#include "src/compiler/node-matchers.h"
#include "src/compiler/node-properties.h"

namespace v8 {
namespace internal {
namespace compiler {

#define TRACE_UNIMPL() \
  PrintF("UNIMPLEMENTED instr_sel: %s at line %d\n", __FUNCTION__, __LINE__)

#define TRACE() PrintF("instr_sel: %s at line %d\n", __FUNCTION__, __LINE__)


// Adds Mips-specific methods for generating InstructionOperands.
class Mips64OperandGenerator final : public OperandGenerator {
 public:
  explicit Mips64OperandGenerator(InstructionSelector* selector)
      : OperandGenerator(selector) {}

  InstructionOperand UseOperand(Node* node, InstructionCode opcode) {
    if (CanBeImmediate(node, opcode)) {
      return UseImmediate(node);
    }
    return UseRegister(node);
  }

  // Use the zero register if the node has the immediate value zero, otherwise
  // assign a register.
  InstructionOperand UseRegisterOrImmediateZero(Node* node) {
    if ((IsIntegerConstant(node) && (GetIntegerConstantValue(node) == 0)) ||
        (IsFloatConstant(node) &&
         (bit_cast<int64_t>(GetFloatConstantValue(node)) == V8_INT64_C(0)))) {
      return UseImmediate(node);
    }
    return UseRegister(node);
  }

  bool IsIntegerConstant(Node* node) {
    return (node->opcode() == IrOpcode::kInt32Constant) ||
           (node->opcode() == IrOpcode::kInt64Constant);
  }

  int64_t GetIntegerConstantValue(Node* node) {
    if (node->opcode() == IrOpcode::kInt32Constant) {
      return OpParameter<int32_t>(node);
    }
    DCHECK(node->opcode() == IrOpcode::kInt64Constant);
    return OpParameter<int64_t>(node);
  }

  bool IsFloatConstant(Node* node) {
    return (node->opcode() == IrOpcode::kFloat32Constant) ||
           (node->opcode() == IrOpcode::kFloat64Constant);
  }

  double GetFloatConstantValue(Node* node) {
    if (node->opcode() == IrOpcode::kFloat32Constant) {
      return OpParameter<float>(node);
    }
    DCHECK_EQ(IrOpcode::kFloat64Constant, node->opcode());
    return OpParameter<double>(node);
  }

  bool CanBeImmediate(Node* node, InstructionCode mode) {
    return IsIntegerConstant(node) &&
           CanBeImmediate(GetIntegerConstantValue(node), mode);
  }

  bool CanBeImmediate(int64_t value, InstructionCode opcode) {
    switch (ArchOpcodeField::decode(opcode)) {
      case kMips64Shl:
      case kMips64Sar:
      case kMips64Shr:
        return is_uint5(value);
      case kMips64Dshl:
      case kMips64Dsar:
      case kMips64Dshr:
        return is_uint6(value);
      case kMips64Add:
      case kMips64And32:
      case kMips64And:
      case kMips64Dadd:
      case kMips64Or32:
      case kMips64Or:
      case kMips64Tst:
      case kMips64Xor:
        return is_uint16(value);
      case kMips64Lb:
      case kMips64Lbu:
      case kMips64Sb:
      case kMips64Lh:
      case kMips64Lhu:
      case kMips64Sh:
      case kMips64Lw:
      case kMips64Sw:
      case kMips64Ld:
      case kMips64Sd:
      case kMips64Lwc1:
      case kMips64Swc1:
      case kMips64Ldc1:
      case kMips64Sdc1:
      case kCheckedLoadInt8:
      case kCheckedLoadUint8:
      case kCheckedLoadInt16:
      case kCheckedLoadUint16:
      case kCheckedLoadWord32:
      case kCheckedLoadWord64:
      case kCheckedStoreWord8:
      case kCheckedStoreWord16:
      case kCheckedStoreWord32:
      case kCheckedStoreWord64:
      case kCheckedLoadFloat32:
      case kCheckedLoadFloat64:
      case kCheckedStoreFloat32:
      case kCheckedStoreFloat64:
        return is_int32(value);
      default:
        return is_int16(value);
    }
  }

 private:
  bool ImmediateFitsAddrMode1Instruction(int32_t imm) const {
    TRACE_UNIMPL();
    return false;
  }
};


static void VisitRR(InstructionSelector* selector, ArchOpcode opcode,
                    Node* node) {
  Mips64OperandGenerator g(selector);
  selector->Emit(opcode, g.DefineAsRegister(node),
                 g.UseRegister(node->InputAt(0)));
}

static void VisitRRI(InstructionSelector* selector, ArchOpcode opcode,
                     Node* node) {
  Mips64OperandGenerator g(selector);
  int32_t imm = OpParameter<int32_t>(node);
  selector->Emit(opcode, g.DefineAsRegister(node),
                 g.UseRegister(node->InputAt(0)), g.UseImmediate(imm));
}

static void VisitRRIR(InstructionSelector* selector, ArchOpcode opcode,
                      Node* node) {
  Mips64OperandGenerator g(selector);
  int32_t imm = OpParameter<int32_t>(node);
  selector->Emit(opcode, g.DefineAsRegister(node),
                 g.UseRegister(node->InputAt(0)), g.UseImmediate(imm),
                 g.UseRegister(node->InputAt(1)));
}

static void VisitRRR(InstructionSelector* selector, ArchOpcode opcode,
                     Node* node) {
  Mips64OperandGenerator g(selector);
  selector->Emit(opcode, g.DefineAsRegister(node),
                 g.UseRegister(node->InputAt(0)),
                 g.UseRegister(node->InputAt(1)));
}

void VisitRRRR(InstructionSelector* selector, ArchOpcode opcode, Node* node) {
  Mips64OperandGenerator g(selector);
  selector->Emit(
      opcode, g.DefineSameAsFirst(node), g.UseRegister(node->InputAt(0)),
      g.UseRegister(node->InputAt(1)), g.UseRegister(node->InputAt(2)));
}

static void VisitRRO(InstructionSelector* selector, ArchOpcode opcode,
                     Node* node) {
  Mips64OperandGenerator g(selector);
  selector->Emit(opcode, g.DefineAsRegister(node),
                 g.UseRegister(node->InputAt(0)),
                 g.UseOperand(node->InputAt(1), opcode));
}

struct ExtendingLoadMatcher {
  ExtendingLoadMatcher(Node* node, InstructionSelector* selector)
      : matches_(false), selector_(selector), base_(nullptr), immediate_(0) {
    Initialize(node);
  }

  bool Matches() const { return matches_; }

  Node* base() const {
    DCHECK(Matches());
    return base_;
  }
  int64_t immediate() const {
    DCHECK(Matches());
    return immediate_;
  }
  ArchOpcode opcode() const {
    DCHECK(Matches());
    return opcode_;
  }

 private:
  bool matches_;
  InstructionSelector* selector_;
  Node* base_;
  int64_t immediate_;
  ArchOpcode opcode_;

  void Initialize(Node* node) {
    Int64BinopMatcher m(node);
    // When loading a 64-bit value and shifting by 32, we should
    // just load and sign-extend the interesting 4 bytes instead.
    // This happens, for example, when we're loading and untagging SMIs.
    DCHECK(m.IsWord64Sar());
    if (m.left().IsLoad() && m.right().Is(32) &&
        selector_->CanCover(m.node(), m.left().node())) {
      MachineRepresentation rep =
          LoadRepresentationOf(m.left().node()->op()).representation();
      DCHECK(ElementSizeLog2Of(rep) == 3);
      if (rep != MachineRepresentation::kTaggedSigned &&
          rep != MachineRepresentation::kTaggedPointer &&
          rep != MachineRepresentation::kTagged &&
          rep != MachineRepresentation::kWord64) {
        return;
      }

      Mips64OperandGenerator g(selector_);
      Node* load = m.left().node();
      Node* offset = load->InputAt(1);
      base_ = load->InputAt(0);
      opcode_ = kMips64Lw;
      if (g.CanBeImmediate(offset, opcode_)) {
#if defined(V8_TARGET_LITTLE_ENDIAN)
        immediate_ = g.GetIntegerConstantValue(offset) + 4;
#elif defined(V8_TARGET_BIG_ENDIAN)
        immediate_ = g.GetIntegerConstantValue(offset);
#endif
        matches_ = g.CanBeImmediate(immediate_, kMips64Lw);
      }
    }
  }
};

bool TryEmitExtendingLoad(InstructionSelector* selector, Node* node,
                          Node* output_node) {
  ExtendingLoadMatcher m(node, selector);
  Mips64OperandGenerator g(selector);
  if (m.Matches()) {
    InstructionOperand inputs[2];
    inputs[0] = g.UseRegister(m.base());
    InstructionCode opcode =
        m.opcode() | AddressingModeField::encode(kMode_MRI);
    DCHECK(is_int32(m.immediate()));
    inputs[1] = g.TempImmediate(static_cast<int32_t>(m.immediate()));
    InstructionOperand outputs[] = {g.DefineAsRegister(output_node)};
    selector->Emit(opcode, arraysize(outputs), outputs, arraysize(inputs),
                   inputs);
    return true;
  }
  return false;
}

bool TryMatchImmediate(InstructionSelector* selector,
                       InstructionCode* opcode_return, Node* node,
                       size_t* input_count_return, InstructionOperand* inputs) {
  Mips64OperandGenerator g(selector);
  if (g.CanBeImmediate(node, *opcode_return)) {
    *opcode_return |= AddressingModeField::encode(kMode_MRI);
    inputs[0] = g.UseImmediate(node);
    *input_count_return = 1;
    return true;
  }
  return false;
}

static void VisitBinop(InstructionSelector* selector, Node* node,
                       InstructionCode opcode, bool has_reverse_opcode,
                       InstructionCode reverse_opcode,
                       FlagsContinuation* cont) {
  Mips64OperandGenerator g(selector);
  Int32BinopMatcher m(node);
  InstructionOperand inputs[4];
  size_t input_count = 0;
  InstructionOperand outputs[2];
  size_t output_count = 0;

  if (TryMatchImmediate(selector, &opcode, m.right().node(), &input_count,
                        &inputs[1])) {
    inputs[0] = g.UseRegister(m.left().node());
    input_count++;
  } else if (has_reverse_opcode &&
             TryMatchImmediate(selector, &reverse_opcode, m.left().node(),
                               &input_count, &inputs[1])) {
    inputs[0] = g.UseRegister(m.right().node());
    opcode = reverse_opcode;
    input_count++;
  } else {
    inputs[input_count++] = g.UseRegister(m.left().node());
    inputs[input_count++] = g.UseOperand(m.right().node(), opcode);
  }

  if (cont->IsBranch()) {
    inputs[input_count++] = g.Label(cont->true_block());
    inputs[input_count++] = g.Label(cont->false_block());
  } else if (cont->IsTrap()) {
    inputs[input_count++] = g.TempImmediate(cont->trap_id());
  }

  if (cont->IsDeoptimize()) {
    // If we can deoptimize as a result of the binop, we need to make sure that
    // the deopt inputs are not overwritten by the binop result. One way
    // to achieve that is to declare the output register as same-as-first.
    outputs[output_count++] = g.DefineSameAsFirst(node);
  } else {
    outputs[output_count++] = g.DefineAsRegister(node);
  }
  if (cont->IsSet()) {
    outputs[output_count++] = g.DefineAsRegister(cont->result());
  }

  DCHECK_NE(0u, input_count);
  DCHECK_NE(0u, output_count);
  DCHECK_GE(arraysize(inputs), input_count);
  DCHECK_GE(arraysize(outputs), output_count);

  opcode = cont->Encode(opcode);
  if (cont->IsDeoptimize()) {
    selector->EmitDeoptimize(opcode, output_count, outputs, input_count, inputs,
                             cont->kind(), cont->reason(), cont->frame_state());
  } else {
    selector->Emit(opcode, output_count, outputs, input_count, inputs);
  }
}

static void VisitBinop(InstructionSelector* selector, Node* node,
                       InstructionCode opcode, bool has_reverse_opcode,
                       InstructionCode reverse_opcode) {
  FlagsContinuation cont;
  VisitBinop(selector, node, opcode, has_reverse_opcode, reverse_opcode, &cont);
}

static void VisitBinop(InstructionSelector* selector, Node* node,
                       InstructionCode opcode, FlagsContinuation* cont) {
  VisitBinop(selector, node, opcode, false, kArchNop, cont);
}

static void VisitBinop(InstructionSelector* selector, Node* node,
                       InstructionCode opcode) {
  VisitBinop(selector, node, opcode, false, kArchNop);
}

void InstructionSelector::VisitStackSlot(Node* node) {
  StackSlotRepresentation rep = StackSlotRepresentationOf(node->op());
  int alignment = rep.alignment();
  int slot = frame_->AllocateSpillSlot(rep.size(), alignment);
  OperandGenerator g(this);

  Emit(kArchStackSlot, g.DefineAsRegister(node),
       sequence()->AddImmediate(Constant(slot)),
       sequence()->AddImmediate(Constant(alignment)), 0, nullptr);
}

void EmitLoad(InstructionSelector* selector, Node* node, InstructionCode opcode,
              Node* output = nullptr) {
  Mips64OperandGenerator g(selector);
  Node* base = node->InputAt(0);
  Node* index = node->InputAt(1);

  if (g.CanBeImmediate(index, opcode)) {
    selector->Emit(opcode | AddressingModeField::encode(kMode_MRI),
                   g.DefineAsRegister(output == nullptr ? node : output),
                   g.UseRegister(base), g.UseImmediate(index));
  } else {
    InstructionOperand addr_reg = g.TempRegister();
    selector->Emit(kMips64Dadd | AddressingModeField::encode(kMode_None),
                   addr_reg, g.UseRegister(index), g.UseRegister(base));
    // Emit desired load opcode, using temp addr_reg.
    selector->Emit(opcode | AddressingModeField::encode(kMode_MRI),
                   g.DefineAsRegister(output == nullptr ? node : output),
                   addr_reg, g.TempImmediate(0));
  }
}

void InstructionSelector::VisitLoad(Node* node) {
  LoadRepresentation load_rep = LoadRepresentationOf(node->op());

  ArchOpcode opcode = kArchNop;
  switch (load_rep.representation()) {
    case MachineRepresentation::kFloat32:
      opcode = kMips64Lwc1;
      break;
    case MachineRepresentation::kFloat64:
      opcode = kMips64Ldc1;
      break;
    case MachineRepresentation::kBit:  // Fall through.
    case MachineRepresentation::kWord8:
      opcode = load_rep.IsUnsigned() ? kMips64Lbu : kMips64Lb;
      break;
    case MachineRepresentation::kWord16:
      opcode = load_rep.IsUnsigned() ? kMips64Lhu : kMips64Lh;
      break;
    case MachineRepresentation::kWord32:
      opcode = load_rep.IsUnsigned() ? kMips64Lwu : kMips64Lw;
      break;
    case MachineRepresentation::kTaggedSigned:   // Fall through.
    case MachineRepresentation::kTaggedPointer:  // Fall through.
    case MachineRepresentation::kTagged:  // Fall through.
    case MachineRepresentation::kWord64:
      opcode = kMips64Ld;
      break;
    case MachineRepresentation::kSimd128:
      opcode = kMips64MsaLd;
      break;
    case MachineRepresentation::kNone:
      UNREACHABLE();
      return;
  }

  EmitLoad(this, node, opcode);
}

void InstructionSelector::VisitProtectedLoad(Node* node) {
  // TODO (eholk) id:2568
  UNIMPLEMENTED();
}

void InstructionSelector::VisitStore(Node* node) {
  Mips64OperandGenerator g(this);
  Node* base = node->InputAt(0);
  Node* index = node->InputAt(1);
  Node* value = node->InputAt(2);

  StoreRepresentation store_rep = StoreRepresentationOf(node->op());
  WriteBarrierKind write_barrier_kind = store_rep.write_barrier_kind();
  MachineRepresentation rep = store_rep.representation();

  // TODO (mips): I guess this could be done in a better way. id:1854
  if (write_barrier_kind != kNoWriteBarrier) {
    DCHECK(CanBeTaggedPointer(rep));
    InstructionOperand inputs[3];
    size_t input_count = 0;
    inputs[input_count++] = g.UseUniqueRegister(base);
    inputs[input_count++] = g.UseUniqueRegister(index);
    inputs[input_count++] = g.UseUniqueRegister(value);
    RecordWriteMode record_write_mode = RecordWriteMode::kValueIsAny;
    switch (write_barrier_kind) {
      case kNoWriteBarrier:
        UNREACHABLE();
        break;
      case kMapWriteBarrier:
        record_write_mode = RecordWriteMode::kValueIsMap;
        break;
      case kPointerWriteBarrier:
        record_write_mode = RecordWriteMode::kValueIsPointer;
        break;
      case kFullWriteBarrier:
        record_write_mode = RecordWriteMode::kValueIsAny;
        break;
    }
    InstructionOperand temps[] = {g.TempRegister(), g.TempRegister()};
    size_t const temp_count = arraysize(temps);
    InstructionCode code = kArchStoreWithWriteBarrier;
    code |= MiscField::encode(static_cast<int>(record_write_mode));
    Emit(code, 0, nullptr, input_count, inputs, temp_count, temps);
  } else {
    ArchOpcode opcode = kArchNop;
    switch (rep) {
      case MachineRepresentation::kFloat32:
        opcode = kMips64Swc1;
        break;
      case MachineRepresentation::kFloat64:
        opcode = kMips64Sdc1;
        break;
      case MachineRepresentation::kBit:  // Fall through.
      case MachineRepresentation::kWord8:
        opcode = kMips64Sb;
        break;
      case MachineRepresentation::kWord16:
        opcode = kMips64Sh;
        break;
      case MachineRepresentation::kWord32:
        opcode = kMips64Sw;
        break;
      case MachineRepresentation::kTaggedSigned:   // Fall through.
      case MachineRepresentation::kTaggedPointer:  // Fall through.
      case MachineRepresentation::kTagged:  // Fall through.
      case MachineRepresentation::kWord64:
        opcode = kMips64Sd;
        break;
      case MachineRepresentation::kSimd128:
        opcode = kMips64MsaSt;
        break;
      case MachineRepresentation::kNone:
        UNREACHABLE();
        return;
    }

    if (g.CanBeImmediate(index, opcode)) {
      Emit(opcode | AddressingModeField::encode(kMode_MRI), g.NoOutput(),
           g.UseRegister(base), g.UseImmediate(index),
           g.UseRegisterOrImmediateZero(value));
    } else {
      InstructionOperand addr_reg = g.TempRegister();
      Emit(kMips64Dadd | AddressingModeField::encode(kMode_None), addr_reg,
           g.UseRegister(index), g.UseRegister(base));
      // Emit desired store opcode, using temp addr_reg.
      Emit(opcode | AddressingModeField::encode(kMode_MRI), g.NoOutput(),
           addr_reg, g.TempImmediate(0), g.UseRegisterOrImmediateZero(value));
    }
  }
}

void InstructionSelector::VisitProtectedStore(Node* node) {
  // TODO (eholk) id:1966
  UNIMPLEMENTED();
}

void InstructionSelector::VisitWord32And(Node* node) {
  Mips64OperandGenerator g(this);
  Int32BinopMatcher m(node);
  if (m.left().IsWord32Shr() && CanCover(node, m.left().node()) &&
      m.right().HasValue()) {
    uint32_t mask = m.right().Value();
    uint32_t mask_width = base::bits::CountPopulation32(mask);
    uint32_t mask_msb = base::bits::CountLeadingZeros32(mask);
    if ((mask_width != 0) && (mask_msb + mask_width == 32)) {
      // The mask must be contiguous, and occupy the least-significant bits.
      DCHECK_EQ(0u, base::bits::CountTrailingZeros32(mask));

      // Select Ext for And(Shr(x, imm), mask) where the mask is in the least
      // significant bits.
      Int32BinopMatcher mleft(m.left().node());
      if (mleft.right().HasValue()) {
        // Any shift value can match; int32 shifts use `value % 32`.
        uint32_t lsb = mleft.right().Value() & 0x1f;

        // Ext cannot extract bits past the register size, however since
        // shifting the original value would have introduced some zeros we can
        // still use Ext with a smaller mask and the remaining bits will be
        // zeros.
        if (lsb + mask_width > 32) mask_width = 32 - lsb;

        Emit(kMips64Ext, g.DefineAsRegister(node),
             g.UseRegister(mleft.left().node()), g.TempImmediate(lsb),
             g.TempImmediate(mask_width));
        return;
      }
      // Other cases fall through to the normal And operation.
    }
  }
  if (m.right().HasValue()) {
    uint32_t mask = m.right().Value();
    uint32_t shift = base::bits::CountPopulation32(~mask);
    uint32_t msb = base::bits::CountLeadingZeros32(~mask);
    if (shift != 0 && shift != 32 && msb + shift == 32) {
      // Insert zeros for (x >> K) << K => x & ~(2^K - 1) expression reduction
      // and remove constant loading of inverted mask.
      Emit(kMips64Ins, g.DefineSameAsFirst(node),
           g.UseRegister(m.left().node()), g.TempImmediate(0),
           g.TempImmediate(shift));
      return;
    }
  }
  VisitBinop(this, node, kMips64And32, true, kMips64And32);
}


void InstructionSelector::VisitWord64And(Node* node) {
  Mips64OperandGenerator g(this);
  Int64BinopMatcher m(node);
  if (m.left().IsWord64Shr() && CanCover(node, m.left().node()) &&
      m.right().HasValue()) {
    uint64_t mask = m.right().Value();
    uint32_t mask_width = base::bits::CountPopulation64(mask);
    uint32_t mask_msb = base::bits::CountLeadingZeros64(mask);
    if ((mask_width != 0) && (mask_msb + mask_width == 64)) {
      // The mask must be contiguous, and occupy the least-significant bits.
      DCHECK_EQ(0u, base::bits::CountTrailingZeros64(mask));

      // Select Dext for And(Shr(x, imm), mask) where the mask is in the least
      // significant bits.
      Int64BinopMatcher mleft(m.left().node());
      if (mleft.right().HasValue()) {
        // Any shift value can match; int64 shifts use `value % 64`.
        uint32_t lsb = static_cast<uint32_t>(mleft.right().Value() & 0x3f);

        // Dext cannot extract bits past the register size, however since
        // shifting the original value would have introduced some zeros we can
        // still use Dext with a smaller mask and the remaining bits will be
        // zeros.
        if (lsb + mask_width > 64) mask_width = 64 - lsb;

        if (lsb == 0 && mask_width == 64) {
          Emit(kArchNop, g.DefineSameAsFirst(node), g.Use(mleft.left().node()));
        } else {
          Emit(kMips64Dext, g.DefineAsRegister(node),
               g.UseRegister(mleft.left().node()), g.TempImmediate(lsb),
               g.TempImmediate(static_cast<int32_t>(mask_width)));
        }
        return;
      }
      // Other cases fall through to the normal And operation.
    }
  }
  if (m.right().HasValue()) {
    uint64_t mask = m.right().Value();
    uint32_t shift = base::bits::CountPopulation64(~mask);
    uint32_t msb = base::bits::CountLeadingZeros64(~mask);
    if (shift != 0 && shift < 32 && msb + shift == 64) {
      // Insert zeros for (x >> K) << K => x & ~(2^K - 1) expression reduction
      // and remove constant loading of inverted mask. Dins cannot insert bits
      // past word size, so shifts smaller than 32 are covered.
      Emit(kMips64Dins, g.DefineSameAsFirst(node),
           g.UseRegister(m.left().node()), g.TempImmediate(0),
           g.TempImmediate(shift));
      return;
    }
  }
  VisitBinop(this, node, kMips64And, true, kMips64And);
}


void InstructionSelector::VisitWord32Or(Node* node) {
  VisitBinop(this, node, kMips64Or32, true, kMips64Or32);
}


void InstructionSelector::VisitWord64Or(Node* node) {
  VisitBinop(this, node, kMips64Or, true, kMips64Or);
}


void InstructionSelector::VisitWord32Xor(Node* node) {
  Int32BinopMatcher m(node);
  if (m.left().IsWord32Or() && CanCover(node, m.left().node()) &&
      m.right().Is(-1)) {
    Int32BinopMatcher mleft(m.left().node());
    if (!mleft.right().HasValue()) {
      Mips64OperandGenerator g(this);
      Emit(kMips64Nor32, g.DefineAsRegister(node),
           g.UseRegister(mleft.left().node()),
           g.UseRegister(mleft.right().node()));
      return;
    }
  }
  if (m.right().Is(-1)) {
    // Use Nor for bit negation and eliminate constant loading for xori.
    Mips64OperandGenerator g(this);
    Emit(kMips64Nor32, g.DefineAsRegister(node), g.UseRegister(m.left().node()),
         g.TempImmediate(0));
    return;
  }
  VisitBinop(this, node, kMips64Xor32, true, kMips64Xor32);
}


void InstructionSelector::VisitWord64Xor(Node* node) {
  Int64BinopMatcher m(node);
  if (m.left().IsWord64Or() && CanCover(node, m.left().node()) &&
      m.right().Is(-1)) {
    Int64BinopMatcher mleft(m.left().node());
    if (!mleft.right().HasValue()) {
      Mips64OperandGenerator g(this);
      Emit(kMips64Nor, g.DefineAsRegister(node),
           g.UseRegister(mleft.left().node()),
           g.UseRegister(mleft.right().node()));
      return;
    }
  }
  if (m.right().Is(-1)) {
    // Use Nor for bit negation and eliminate constant loading for xori.
    Mips64OperandGenerator g(this);
    Emit(kMips64Nor, g.DefineAsRegister(node), g.UseRegister(m.left().node()),
         g.TempImmediate(0));
    return;
  }
  VisitBinop(this, node, kMips64Xor, true, kMips64Xor);
}


void InstructionSelector::VisitWord32Shl(Node* node) {
  Int32BinopMatcher m(node);
  if (m.left().IsWord32And() && CanCover(node, m.left().node()) &&
      m.right().IsInRange(1, 31)) {
    Mips64OperandGenerator g(this);
    Int32BinopMatcher mleft(m.left().node());
    // Match Word32Shl(Word32And(x, mask), imm) to Shl where the mask is
    // contiguous, and the shift immediate non-zero.
    if (mleft.right().HasValue()) {
      uint32_t mask = mleft.right().Value();
      uint32_t mask_width = base::bits::CountPopulation32(mask);
      uint32_t mask_msb = base::bits::CountLeadingZeros32(mask);
      if ((mask_width != 0) && (mask_msb + mask_width == 32)) {
        uint32_t shift = m.right().Value();
        DCHECK_EQ(0u, base::bits::CountTrailingZeros32(mask));
        DCHECK_NE(0u, shift);
        if ((shift + mask_width) >= 32) {
          // If the mask is contiguous and reaches or extends beyond the top
          // bit, only the shift is needed.
          Emit(kMips64Shl, g.DefineAsRegister(node),
               g.UseRegister(mleft.left().node()),
               g.UseImmediate(m.right().node()));
          return;
        }
      }
    }
  }
  VisitRRO(this, kMips64Shl, node);
}


void InstructionSelector::VisitWord32Shr(Node* node) {
  Int32BinopMatcher m(node);
  if (m.left().IsWord32And() && m.right().HasValue()) {
    uint32_t lsb = m.right().Value() & 0x1f;
    Int32BinopMatcher mleft(m.left().node());
    if (mleft.right().HasValue() && mleft.right().Value() != 0) {
      // Select Ext for Shr(And(x, mask), imm) where the result of the mask is
      // shifted into the least-significant bits.
      uint32_t mask = (mleft.right().Value() >> lsb) << lsb;
      unsigned mask_width = base::bits::CountPopulation32(mask);
      unsigned mask_msb = base::bits::CountLeadingZeros32(mask);
      if ((mask_msb + mask_width + lsb) == 32) {
        Mips64OperandGenerator g(this);
        DCHECK_EQ(lsb, base::bits::CountTrailingZeros32(mask));
        Emit(kMips64Ext, g.DefineAsRegister(node),
             g.UseRegister(mleft.left().node()), g.TempImmediate(lsb),
             g.TempImmediate(mask_width));
        return;
      }
    }
  }
  VisitRRO(this, kMips64Shr, node);
}


void InstructionSelector::VisitWord32Sar(Node* node) {
  Int32BinopMatcher m(node);
  if (m.left().IsWord32Shl() && CanCover(node, m.left().node())) {
    Int32BinopMatcher mleft(m.left().node());
    if (m.right().HasValue() && mleft.right().HasValue()) {
      Mips64OperandGenerator g(this);
      uint32_t sar = m.right().Value();
      uint32_t shl = mleft.right().Value();
      if ((sar == shl) && (sar == 16)) {
        Emit(kMips64Seh, g.DefineAsRegister(node),
             g.UseRegister(mleft.left().node()));
        return;
      } else if ((sar == shl) && (sar == 24)) {
        Emit(kMips64Seb, g.DefineAsRegister(node),
             g.UseRegister(mleft.left().node()));
        return;
      } else if ((sar == shl) && (sar == 32)) {
        Emit(kMips64Shl, g.DefineAsRegister(node),
             g.UseRegister(mleft.left().node()), g.TempImmediate(0));
        return;
      }
    }
  }
  VisitRRO(this, kMips64Sar, node);
}


void InstructionSelector::VisitWord64Shl(Node* node) {
  Mips64OperandGenerator g(this);
  Int64BinopMatcher m(node);
  if ((m.left().IsChangeInt32ToInt64() || m.left().IsChangeUint32ToUint64()) &&
      m.right().IsInRange(32, 63) && CanCover(node, m.left().node())) {
    // There's no need to sign/zero-extend to 64-bit if we shift out the upper
    // 32 bits anyway.
    Emit(kMips64Dshl, g.DefineSameAsFirst(node),
         g.UseRegister(m.left().node()->InputAt(0)),
         g.UseImmediate(m.right().node()));
    return;
  }
  if (m.left().IsWord64And() && CanCover(node, m.left().node()) &&
      m.right().IsInRange(1, 63)) {
    // Match Word64Shl(Word64And(x, mask), imm) to Dshl where the mask is
    // contiguous, and the shift immediate non-zero.
    Int64BinopMatcher mleft(m.left().node());
    if (mleft.right().HasValue()) {
      uint64_t mask = mleft.right().Value();
      uint32_t mask_width = base::bits::CountPopulation64(mask);
      uint32_t mask_msb = base::bits::CountLeadingZeros64(mask);
      if ((mask_width != 0) && (mask_msb + mask_width == 64)) {
        uint64_t shift = m.right().Value();
        DCHECK_EQ(0u, base::bits::CountTrailingZeros64(mask));
        DCHECK_NE(0u, shift);

        if ((shift + mask_width) >= 64) {
          // If the mask is contiguous and reaches or extends beyond the top
          // bit, only the shift is needed.
          Emit(kMips64Dshl, g.DefineAsRegister(node),
               g.UseRegister(mleft.left().node()),
               g.UseImmediate(m.right().node()));
          return;
        }
      }
    }
  }
  VisitRRO(this, kMips64Dshl, node);
}


void InstructionSelector::VisitWord64Shr(Node* node) {
  Int64BinopMatcher m(node);
  if (m.left().IsWord64And() && m.right().HasValue()) {
    uint32_t lsb = m.right().Value() & 0x3f;
    Int64BinopMatcher mleft(m.left().node());
    if (mleft.right().HasValue() && mleft.right().Value() != 0) {
      // Select Dext for Shr(And(x, mask), imm) where the result of the mask is
      // shifted into the least-significant bits.
      uint64_t mask = (mleft.right().Value() >> lsb) << lsb;
      unsigned mask_width = base::bits::CountPopulation64(mask);
      unsigned mask_msb = base::bits::CountLeadingZeros64(mask);
      if ((mask_msb + mask_width + lsb) == 64) {
        Mips64OperandGenerator g(this);
        DCHECK_EQ(lsb, base::bits::CountTrailingZeros64(mask));
        Emit(kMips64Dext, g.DefineAsRegister(node),
             g.UseRegister(mleft.left().node()), g.TempImmediate(lsb),
             g.TempImmediate(mask_width));
        return;
      }
    }
  }
  VisitRRO(this, kMips64Dshr, node);
}


void InstructionSelector::VisitWord64Sar(Node* node) {
  if (TryEmitExtendingLoad(this, node, node)) return;
  VisitRRO(this, kMips64Dsar, node);
}


void InstructionSelector::VisitWord32Ror(Node* node) {
  VisitRRO(this, kMips64Ror, node);
}


void InstructionSelector::VisitWord32Clz(Node* node) {
  VisitRR(this, kMips64Clz, node);
}


void InstructionSelector::VisitWord32ReverseBits(Node* node) { UNREACHABLE(); }


void InstructionSelector::VisitWord64ReverseBits(Node* node) { UNREACHABLE(); }

void InstructionSelector::VisitWord64ReverseBytes(Node* node) {
  Mips64OperandGenerator g(this);
  Emit(kMips64ByteSwap64, g.DefineAsRegister(node),
       g.UseRegister(node->InputAt(0)));
}

void InstructionSelector::VisitWord32ReverseBytes(Node* node) {
  Mips64OperandGenerator g(this);
  Emit(kMips64ByteSwap32, g.DefineAsRegister(node),
       g.UseRegister(node->InputAt(0)));
}

void InstructionSelector::VisitWord32Ctz(Node* node) {
  Mips64OperandGenerator g(this);
  Emit(kMips64Ctz, g.DefineAsRegister(node), g.UseRegister(node->InputAt(0)));
}


void InstructionSelector::VisitWord64Ctz(Node* node) {
  Mips64OperandGenerator g(this);
  Emit(kMips64Dctz, g.DefineAsRegister(node), g.UseRegister(node->InputAt(0)));
}


void InstructionSelector::VisitWord32Popcnt(Node* node) {
  Mips64OperandGenerator g(this);
  Emit(kMips64Popcnt, g.DefineAsRegister(node),
       g.UseRegister(node->InputAt(0)));
}


void InstructionSelector::VisitWord64Popcnt(Node* node) {
  Mips64OperandGenerator g(this);
  Emit(kMips64Dpopcnt, g.DefineAsRegister(node),
       g.UseRegister(node->InputAt(0)));
}


void InstructionSelector::VisitWord64Ror(Node* node) {
  VisitRRO(this, kMips64Dror, node);
}


void InstructionSelector::VisitWord64Clz(Node* node) {
  VisitRR(this, kMips64Dclz, node);
}


void InstructionSelector::VisitInt32Add(Node* node) {
  Mips64OperandGenerator g(this);
  Int32BinopMatcher m(node);

  // Select Lsa for (left + (left_of_right << imm)).
  if (m.right().opcode() == IrOpcode::kWord32Shl &&
      CanCover(node, m.left().node()) && CanCover(node, m.right().node())) {
    Int32BinopMatcher mright(m.right().node());
    if (mright.right().HasValue() && !m.left().HasValue()) {
      int32_t shift_value = static_cast<int32_t>(mright.right().Value());
      Emit(kMips64Lsa, g.DefineAsRegister(node), g.UseRegister(m.left().node()),
           g.UseRegister(mright.left().node()), g.TempImmediate(shift_value));
      return;
    }
  }

  // Select Lsa for ((left_of_left << imm) + right).
  if (m.left().opcode() == IrOpcode::kWord32Shl &&
      CanCover(node, m.right().node()) && CanCover(node, m.left().node())) {
    Int32BinopMatcher mleft(m.left().node());
    if (mleft.right().HasValue() && !m.right().HasValue()) {
      int32_t shift_value = static_cast<int32_t>(mleft.right().Value());
      Emit(kMips64Lsa, g.DefineAsRegister(node),
           g.UseRegister(m.right().node()), g.UseRegister(mleft.left().node()),
           g.TempImmediate(shift_value));
      return;
    }
  }
  VisitBinop(this, node, kMips64Add, true, kMips64Add);
}


void InstructionSelector::VisitInt64Add(Node* node) {
  Mips64OperandGenerator g(this);
  Int64BinopMatcher m(node);

  // Select Dlsa for (left + (left_of_right << imm)).
  if (m.right().opcode() == IrOpcode::kWord64Shl &&
      CanCover(node, m.left().node()) && CanCover(node, m.right().node())) {
    Int64BinopMatcher mright(m.right().node());
    if (mright.right().HasValue() && !m.left().HasValue()) {
      int32_t shift_value = static_cast<int32_t>(mright.right().Value());
      Emit(kMips64Dlsa, g.DefineAsRegister(node),
           g.UseRegister(m.left().node()), g.UseRegister(mright.left().node()),
           g.TempImmediate(shift_value));
      return;
    }
  }

  // Select Dlsa for ((left_of_left << imm) + right).
  if (m.left().opcode() == IrOpcode::kWord64Shl &&
      CanCover(node, m.right().node()) && CanCover(node, m.left().node())) {
    Int64BinopMatcher mleft(m.left().node());
    if (mleft.right().HasValue() && !m.right().HasValue()) {
      int32_t shift_value = static_cast<int32_t>(mleft.right().Value());
      Emit(kMips64Dlsa, g.DefineAsRegister(node),
           g.UseRegister(m.right().node()), g.UseRegister(mleft.left().node()),
           g.TempImmediate(shift_value));
      return;
    }
  }

  VisitBinop(this, node, kMips64Dadd, true, kMips64Dadd);
}


void InstructionSelector::VisitInt32Sub(Node* node) {
  VisitBinop(this, node, kMips64Sub);
}


void InstructionSelector::VisitInt64Sub(Node* node) {
  VisitBinop(this, node, kMips64Dsub);
}


void InstructionSelector::VisitInt32Mul(Node* node) {
  Mips64OperandGenerator g(this);
  Int32BinopMatcher m(node);
  if (m.right().HasValue() && m.right().Value() > 0) {
    uint32_t value = static_cast<uint32_t>(m.right().Value());
    if (base::bits::IsPowerOfTwo(value)) {
      Emit(kMips64Shl | AddressingModeField::encode(kMode_None),
           g.DefineAsRegister(node), g.UseRegister(m.left().node()),
           g.TempImmediate(WhichPowerOf2(value)));
      return;
    }
    if (base::bits::IsPowerOfTwo(value - 1)) {
      Emit(kMips64Lsa, g.DefineAsRegister(node), g.UseRegister(m.left().node()),
           g.UseRegister(m.left().node()),
           g.TempImmediate(WhichPowerOf2(value - 1)));
      return;
    }
    if (base::bits::IsPowerOfTwo(value + 1)) {
      InstructionOperand temp = g.TempRegister();
      Emit(kMips64Shl | AddressingModeField::encode(kMode_None), temp,
           g.UseRegister(m.left().node()),
           g.TempImmediate(WhichPowerOf2(value + 1)));
      Emit(kMips64Sub | AddressingModeField::encode(kMode_None),
           g.DefineAsRegister(node), temp, g.UseRegister(m.left().node()));
      return;
    }
  }
  Node* left = node->InputAt(0);
  Node* right = node->InputAt(1);
  if (CanCover(node, left) && CanCover(node, right)) {
    if (left->opcode() == IrOpcode::kWord64Sar &&
        right->opcode() == IrOpcode::kWord64Sar) {
      Int64BinopMatcher leftInput(left), rightInput(right);
      if (leftInput.right().Is(32) && rightInput.right().Is(32)) {
        // Combine untagging shifts with Dmul high.
        Emit(kMips64DMulHigh, g.DefineSameAsFirst(node),
             g.UseRegister(leftInput.left().node()),
             g.UseRegister(rightInput.left().node()));
        return;
      }
    }
  }
  VisitRRR(this, kMips64Mul, node);
}


void InstructionSelector::VisitInt32MulHigh(Node* node) {
  VisitRRR(this, kMips64MulHigh, node);
}


void InstructionSelector::VisitUint32MulHigh(Node* node) {
  VisitRRR(this, kMips64MulHighU, node);
}


void InstructionSelector::VisitInt64Mul(Node* node) {
  Mips64OperandGenerator g(this);
  Int64BinopMatcher m(node);
  // TODO (dusmil): Add optimization for shifts larger than 32. id:1818
  if (m.right().HasValue() && m.right().Value() > 0) {
    uint32_t value = static_cast<uint32_t>(m.right().Value());
    if (base::bits::IsPowerOfTwo(value)) {
      Emit(kMips64Dshl | AddressingModeField::encode(kMode_None),
           g.DefineAsRegister(node), g.UseRegister(m.left().node()),
           g.TempImmediate(WhichPowerOf2(value)));
      return;
    }
    if (base::bits::IsPowerOfTwo(value - 1)) {
      // Dlsa macro will handle the shifting value out of bound cases.
      Emit(kMips64Dlsa, g.DefineAsRegister(node),
           g.UseRegister(m.left().node()), g.UseRegister(m.left().node()),
           g.TempImmediate(WhichPowerOf2(value - 1)));
      return;
    }
    if (base::bits::IsPowerOfTwo(value + 1)) {
      InstructionOperand temp = g.TempRegister();
      Emit(kMips64Dshl | AddressingModeField::encode(kMode_None), temp,
           g.UseRegister(m.left().node()),
           g.TempImmediate(WhichPowerOf2(value + 1)));
      Emit(kMips64Dsub | AddressingModeField::encode(kMode_None),
           g.DefineAsRegister(node), temp, g.UseRegister(m.left().node()));
      return;
    }
  }
  Emit(kMips64Dmul, g.DefineAsRegister(node), g.UseRegister(m.left().node()),
       g.UseRegister(m.right().node()));
}


void InstructionSelector::VisitInt32Div(Node* node) {
  Mips64OperandGenerator g(this);
  Int32BinopMatcher m(node);
  Node* left = node->InputAt(0);
  Node* right = node->InputAt(1);
  if (CanCover(node, left) && CanCover(node, right)) {
    if (left->opcode() == IrOpcode::kWord64Sar &&
        right->opcode() == IrOpcode::kWord64Sar) {
      Int64BinopMatcher rightInput(right), leftInput(left);
      if (rightInput.right().Is(32) && leftInput.right().Is(32)) {
        // Combine both shifted operands with Ddiv.
        Emit(kMips64Ddiv, g.DefineSameAsFirst(node),
             g.UseRegister(leftInput.left().node()),
             g.UseRegister(rightInput.left().node()));
        return;
      }
    }
  }
  Emit(kMips64Div, g.DefineSameAsFirst(node), g.UseRegister(m.left().node()),
       g.UseRegister(m.right().node()));
}


void InstructionSelector::VisitUint32Div(Node* node) {
  Mips64OperandGenerator g(this);
  Int32BinopMatcher m(node);
  Emit(kMips64DivU, g.DefineSameAsFirst(node), g.UseRegister(m.left().node()),
       g.UseRegister(m.right().node()));
}


void InstructionSelector::VisitInt32Mod(Node* node) {
  Mips64OperandGenerator g(this);
  Int32BinopMatcher m(node);
  Node* left = node->InputAt(0);
  Node* right = node->InputAt(1);
  if (CanCover(node, left) && CanCover(node, right)) {
    if (left->opcode() == IrOpcode::kWord64Sar &&
        right->opcode() == IrOpcode::kWord64Sar) {
      Int64BinopMatcher rightInput(right), leftInput(left);
      if (rightInput.right().Is(32) && leftInput.right().Is(32)) {
        // Combine both shifted operands with Dmod.
        Emit(kMips64Dmod, g.DefineSameAsFirst(node),
             g.UseRegister(leftInput.left().node()),
             g.UseRegister(rightInput.left().node()));
        return;
      }
    }
  }
  Emit(kMips64Mod, g.DefineAsRegister(node), g.UseRegister(m.left().node()),
       g.UseRegister(m.right().node()));
}


void InstructionSelector::VisitUint32Mod(Node* node) {
  Mips64OperandGenerator g(this);
  Int32BinopMatcher m(node);
  Emit(kMips64ModU, g.DefineAsRegister(node), g.UseRegister(m.left().node()),
       g.UseRegister(m.right().node()));
}


void InstructionSelector::VisitInt64Div(Node* node) {
  Mips64OperandGenerator g(this);
  Int64BinopMatcher m(node);
  Emit(kMips64Ddiv, g.DefineSameAsFirst(node), g.UseRegister(m.left().node()),
       g.UseRegister(m.right().node()));
}


void InstructionSelector::VisitUint64Div(Node* node) {
  Mips64OperandGenerator g(this);
  Int64BinopMatcher m(node);
  Emit(kMips64DdivU, g.DefineSameAsFirst(node), g.UseRegister(m.left().node()),
       g.UseRegister(m.right().node()));
}


void InstructionSelector::VisitInt64Mod(Node* node) {
  Mips64OperandGenerator g(this);
  Int64BinopMatcher m(node);
  Emit(kMips64Dmod, g.DefineAsRegister(node), g.UseRegister(m.left().node()),
       g.UseRegister(m.right().node()));
}


void InstructionSelector::VisitUint64Mod(Node* node) {
  Mips64OperandGenerator g(this);
  Int64BinopMatcher m(node);
  Emit(kMips64DmodU, g.DefineAsRegister(node), g.UseRegister(m.left().node()),
       g.UseRegister(m.right().node()));
}


void InstructionSelector::VisitChangeFloat32ToFloat64(Node* node) {
  VisitRR(this, kMips64CvtDS, node);
}


void InstructionSelector::VisitRoundInt32ToFloat32(Node* node) {
  VisitRR(this, kMips64CvtSW, node);
}


void InstructionSelector::VisitRoundUint32ToFloat32(Node* node) {
  VisitRR(this, kMips64CvtSUw, node);
}


void InstructionSelector::VisitChangeInt32ToFloat64(Node* node) {
  VisitRR(this, kMips64CvtDW, node);
}


void InstructionSelector::VisitChangeUint32ToFloat64(Node* node) {
  VisitRR(this, kMips64CvtDUw, node);
}


void InstructionSelector::VisitTruncateFloat32ToInt32(Node* node) {
  VisitRR(this, kMips64TruncWS, node);
}


void InstructionSelector::VisitTruncateFloat32ToUint32(Node* node) {
  VisitRR(this, kMips64TruncUwS, node);
}


void InstructionSelector::VisitChangeFloat64ToInt32(Node* node) {
  Mips64OperandGenerator g(this);
  Node* value = node->InputAt(0);
  // Match ChangeFloat64ToInt32(Float64Round##OP) to corresponding instruction
  // which does rounding and conversion to integer format.
  if (CanCover(node, value)) {
    switch (value->opcode()) {
      case IrOpcode::kFloat64RoundDown:
        Emit(kMips64FloorWD, g.DefineAsRegister(node),
             g.UseRegister(value->InputAt(0)));
        return;
      case IrOpcode::kFloat64RoundUp:
        Emit(kMips64CeilWD, g.DefineAsRegister(node),
             g.UseRegister(value->InputAt(0)));
        return;
      case IrOpcode::kFloat64RoundTiesEven:
        Emit(kMips64RoundWD, g.DefineAsRegister(node),
             g.UseRegister(value->InputAt(0)));
        return;
      case IrOpcode::kFloat64RoundTruncate:
        Emit(kMips64TruncWD, g.DefineAsRegister(node),
             g.UseRegister(value->InputAt(0)));
        return;
      default:
        break;
    }
    if (value->opcode() == IrOpcode::kChangeFloat32ToFloat64) {
      Node* next = value->InputAt(0);
      if (CanCover(value, next)) {
        // Match ChangeFloat64ToInt32(ChangeFloat32ToFloat64(Float64Round##OP))
        switch (next->opcode()) {
          case IrOpcode::kFloat32RoundDown:
            Emit(kMips64FloorWS, g.DefineAsRegister(node),
                 g.UseRegister(next->InputAt(0)));
            return;
          case IrOpcode::kFloat32RoundUp:
            Emit(kMips64CeilWS, g.DefineAsRegister(node),
                 g.UseRegister(next->InputAt(0)));
            return;
          case IrOpcode::kFloat32RoundTiesEven:
            Emit(kMips64RoundWS, g.DefineAsRegister(node),
                 g.UseRegister(next->InputAt(0)));
            return;
          case IrOpcode::kFloat32RoundTruncate:
            Emit(kMips64TruncWS, g.DefineAsRegister(node),
                 g.UseRegister(next->InputAt(0)));
            return;
          default:
            Emit(kMips64TruncWS, g.DefineAsRegister(node),
                 g.UseRegister(value->InputAt(0)));
            return;
        }
      } else {
        // Match float32 -> float64 -> int32 representation change path.
        Emit(kMips64TruncWS, g.DefineAsRegister(node),
             g.UseRegister(value->InputAt(0)));
        return;
      }
    }
  }
  VisitRR(this, kMips64TruncWD, node);
}


void InstructionSelector::VisitChangeFloat64ToUint32(Node* node) {
  VisitRR(this, kMips64TruncUwD, node);
}

void InstructionSelector::VisitChangeFloat64ToUint64(Node* node) {
  VisitRR(this, kMips64TruncUlD, node);
}

void InstructionSelector::VisitTruncateFloat64ToUint32(Node* node) {
  VisitRR(this, kMips64TruncUwD, node);
}

void InstructionSelector::VisitTryTruncateFloat32ToInt64(Node* node) {
  Mips64OperandGenerator g(this);
  InstructionOperand inputs[] = {g.UseRegister(node->InputAt(0))};
  InstructionOperand outputs[2];
  size_t output_count = 0;
  outputs[output_count++] = g.DefineAsRegister(node);

  Node* success_output = NodeProperties::FindProjection(node, 1);
  if (success_output) {
    outputs[output_count++] = g.DefineAsRegister(success_output);
  }

  this->Emit(kMips64TruncLS, output_count, outputs, 1, inputs);
}


void InstructionSelector::VisitTryTruncateFloat64ToInt64(Node* node) {
  Mips64OperandGenerator g(this);
  InstructionOperand inputs[] = {g.UseRegister(node->InputAt(0))};
  InstructionOperand outputs[2];
  size_t output_count = 0;
  outputs[output_count++] = g.DefineAsRegister(node);

  Node* success_output = NodeProperties::FindProjection(node, 1);
  if (success_output) {
    outputs[output_count++] = g.DefineAsRegister(success_output);
  }

  Emit(kMips64TruncLD, output_count, outputs, 1, inputs);
}


void InstructionSelector::VisitTryTruncateFloat32ToUint64(Node* node) {
  Mips64OperandGenerator g(this);
  InstructionOperand inputs[] = {g.UseRegister(node->InputAt(0))};
  InstructionOperand outputs[2];
  size_t output_count = 0;
  outputs[output_count++] = g.DefineAsRegister(node);

  Node* success_output = NodeProperties::FindProjection(node, 1);
  if (success_output) {
    outputs[output_count++] = g.DefineAsRegister(success_output);
  }

  Emit(kMips64TruncUlS, output_count, outputs, 1, inputs);
}


void InstructionSelector::VisitTryTruncateFloat64ToUint64(Node* node) {
  Mips64OperandGenerator g(this);

  InstructionOperand inputs[] = {g.UseRegister(node->InputAt(0))};
  InstructionOperand outputs[2];
  size_t output_count = 0;
  outputs[output_count++] = g.DefineAsRegister(node);

  Node* success_output = NodeProperties::FindProjection(node, 1);
  if (success_output) {
    outputs[output_count++] = g.DefineAsRegister(success_output);
  }

  Emit(kMips64TruncUlD, output_count, outputs, 1, inputs);
}


void InstructionSelector::VisitChangeInt32ToInt64(Node* node) {
  Node* value = node->InputAt(0);
  if (value->opcode() == IrOpcode::kLoad && CanCover(node, value)) {
    // Generate sign-extending load.
    LoadRepresentation load_rep = LoadRepresentationOf(value->op());
    InstructionCode opcode = kArchNop;
    switch (load_rep.representation()) {
      case MachineRepresentation::kBit:  // Fall through.
      case MachineRepresentation::kWord8:
        opcode = load_rep.IsUnsigned() ? kMips64Lbu : kMips64Lb;
        break;
      case MachineRepresentation::kWord16:
        opcode = load_rep.IsUnsigned() ? kMips64Lhu : kMips64Lh;
        break;
      case MachineRepresentation::kWord32:
        opcode = kMips64Lw;
        break;
      default:
        UNREACHABLE();
        return;
    }
    EmitLoad(this, value, opcode, node);
  } else {
    Mips64OperandGenerator g(this);
    Emit(kMips64Shl, g.DefineAsRegister(node), g.UseRegister(node->InputAt(0)),
         g.TempImmediate(0));
  }
}


void InstructionSelector::VisitChangeUint32ToUint64(Node* node) {
  Mips64OperandGenerator g(this);
  Node* value = node->InputAt(0);
  switch (value->opcode()) {
    // 32-bit operations will write their result in a 64 bit register,
    // clearing the top 32 bits of the destination register.
    case IrOpcode::kUint32Div:
    case IrOpcode::kUint32Mod:
    case IrOpcode::kUint32MulHigh: {
      Emit(kArchNop, g.DefineSameAsFirst(node), g.Use(value));
      return;
    }
    case IrOpcode::kLoad: {
      LoadRepresentation load_rep = LoadRepresentationOf(value->op());
      if (load_rep.IsUnsigned()) {
        switch (load_rep.representation()) {
          case MachineRepresentation::kWord8:
          case MachineRepresentation::kWord16:
          case MachineRepresentation::kWord32:
            Emit(kArchNop, g.DefineSameAsFirst(node), g.Use(value));
            return;
          default:
            break;
        }
      }
    }
    default:
      break;
  }
  Emit(kMips64Dext, g.DefineAsRegister(node), g.UseRegister(node->InputAt(0)),
       g.TempImmediate(0), g.TempImmediate(32));
}


void InstructionSelector::VisitTruncateInt64ToInt32(Node* node) {
  Mips64OperandGenerator g(this);
  Node* value = node->InputAt(0);
  if (CanCover(node, value)) {
    switch (value->opcode()) {
      case IrOpcode::kWord64Sar: {
        if (TryEmitExtendingLoad(this, value, node)) {
          return;
        } else {
          Int64BinopMatcher m(value);
          if (m.right().IsInRange(32, 63)) {
            // After smi untagging no need for truncate. Combine sequence.
            Emit(kMips64Dsar, g.DefineSameAsFirst(node),
                 g.UseRegister(m.left().node()),
                 g.UseImmediate(m.right().node()));
            return;
          }
        }
        break;
      }
      default:
        break;
    }
  }
  Emit(kMips64Ext, g.DefineAsRegister(node), g.UseRegister(node->InputAt(0)),
       g.TempImmediate(0), g.TempImmediate(32));
}


void InstructionSelector::VisitTruncateFloat64ToFloat32(Node* node) {
  Mips64OperandGenerator g(this);
  Node* value = node->InputAt(0);
  // Match TruncateFloat64ToFloat32(ChangeInt32ToFloat64) to corresponding
  // instruction.
  if (CanCover(node, value) &&
      value->opcode() == IrOpcode::kChangeInt32ToFloat64) {
    Emit(kMips64CvtSW, g.DefineAsRegister(node),
         g.UseRegister(value->InputAt(0)));
    return;
  }
  VisitRR(this, kMips64CvtSD, node);
}

void InstructionSelector::VisitTruncateFloat64ToWord32(Node* node) {
  VisitRR(this, kArchTruncateDoubleToI, node);
}

void InstructionSelector::VisitRoundFloat64ToInt32(Node* node) {
  VisitRR(this, kMips64TruncWD, node);
}

void InstructionSelector::VisitRoundInt64ToFloat32(Node* node) {
  VisitRR(this, kMips64CvtSL, node);
}


void InstructionSelector::VisitRoundInt64ToFloat64(Node* node) {
  VisitRR(this, kMips64CvtDL, node);
}


void InstructionSelector::VisitRoundUint64ToFloat32(Node* node) {
  VisitRR(this, kMips64CvtSUl, node);
}


void InstructionSelector::VisitRoundUint64ToFloat64(Node* node) {
  VisitRR(this, kMips64CvtDUl, node);
}


void InstructionSelector::VisitBitcastFloat32ToInt32(Node* node) {
  VisitRR(this, kMips64Float64ExtractLowWord32, node);
}


void InstructionSelector::VisitBitcastFloat64ToInt64(Node* node) {
  VisitRR(this, kMips64BitcastDL, node);
}


void InstructionSelector::VisitBitcastInt32ToFloat32(Node* node) {
  Mips64OperandGenerator g(this);
  Emit(kMips64Float64InsertLowWord32, g.DefineAsRegister(node),
       ImmediateOperand(ImmediateOperand::INLINE, 0),
       g.UseRegister(node->InputAt(0)));
}


void InstructionSelector::VisitBitcastInt64ToFloat64(Node* node) {
  VisitRR(this, kMips64BitcastLD, node);
}


void InstructionSelector::VisitFloat32Add(Node* node) {
  // Optimization with Madd.S(z, x, y) is intentionally removed.
  // See explanation for madd_s in assembler-mips64.cc.
  VisitRRR(this, kMips64AddS, node);
}


void InstructionSelector::VisitFloat64Add(Node* node) {
  // Optimization with Madd.D(z, x, y) is intentionally removed.
  // See explanation for madd_d in assembler-mips64.cc.
  VisitRRR(this, kMips64AddD, node);
}


void InstructionSelector::VisitFloat32Sub(Node* node) {
  // Optimization with Msub.S(z, x, y) is intentionally removed.
  // See explanation for madd_s in assembler-mips64.cc.
  VisitRRR(this, kMips64SubS, node);
}

void InstructionSelector::VisitFloat64Sub(Node* node) {
  // Optimization with Msub.D(z, x, y) is intentionally removed.
  // See explanation for madd_d in assembler-mips64.cc.
  VisitRRR(this, kMips64SubD, node);
}

void InstructionSelector::VisitFloat32Mul(Node* node) {
  VisitRRR(this, kMips64MulS, node);
}


void InstructionSelector::VisitFloat64Mul(Node* node) {
  VisitRRR(this, kMips64MulD, node);
}


void InstructionSelector::VisitFloat32Div(Node* node) {
  VisitRRR(this, kMips64DivS, node);
}


void InstructionSelector::VisitFloat64Div(Node* node) {
  VisitRRR(this, kMips64DivD, node);
}


void InstructionSelector::VisitFloat64Mod(Node* node) {
  Mips64OperandGenerator g(this);
  Emit(kMips64ModD, g.DefineAsFixed(node, f0),
       g.UseFixed(node->InputAt(0), f12),
       g.UseFixed(node->InputAt(1), f14))->MarkAsCall();
}

void InstructionSelector::VisitFloat32Max(Node* node) {
  Mips64OperandGenerator g(this);
  Emit(kMips64Float32Max, g.DefineAsRegister(node),
       g.UseRegister(node->InputAt(0)), g.UseRegister(node->InputAt(1)));
}

void InstructionSelector::VisitFloat64Max(Node* node) {
  Mips64OperandGenerator g(this);
  Emit(kMips64Float64Max, g.DefineAsRegister(node),
       g.UseRegister(node->InputAt(0)), g.UseRegister(node->InputAt(1)));
}

void InstructionSelector::VisitFloat32Min(Node* node) {
  Mips64OperandGenerator g(this);
  Emit(kMips64Float32Min, g.DefineAsRegister(node),
       g.UseRegister(node->InputAt(0)), g.UseRegister(node->InputAt(1)));
}

void InstructionSelector::VisitFloat64Min(Node* node) {
  Mips64OperandGenerator g(this);
  Emit(kMips64Float64Min, g.DefineAsRegister(node),
       g.UseRegister(node->InputAt(0)), g.UseRegister(node->InputAt(1)));
}


void InstructionSelector::VisitFloat32Abs(Node* node) {
  VisitRR(this, kMips64AbsS, node);
}


void InstructionSelector::VisitFloat64Abs(Node* node) {
  VisitRR(this, kMips64AbsD, node);
}

void InstructionSelector::VisitFloat32Sqrt(Node* node) {
  VisitRR(this, kMips64SqrtS, node);
}


void InstructionSelector::VisitFloat64Sqrt(Node* node) {
  VisitRR(this, kMips64SqrtD, node);
}


void InstructionSelector::VisitFloat32RoundDown(Node* node) {
  VisitRR(this, kMips64Float32RoundDown, node);
}


void InstructionSelector::VisitFloat64RoundDown(Node* node) {
  VisitRR(this, kMips64Float64RoundDown, node);
}


void InstructionSelector::VisitFloat32RoundUp(Node* node) {
  VisitRR(this, kMips64Float32RoundUp, node);
}


void InstructionSelector::VisitFloat64RoundUp(Node* node) {
  VisitRR(this, kMips64Float64RoundUp, node);
}


void InstructionSelector::VisitFloat32RoundTruncate(Node* node) {
  VisitRR(this, kMips64Float32RoundTruncate, node);
}


void InstructionSelector::VisitFloat64RoundTruncate(Node* node) {
  VisitRR(this, kMips64Float64RoundTruncate, node);
}


void InstructionSelector::VisitFloat64RoundTiesAway(Node* node) {
  UNREACHABLE();
}


void InstructionSelector::VisitFloat32RoundTiesEven(Node* node) {
  VisitRR(this, kMips64Float32RoundTiesEven, node);
}


void InstructionSelector::VisitFloat64RoundTiesEven(Node* node) {
  VisitRR(this, kMips64Float64RoundTiesEven, node);
}

void InstructionSelector::VisitFloat32Neg(Node* node) {
  VisitRR(this, kMips64NegS, node);
}

void InstructionSelector::VisitFloat64Neg(Node* node) {
  VisitRR(this, kMips64NegD, node);
}

void InstructionSelector::VisitFloat64Ieee754Binop(Node* node,
                                                   InstructionCode opcode) {
  Mips64OperandGenerator g(this);
  Emit(opcode, g.DefineAsFixed(node, f0), g.UseFixed(node->InputAt(0), f2),
       g.UseFixed(node->InputAt(1), f4))
      ->MarkAsCall();
}

void InstructionSelector::VisitFloat64Ieee754Unop(Node* node,
                                                  InstructionCode opcode) {
  Mips64OperandGenerator g(this);
  Emit(opcode, g.DefineAsFixed(node, f0), g.UseFixed(node->InputAt(0), f12))
      ->MarkAsCall();
}

void InstructionSelector::EmitPrepareArguments(
    ZoneVector<PushParameter>* arguments, const CallDescriptor* descriptor,
    Node* node) {
  Mips64OperandGenerator g(this);

  // Prepare for C function call.
  if (descriptor->IsCFunctionCall()) {
    Emit(kArchPrepareCallCFunction |
             MiscField::encode(static_cast<int>(descriptor->ParameterCount())),
         0, nullptr, 0, nullptr);

    // Poke any stack arguments.
    int slot = kCArgSlotCount;
    for (PushParameter input : (*arguments)) {
      Emit(kMips64StoreToStackSlot, g.NoOutput(), g.UseRegister(input.node()),
           g.TempImmediate(slot << kPointerSizeLog2));
      ++slot;
    }
  } else {
    int push_count = static_cast<int>(descriptor->StackParameterCount());
    if (push_count > 0) {
      Emit(kMips64StackClaim, g.NoOutput(),
           g.TempImmediate(push_count << kPointerSizeLog2));
    }
    for (size_t n = 0; n < arguments->size(); ++n) {
      PushParameter input = (*arguments)[n];
      if (input.node()) {
        Emit(kMips64StoreToStackSlot, g.NoOutput(), g.UseRegister(input.node()),
             g.TempImmediate(static_cast<int>(n << kPointerSizeLog2)));
      }
    }
  }
}


bool InstructionSelector::IsTailCallAddressImmediate() { return false; }

int InstructionSelector::GetTempsCountForTailCallFromJSFunction() { return 3; }

void InstructionSelector::VisitUnalignedLoad(Node* node) {
  UnalignedLoadRepresentation load_rep =
      UnalignedLoadRepresentationOf(node->op());
  Mips64OperandGenerator g(this);
  Node* base = node->InputAt(0);
  Node* index = node->InputAt(1);

  ArchOpcode opcode = kArchNop;
  switch (load_rep.representation()) {
    case MachineRepresentation::kFloat32:
      opcode = kMips64Ulwc1;
      break;
    case MachineRepresentation::kFloat64:
      opcode = kMips64Uldc1;
      break;
    case MachineRepresentation::kBit:  // Fall through.
    case MachineRepresentation::kWord8:
      UNREACHABLE();
      break;
    case MachineRepresentation::kWord16:
      opcode = load_rep.IsUnsigned() ? kMips64Ulhu : kMips64Ulh;
      break;
    case MachineRepresentation::kWord32:
      opcode = load_rep.IsUnsigned() ? kMips64Ulwu : kMips64Ulw;
      break;
    case MachineRepresentation::kTaggedSigned:   // Fall through.
    case MachineRepresentation::kTaggedPointer:  // Fall through.
    case MachineRepresentation::kTagged:  // Fall through.
    case MachineRepresentation::kWord64:
      opcode = kMips64Uld;
      break;
    case MachineRepresentation::kSimd128:
      opcode = kMips64MsaLd;
      break;
    case MachineRepresentation::kNone:
      UNREACHABLE();
      return;
  }

  if (g.CanBeImmediate(index, opcode)) {
    Emit(opcode | AddressingModeField::encode(kMode_MRI),
         g.DefineAsRegister(node), g.UseRegister(base), g.UseImmediate(index));
  } else {
    InstructionOperand addr_reg = g.TempRegister();
    Emit(kMips64Dadd | AddressingModeField::encode(kMode_None), addr_reg,
         g.UseRegister(index), g.UseRegister(base));
    // Emit desired load opcode, using temp addr_reg.
    Emit(opcode | AddressingModeField::encode(kMode_MRI),
         g.DefineAsRegister(node), addr_reg, g.TempImmediate(0));
  }
}

void InstructionSelector::VisitUnalignedStore(Node* node) {
  Mips64OperandGenerator g(this);
  Node* base = node->InputAt(0);
  Node* index = node->InputAt(1);
  Node* value = node->InputAt(2);

  UnalignedStoreRepresentation rep = UnalignedStoreRepresentationOf(node->op());
  ArchOpcode opcode = kArchNop;
  switch (rep) {
    case MachineRepresentation::kFloat32:
      opcode = kMips64Uswc1;
      break;
    case MachineRepresentation::kFloat64:
      opcode = kMips64Usdc1;
      break;
    case MachineRepresentation::kBit:  // Fall through.
    case MachineRepresentation::kWord8:
      UNREACHABLE();
      break;
    case MachineRepresentation::kWord16:
      opcode = kMips64Ush;
      break;
    case MachineRepresentation::kWord32:
      opcode = kMips64Usw;
      break;
    case MachineRepresentation::kTaggedSigned:   // Fall through.
    case MachineRepresentation::kTaggedPointer:  // Fall through.
    case MachineRepresentation::kTagged:  // Fall through.
    case MachineRepresentation::kWord64:
      opcode = kMips64Usd;
      break;
    case MachineRepresentation::kSimd128:
      opcode = kMips64MsaSt;
      break;
    case MachineRepresentation::kNone:
      UNREACHABLE();
      return;
  }

  if (g.CanBeImmediate(index, opcode)) {
    Emit(opcode | AddressingModeField::encode(kMode_MRI), g.NoOutput(),
         g.UseRegister(base), g.UseImmediate(index),
         g.UseRegisterOrImmediateZero(value));
  } else {
    InstructionOperand addr_reg = g.TempRegister();
    Emit(kMips64Dadd | AddressingModeField::encode(kMode_None), addr_reg,
         g.UseRegister(index), g.UseRegister(base));
    // Emit desired store opcode, using temp addr_reg.
    Emit(opcode | AddressingModeField::encode(kMode_MRI), g.NoOutput(),
         addr_reg, g.TempImmediate(0), g.UseRegisterOrImmediateZero(value));
  }
}

void InstructionSelector::VisitCheckedLoad(Node* node) {
  CheckedLoadRepresentation load_rep = CheckedLoadRepresentationOf(node->op());
  Mips64OperandGenerator g(this);
  Node* const buffer = node->InputAt(0);
  Node* const offset = node->InputAt(1);
  Node* const length = node->InputAt(2);
  ArchOpcode opcode = kArchNop;
  switch (load_rep.representation()) {
    case MachineRepresentation::kWord8:
      opcode = load_rep.IsSigned() ? kCheckedLoadInt8 : kCheckedLoadUint8;
      break;
    case MachineRepresentation::kWord16:
      opcode = load_rep.IsSigned() ? kCheckedLoadInt16 : kCheckedLoadUint16;
      break;
    case MachineRepresentation::kWord32:
      opcode = kCheckedLoadWord32;
      break;
    case MachineRepresentation::kWord64:
      opcode = kCheckedLoadWord64;
      break;
    case MachineRepresentation::kFloat32:
      opcode = kCheckedLoadFloat32;
      break;
    case MachineRepresentation::kFloat64:
      opcode = kCheckedLoadFloat64;
      break;
    case MachineRepresentation::kBit:
    case MachineRepresentation::kTaggedSigned:   // Fall through.
    case MachineRepresentation::kTaggedPointer:  // Fall through.
    case MachineRepresentation::kTagged:
    case MachineRepresentation::kSimd128:
    case MachineRepresentation::kNone:
      UNREACHABLE();
      return;
  }
  InstructionOperand offset_operand = g.CanBeImmediate(offset, opcode)
                                          ? g.UseImmediate(offset)
                                          : g.UseRegister(offset);

  InstructionOperand length_operand = (!g.CanBeImmediate(offset, opcode))
                                          ? g.CanBeImmediate(length, opcode)
                                                ? g.UseImmediate(length)
                                                : g.UseRegister(length)
                                          : g.UseRegister(length);

  if (length->opcode() == IrOpcode::kInt32Constant) {
    Int32Matcher m(length);
    if (m.IsPowerOf2()) {
      Emit(opcode, g.DefineAsRegister(node), offset_operand,
           g.UseImmediate(length), g.UseRegister(buffer));
      return;
    }
  }

  Emit(opcode | AddressingModeField::encode(kMode_MRI),
       g.DefineAsRegister(node), offset_operand, length_operand,
       g.UseRegister(buffer));
}


void InstructionSelector::VisitCheckedStore(Node* node) {
  MachineRepresentation rep = CheckedStoreRepresentationOf(node->op());
  Mips64OperandGenerator g(this);
  Node* const buffer = node->InputAt(0);
  Node* const offset = node->InputAt(1);
  Node* const length = node->InputAt(2);
  Node* const value = node->InputAt(3);
  ArchOpcode opcode = kArchNop;
  switch (rep) {
    case MachineRepresentation::kWord8:
      opcode = kCheckedStoreWord8;
      break;
    case MachineRepresentation::kWord16:
      opcode = kCheckedStoreWord16;
      break;
    case MachineRepresentation::kWord32:
      opcode = kCheckedStoreWord32;
      break;
    case MachineRepresentation::kWord64:
      opcode = kCheckedStoreWord64;
      break;
    case MachineRepresentation::kFloat32:
      opcode = kCheckedStoreFloat32;
      break;
    case MachineRepresentation::kFloat64:
      opcode = kCheckedStoreFloat64;
      break;
    case MachineRepresentation::kBit:
    case MachineRepresentation::kTaggedSigned:   // Fall through.
    case MachineRepresentation::kTaggedPointer:  // Fall through.
    case MachineRepresentation::kTagged:
    case MachineRepresentation::kSimd128:
    case MachineRepresentation::kNone:
      UNREACHABLE();
      return;
  }
  InstructionOperand offset_operand = g.CanBeImmediate(offset, opcode)
                                          ? g.UseImmediate(offset)
                                          : g.UseRegister(offset);

  InstructionOperand length_operand = (!g.CanBeImmediate(offset, opcode))
                                          ? g.CanBeImmediate(length, opcode)
                                                ? g.UseImmediate(length)
                                                : g.UseRegister(length)
                                          : g.UseRegister(length);

  if (length->opcode() == IrOpcode::kInt32Constant) {
    Int32Matcher m(length);
    if (m.IsPowerOf2()) {
      Emit(opcode, g.NoOutput(), offset_operand, g.UseImmediate(length),
           g.UseRegisterOrImmediateZero(value), g.UseRegister(buffer));
      return;
    }
  }

  Emit(opcode | AddressingModeField::encode(kMode_MRI), g.NoOutput(),
       offset_operand, length_operand, g.UseRegisterOrImmediateZero(value),
       g.UseRegister(buffer));
}


namespace {

// Shared routine for multiple compare operations.
static void VisitCompare(InstructionSelector* selector, InstructionCode opcode,
                         InstructionOperand left, InstructionOperand right,
                         FlagsContinuation* cont) {
  Mips64OperandGenerator g(selector);
  opcode = cont->Encode(opcode);
  if (cont->IsBranch()) {
    selector->Emit(opcode, g.NoOutput(), left, right,
                   g.Label(cont->true_block()), g.Label(cont->false_block()));
  } else if (cont->IsDeoptimize()) {
    selector->EmitDeoptimize(opcode, g.NoOutput(), left, right, cont->kind(),
                             cont->reason(), cont->frame_state());
  } else if (cont->IsSet()) {
    selector->Emit(opcode, g.DefineAsRegister(cont->result()), left, right);
  } else {
    DCHECK(cont->IsTrap());
    selector->Emit(opcode, g.NoOutput(), left, right,
                   g.TempImmediate(cont->trap_id()));
  }
}


// Shared routine for multiple float32 compare operations.
void VisitFloat32Compare(InstructionSelector* selector, Node* node,
                         FlagsContinuation* cont) {
  Mips64OperandGenerator g(selector);
  Float32BinopMatcher m(node);
  InstructionOperand lhs, rhs;

  lhs = m.left().IsZero() ? g.UseImmediate(m.left().node())
                          : g.UseRegister(m.left().node());
  rhs = m.right().IsZero() ? g.UseImmediate(m.right().node())
                           : g.UseRegister(m.right().node());
  VisitCompare(selector, kMips64CmpS, lhs, rhs, cont);
}


// Shared routine for multiple float64 compare operations.
void VisitFloat64Compare(InstructionSelector* selector, Node* node,
                         FlagsContinuation* cont) {
  Mips64OperandGenerator g(selector);
  Float64BinopMatcher m(node);
  InstructionOperand lhs, rhs;

  lhs = m.left().IsZero() ? g.UseImmediate(m.left().node())
                          : g.UseRegister(m.left().node());
  rhs = m.right().IsZero() ? g.UseImmediate(m.right().node())
                           : g.UseRegister(m.right().node());
  VisitCompare(selector, kMips64CmpD, lhs, rhs, cont);
}


// Shared routine for multiple word compare operations.
void VisitWordCompare(InstructionSelector* selector, Node* node,
                      InstructionCode opcode, FlagsContinuation* cont,
                      bool commutative) {
  Mips64OperandGenerator g(selector);
  Node* left = node->InputAt(0);
  Node* right = node->InputAt(1);

  // Match immediates on left or right side of comparison.
  if (g.CanBeImmediate(right, opcode)) {
    if (opcode == kMips64Tst) {
      VisitCompare(selector, opcode, g.UseRegister(left), g.UseImmediate(right),
                   cont);
    } else {
      switch (cont->condition()) {
        case kEqual:
        case kNotEqual:
          if (cont->IsSet()) {
            VisitCompare(selector, opcode, g.UseRegister(left),
                         g.UseImmediate(right), cont);
          } else {
            VisitCompare(selector, opcode, g.UseRegister(left),
                         g.UseRegister(right), cont);
          }
          break;
        case kSignedLessThan:
        case kSignedGreaterThanOrEqual:
        case kUnsignedLessThan:
        case kUnsignedGreaterThanOrEqual:
          VisitCompare(selector, opcode, g.UseRegister(left),
                       g.UseImmediate(right), cont);
          break;
        default:
          VisitCompare(selector, opcode, g.UseRegister(left),
                       g.UseRegister(right), cont);
      }
    }
  } else if (g.CanBeImmediate(left, opcode)) {
    if (!commutative) cont->Commute();
    if (opcode == kMips64Tst) {
      VisitCompare(selector, opcode, g.UseRegister(right), g.UseImmediate(left),
                   cont);
    } else {
      switch (cont->condition()) {
        case kEqual:
        case kNotEqual:
          if (cont->IsSet()) {
            VisitCompare(selector, opcode, g.UseRegister(right),
                         g.UseImmediate(left), cont);
          } else {
            VisitCompare(selector, opcode, g.UseRegister(right),
                         g.UseRegister(left), cont);
          }
          break;
        case kSignedLessThan:
        case kSignedGreaterThanOrEqual:
        case kUnsignedLessThan:
        case kUnsignedGreaterThanOrEqual:
          VisitCompare(selector, opcode, g.UseRegister(right),
                       g.UseImmediate(left), cont);
          break;
        default:
          VisitCompare(selector, opcode, g.UseRegister(right),
                       g.UseRegister(left), cont);
      }
    }
  } else {
    VisitCompare(selector, opcode, g.UseRegister(left), g.UseRegister(right),
                 cont);
  }
}

bool IsNodeUnsigned(Node* n) {
  NodeMatcher m(n);

  if (m.IsLoad()) {
    LoadRepresentation load_rep = LoadRepresentationOf(n->op());
    return load_rep.IsUnsigned();
  } else if (m.IsUnalignedLoad()) {
    UnalignedLoadRepresentation load_rep =
        UnalignedLoadRepresentationOf(n->op());
    return load_rep.IsUnsigned();
  } else {
    return m.IsUint32Div() || m.IsUint32LessThan() ||
           m.IsUint32LessThanOrEqual() || m.IsUint32Mod() ||
           m.IsUint32MulHigh() || m.IsChangeFloat64ToUint32() ||
           m.IsTruncateFloat64ToUint32() || m.IsTruncateFloat32ToUint32();
  }
}

// Shared routine for multiple word compare operations.
void VisitFullWord32Compare(InstructionSelector* selector, Node* node,
                            InstructionCode opcode, FlagsContinuation* cont) {
  Mips64OperandGenerator g(selector);
  InstructionOperand leftOp = g.TempRegister();
  InstructionOperand rightOp = g.TempRegister();

  selector->Emit(kMips64Dshl, leftOp, g.UseRegister(node->InputAt(0)),
                 g.TempImmediate(32));
  selector->Emit(kMips64Dshl, rightOp, g.UseRegister(node->InputAt(1)),
                 g.TempImmediate(32));

  VisitCompare(selector, opcode, leftOp, rightOp, cont);
}

void VisitOptimizedWord32Compare(InstructionSelector* selector, Node* node,
                                 InstructionCode opcode,
                                 FlagsContinuation* cont) {
  if (FLAG_debug_code) {
    Mips64OperandGenerator g(selector);
    InstructionOperand leftOp = g.TempRegister();
    InstructionOperand rightOp = g.TempRegister();
    InstructionOperand optimizedResult = g.TempRegister();
    InstructionOperand fullResult = g.TempRegister();
    FlagsCondition condition = cont->condition();
    InstructionCode testOpcode = opcode |
                                 FlagsConditionField::encode(condition) |
                                 FlagsModeField::encode(kFlags_set);

    selector->Emit(testOpcode, optimizedResult, g.UseRegister(node->InputAt(0)),
                   g.UseRegister(node->InputAt(1)));

    selector->Emit(kMips64Dshl, leftOp, g.UseRegister(node->InputAt(0)),
                   g.TempImmediate(32));
    selector->Emit(kMips64Dshl, rightOp, g.UseRegister(node->InputAt(1)),
                   g.TempImmediate(32));
    selector->Emit(testOpcode, fullResult, leftOp, rightOp);

    selector->Emit(
        kMips64AssertEqual, g.NoOutput(), optimizedResult, fullResult,
        g.TempImmediate(BailoutReason::kUnsupportedNonPrimitiveCompare));
  }

  VisitWordCompare(selector, node, opcode, cont, false);
}

void VisitWord32Compare(InstructionSelector* selector, Node* node,
                        FlagsContinuation* cont) {
  // MIPS64 doesn't support Word32 compare instructions. Instead it relies
  // that the values in registers are correctly sign-extended and uses
  // Word64 comparison instead. This behavior is correct in most cases,
  // but doesn't work when comparing signed with unsigned operands.
  // We could simulate full Word32 compare in all cases but this would
  // create an unnecessary overhead since unsigned integers are rarely
  // used in JavaScript.
  // The solution proposed here tries to match a comparison of signed
  // with unsigned operand, and perform full Word32Compare only
  // in those cases. Unfortunately, the solution is not complete because
  // it might skip cases where Word32 full compare is needed, so
  // basically it is a hack.
  if (IsNodeUnsigned(node->InputAt(0)) != IsNodeUnsigned(node->InputAt(1))) {
    VisitFullWord32Compare(selector, node, kMips64Cmp, cont);
  } else {
    VisitOptimizedWord32Compare(selector, node, kMips64Cmp, cont);
  }
}


void VisitWord64Compare(InstructionSelector* selector, Node* node,
                        FlagsContinuation* cont) {
  VisitWordCompare(selector, node, kMips64Cmp, cont, false);
}



void EmitWordCompareZero(InstructionSelector* selector, Node* value,
                         FlagsContinuation* cont) {
  Mips64OperandGenerator g(selector);
  InstructionCode opcode = cont->Encode(kMips64Cmp);
  InstructionOperand const value_operand = g.UseRegister(value);
  if (cont->IsBranch()) {
    selector->Emit(opcode, g.NoOutput(), value_operand, g.TempImmediate(0),
                   g.Label(cont->true_block()), g.Label(cont->false_block()));
  } else if (cont->IsDeoptimize()) {
    selector->EmitDeoptimize(opcode, g.NoOutput(), value_operand,
                             g.TempImmediate(0), cont->kind(), cont->reason(),
                             cont->frame_state());
  } else if (cont->IsTrap()) {
    selector->Emit(opcode, g.NoOutput(), value_operand, g.TempImmediate(0),
                   g.TempImmediate(cont->trap_id()));
  } else {
    selector->Emit(opcode, g.DefineAsRegister(cont->result()), value_operand,
                   g.TempImmediate(0));
  }
}


// Shared routine for word comparisons against zero.
void VisitWordCompareZero(InstructionSelector* selector, Node* user,
                          Node* value, FlagsContinuation* cont) {
  // Try to combine with comparisons against 0 by simply inverting the branch.
  while (selector->CanCover(user, value)) {
    if (value->opcode() == IrOpcode::kWord32Equal) {
      Int32BinopMatcher m(value);
      if (!m.right().Is(0)) break;
      user = value;
      value = m.left().node();
    } else if (value->opcode() == IrOpcode::kWord64Equal) {
      Int64BinopMatcher m(value);
      if (!m.right().Is(0)) break;
      user = value;
      value = m.left().node();
    } else {
      break;
    }

    cont->Negate();
  }

  if (selector->CanCover(user, value)) {
    switch (value->opcode()) {
      case IrOpcode::kWord32Equal:
        cont->OverwriteAndNegateIfEqual(kEqual);
        return VisitWord32Compare(selector, value, cont);
      case IrOpcode::kInt32LessThan:
        cont->OverwriteAndNegateIfEqual(kSignedLessThan);
        return VisitWord32Compare(selector, value, cont);
      case IrOpcode::kInt32LessThanOrEqual:
        cont->OverwriteAndNegateIfEqual(kSignedLessThanOrEqual);
        return VisitWord32Compare(selector, value, cont);
      case IrOpcode::kUint32LessThan:
        cont->OverwriteAndNegateIfEqual(kUnsignedLessThan);
        return VisitWord32Compare(selector, value, cont);
      case IrOpcode::kUint32LessThanOrEqual:
        cont->OverwriteAndNegateIfEqual(kUnsignedLessThanOrEqual);
        return VisitWord32Compare(selector, value, cont);
      case IrOpcode::kWord64Equal:
        cont->OverwriteAndNegateIfEqual(kEqual);
        return VisitWord64Compare(selector, value, cont);
      case IrOpcode::kInt64LessThan:
        cont->OverwriteAndNegateIfEqual(kSignedLessThan);
        return VisitWord64Compare(selector, value, cont);
      case IrOpcode::kInt64LessThanOrEqual:
        cont->OverwriteAndNegateIfEqual(kSignedLessThanOrEqual);
        return VisitWord64Compare(selector, value, cont);
      case IrOpcode::kUint64LessThan:
        cont->OverwriteAndNegateIfEqual(kUnsignedLessThan);
        return VisitWord64Compare(selector, value, cont);
      case IrOpcode::kUint64LessThanOrEqual:
        cont->OverwriteAndNegateIfEqual(kUnsignedLessThanOrEqual);
        return VisitWord64Compare(selector, value, cont);
      case IrOpcode::kFloat32Equal:
        cont->OverwriteAndNegateIfEqual(kEqual);
        return VisitFloat32Compare(selector, value, cont);
      case IrOpcode::kFloat32LessThan:
        cont->OverwriteAndNegateIfEqual(kUnsignedLessThan);
        return VisitFloat32Compare(selector, value, cont);
      case IrOpcode::kFloat32LessThanOrEqual:
        cont->OverwriteAndNegateIfEqual(kUnsignedLessThanOrEqual);
        return VisitFloat32Compare(selector, value, cont);
      case IrOpcode::kFloat64Equal:
        cont->OverwriteAndNegateIfEqual(kEqual);
        return VisitFloat64Compare(selector, value, cont);
      case IrOpcode::kFloat64LessThan:
        cont->OverwriteAndNegateIfEqual(kUnsignedLessThan);
        return VisitFloat64Compare(selector, value, cont);
      case IrOpcode::kFloat64LessThanOrEqual:
        cont->OverwriteAndNegateIfEqual(kUnsignedLessThanOrEqual);
        return VisitFloat64Compare(selector, value, cont);
      case IrOpcode::kProjection:
        // Check if this is the overflow output projection of an
        // <Operation>WithOverflow node.
        if (ProjectionIndexOf(value->op()) == 1u) {
          // We cannot combine the <Operation>WithOverflow with this branch
          // unless the 0th projection (the use of the actual value of the
          // <Operation> is either nullptr, which means there's no use of the
          // actual value, or was already defined, which means it is scheduled
          // *AFTER* this branch).
          Node* const node = value->InputAt(0);
          Node* const result = NodeProperties::FindProjection(node, 0);
          if (result == nullptr || selector->IsDefined(result)) {
            switch (node->opcode()) {
              case IrOpcode::kInt32AddWithOverflow:
                cont->OverwriteAndNegateIfEqual(kOverflow);
                return VisitBinop(selector, node, kMips64Dadd, cont);
              case IrOpcode::kInt32SubWithOverflow:
                cont->OverwriteAndNegateIfEqual(kOverflow);
                return VisitBinop(selector, node, kMips64Dsub, cont);
              case IrOpcode::kInt32MulWithOverflow:
                cont->OverwriteAndNegateIfEqual(kOverflow);
                return VisitBinop(selector, node, kMips64MulOvf, cont);
              case IrOpcode::kInt64AddWithOverflow:
                cont->OverwriteAndNegateIfEqual(kOverflow);
                return VisitBinop(selector, node, kMips64DaddOvf, cont);
              case IrOpcode::kInt64SubWithOverflow:
                cont->OverwriteAndNegateIfEqual(kOverflow);
                return VisitBinop(selector, node, kMips64DsubOvf, cont);
              default:
                break;
            }
          }
        }
        break;
      case IrOpcode::kWord32And:
      case IrOpcode::kWord64And:
        return VisitWordCompare(selector, value, kMips64Tst, cont, true);
      default:
        break;
    }
  }

  // Continuation could not be combined with a compare, emit compare against 0.
  EmitWordCompareZero(selector, value, cont);
}

}  // namespace

void InstructionSelector::VisitBranch(Node* branch, BasicBlock* tbranch,
                                      BasicBlock* fbranch) {
  FlagsContinuation cont(kNotEqual, tbranch, fbranch);
  VisitWordCompareZero(this, branch, branch->InputAt(0), &cont);
}

void InstructionSelector::VisitDeoptimizeIf(Node* node) {
  DeoptimizeParameters p = DeoptimizeParametersOf(node->op());
  FlagsContinuation cont = FlagsContinuation::ForDeoptimize(
      kNotEqual, p.kind(), p.reason(), node->InputAt(1));
  VisitWordCompareZero(this, node, node->InputAt(0), &cont);
}

void InstructionSelector::VisitDeoptimizeUnless(Node* node) {
  DeoptimizeParameters p = DeoptimizeParametersOf(node->op());
  FlagsContinuation cont = FlagsContinuation::ForDeoptimize(
      kEqual, p.kind(), p.reason(), node->InputAt(1));
  VisitWordCompareZero(this, node, node->InputAt(0), &cont);
}

void InstructionSelector::VisitTrapIf(Node* node, Runtime::FunctionId func_id) {
  FlagsContinuation cont =
      FlagsContinuation::ForTrap(kNotEqual, func_id, node->InputAt(1));
  VisitWordCompareZero(this, node, node->InputAt(0), &cont);
}

void InstructionSelector::VisitTrapUnless(Node* node,
                                          Runtime::FunctionId func_id) {
  FlagsContinuation cont =
      FlagsContinuation::ForTrap(kEqual, func_id, node->InputAt(1));
  VisitWordCompareZero(this, node, node->InputAt(0), &cont);
}

void InstructionSelector::VisitSwitch(Node* node, const SwitchInfo& sw) {
  Mips64OperandGenerator g(this);
  InstructionOperand value_operand = g.UseRegister(node->InputAt(0));

  // Emit either ArchTableSwitch or ArchLookupSwitch.
  static const size_t kMaxTableSwitchValueRange = 2 << 16;
  size_t table_space_cost = 10 + 2 * sw.value_range;
  size_t table_time_cost = 3;
  size_t lookup_space_cost = 2 + 2 * sw.case_count;
  size_t lookup_time_cost = sw.case_count;
  if (sw.case_count > 0 &&
      table_space_cost + 3 * table_time_cost <=
          lookup_space_cost + 3 * lookup_time_cost &&
      sw.min_value > std::numeric_limits<int32_t>::min() &&
      sw.value_range <= kMaxTableSwitchValueRange) {
    InstructionOperand index_operand = value_operand;
    if (sw.min_value) {
      index_operand = g.TempRegister();
      Emit(kMips64Sub, index_operand, value_operand,
           g.TempImmediate(sw.min_value));
    }
    // Generate a table lookup.
    return EmitTableSwitch(sw, index_operand);
  }

  // Generate a sequence of conditional jumps.
  return EmitLookupSwitch(sw, value_operand);
}


void InstructionSelector::VisitWord32Equal(Node* const node) {
  FlagsContinuation cont = FlagsContinuation::ForSet(kEqual, node);
  Int32BinopMatcher m(node);
  if (m.right().Is(0)) {
    return VisitWordCompareZero(this, m.node(), m.left().node(), &cont);
  }

  VisitWord32Compare(this, node, &cont);
}


void InstructionSelector::VisitInt32LessThan(Node* node) {
  FlagsContinuation cont = FlagsContinuation::ForSet(kSignedLessThan, node);
  VisitWord32Compare(this, node, &cont);
}


void InstructionSelector::VisitInt32LessThanOrEqual(Node* node) {
  FlagsContinuation cont =
      FlagsContinuation::ForSet(kSignedLessThanOrEqual, node);
  VisitWord32Compare(this, node, &cont);
}


void InstructionSelector::VisitUint32LessThan(Node* node) {
  FlagsContinuation cont = FlagsContinuation::ForSet(kUnsignedLessThan, node);
  VisitWord32Compare(this, node, &cont);
}


void InstructionSelector::VisitUint32LessThanOrEqual(Node* node) {
  FlagsContinuation cont =
      FlagsContinuation::ForSet(kUnsignedLessThanOrEqual, node);
  VisitWord32Compare(this, node, &cont);
}


void InstructionSelector::VisitInt32AddWithOverflow(Node* node) {
  if (Node* ovf = NodeProperties::FindProjection(node, 1)) {
    FlagsContinuation cont = FlagsContinuation::ForSet(kOverflow, ovf);
    return VisitBinop(this, node, kMips64Dadd, &cont);
  }
  FlagsContinuation cont;
  VisitBinop(this, node, kMips64Dadd, &cont);
}


void InstructionSelector::VisitInt32SubWithOverflow(Node* node) {
  if (Node* ovf = NodeProperties::FindProjection(node, 1)) {
    FlagsContinuation cont = FlagsContinuation::ForSet(kOverflow, ovf);
    return VisitBinop(this, node, kMips64Dsub, &cont);
  }
  FlagsContinuation cont;
  VisitBinop(this, node, kMips64Dsub, &cont);
}

void InstructionSelector::VisitInt32MulWithOverflow(Node* node) {
  if (Node* ovf = NodeProperties::FindProjection(node, 1)) {
    FlagsContinuation cont = FlagsContinuation::ForSet(kOverflow, ovf);
    return VisitBinop(this, node, kMips64MulOvf, &cont);
  }
  FlagsContinuation cont;
  VisitBinop(this, node, kMips64MulOvf, &cont);
}

void InstructionSelector::VisitInt64AddWithOverflow(Node* node) {
  if (Node* ovf = NodeProperties::FindProjection(node, 1)) {
    FlagsContinuation cont = FlagsContinuation::ForSet(kOverflow, ovf);
    return VisitBinop(this, node, kMips64DaddOvf, &cont);
  }
  FlagsContinuation cont;
  VisitBinop(this, node, kMips64DaddOvf, &cont);
}


void InstructionSelector::VisitInt64SubWithOverflow(Node* node) {
  if (Node* ovf = NodeProperties::FindProjection(node, 1)) {
    FlagsContinuation cont = FlagsContinuation::ForSet(kOverflow, ovf);
    return VisitBinop(this, node, kMips64DsubOvf, &cont);
  }
  FlagsContinuation cont;
  VisitBinop(this, node, kMips64DsubOvf, &cont);
}


void InstructionSelector::VisitWord64Equal(Node* const node) {
  FlagsContinuation cont = FlagsContinuation::ForSet(kEqual, node);
  Int64BinopMatcher m(node);
  if (m.right().Is(0)) {
    return VisitWordCompareZero(this, m.node(), m.left().node(), &cont);
  }

  VisitWord64Compare(this, node, &cont);
}


void InstructionSelector::VisitInt64LessThan(Node* node) {
  FlagsContinuation cont = FlagsContinuation::ForSet(kSignedLessThan, node);
  VisitWord64Compare(this, node, &cont);
}


void InstructionSelector::VisitInt64LessThanOrEqual(Node* node) {
  FlagsContinuation cont =
      FlagsContinuation::ForSet(kSignedLessThanOrEqual, node);
  VisitWord64Compare(this, node, &cont);
}


void InstructionSelector::VisitUint64LessThan(Node* node) {
  FlagsContinuation cont = FlagsContinuation::ForSet(kUnsignedLessThan, node);
  VisitWord64Compare(this, node, &cont);
}


void InstructionSelector::VisitUint64LessThanOrEqual(Node* node) {
  FlagsContinuation cont =
      FlagsContinuation::ForSet(kUnsignedLessThanOrEqual, node);
  VisitWord64Compare(this, node, &cont);
}


void InstructionSelector::VisitFloat32Equal(Node* node) {
  FlagsContinuation cont = FlagsContinuation::ForSet(kEqual, node);
  VisitFloat32Compare(this, node, &cont);
}


void InstructionSelector::VisitFloat32LessThan(Node* node) {
  FlagsContinuation cont = FlagsContinuation::ForSet(kUnsignedLessThan, node);
  VisitFloat32Compare(this, node, &cont);
}


void InstructionSelector::VisitFloat32LessThanOrEqual(Node* node) {
  FlagsContinuation cont =
      FlagsContinuation::ForSet(kUnsignedLessThanOrEqual, node);
  VisitFloat32Compare(this, node, &cont);
}


void InstructionSelector::VisitFloat64Equal(Node* node) {
  FlagsContinuation cont = FlagsContinuation::ForSet(kEqual, node);
  VisitFloat64Compare(this, node, &cont);
}


void InstructionSelector::VisitFloat64LessThan(Node* node) {
  FlagsContinuation cont = FlagsContinuation::ForSet(kUnsignedLessThan, node);
  VisitFloat64Compare(this, node, &cont);
}


void InstructionSelector::VisitFloat64LessThanOrEqual(Node* node) {
  FlagsContinuation cont =
      FlagsContinuation::ForSet(kUnsignedLessThanOrEqual, node);
  VisitFloat64Compare(this, node, &cont);
}


void InstructionSelector::VisitFloat64ExtractLowWord32(Node* node) {
  VisitRR(this, kMips64Float64ExtractLowWord32, node);
}


void InstructionSelector::VisitFloat64ExtractHighWord32(Node* node) {
  VisitRR(this, kMips64Float64ExtractHighWord32, node);
}

void InstructionSelector::VisitFloat64SilenceNaN(Node* node) {
  VisitRR(this, kMips64Float64SilenceNaN, node);
}

void InstructionSelector::VisitFloat64InsertLowWord32(Node* node) {
  Mips64OperandGenerator g(this);
  Node* left = node->InputAt(0);
  Node* right = node->InputAt(1);
  Emit(kMips64Float64InsertLowWord32, g.DefineSameAsFirst(node),
       g.UseRegister(left), g.UseRegister(right));
}


void InstructionSelector::VisitFloat64InsertHighWord32(Node* node) {
  Mips64OperandGenerator g(this);
  Node* left = node->InputAt(0);
  Node* right = node->InputAt(1);
  Emit(kMips64Float64InsertHighWord32, g.DefineSameAsFirst(node),
       g.UseRegister(left), g.UseRegister(right));
}

void InstructionSelector::VisitAtomicLoad(Node* node) {
  LoadRepresentation load_rep = LoadRepresentationOf(node->op());
  Mips64OperandGenerator g(this);
  Node* base = node->InputAt(0);
  Node* index = node->InputAt(1);
  ArchOpcode opcode = kArchNop;
  switch (load_rep.representation()) {
    case MachineRepresentation::kWord8:
      opcode = load_rep.IsSigned() ? kAtomicLoadInt8 : kAtomicLoadUint8;
      break;
    case MachineRepresentation::kWord16:
      opcode = load_rep.IsSigned() ? kAtomicLoadInt16 : kAtomicLoadUint16;
      break;
    case MachineRepresentation::kWord32:
      opcode = kAtomicLoadWord32;
      break;
    default:
      UNREACHABLE();
      return;
  }
  if (g.CanBeImmediate(index, opcode)) {
    Emit(opcode | AddressingModeField::encode(kMode_MRI),
         g.DefineAsRegister(node), g.UseRegister(base), g.UseImmediate(index));
  } else {
    InstructionOperand addr_reg = g.TempRegister();
    Emit(kMips64Dadd | AddressingModeField::encode(kMode_None), addr_reg,
         g.UseRegister(index), g.UseRegister(base));
    // Emit desired load opcode, using temp addr_reg.
    Emit(opcode | AddressingModeField::encode(kMode_MRI),
         g.DefineAsRegister(node), addr_reg, g.TempImmediate(0));
  }
}

void InstructionSelector::VisitAtomicStore(Node* node) {
  MachineRepresentation rep = AtomicStoreRepresentationOf(node->op());
  Mips64OperandGenerator g(this);
  Node* base = node->InputAt(0);
  Node* index = node->InputAt(1);
  Node* value = node->InputAt(2);
  ArchOpcode opcode = kArchNop;
  switch (rep) {
    case MachineRepresentation::kWord8:
      opcode = kAtomicStoreWord8;
      break;
    case MachineRepresentation::kWord16:
      opcode = kAtomicStoreWord16;
      break;
    case MachineRepresentation::kWord32:
      opcode = kAtomicStoreWord32;
      break;
    default:
      UNREACHABLE();
      return;
  }

  if (g.CanBeImmediate(index, opcode)) {
    Emit(opcode | AddressingModeField::encode(kMode_MRI), g.NoOutput(),
         g.UseRegister(base), g.UseImmediate(index),
         g.UseRegisterOrImmediateZero(value));
  } else {
    InstructionOperand addr_reg = g.TempRegister();
    Emit(kMips64Dadd | AddressingModeField::encode(kMode_None), addr_reg,
         g.UseRegister(index), g.UseRegister(base));
    // Emit desired store opcode, using temp addr_reg.
    Emit(opcode | AddressingModeField::encode(kMode_MRI), g.NoOutput(),
         addr_reg, g.TempImmediate(0), g.UseRegisterOrImmediateZero(value));
  }
}

void InstructionSelector::VisitAtomicExchange(Node* node) { UNIMPLEMENTED(); }

void InstructionSelector::VisitAtomicCompareExchange(Node* node) {
  UNIMPLEMENTED();
}

void InstructionSelector::VisitAtomicAdd(Node* node) { UNIMPLEMENTED(); }

void InstructionSelector::VisitAtomicSub(Node* node) { UNIMPLEMENTED(); }

void InstructionSelector::VisitAtomicAnd(Node* node) { UNIMPLEMENTED(); }

void InstructionSelector::VisitAtomicOr(Node* node) { UNIMPLEMENTED(); }

void InstructionSelector::VisitAtomicXor(Node* node) { UNIMPLEMENTED(); }

void InstructionSelector::VisitInt32AbsWithOverflow(Node* node) {
  UNREACHABLE();
}

void InstructionSelector::VisitInt64AbsWithOverflow(Node* node) {
  UNREACHABLE();
}

#define SIMD_TYPE_LIST(V) \
  V(F32x4)                \
  V(I32x4)                \
  V(I16x8)                \
  V(I8x16)

#define SIMD_FORMAT_LIST(V) \
  V(32x4)                   \
  V(16x8)                   \
  V(8x16)

#define SIMD_UNOP_LIST(V)                                  \
  V(F32x4SConvertI32x4, kMips64F32x4SConvertI32x4)         \
  V(F32x4UConvertI32x4, kMips64F32x4UConvertI32x4)         \
  V(F32x4Abs, kMips64F32x4Abs)                             \
  V(F32x4Neg, kMips64F32x4Neg)                             \
  V(F32x4RecipApprox, kMips64F32x4RecipApprox)             \
  V(F32x4RecipSqrtApprox, kMips64F32x4RecipSqrtApprox)     \
  V(I32x4SConvertF32x4, kMips64I32x4SConvertF32x4)         \
  V(I32x4UConvertF32x4, kMips64I32x4UConvertF32x4)         \
  V(I32x4Neg, kMips64I32x4Neg)                             \
  V(I32x4SConvertI16x8Low, kMips64I32x4SConvertI16x8Low)   \
  V(I32x4SConvertI16x8High, kMips64I32x4SConvertI16x8High) \
  V(I32x4UConvertI16x8Low, kMips64I32x4UConvertI16x8Low)   \
  V(I32x4UConvertI16x8High, kMips64I32x4UConvertI16x8High) \
  V(I16x8Neg, kMips64I16x8Neg)                             \
  V(I16x8SConvertI8x16Low, kMips64I16x8SConvertI8x16Low)   \
  V(I16x8SConvertI8x16High, kMips64I16x8SConvertI8x16High) \
  V(I16x8UConvertI8x16Low, kMips64I16x8UConvertI8x16Low)   \
  V(I16x8UConvertI8x16High, kMips64I16x8UConvertI8x16High) \
  V(I8x16Neg, kMips64I8x16Neg)                             \
  V(S128Not, kMips64S128Not)                               \
  V(S1x4AnyTrue, kMips64S1x4AnyTrue)                       \
  V(S1x4AllTrue, kMips64S1x4AllTrue)                       \
  V(S1x8AnyTrue, kMips64S1x8AnyTrue)                       \
  V(S1x8AllTrue, kMips64S1x8AllTrue)                       \
  V(S1x16AnyTrue, kMips64S1x16AnyTrue)                     \
  V(S1x16AllTrue, kMips64S1x16AllTrue)

#define SIMD_SHIFT_OP_LIST(V) \
  V(I32x4Shl)                 \
  V(I32x4ShrS)                \
  V(I32x4ShrU)                \
  V(I16x8Shl)                 \
  V(I16x8ShrS)                \
  V(I16x8ShrU)                \
  V(I8x16Shl)                 \
  V(I8x16ShrS)                \
  V(I8x16ShrU)

#define SIMD_BINOP_LIST(V)                         \
  V(F32x4Add, kMips64F32x4Add)                     \
  V(F32x4AddHoriz, kMips64F32x4AddHoriz)           \
  V(F32x4Sub, kMips64F32x4Sub)                     \
  V(F32x4Mul, kMips64F32x4Mul)                     \
  V(F32x4Max, kMips64F32x4Max)                     \
  V(F32x4Min, kMips64F32x4Min)                     \
  V(F32x4Eq, kMips64F32x4Eq)                       \
  V(F32x4Ne, kMips64F32x4Ne)                       \
  V(F32x4Lt, kMips64F32x4Lt)                       \
  V(F32x4Le, kMips64F32x4Le)                       \
  V(I32x4Add, kMips64I32x4Add)                     \
  V(I32x4AddHoriz, kMips64I32x4AddHoriz)           \
  V(I32x4Sub, kMips64I32x4Sub)                     \
  V(I32x4Mul, kMips64I32x4Mul)                     \
  V(I32x4MaxS, kMips64I32x4MaxS)                   \
  V(I32x4MinS, kMips64I32x4MinS)                   \
  V(I32x4MaxU, kMips64I32x4MaxU)                   \
  V(I32x4MinU, kMips64I32x4MinU)                   \
  V(I32x4Eq, kMips64I32x4Eq)                       \
  V(I32x4Ne, kMips64I32x4Ne)                       \
  V(I32x4GtS, kMips64I32x4GtS)                     \
  V(I32x4GeS, kMips64I32x4GeS)                     \
  V(I32x4GtU, kMips64I32x4GtU)                     \
  V(I32x4GeU, kMips64I32x4GeU)                     \
  V(I16x8Add, kMips64I16x8Add)                     \
  V(I16x8AddSaturateS, kMips64I16x8AddSaturateS)   \
  V(I16x8AddSaturateU, kMips64I16x8AddSaturateU)   \
  V(I16x8AddHoriz, kMips64I16x8AddHoriz)           \
  V(I16x8Sub, kMips64I16x8Sub)                     \
  V(I16x8SubSaturateS, kMips64I16x8SubSaturateS)   \
  V(I16x8SubSaturateU, kMips64I16x8SubSaturateU)   \
  V(I16x8Mul, kMips64I16x8Mul)                     \
  V(I16x8MaxS, kMips64I16x8MaxS)                   \
  V(I16x8MinS, kMips64I16x8MinS)                   \
  V(I16x8MaxU, kMips64I16x8MaxU)                   \
  V(I16x8MinU, kMips64I16x8MinU)                   \
  V(I16x8Eq, kMips64I16x8Eq)                       \
  V(I16x8Ne, kMips64I16x8Ne)                       \
  V(I16x8GtS, kMips64I16x8GtS)                     \
  V(I16x8GeS, kMips64I16x8GeS)                     \
  V(I16x8GtU, kMips64I16x8GtU)                     \
  V(I16x8GeU, kMips64I16x8GeU)                     \
  V(I16x8SConvertI32x4, kMips64I16x8SConvertI32x4) \
  V(I16x8UConvertI32x4, kMips64I16x8UConvertI32x4) \
  V(I8x16Add, kMips64I8x16Add)                     \
  V(I8x16AddSaturateS, kMips64I8x16AddSaturateS)   \
  V(I8x16AddSaturateU, kMips64I8x16AddSaturateU)   \
  V(I8x16Sub, kMips64I8x16Sub)                     \
  V(I8x16SubSaturateS, kMips64I8x16SubSaturateS)   \
  V(I8x16SubSaturateU, kMips64I8x16SubSaturateU)   \
  V(I8x16Mul, kMips64I8x16Mul)                     \
  V(I8x16MaxS, kMips64I8x16MaxS)                   \
  V(I8x16MinS, kMips64I8x16MinS)                   \
  V(I8x16MaxU, kMips64I8x16MaxU)                   \
  V(I8x16MinU, kMips64I8x16MinU)                   \
  V(I8x16Eq, kMips64I8x16Eq)                       \
  V(I8x16Ne, kMips64I8x16Ne)                       \
  V(I8x16GtS, kMips64I8x16GtS)                     \
  V(I8x16GeS, kMips64I8x16GeS)                     \
  V(I8x16GtU, kMips64I8x16GtU)                     \
  V(I8x16GeU, kMips64I8x16GeU)                     \
  V(I8x16SConvertI16x8, kMips64I8x16SConvertI16x8) \
  V(I8x16UConvertI16x8, kMips64I8x16UConvertI16x8) \
  V(S128And, kMips64S128And)                       \
  V(S128Or, kMips64S128Or)                         \
  V(S128Xor, kMips64S128Xor)

void InstructionSelector::VisitS128Zero(Node* node) {
  Mips64OperandGenerator g(this);
  Emit(kMips64S128Zero, g.DefineSameAsFirst(node));
}

#define SIMD_VISIT_SPLAT(Type)                               \
  void InstructionSelector::Visit##Type##Splat(Node* node) { \
    VisitRR(this, kMips64##Type##Splat, node);               \
  }
SIMD_TYPE_LIST(SIMD_VISIT_SPLAT)
#undef SIMD_VISIT_SPLAT

#define SIMD_VISIT_EXTRACT_LANE(Type)                              \
  void InstructionSelector::Visit##Type##ExtractLane(Node* node) { \
    VisitRRI(this, kMips64##Type##ExtractLane, node);              \
  }
SIMD_TYPE_LIST(SIMD_VISIT_EXTRACT_LANE)
#undef SIMD_VISIT_EXTRACT_LANE

#define SIMD_VISIT_REPLACE_LANE(Type)                              \
  void InstructionSelector::Visit##Type##ReplaceLane(Node* node) { \
    VisitRRIR(this, kMips64##Type##ReplaceLane, node);             \
  }
SIMD_TYPE_LIST(SIMD_VISIT_REPLACE_LANE)
#undef SIMD_VISIT_REPLACE_LANE

#define SIMD_VISIT_UNOP(Name, instruction)            \
  void InstructionSelector::Visit##Name(Node* node) { \
    VisitRR(this, instruction, node);                 \
  }
SIMD_UNOP_LIST(SIMD_VISIT_UNOP)
#undef SIMD_VISIT_UNOP

#define SIMD_VISIT_SHIFT_OP(Name)                     \
  void InstructionSelector::Visit##Name(Node* node) { \
    VisitRRI(this, kMips64##Name, node);              \
  }
SIMD_SHIFT_OP_LIST(SIMD_VISIT_SHIFT_OP)
#undef SIMD_VISIT_SHIFT_OP

#define SIMD_VISIT_BINOP(Name, instruction)           \
  void InstructionSelector::Visit##Name(Node* node) { \
    VisitRRR(this, instruction, node);                \
  }
SIMD_BINOP_LIST(SIMD_VISIT_BINOP)
#undef SIMD_VISIT_BINOP

void InstructionSelector::VisitS128Select(Node* node) {
  VisitRRRR(this, kMips64S128Select, node);
}

namespace {

// Tries to match 8x16 byte shuffle to equivalent 32x4 word shuffle.
bool TryMatch32x4Shuffle(const uint8_t* shuffle, uint8_t* shuffle32x4) {
  static const int kLanes = 4;
  static const int kLaneSize = 4;
  for (int i = 0; i < kLanes; ++i) {
    if (shuffle[i * kLaneSize] % kLaneSize != 0) return false;
    for (int j = 1; j < kLaneSize; ++j) {
      if (shuffle[i * kLaneSize + j] - shuffle[i * kLaneSize + j - 1] != 1)
        return false;
    }
    shuffle32x4[i] = shuffle[i * kLaneSize] / kLaneSize;
  }
  return true;
}

// Tries to match byte shuffle to concatenate (sldi) operation.
bool TryMatchConcat(const uint8_t* shuffle, uint8_t mask, uint8_t* offset) {
  uint8_t start = shuffle[0];
  for (int i = 1; i < kSimd128Size - start; ++i) {
    if ((shuffle[i] & mask) != ((shuffle[i - 1] + 1) & mask)) return false;
  }
  uint8_t wrap = kSimd128Size;
  for (int i = kSimd128Size - start; i < kSimd128Size; ++i, ++wrap) {
    if ((shuffle[i] & mask) != (wrap & mask)) return false;
  }
  *offset = start;
  return true;
}

struct ShuffleEntry {
  uint8_t shuffle[kSimd128Size];
  ArchOpcode opcode;
};

static const ShuffleEntry arch_shuffles[] = {
    {{0, 1, 2, 3, 16, 17, 18, 19, 4, 5, 6, 7, 20, 21, 22, 23},
     kMips64S32x4InterleaveRight},
    {{8, 9, 10, 11, 24, 25, 26, 27, 12, 13, 14, 15, 28, 29, 30, 31},
     kMips64S32x4InterleaveLeft},
    {{0, 1, 2, 3, 8, 9, 10, 11, 16, 17, 18, 19, 24, 25, 26, 27},
     kMips64S32x4PackEven},
    {{4, 5, 6, 7, 12, 13, 14, 15, 20, 21, 22, 23, 28, 29, 30, 31},
     kMips64S32x4PackOdd},
    {{0, 1, 2, 3, 16, 17, 18, 19, 8, 9, 10, 11, 24, 25, 26, 27},
     kMips64S32x4InterleaveEven},
    {{4, 5, 6, 7, 20, 21, 22, 23, 12, 13, 14, 15, 28, 29, 30, 31},
     kMips64S32x4InterleaveOdd},

    {{0, 1, 16, 17, 2, 3, 18, 19, 4, 5, 20, 21, 6, 7, 22, 23},
     kMips64S16x8InterleaveRight},
    {{8, 9, 24, 25, 10, 11, 26, 27, 12, 13, 28, 29, 14, 15, 30, 31},
     kMips64S16x8InterleaveLeft},
    {{0, 1, 4, 5, 8, 9, 12, 13, 16, 17, 20, 21, 24, 25, 28, 29},
     kMips64S16x8PackEven},
    {{2, 3, 6, 7, 10, 11, 14, 15, 18, 19, 22, 23, 26, 27, 30, 31},
     kMips64S16x8PackOdd},
    {{0, 1, 16, 17, 4, 5, 20, 21, 8, 9, 24, 25, 12, 13, 28, 29},
     kMips64S16x8InterleaveEven},
    {{2, 3, 18, 19, 6, 7, 22, 23, 10, 11, 26, 27, 14, 15, 30, 31},
     kMips64S16x8InterleaveOdd},
    {{6, 7, 4, 5, 2, 3, 0, 1, 14, 15, 12, 13, 10, 11, 8, 9},
     kMips64S16x4Reverse},
    {{2, 3, 0, 1, 6, 7, 4, 5, 10, 11, 8, 9, 14, 15, 12, 13},
     kMips64S16x2Reverse},

    {{0, 16, 1, 17, 2, 18, 3, 19, 4, 20, 5, 21, 6, 22, 7, 23},
     kMips64S8x16InterleaveRight},
    {{8, 24, 9, 25, 10, 26, 11, 27, 12, 28, 13, 29, 14, 30, 15, 31},
     kMips64S8x16InterleaveLeft},
    {{0, 2, 4, 6, 8, 10, 12, 14, 16, 18, 20, 22, 24, 26, 28, 30},
     kMips64S8x16PackEven},
    {{1, 3, 5, 7, 9, 11, 13, 15, 17, 19, 21, 23, 25, 27, 29, 31},
     kMips64S8x16PackOdd},
    {{0, 16, 2, 18, 4, 20, 6, 22, 8, 24, 10, 26, 12, 28, 14, 30},
     kMips64S8x16InterleaveEven},
    {{1, 17, 3, 19, 5, 21, 7, 23, 9, 25, 11, 27, 13, 29, 15, 31},
     kMips64S8x16InterleaveOdd},
    {{7, 6, 5, 4, 3, 2, 1, 0, 15, 14, 13, 12, 11, 10, 9, 8},
     kMips64S8x8Reverse},
    {{3, 2, 1, 0, 7, 6, 5, 4, 11, 10, 9, 8, 15, 14, 13, 12},
     kMips64S8x4Reverse},
    {{1, 0, 3, 2, 5, 4, 7, 6, 9, 8, 11, 10, 13, 12, 15, 14},
     kMips64S8x2Reverse}};

bool TryMatchArchShuffle(const uint8_t* shuffle, const ShuffleEntry* table,
                         size_t num_entries, uint8_t mask, ArchOpcode* opcode) {
  for (size_t i = 0; i < num_entries; ++i) {
    const ShuffleEntry& entry = table[i];
    int j = 0;
    for (; j < kSimd128Size; ++j) {
      if ((entry.shuffle[j] & mask) != (shuffle[j] & mask)) {
        break;
      }
    }
    if (j == kSimd128Size) {
      *opcode = entry.opcode;
      return true;
    }
  }
  return false;
}

// Canonicalize shuffles to make pattern matching simpler. Returns a mask that
// will ignore the high bit of indices in some cases.
uint8_t CanonicalizeShuffle(InstructionSelector* selector, Node* node) {
  static const int kUnaryShuffleMask = kSimd128Size - 1;
  const uint8_t* shuffle = OpParameter<uint8_t*>(node);
  uint8_t mask = 0xff;
  // If shuffle is unary, set 'mask' to ignore the high bit of the indices.
  // Replace any unused source with the other.
  if (selector->GetVirtualRegister(node->InputAt(0)) ==
      selector->GetVirtualRegister(node->InputAt(1))) {
    // unary, src0 == src1.
    mask = kUnaryShuffleMask;
  } else {
    bool src0_is_used = false;
    bool src1_is_used = false;
    for (int i = 0; i < kSimd128Size; i++) {
      if (shuffle[i] < kSimd128Size) {
        src0_is_used = true;
      } else {
        src1_is_used = true;
      }
    }
    if (src0_is_used && !src1_is_used) {
      node->ReplaceInput(1, node->InputAt(0));
      mask = kUnaryShuffleMask;
    } else if (src1_is_used && !src0_is_used) {
      node->ReplaceInput(0, node->InputAt(1));
      mask = kUnaryShuffleMask;
    }
  }
  return mask;
}

int32_t Pack4Lanes(const uint8_t* shuffle, uint8_t mask) {
  int32_t result = 0;
  for (int i = 3; i >= 0; --i) {
    result <<= 8;
    result |= shuffle[i] & mask;
  }
  return result;
}

}  // namespace

void InstructionSelector::VisitS8x16Shuffle(Node* node) {
  const uint8_t* shuffle = OpParameter<uint8_t*>(node);
  uint8_t mask = CanonicalizeShuffle(this, node);
  uint8_t shuffle32x4[4];
  ArchOpcode opcode;
  if (TryMatchArchShuffle(shuffle, arch_shuffles, arraysize(arch_shuffles),
                          mask, &opcode)) {
    VisitRRR(this, opcode, node);
    return;
  }
  uint8_t offset;
  Mips64OperandGenerator g(this);
  if (TryMatchConcat(shuffle, mask, &offset)) {
    Emit(kMips64S8x16Concat, g.DefineSameAsFirst(node),
         g.UseRegister(node->InputAt(1)), g.UseRegister(node->InputAt(0)),
         g.UseImmediate(offset));
    return;
  }
  if (TryMatch32x4Shuffle(shuffle, shuffle32x4)) {
    Emit(kMips64S32x4Shuffle, g.DefineAsRegister(node),
         g.UseRegister(node->InputAt(0)), g.UseRegister(node->InputAt(1)),
         g.UseImmediate(Pack4Lanes(shuffle32x4, mask)));
    return;
  }
  Emit(kMips64S8x16Shuffle, g.DefineAsRegister(node),
       g.UseRegister(node->InputAt(0)), g.UseRegister(node->InputAt(1)),
       g.UseImmediate(Pack4Lanes(shuffle, mask)),
       g.UseImmediate(Pack4Lanes(shuffle + 4, mask)),
       g.UseImmediate(Pack4Lanes(shuffle + 8, mask)),
       g.UseImmediate(Pack4Lanes(shuffle + 12, mask)));
}

// static
MachineOperatorBuilder::Flags
InstructionSelector::SupportedMachineOperatorFlags() {
  MachineOperatorBuilder::Flags flags = MachineOperatorBuilder::kNoFlags;
  return flags | MachineOperatorBuilder::kWord32Ctz |
         MachineOperatorBuilder::kWord64Ctz |
         MachineOperatorBuilder::kWord32Popcnt |
         MachineOperatorBuilder::kWord64Popcnt |
         MachineOperatorBuilder::kWord32ShiftIsSafe |
         MachineOperatorBuilder::kInt32DivIsSafe |
         MachineOperatorBuilder::kUint32DivIsSafe |
         MachineOperatorBuilder::kFloat64RoundDown |
         MachineOperatorBuilder::kFloat32RoundDown |
         MachineOperatorBuilder::kFloat64RoundUp |
         MachineOperatorBuilder::kFloat32RoundUp |
         MachineOperatorBuilder::kFloat64RoundTruncate |
         MachineOperatorBuilder::kFloat32RoundTruncate |
         MachineOperatorBuilder::kFloat64RoundTiesEven |
         MachineOperatorBuilder::kFloat32RoundTiesEven |
         MachineOperatorBuilder::kWord32ReverseBytes |
         MachineOperatorBuilder::kWord64ReverseBytes;
}

// static
MachineOperatorBuilder::AlignmentRequirements
InstructionSelector::AlignmentRequirements() {
  if (kArchVariant == kMips64r6) {
    return MachineOperatorBuilder::AlignmentRequirements::
        FullUnalignedAccessSupport();
  } else {
    DCHECK(kArchVariant == kMips64r2);
    return MachineOperatorBuilder::AlignmentRequirements::
        NoUnalignedAccessSupport();
  }
}

}  // namespace compiler
}  // namespace internal
}  // namespace v8
