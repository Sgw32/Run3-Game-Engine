#include "run3/legacy/LegacySmoke.hpp"
#include <run3/content/XmlParser.hpp>

#include "../../CTParameters.h"
#include "../../Tokenizer.h"

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

    const auto document = run3::content::parseXml(
        "<run3 step=\"8\"/>", "legacy-smoke.xml",
        run3::content::XmlSchema::configAdjacent);
    return document.root.name == "run3";
}

} // namespace run3::legacy
