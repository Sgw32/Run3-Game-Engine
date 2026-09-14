#include <filesystem>
#include <fstream>
#include <string>

int main(int argc, char **argv) {
  if (argc < 3) {
    return 2;
  }
  const std::filesystem::path input = argv[argc - 2];
  const std::filesystem::path output = argv[argc - 1];
  std::error_code error;
  std::filesystem::copy_file(input, output,
                             std::filesystem::copy_options::none, error);
  if (error) {
    return 3;
  }
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) == "-log") {
      std::ofstream(argv[index + 1]) << "converter stub\n";
      break;
    }
  }
  return 0;
}
