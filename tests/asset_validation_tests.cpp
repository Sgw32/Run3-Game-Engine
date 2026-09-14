#include <run3/content/AssetValidation.hpp>

#include <catch2/catch_test_macros.hpp>

#include <filesystem>
#include <fstream>
#include <string>

namespace fs = std::filesystem;

namespace {

class TemporaryDirectory {
public:
  explicit TemporaryDirectory(std::string name)
      : path_(fs::temp_directory_path() / std::move(name)) {
    std::error_code error;
    fs::remove_all(path_, error);
    fs::create_directories(path_);
  }
  ~TemporaryDirectory() {
    std::error_code error;
    fs::remove_all(path_, error);
  }
  const fs::path &path() const { return path_; }

private:
  fs::path path_;
};

void write(const fs::path &path, const std::string &contents) {
  fs::create_directories(path.parent_path());
  std::ofstream output(path, std::ios::binary);
  REQUIRE(output);
  output << contents;
}

} // namespace

TEST_CASE("miniature content fixture passes structural validation") {
  const fs::path root = RUN3_ASSET_FIXTURE_ROOT;
  const run3::AssetReport report = run3::validateContent(
      {root, root / "manifest.json"});
  INFO(report.conciseReport());
  CHECK(report.errorCount() == 0);
  CHECK(report.checks.at("xml_files") == 2);
  CHECK(report.checks.at("lua_files") == 1);
  CHECK(report.checks.at("ogre_script_files") == 3);
}

TEST_CASE("SHA-256 values are stable") {
  TemporaryDirectory temporary("run3-asset-sha-test");
  const fs::path file = temporary.path() / "abc.txt";
  write(file, "abc");
  CHECK(run3::sha256File(file) ==
        "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
}

TEST_CASE("fixture failures are categorized without modifying content") {
  TemporaryDirectory temporary("run3-asset-invalid-test");
  const fs::path root = temporary.path();
  write(root / "manifest.json",
        R"({"schema_version":1,"manifest_version":1,"content_id":"invalid","content_version":"1","compatibility_runtime":{"lua":"5.4.8","ogre":"14.5.2","resource_profile":"resources.cfg"},"scan_roots":["."],"resource_configs":["resources.cfg"],"xml_extensions":[".xml"],"lua_extensions":[".lua"],"ogre_script_extensions":[]})");
  write(root / "resources.cfg",
        "[General]\nFileSystem=Assets\nFileSystem=second\n"
        "FileSystem=casepath\n");
  write(root / "Assets" / "same.txt", "first");
  write(root / "second" / "same.txt", "second");
  write(root / "CasePath" / "case-only.txt", "case");
  write(root / "bad.xml", "<root><broken></root>");
  write(root / "bad.lua", "local = broken");
  write(root / "reference.xml", "<root file=\"missing.mesh\"/>");

  const run3::AssetReport report =
      run3::validateContent({root, root / "manifest.json"});
  std::map<std::string, std::size_t> categories;
  for (const run3::AssetIssue &issue : report.issues) {
    ++categories[issue.category];
  }
  CHECK(categories["duplicate-logical-resource"] == 1);
  CHECK(categories["xml-not-well-formed"] == 1);
  CHECK(categories["lua-parse"] == 1);
  CHECK(categories["referenced-file-missing"] == 1);
  CHECK(categories["case-mismatch"] == 1);
}
