#include <iostream>
#include <sstream>
#include <iomanip>
#include <string>
#include <vector>
#include <cstdlib>

#include "SBertGGML.hpp"
#include "HnswConfig.hpp"
#include "ConfigBuilder.hpp"
#include "BertIndexManager.hpp"
#include "Logger.hpp"
#include "StderrCapture.hpp"
#include "ParseArgs.hpp"

#include <unistd.h>

using namespace std;

static void print_help() {
    cout <<
    "Commands:\n"
    "  use <name>\n"
    "  append <sentence>\n"
    "  appendid <sid> <sentence>\n"
    "  search <query>\n"
    "  knn [k] <query>\n"
//    "  pknn [k] <query>\n"
    "  radius [minScore] <query>\n"
//    "  pradius [minScore] <query>\n"
    "  relative [alpha] <query>\n"
    "  adaptive [alpha] [minN] [lookahead] [gapDelta] <query>\n"
    "  epsilon [radius]\n"
    "  delete <label> [shard]\n"
    "  undelete <label> [shard]\n"
    "  delete_addr <address> [shard]\n"
    "  undelete_addr <address> [shard]\n"
    "  merge\n"
    "  flush\n"
    "  shard_count [name]\n"
    "  reconstruct_label <label>\n"
    "  reconstruct_sid <sid>\n"
    "  set <key> <value>\n"
    "  list keys\n"
    "  show config\n"
    "  help\n"
    "  quit\n";
}

// Unified printResults() for all search modes
template <typename ResultVec>
inline void printResults(const ResultVec &results, bool debug = false) {
    using std::cout;
    using std::endl;

#ifdef NO_COLOR 
    bool use_color = false;
#else
    bool use_color = isatty(STDOUT_FILENO);
#endif

// --- Optional ANSI terminal colors  ---
    static const char *COLOR_RESET = "\033[0m";
    static const char *COLOR_ERROR = "\033[31;1;4m";
    static const char *COLOR_SCORE = "\033[38;5;39m";  // blue
    static const char *COLOR_LABEL = "\033[38;5;208m"; // orange
    static const char *COLOR_TEXT  = "\033[38;5;250m"; // gray
    static const char *COLOR_SID   = "\033[38;5;82m";  // green

    if (!use_color)
        COLOR_RESET = COLOR_ERROR = COLOR_SCORE = COLOR_LABEL = COLOR_TEXT = COLOR_SID = "";

    if (results.empty()) {
        cout << " - " << COLOR_ERROR <<  "(no results)" << COLOR_RESET << " -" << endl;
        return;
    }

    for (const auto &r : results) {
        cout << " - [score=" << COLOR_SCORE << std::fixed << std::setprecision(6)
             << r.score << COLOR_RESET
             << ", sid=" << COLOR_SID << r.sentence_id << COLOR_RESET
             << ", label=" << COLOR_LABEL << r.label << COLOR_RESET
             << ", tokens=[" << r.token_start << "," << r.token_end << "]] ";

        cout << COLOR_TEXT << r.text << COLOR_RESET << endl;

        if (debug) {
            cout << "   file=[" << r.file_start << "," << r.file_end << "]";
            // if (r.address) cout << " addr=" << r.address;
            cout << endl;
        }
    }
}



/*

SearchResult r;
r.score = d;
r.label = label;

auto it = label_to_entry.find(label);
if (it != label_to_entry.end()) {
    r.sentence_id = it->second.sid;
    r.start_tok   = it->second.start_tok;
    r.end_tok     = it->second.end_tok;
    r.file_start  = it->second.file_start;
    r.file_end    = it->second.file_end;
} else {
    r.sentence_id = -1; // unknown
}


*/


int main(int argc, char **argv) {
    if (argc < 2) {
        cerr << "Usage: " << argv[0] << " <sbert.ggml> [--debug] [--metric l2|ip|cos]\n";
        return 1;
    }
    string model = find_ggml_model(argv[1], "../lib:../bin:lib/:.").first;

    bool debug = false;
    MetricSpace metric = MetricSpace::Undefined;

    for (int i = 2; i < argc; ++i) {
        string a = argv[i];
        if (a == "--debug") debug = true;
        if (a == "--metric" && i+1 < argc) {
            string m = argv[++i];
            if (m == "l2") metric = MetricSpace::L2;
            else if (m == "ip") metric = MetricSpace::InnerProduct;
            else if (m == "cos") metric = MetricSpace::Cosine;
        }
    }

    ConfigLoader loader;
    HnswConfig cfg = loader.load(debug);
    if (metric != MetricSpace::Undefined) cfg.metric = metric;
  

   if (cfg.debug) Logger::instance().set_level(LogLevel::DEBUG);  // Show everything

   //StderrCapture::instance().start(); // redirect stderr 

    try {
        // create embedder first
        SBertGGML embedder(model);
        // manager uses references to embedder? our manager takes embedder ref in constructor earlier.
#if USE_LRUCACHE
        size_t cache_size = determine_optimal_hnsw_cache_size(cfg, embedder.n_embd);
	if (cfg.debug) LOG_DEBUG_S() << "Optimal Index Cache Size: " << cache_size;
        BertIndexManager manager(embedder, cfg, cache_size);
#else
        BertIndexManager manager(embedder, cfg);
#endif

        string current = "default";
        manager.getOrCreate(current);

        cout << "Interactive mode. Type 'help' for commands.\n";
        print_help();

        string line;
        while (true) {
            cout << "[" << current << "]> ";
            if (!getline(cin, line)) break;
            if (line.empty()) continue;

            if (line == "quit") break;
            if (line == "help") { print_help(); continue; }

            if (line.rfind("use ", 0) == 0) {
                current = line.substr(4);
                manager.getOrCreate(current);
                cout << "Switched to index: " << current << "\n";
                continue;
            }

            if (line.rfind("appendid ", 0) == 0) {
                istringstream iss(line.substr(9));
                int64_t sid; if (!(iss >> sid)) { cout<<"bad sid\n"; continue; }
                string rest; getline(iss, rest);
                if (!rest.empty() && rest[0] == ' ') rest = rest.substr(1);
                manager.append(current, rest, sid);
                cout << "Appended with sid=" << sid << "\n";
                continue;
            }

            if (line.rfind("append ", 0) == 0) {
                string txt = line.substr(7);
                manager.append(current, txt);
                cout << "Appended.\n";
                continue;
            }


// -------------------------------
// search
// -------------------------------

if (line.rfind("search ", 0) == 0) {
   string txt = line.substr(7);
   auto res = manager.search(current, txt);
   printResults(res, cfg.debug);
   continue;
}

// ---- KNN ----
if (line.rfind("knn", 0) == 0) {
    auto parsed = parseCommandArgs(line.substr(4), 1);
    size_t k = cfg.default_k;
    if (!parsed.args.empty()) parseInt(parsed.args[0], k);
    std::cerr << "[DEBUG] knn k=" << k << " q='" << parsed.query << "'\n";

    auto res = manager.knn(current, parsed.query, k);

    printResults(res, cfg.debug);

    continue;
}

// ---- RADIUS ----
if (line.rfind("radius", 0) == 0) {
    auto parsed = parseCommandArgs(line.substr(7), 1);
    float minScore = cfg.default_radius;
    if (!parsed.args.empty()) parseFloat(parsed.args[0], minScore);
    std::cerr << "[DEBUG] radius minScore=" << minScore << " q='" << parsed.query << "'\n";

    auto res = manager.radius(current, parsed.query, minScore);
    printResults(res, cfg.debug);

    continue;
}

// ---- RELATIVE ----
if (line.rfind("relative", 0) == 0) {
    auto parsed = parseCommandArgs(line.substr(9), 1);
    float alpha = cfg.default_alpha;
    if (!parsed.args.empty()) parseFloat(parsed.args[0], alpha);
    std::cerr << "[DEBUG] relative alpha=" << alpha << " q='" << parsed.query << "'\n";

    auto res = manager.relative(current, parsed.query, alpha);
    printResults(res, cfg.debug);
    continue;
}

// ---- ADAPTIVE ----
if (line.rfind("adaptive", 0) == 0) {
    auto parsed = parseCommandArgs(line.substr(9), 4);
    float alpha = cfg.default_alpha;
    size_t minN = cfg.default_minN;
    size_t lookahead = cfg.default_lookahead;
    float gapDelta = cfg.default_gapDelta;

    if (parsed.args.size() > 0) parseFloat(parsed.args[0], alpha);
    if (parsed.args.size() > 1) parseInt(parsed.args[1], minN);
    if (parsed.args.size() > 2) parseInt(parsed.args[2], lookahead);
    if (parsed.args.size() > 3) parseFloat(parsed.args[3], gapDelta);

    std::cerr << "[DEBUG] adaptive alpha=" << alpha
              << " minN=" << minN << " lookahead=" << lookahead
              << " gapDelta=" << gapDelta << " q='" << parsed.query << "'\n";

    auto res = manager.adaptive(current, parsed.query, alpha, minN, lookahead, gapDelta);
    printResults(res, cfg.debug);

    continue;
}

// epsilon [value] <query>
if (line.rfind("epsilon", 0) == 0) {
    auto parsed = parseCommandArgs(line.substr(8), 1);
    float epsilon = 0.0f;
    if (!parsed.args.empty()) parseFloat(parsed.args[0], epsilon);
//    std::cerr << "[DEBUG] epsilon epsilon=" << epsilon << " q='" << parsed.query << "'\n";

    auto res = manager.epsilon_search(current, parsed.query, epsilon);
    printResults(res, cfg.debug);

    continue;
}


        // --- New: showfull command ---
        if (line.rfind("showfull ", 0) == 0) {
            std::string q = line.substr(9);
            auto results = manager.knn(current, q, cfg.default_k);
            std::cout << "Results for '" << q << "' (full sentences):\n";
            for (auto &r : results) {
                std::string sentence = manager.get_text(current, r, true); // full sentence

                std::cout << " - [score=" << r.score
                          << " sid=" << r.sentence_id
                          << "] " << sentence << "\n\n";
            }
            continue;
        }


            if (line.rfind("delete_addr ",0)==0) {
                istringstream iss(line.substr(12));
                int64_t addr; iss>>addr;
                size_t shard=0; if (iss>>shard) {}
                manager.delete_byAddress(current, addr, shard);
                cout << "delete_addr executed\n";
                continue;
            }
            if (line.rfind("undelete_addr ",0)==0) {
                istringstream iss(line.substr(14));
                int64_t addr; iss>>addr;
                size_t shard=0; if (iss>>shard) {}
                manager.undelete_byAddress(current, addr, shard);
                cout << "undelete_addr executed\n";
                continue;
            }

            if (line.rfind("delete ",0)==0) {
                istringstream iss(line.substr(7));
                size_t label; iss>>label;
                size_t shard=0; if (iss>>shard) {}
                manager.remove(current, label, shard);
                cout << "Deleted label\n";
                continue;
            }

            if (line.rfind("undelete ",0)==0) {
                istringstream iss(line.substr(9));
                size_t label; iss>>label;
                size_t shard=0; if (iss>>shard) {}
                manager.undelete(current, label, shard);
                cout << "Undeleted\n";
                continue;
            }

            if (line == "merge") {
                manager.merge(current);
                cout << "Merged last two shards (if any).\n";
                continue;
            }
            if (line == "flush") {
                manager.flush(current);
                cout << "Flushed.\n";
                continue;
            }
            if (line.rfind ("shard_count", 0) == 0) {
                istringstream iss(line.substr(11));
                string index; iss>>index;
		if (index.empty()) index = current;
                cout << "Index \"" << index << "\" shard count: " << manager.shard_count(index) << "\n";
                continue;
            }
            if (line.rfind("reconstruct_label ",0)==0) {
                size_t label = stoul(line.substr(18));
                cout << manager.reconstruct_label(current, label) << "\n";
                continue;
            }
            if (line.rfind("reconstruct_sid ",0)==0) {
                int64_t sid = stoll(line.substr(16));
                cout << manager.reconstruct_sid(current, sid) << "\n";
                continue;
            }

            if (line.rfind("set ",0)==0) {
                istringstream iss(line.substr(4));
                string key; iss>>key;
		string value; iss >> value;
		if (cfg.set(key, value))
		    std::cout << key << "=" << value << std::endl;
		if (!cfg.validate()) 
		    std::cout << "Invalid value set" << std::endl;
		continue;
	    }
		
            if (line == "show config") {
		cfg.print();
                continue;
            }
	    if (line == "list keys") {
	        auto keys = cfg.get_all_keys();
		for (string i: keys)
		    std::cout << "\"" <<  i << "\" ";
		std::cout << std::endl;
                continue;
            }   

            cout << "Unknown command. Type 'help'\n";
        }

    } catch (const std::exception & ex) {
        cerr << "Fatal: " << ex.what() << endl;
        return 1;
    }

    return 0;
}



