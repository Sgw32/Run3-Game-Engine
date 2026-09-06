#include "run3/legacy/LegacySmoke.hpp"

#include "../../CTParameters.h"
#include "../../Tokenizer.h"
#include "../../tinyxml.h"

#include <string>

namespace run3::legacy {

bool reusableSourcesSmoke() {
    CaduneTree::Parameters parameters;
    if (parameters.getNumLevels() == 0) {
        return false;
    }

    Tokenizer tokenizer;
    tokenizer.setDelim(" ");
    tokenizer.setString("run3 legacy");

    TiXmlDocument document;
    document.Parse("<run3 step=\"3\"/>");
    return !document.Error() && document.RootElement() != nullptr &&
           std::string{document.RootElement()->Value()} == "run3";
}

} // namespace run3::legacy
