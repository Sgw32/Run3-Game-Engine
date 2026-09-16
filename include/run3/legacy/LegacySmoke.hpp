#pragma once

namespace run3::legacy {

// Links representative symbols from the tokenizer and CaduneTree sources and
// verifies that legacy callers can use the Run3-owned XML boundary.
[[nodiscard]] bool reusableSourcesSmoke();

} // namespace run3::legacy
