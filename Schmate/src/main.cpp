
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <cstdlib>

#include "SBertGGML.hpp"
#include "HnswConfig.hpp"
#include "BertIndexManager.hpp"
#include "Logger.hpp"
#include "StderrCapture.hpp"

using namespace std;

static void print_help() {
    cout <<
    "Commands:\n"
    "  use <name>\n"
    "  append <sentence>\n"
    "  appendid <sid> <sentence>\n"
    "  knn [k] <query>\n"
//    "  pknn [k] <query>\n"
    "  radius [minScore] <query>\n"
//    "  pradius [minScore] <query>\n"
    "  relative [alpha] <query>\n"
    "  adaptive [alpha] [minN] [lookahead] [gapDelta] <query>\n"
    "  delete <label> [shard]\n"
    "  undelete <label> [shard]\n"
    "  delete_addr <address> [shard]\n"
    "  undelete_addr <address> [shard]\n"
    "  merge\n"
    "  flush\n"
    "  shard_count\n"
    "  reconstruct_label <label>\n"
    "  reconstruct_sid <sid>\n"
    "  set <key> <value>\n"
    "  show config\n"
    "  help\n"
    "  quit\n";
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
    string model = find_ggml_model(argv[1], "../lib:../bin:lib/:.");;
    HnswConfig cfg;
    for (int i = 2; i < argc; ++i) {
        string a = argv[i];
        if (a == "--debug") cfg.debug = true;
        if (a == "--metric" && i+1 < argc) {
            string m = argv[++i];
            if (m == "l2") cfg.metric = Metric::L2;
            else if (m == "ip") cfg.metric = Metric::InnerProduct;
            else if (m == "cos") cfg.metric = Metric::Cosine;
        }
    }

   if (cfg.debug) Logger::instance().set_level(LogLevel::DEBUG);  // Show everything

   //StderrCapture::instance().start(); // redirect stderr 

    try {
        // create embedder first
        SBertGGML embedder(model);
        // manager uses references to embedder? our manager takes embedder ref in constructor earlier.
        BertIndexManager manager(embedder, cfg);

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
// KNN search
// -------------------------------
if (line.rfind("knn", 0) == 0) {
    std::string payload = (line.size() > 3) ? line.substr(4) : "";
    std::istringstream iss(payload);
    std::string first;
    size_t k = cfg.default_k;
    std::string q;

    if (iss >> first) {
        if (std::all_of(first.begin(), first.end(), ::isdigit))
            k = std::stoul(first);
        else
            q = first;
        std::string tail;
        std::getline(iss, tail);
        q += tail;
    }

    if (!q.empty() && q[0] == ' ') q.erase(0, 1);
    std::cerr << "[DEBUG] knn k=" << k << " q='" << q << "'\n";

    auto res = manager.knn(current, q, k);
    for (auto &r : res)
        std::cout << " - [score=" << r.score << ", sid=" << r.sentence_id
                  << ", label=" << r.label << ", tokens=[" << r.token_start << "," << r.token_end
                  << "]] " << r.text << "\n";
    continue;
}

// -------------------------------
// RADIUS search
// -------------------------------
if (line.rfind("radius", 0) == 0) {
    std::string payload = (line.size() > 6) ? line.substr(7) : "";
    std::istringstream iss(payload);
    std::string first;
    float minScore = cfg.default_radius;
    std::string q;

    if (iss >> first) {
        char* end;
        float v = std::strtof(first.c_str(), &end);
        if (end != first.c_str() && *end == '\0')
            minScore = v;
        else
            q = first;
        std::string tail;
        std::getline(iss, tail);
        q += tail;
    }

    if (!q.empty() && q[0] == ' ') q.erase(0, 1);
    std::cerr << "[DEBUG] radius minScore=" << minScore << " q='" << q << "'\n";

    auto res = manager.radius(current, q, minScore);
    for (auto &r : res)
        std::cout << " - [score=" << r.score << ", sid=" << r.sentence_id
                  << ", label=" << r.label << ", tokens=[" << r.token_start << "," << r.token_end
                  << "]] " << r.text << "\n";
    continue;
}

// -------------------------------
// RELATIVE search
// -------------------------------
if (line.rfind("relative", 0) == 0) {
    std::string payload = (line.size() > 8) ? line.substr(9) : "";
    std::istringstream iss(payload);
    std::string first;
    float alpha = cfg.default_alpha;
    std::string q;

    if (iss >> first) {
        char* end;
        float v = std::strtof(first.c_str(), &end);
        if (end != first.c_str() && *end == '\0')
            alpha = v;
        else
            q = first;
        std::string tail;
        std::getline(iss, tail);
        q += tail;
    }

    if (!q.empty() && q[0] == ' ') q.erase(0, 1);
    std::cerr << "[DEBUG] relative alpha=" << alpha << " q='" << q << "'\n";

    auto res = manager.relative(current, q, alpha);
    for (auto &r : res)
        std::cout << " - [score=" << r.score << ", sid=" << r.sentence_id
                  << ", label=" << r.label << ", tokens=[" << r.token_start << "," << r.token_end
                  << "]] " << r.text << "\n";
    continue;
}

// -------------------------------
// ADAPTIVE search
// -------------------------------
if (line.rfind("adaptive", 0) == 0) {
    std::string payload = (line.size() > 8) ? line.substr(9) : "";
    std::istringstream iss(payload);
    std::string first;
    float alpha = cfg.default_alpha;
    size_t minN = cfg.default_minN;
    size_t lookahead = cfg.default_lookahead;
    float gapDelta = cfg.default_gapDelta;
    std::string q;

    // Try reading up to 4 optional numeric params
    std::vector<std::string> args;
    std::string tok;
    while (iss >> tok && args.size() < 4)
        args.push_back(tok);
    std::getline(iss, q);

    auto parse_float = [](const std::string &s, float &out) {
        char* end;
        float v = std::strtof(s.c_str(), &end);
        if (end != s.c_str() && *end == '\0') { out = v; return true; }
        return false;
    };
    auto parse_int = [](const std::string &s, size_t &out) {
        char* end;
        long v = std::strtol(s.c_str(), &end, 10);
        if (end != s.c_str() && *end == '\0') { out = v; return true; }
        return false;
    };

    if (!args.empty()) {
        parse_float(args[0], alpha);
        if (args.size() > 1) parse_int(args[1], minN);
        if (args.size() > 2) parse_int(args[2], lookahead);
        if (args.size() > 3) parse_float(args[3], gapDelta);
    }

    if (!q.empty() && q[0] == ' ') q.erase(0, 1);
    std::cerr << "[DEBUG] adaptive alpha=" << alpha
              << " minN=" << minN << " lookahead=" << lookahead
              << " gapDelta=" << gapDelta << " q='" << q << "'\n";

    auto res = manager.adaptive(current, q, alpha, minN, lookahead, gapDelta);
    for (auto &r : res)
        std::cout << " - [score=" << r.score << ", sid=" << r.sentence_id
                  << ", label=" << r.label << ", tokens=[" << r.token_start << "," << r.token_end
                  << "]] " << r.text << "\n";
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
            if (line == "shard_count") {
                cout << "Shard count: " << manager.shard_count(current) << "\n";
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
                if (key=="default_k") { size_t v; iss>>v; cfg.default_k=v; cout<<"Set default_k="<<cfg.default_k<<"\n"; }
                else if (key=="default_radius") { float v; iss>>v; cfg.default_radius=v; cout<<"Set default_radius="<<cfg.default_radius<<"\n"; }
                else if (key=="default_alpha") { float v; iss>>v; cfg.default_alpha=v; cout<<"Set default_alpha="<<cfg.default_alpha<<"\n"; }
                else if (key=="default_minN") { size_t v; iss>>v; cfg.default_minN=v; cout<<"Set default_minN="<<cfg.default_minN<<"\n"; }
                else if (key=="default_lookahead") { size_t v; iss>>v; cfg.default_lookahead=v; cout<<"Set default_lookahead="<<cfg.default_lookahead<<"\n"; }
                else if (key=="default_gapDelta") { float v; iss>>v; cfg.default_gapDelta=v; cout<<"Set default_gapDelta="<<cfg.default_gapDelta<<"\n"; }
                else if (key=="debug") { string v; iss>>v; cfg.debug = (v=="1" || v=="true"); cout<<"Set debug="<<cfg.debug<<"\n"; }
                else cout << "Unknown key\n";
                continue;
            }

            if (line == "show config") {
                cout << "default_k="<<cfg.default_k<<" default_radius="<<cfg.default_radius<<" default_alpha="<<cfg.default_alpha
                     <<" default_minN="<<cfg.default_minN<<" default_lookahead="<<cfg.default_lookahead<<" default_gapDelta="<<cfg.default_gapDelta
                     <<" debug="<<cfg.debug<<"\n";
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



