#include <run3/gameplay/NpcFacialAnimation.hpp>

#include <run3/content/XmlParser.hpp>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <stdexcept>

namespace run3::gameplay {
namespace {

std::string context(const std::filesystem::path &path, std::size_t line) {
  return path.generic_string() + ":" + std::to_string(line) +
         " [facial-animation]: ";
}

bool boolean(const content::XmlNode &node, std::string_view name,
             bool fallback, const std::filesystem::path &path) {
  const std::string *text = node.attribute(name);
  if (text == nullptr) return fallback;
  if (*text == "true" || *text == "1") return true;
  if (*text == "false" || *text == "0") return false;
  throw std::runtime_error(context(path, node.line) + "invalid boolean " +
                           std::string(name) + "='" + *text + "'");
}

double number(const content::XmlNode &node, std::string_view name,
              const std::filesystem::path &path) {
  const std::string *text = node.attribute(name);
  if (text == nullptr)
    throw std::runtime_error(context(path, node.line) + "missing " +
                             std::string(name));
  try {
    std::size_t used{};
    const double value = std::stod(*text, &used);
    if (used != text->size() || !std::isfinite(value))
      throw std::invalid_argument("not finite");
    return value;
  } catch (const std::exception &) {
    throw std::runtime_error(context(path, node.line) + "invalid " +
                             std::string(name) + "='" + *text + "'");
  }
}

void appendInfluence(FacialPoseSample &sample, const int poseIndex,
                     const double weight) {
  if (poseIndex < 0 || weight <= 0.0) return;
  const float bounded = static_cast<float>(std::clamp(weight, 0.0, 1.0));
  const auto found = std::find_if(
      sample.influences.begin(), sample.influences.end(),
      [poseIndex](const FacialPoseInfluence &influence) {
        return influence.poseIndex == poseIndex;
      });
  if (found == sample.influences.end())
    sample.influences.push_back({poseIndex, bounded});
  else
    found->weight = std::min(1.0F, found->weight + bounded);
}

} // namespace

double FacialAnimationDefinition::durationSeconds() const noexcept {
  return phonemes.empty() ? 0.0 : phonemes.back().second;
}

FacialAnimationDefinition
loadFacialAnimationDefinition(const std::filesystem::path &path) {
  const content::XmlDocument document =
      content::parseXmlFile(path, content::XmlSchema::facialAnimation);
  FacialAnimationDefinition result;
  result.source = path;
  if (const std::string *subtitle =
          document.root.attribute("subtitle_text"))
    result.subtitle = *subtitle;
  result.patched = boolean(document.root, "patched", false, path);

  for (const content::XmlNode &node : document.root.children) {
    if (node.name == "file") {
      if (const std::string *name = node.attribute("name")) result.sound = *name;
      continue;
    }
    if (node.name != "node") continue;
    const double second = number(node, "time", path);
    if (second < 0.0)
      throw std::runtime_error(context(path, node.line) +
                               "phoneme time must be non-negative");
    if (!result.phonemes.empty() &&
        second < result.phonemes.back().second)
      throw std::runtime_error(context(path, node.line) +
                               "phoneme times must be non-decreasing");
    const std::string *letter = node.attribute("letter");
    if (letter == nullptr || letter->empty())
      throw std::runtime_error(context(path, node.line) +
                               "phoneme is missing letter");
    result.phonemes.push_back(
        {second, *letter, boolean(node, "patch", false, path)});
  }
  if (result.sound.empty())
    throw std::runtime_error(context(path, document.root.line) +
                             "missing <file name>");
  if (result.phonemes.empty())
    throw std::runtime_error(context(path, document.root.line) +
                             "facial animation has no phoneme nodes");
  return result;
}

int facialPoseIndex(const std::string_view letter) noexcept {
  if (letter.empty()) return 8;
  const char key = static_cast<char>(
      std::toupper(static_cast<unsigned char>(letter.front())));
  switch (key) {
  case 'A': return 2;
  case 'E': return 3;
  case 'O': return 4;
  case 'U': return 5;
  case 'I': return 6;
  case 'B': case 'D': case 'G': case 'J': case 'N': case 'R': case 'S':
  case 'T': case 'W': case 'Y': case 'Z': return 7;
  case 'C': case 'F': case 'H': case 'K': case 'Q': case 'V': case 'X':
    return 8;
  case 'L': case 'M': case 'P': return -1;
  default: return 8;
  }
}

FacialPoseSample
sampleFacialAnimation(const FacialAnimationDefinition &definition,
                      const double soundSecond) {
  FacialPoseSample sample;
  if (definition.phonemes.empty() || !std::isfinite(soundSecond)) return sample;

  const FacialPhoneme &first = definition.phonemes.front();
  if (soundSecond < first.second) {
    const double start = first.second - facialAnimationLeadInSeconds;
    if (soundSecond < start) return sample;
    sample.active = true;
    appendInfluence(sample, facialPoseIndex(first.letter),
                    (soundSecond - start) / facialAnimationLeadInSeconds);
    return sample;
  }

  const FacialPhoneme &last = definition.phonemes.back();
  if (soundSecond >= last.second) {
    const double fade = soundSecond - last.second;
    if (fade > facialAnimationLeadInSeconds) return sample;
    sample.active = true;
    appendInfluence(sample, facialPoseIndex(last.letter),
                    1.0 - fade / facialAnimationLeadInSeconds);
    return sample;
  }

  const auto after = std::upper_bound(
      definition.phonemes.begin(), definition.phonemes.end(), soundSecond,
      [](double second, const FacialPhoneme &phoneme) {
        return second < phoneme.second;
      });
  const FacialPhoneme &next = *after;
  const FacialPhoneme &previous = *(after - 1);
  const double span = next.second - previous.second;
  const double nextWeight = span <= 0.0
                                ? 1.0
                                : (soundSecond - previous.second) / span;
  sample.active = true;
  appendInfluence(sample, facialPoseIndex(previous.letter), 1.0 - nextWeight);
  appendInfluence(sample, facialPoseIndex(next.letter), nextWeight);
  return sample;
}

} // namespace run3::gameplay
