#include "hf_mapper.hpp"
#include <string.h>


int main(int argc, char** argv) {
  if (argc != 3) {
        std::cerr << "HuggingFace Map Compiler:\n\
  Usage: " << argv[0] << " input output \n\
  where input is a text file source and output is binary.\n\
  if input is -- then it does a decompile of output to standard out.\n\
  Format: key=repro|filename with # as comment line\n\
  Valid keys may not contain the '=' character." << std::endl;
        return 1;
    }
   if (strcmp(argv[1], "--") == 0) {
     // Decompile to standard out
     HFModelMap map(argv[2]);
     map.dump();
   } else
     HFModelMap::compile(argv[1], argv[2]);

}

