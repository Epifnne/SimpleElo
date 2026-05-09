#include <fstream>
#include <string>

namespace simpleelo::server::repo {

bool ensureSchemaFile(const std::string& schemaPath) {
  std::ofstream output(schemaPath, std::ios::app);
  return output.good();
}

}  // namespace simpleelo::server::repo
