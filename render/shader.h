#ifndef MEINEKRAFT_SHADER_H
#define MEINEKRAFT_SHADER_H

#include <string>
#include <vector>
#include <set>
#include "texture.h"

class Shader {
public:
  Shader() = default;
  Shader(std::string vertex_filepath, std::string fragment_filepath);
  
  bool operator==(const Shader& rhs);

  /// Loads and compiles the shader source, return compile error message in the pair.
  std::pair<bool, std::string> compile();

  /// Loads and recompiles both shaders
  std::pair<bool, std::string> recompile();

  void add(std::string define);

  std::string vertex_filepath;
  std::string fragment_filepath;
  uint64_t gl_program;
  
  uint64_t get_uniform_location(Texture::Type type);

private:
  uint64_t vertex_shader;
  uint64_t fragment_shader;

  std::set<std::string> defines;
  
  /// Validates that all of the work together
  bool validate();

  /// Assumes the file exists and will seg. fault otherwise.
  const std::string load_shader_source(std::string filename) const;

  /// Checks if a file exists
  bool file_exists(std::string filename) const;
};

#endif // MEINEKRAFT_SHADER_H
