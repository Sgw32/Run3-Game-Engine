// Historical VS2003 entrypoint retained only so Run3.vcproj inventory remains
// traceable. The live portable entrypoint is source/shell_main.cpp and owns no
// menu presentation. Menu state/actions live behind run3::ui::IUiSystem; the
// pinned MyGUI/Ogre adapter owns presentation. CEGUI is intentionally retired.

#include <run3/app/Run3App.hpp>

namespace run3::legacy {

// A link-free marker for inventory tools. This file is explicitly excluded
// from run3_legacy and must not become a second executable entrypoint.
const char *modernEntrypoint() noexcept { return "source/shell_main.cpp"; }

} // namespace run3::legacy
