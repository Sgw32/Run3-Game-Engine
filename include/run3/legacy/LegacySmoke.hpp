#pragma once

namespace run3::legacy {

// Links representative symbols from the project-listed TinyXML, tokenizer,
// and CaduneTree sources without exporting their collision-prone root headers.
[[nodiscard]] bool reusableSourcesSmoke();

} // namespace run3::legacy
