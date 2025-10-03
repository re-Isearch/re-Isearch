
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <cstdlib>

#include "SBertGGML.hpp"
#include "HnswConfig.hpp"
#include "BertIndexManager.hpp"

using namespace std;

static void print_help() {
    cout <<
    "Commands:\n"
    "  use <name>\n"
    "  append <sentence>\n"
    "  appendid <sid> <sentence>\n"
    "  knn [k] <query>\n"
    "  pknn [k] <query>\n"
    "  radius [minScore] <query>\n"
    "  pradius [minScore] <query>\n"
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

int main(int argc, char **argv) {
    if (argc < 2) {
        cerr << "Usage: " << argv[0] << " <sbert.ggml> [--debug] [--metric l2|ip|cos]\n";
        return 1;
    }
    string model = argv[1];
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

            if (line.rfind("knn",0) == 0) {
                string payload = (line.size() > 3) ? line.substr(4) : "";
                istringstream iss(payload);
                size_t k = 0;
                if (!(iss >> k)) k = cfg.default_k;
                string q; getline(iss, q); if (!q.empty() && q[0]==' ') q=q.substr(1);
                auto res = manager.knn(current, q, k);
                for (auto &r : res) cout << " - [score="<<r.score<<", sid="<<r.sentence_id<<", label="<<r.label<<", tokens=["<<r.token_start<<","<<r.token_end<<"]] "<< r.text << "\n";
                continue;
            }

            if (line.rfind("pknn",0) == 0) {
                string payload = (line.size() > 5) ? line.substr(5) : "";
                istringstream iss(payload);
                size_t k = 0; if (!(iss >> k)) k = cfg.default_k;
                string q; getline(iss, q); if (!q.empty() && q[0]==' ') q=q.substr(1);
                auto res = manager.pknn(current, q, k);
                for (auto &r : res) cout << " - [score="<<r.score<<", sid="<<r.sentence_id<<", label="<<r.label<<", tokens=["<<r.token_start<<","<<r.token_end<<"]] "<< r.text << "\n";
                continue;
            }

            if (line.rfind("radius ",0) == 0) {
                istringstream iss(line.substr(7));
                float s; if (!(iss>>s)) s = cfg.default_radius;
                string q; getline(iss,q); if (!q.empty() && q[0]==' ') q=q.substr(1);
                auto res = manager.radius(current, q, s);
                for (auto &r : res) cout << " - [score="<<r.score<<", sid="<<r.sentence_id<<", label="<<r.label<<"] "<< r.text << "\n";
                continue;
            }

            if (line.rfind("pradius ",0) == 0) {
                istringstream iss(line.substr(8));
                float s; if (!(iss>>s)) s = cfg.default_radius;
                string q; getline(iss,q); if (!q.empty() && q[0]==' ') q=q.substr(1);
                auto res = manager.pradius(current, q, s);
                for (auto &r : res) cout << " - [score="<<r.score<<", sid="<<r.sentence_id<<", label="<<r.label<<"] "<< r.text << "\n";
                continue;
            }

            if (line.rfind("relative ",0)==0) {
                istringstream iss(line.substr(9));
                float a; if (!(iss>>a)) a = cfg.default_alpha;
                string q; getline(iss,q); if (!q.empty() && q[0]==' ') q=q.substr(1);
                auto res = manager.relative(current, q, a);
                for (auto &r : res) cout << " - [score="<<r.score<<", sid="<<r.sentence_id<<", label="<<r.label<<"] "<< r.text << "\n";
                continue;
            }

            if (line.rfind("adaptive ",0)==0) {
                istringstream iss(line.substr(9));
                float a; size_t m,l; float g;
                if (!(iss>>a)) a = cfg.default_alpha;
                if (!(iss>>m)) m = cfg.default_minN;
                if (!(iss>>l)) l = cfg.default_lookahead;
                if (!(iss>>g)) g = cfg.default_gapDelta;
                string q; getline(iss,q); if (!q.empty() && q[0]==' ') q=q.substr(1);
                auto res = manager.adaptive(current, q, a, m, l, g);
                for (auto &r : res) cout << " - [score="<<r.score<<", sid="<<r.sentence_id<<", label="<<r.label<<"] "<< r.text << "\n";
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



