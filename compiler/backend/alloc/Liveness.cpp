#include "Liveness.h"

namespace backend::alloc {

namespace {

using TempSet = std::unordered_set<std::size_t>;

TempSet defs(const ir::Quadruple& quad) {
    TempSet out;
    if (const auto* temp = std::get_if<ir::Temp>(&quad.result)) {
        out.insert(temp->id);
    }
    return out;
}

TempSet uses(const ir::Quadruple& quad) {
    TempSet out;
    auto collect = [&](const ir::Operand& operand) {
        if (const auto* temp = std::get_if<ir::Temp>(&operand)) {
            out.insert(temp->id);
        }
    };

    switch (quad.opcode) {
        case ir::Opcode::Assign:
            collect(quad.arg1);
            break;
        case ir::Opcode::Add:
        case ir::Opcode::Sub:
        case ir::Opcode::Mul:
        case ir::Opcode::Div:
        case ir::Opcode::Mod:
        case ir::Opcode::CompareEq:
        case ir::Opcode::CompareNe:
        case ir::Opcode::CompareLt:
        case ir::Opcode::CompareLe:
        case ir::Opcode::CompareGt:
        case ir::Opcode::CompareGe:
            collect(quad.arg1);
            collect(quad.arg2);
            break;
        case ir::Opcode::Neg:
        case ir::Opcode::LogicalNot:
        case ir::Opcode::JumpIfTrue:
        case ir::Opcode::JumpIfFalse:
        case ir::Opcode::Param:
        case ir::Opcode::Return:
            collect(quad.arg1);
            break;
        case ir::Opcode::Call:
            collect(quad.arg1);
            break;
        case ir::Opcode::Jump:
        case ir::Opcode::Label:
        case ir::Opcode::Nop:
            break;
    }

    return out;
}

TempSet set_union(const TempSet& lhs, const TempSet& rhs) {
    TempSet out = lhs;
    out.insert(rhs.begin(), rhs.end());
    return out;
}

TempSet set_difference(const TempSet& lhs, const TempSet& rhs) {
    TempSet out;
    for (const auto& value : lhs) {
        if (!rhs.contains(value)) {
            out.insert(value);
        }
    }
    return out;
}

} // namespace

LivenessInfo compute_liveness(const ir::FunctionIR& function) {
    LivenessInfo info;
    const auto count = function.quads.size();
    info.live_in.resize(count);
    info.live_out.resize(count);

    if (count == 0) {
        return info;
    }

    bool changed = true;
    while (changed) {
        changed = false;
        for (std::size_t i = count; i-- > 0;) {
            TempSet out;
            if (i + 1 < count) {
                out = info.live_in[i + 1];
            }

            const auto in = set_union(uses(function.quads[i]), set_difference(out, defs(function.quads[i])));
            if (in != info.live_in[i] || out != info.live_out[i]) {
                info.live_in[i] = in;
                info.live_out[i] = out;
                changed = true;
            }
        }
    }

    return info;
}

} // namespace backend::alloc
