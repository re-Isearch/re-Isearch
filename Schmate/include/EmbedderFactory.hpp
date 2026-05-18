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


  // NOTE: 15 May 2026: We are moving to support the latest ggml library
  // and have also heavily updated the bert.cpp so if using that bert.cpp
  // you won't be able to load ggml files anymore!
  switch (model_type) {
      case GGML_TYPE::GGML: { 
#if !defined(BERT_API_VERSION) &&  (BERT_API_VERSION != 0)
        return std::make_unique<SBertGGML>(model_path);
#else
	throw std::runtime_error("GGML file format is no longer supported: '%s'", model_path.c_str());
#endif
      }
      case GGML_TYPE::GGUF: {
#if defined(BERT_API_VERSION) &&  (BERT_API_VERSION == 2)
        auto info = read_gguf_info(model_path);
        if (info) {
	  if (info->is_bert_cpp_compatible) 
	    return std::make_unique<SBertGGML>(model_path);
	}
#endif
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

