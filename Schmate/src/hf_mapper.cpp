#include <cstring>
#include "hf_mapper.hpp"
#include "Util.hpp"

using namespace hfmap;

static bool is_pow2(uint32_t v) {
    return v && !(v & (v - 1));
}

std::optional<HFModelMapper>
HFModelMapper::load(const void* data, size_t size) {
    if (size < sizeof(Header))
        return std::nullopt;

    const Header* h = static_cast<const Header*>(data);

    if (h->magic != kMagic || h->version != kVersion)
        return std::nullopt;

    if (!is_pow2(h->table_size))
        return std::nullopt;

    size_t needed =
        sizeof(Header) +
        size_t(h->table_size) * sizeof(Entry) +
        size_t(h->pool_size);

    if (needed > size)
        return std::nullopt;

    HFModelMapper m;
    m.header = h;
    m.table  = reinterpret_cast<const Entry*>(h + 1);
    m.pool   = reinterpret_cast<const char*>(m.table + h->table_size);
    m.mask   = h->table_size - 1;

    return m;
}



// Lookup
void HFModelMapper::dump() const {
    size_t items =0;
    // C++20 has  std::countr_zero(x) but we'll use the builtin from GCC/CLang
    std::cout << "# HF Model dump (" <<  __builtin_ctz(header->table_size) << "^2 buckets)\n";

   // Emit entries in table order (stable, deterministic)
    for (uint32_t i = 0; i < header->table_size; ++i) {
        const Entry& e = table[i];
        if (e.key_offset == kEmpty)
            continue;

        if (e.key_offset >= header->pool_size ||
            e.val_offset >= header->pool_size) {
            std::cerr << "Invalid string offset\n";
            return;
        }

        const char* key  = pool + e.key_offset;
        const char* val  = pool + e.val_offset;

        std::cout << key << '=' << val << std::endl;
        items++;
    }
    std::cout << "# " << items << " items" << std::endl;
}



// Lookup
std::string_view HFModelMapper::lookup(std::string_view key) const {
    uint32_t h = fnv1a(key);
    size_t i = h & mask;

    while (table[i].key_offset != kEmpty) {
        if (table[i].hash == h) {
            const char* k = pool + table[i].key_offset;
            if (key == k)
                return pool + table[i].val_offset;
        }
        i = (i + 1) & mask;
    }
    return {};
}


#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>


bool HFModelMap::open(const std::string& filepath) {
    if(data) munmap(data, size);
    data = NULL;
    size = 0;
    mapper = std::nullopt;

    if (!filepath.empty()) { 
      int fd = ::open(filepath.c_str(), O_RDONLY);
      if (fd > 0) {
        size = lseek(fd, 0, SEEK_END); 
        if (size) data = mmap(nullptr, size, PROT_READ, MAP_PRIVATE, fd, 0);
        close(fd);
        if (data == NULL) size = 0;
        if (size) mapper = HFModelMapper::load(data, size); 
      if (mapper) return true;
      } 
    }
    return false; 
}   

HFModelMap::~HFModelMap() {
   if(data) munmap(data, size);
}


/*
# hf_models.txt
llama2_7b=TheBloke/Llama-2-7B-GGUF|llama-2-7b.Q4_K_M.gguf
llama2_13b=TheBloke/Llama-2-13B-GGUF|llama-2-13b.Q4_K_M.gguf
mistral_7b=TheBloke/Mistral-7B-Instruct-v0.2-GGUF|mistral-7b-instruct-v0.2.Q4_K_M.gguf
codellama=TheBloke/CodeLlama-13B-GGUF|codellama-13b.Q4_K_M.gguf
mixtral=TheBloke/Mixtral-8x7B-Instruct-v0.1-GGUF|mixtral-8x7b-instruct-v0.1.Q4_K_M.gguf
*/



static size_t next_pow2(size_t v) {
    v--;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    return ++v;
}


/*
# hf_models.txt
llama2_7b=TheBloke/Llama-2-7B-GGUF|llama-2-7b.Q4_K_M.gguf
llama2_13b=TheBloke/Llama-2-13B-GGUF|llama-2-13b.Q4_K_M.gguf
mistral_7b=TheBloke/Mistral-7B-Instruct-v0.2-GGUF|mistral-7b-instruct-v0.2.Q4_K_M.gguf
codellama=TheBloke/CodeLlama-13B-GGUF|codellama-13b.Q4_K_M.gguf
mixtral=TheBloke/Mixtral-8x7B-Instruct-v0.1-GGUF|mixtral-8x7b-instruct-v0.1.Q4_K_M.gguf
*/


bool HFModelMap::compile(const std::string &source, const std::string &dest ) {
  std::ifstream in(source); // Open as text (default modus)
  if (!in) {
     std::cerr << "# Can't open '" << source << "' to compile.\n";
     return false;
   }

   std::vector<std::pair<std::string, std::string>> items;
   std::string line;

   size_t lineno = 1;
   while (std::getline(in, line)) {
      if (line.empty() || line[0] == '#')
          continue;

        auto pos = line.find('=');
        if (pos == std::string::npos) {
            std::cerr << "Line#" << lineno << " does not contain a = character, skipping.\n";
            continue;
        }
        if (pos >= MAX_MODEL_ID_LENGTH) {
            std::cerr << "Line#" << lineno << " key is >= " << MAX_MODEL_ID_LENGTH << " bytes. Truncating.\n";
            pos = MAX_MODEL_ID_LENGTH - 1;
        }

        if (line.find('|') == std::string::npos) {
            std::cerr << "Line#" << lineno << " does not contain a | character, skipping.\n";
            continue;
        }


        items.emplace_back(
            line.substr(0, pos),
            line.substr(pos + 1)
        );
        lineno++;
    }

    size_t table_size = next_pow2(items.size() * 2);
    std::vector<Entry> table(table_size);
    for (auto& e : table)
        e.key_offset = 0xFFFFFFFF;

    std::vector<char> pool;
    pool.reserve(1024);

    auto add_string = [&](const std::string& s) {
        uint32_t off = pool.size();
        pool.insert(pool.end(), s.begin(), s.end());
        pool.push_back('\0');
        return off;
    };

    for (auto& [k, v] : items) {
        uint32_t h = fnv1a(k);
        size_t i = h & (table_size - 1);

        uint32_t k_off = add_string(k);
        uint32_t v_off = add_string(v);

        while (table[i].key_offset != 0xFFFFFFFF)
            i = (i + 1) & (table_size - 1);

        table[i] = {k_off, v_off, h, 0};
    }

    Header hdr {
        kMagic,
        kVersion,
        static_cast<uint32_t>(items.size()),
        static_cast<uint32_t>(table_size),
        static_cast<uint32_t>(pool.size())
    };

    std::ofstream out(dest, std::ios::binary);
    if (!out) {
      std::cerr << "# Can't write compiled tables to '" << dest << "!\n";
      return false;
    } else  {
      out.write(reinterpret_cast<char*>(&hdr), sizeof(hdr));
      out.write(reinterpret_cast<char*>(table.data()), table.size() * sizeof(Entry));
      out.write(pool.data(), pool.size());

      std::cerr << "# Processed " << lineno-1 << " lines. " << items.size() << " items.\n";
    }
  return true;
}


std::optional<HFModelMap> ModelMap() {
    auto result = findInPathsWithData<HFModelMap>(
        "models.bin",
        "/opt/nonmonotonic/schmate/etc:/usr/local/ib/etc:~/.ib",
        [](const std::filesystem::path& path) -> std::optional<HFModelMap> {
            HFModelMap map(path);
            if (!map.Ok()) return std::nullopt;
            return map;
        });
    
    if (result) {
        return result->second;
    }
    return std::nullopt;
}

#ifdef MAIN

int main() {
    int fd = open("models.bin", O_RDONLY);
    if (fd == -1) {
       std::cerr << "Mapper file does not exist\n";
       return -1;
    }
    size_t size = lseek(fd, 0, SEEK_END);

    void* data = mmap(nullptr, size, PROT_READ, MAP_PRIVATE, fd, 0);
    close(fd);

    auto mapper = HFModelMapper::load(data, size);
    if (!mapper) {
        std::cerr << "Invalid mapper file\n";
        return 1;
    }

    auto v = mapper->lookup("bert-base");
    std::cout << "Looking up \"bert-base\"" << std::endl;
    if (!v.empty())
        std::cout << v << "\n";

    munmap(data, size);
}

#endif
