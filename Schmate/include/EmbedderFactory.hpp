#pragma once
#include <memory>
#include <string>
#include <filesystem>
#include "Embedder.hpp"
#include "SBertGGML.hpp"
#include "LlamaEmbedder.hpp"
#include "Logger.hpp"
#include "Util.hpp"
#include "hf_model.hpp"

class EmbedderFactory {
public:
  static std::unique_ptr<BaseEmbedder> create(const std::string &filename,  const std::string& searchPath =".") {

  const auto model             =  find_ggml_model(filename, searchPath);

  const std::string model_path = model.first; 
  const GGML_TYPE   model_type = model.second;

  switch (model_type) {
      case GGML_TYPE::GGML: { 
        return std::make_unique<SBertGGML>(model_path);
      }
      case GGML_TYPE::GGUF: {
        return std::make_unique<LlamaEmbedder>(model_path);
     }
     default:
        throw std::runtime_error(
		(std::filesystem::exists(model_path) ?
		"Unsupported/Unknown model file format: " :
		"Model file not found: " ) + filename);
    }
 }
};

