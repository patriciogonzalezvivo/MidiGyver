#pragma once

namespace YAML {
enum class EmitterNodeType : char { NoType, Property, Scalar, FlowSeq, BlockSeq, FlowMap, BlockMap };
}
