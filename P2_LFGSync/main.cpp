// an attempt at organization

// essentials
#include <iostream>
#include <fstream>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <syncstream>

// string manip
#include <string>
#include <format>
#include <sstream>
#include <charconv>
#include <optional>
#include <limits>

// rng & time
#include <chrono>
#include <random>

// convenience
#include <algorithm>
#include <vector>
#include <ranges>

std::mutex mtx;
std::condition_variable cv, cv2;
bool ready = false;
bool done = false;
int num_served = 0;
int num_working = 0;
int target_id = -2;
int signal_iter = 0;

// flip to true to see the internal signaler/patience traces
constexpr bool DEBUG = false;

// per-instance stats (index = instance id), sized in main before threads start
std::vector<int> parties_served;
std::vector<int> time_served;
const auto start_time = std::chrono::steady_clock::now();

// seconds since program start, used as a prefix on every event line
std::string timestamp(){
    const auto secs = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - start_time).count();
    return std::format("[t={:>3}s]", secs);
}

// caller must hold mtx
void print_status(const std::vector<uint8_t>& working_list){
    std::osyncstream out(std::cout);
    out << "  Instance status:";
    for (size_t i = 0; i < working_list.size(); ++i){
        out << std::format("  [{}] {}", i, working_list[i] ? "active" : "empty ");
    }
    out << "\n";
}

void instance_worker(int id, int& num_tank, int& num_healer, int& num_dps, int t1, int t2, std::vector<uint8_t>& working_list){
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> distrib(t1, t2);
    bool isWorking = true;
    int run_time = 0;
    int party_id = 0;
    while(isWorking){
        {
            std::unique_lock<std::mutex> lock(mtx);

            if (done) {
                ready = false;
                cv2.notify_all();
                break;
            }

            // -1 meaning emergency
            cv.wait(lock, [&] {
                return (ready && (target_id == id)) || ( target_id == -1 );
            });

            ready = false;

            if(num_dps >= 3 && num_tank >= 1 && num_healer >= 1){
                num_dps -= 3;
                num_tank -= 1;
                num_healer -= 1;
                ++num_working;
                ++num_served;
                party_id = num_served;
                working_list[id] = 1;
                run_time = distrib(gen);
                std::osyncstream(std::cout) << std::format("{} Party #{} entered instance {} (clear time: {}s) | queue left: {} tank, {} healer, {} dps\n",
                    timestamp(), party_id, id, run_time, num_tank, num_healer, num_dps);
                print_status(working_list);

            }
            else{
                isWorking = false;
                cv2.notify_all();
                continue;
            }
        }
        // notify when a worker lets go of lock
        cv2.notify_all();
        // signaler will have the lock if all instances are 'busy'
        std::this_thread::sleep_for(std::chrono::seconds(run_time));
        {
            std::unique_lock<std::mutex> lock(mtx);
            working_list[id] = 0;
            ++parties_served[id];
            time_served[id] += run_time;
            --num_working;
            std::osyncstream(std::cout) << std::format("{} Party #{} cleared instance {} after {}s\n", timestamp(), party_id, id, run_time);
            print_status(working_list);
            // notify when a worker finished a run
            cv2.notify_all();
        }
    }
}

void signaling_worker(int instances, std::vector<uint8_t>& working_list, int& num_tank, int& num_healer, int& num_dps){
   std::vector<int> patience_list(instances, 0);
   while (true){
        {
            std::unique_lock<std::mutex> lock(mtx);
            ++signal_iter;

            if (done) break;

            if constexpr (DEBUG) std::osyncstream(std::cout) << "[SIGNALER] Number of instances working: " << num_working << "\n";
            if (num_working == instances){
                {
                    if constexpr (DEBUG) std::osyncstream(std::cout) << "SIGNALER SLEEPING... " << num_working << "\n";
                    cv2.wait( lock, [&] { return ( num_working < instances ); });
                    if constexpr (DEBUG) std::osyncstream(std::cout) << "SIGNALER AWAKENING!!! " << num_working << "\n";
                }
            }

            if(!(num_dps >= 3 && num_tank >= 1 && num_healer >= 1)){
                target_id = -1;
                done = true;
            }
            else{
                auto ids = std::views::iota(0, instances);
                target_id = *std::ranges::max_element(ids, {}, [&](int i) {
                    return working_list[i] == 1 ? -1 : patience_list[i];
                });
                if constexpr (DEBUG) std::osyncstream(std::cout) << "SIGNALER CHOOSES INSTANCE " << target_id << "\n";
                patience_list[target_id] = 0;

                const int n = static_cast<int>(patience_list.size());
                for (int i = 0; i < n; ++i){
                    if ( i != target_id && !working_list[i] ) {
                        patience_list[i]++;
                        if constexpr (DEBUG) std::osyncstream(std::cout) << "PATIENCE++: " << i << ", total patience: " << patience_list[i] << "\n";
                    }
                }
            }

            ready = true;
        }
        cv.notify_all();
        {
            std::unique_lock<std::mutex> lock(mtx);
            cv2.wait( lock, [&] { return ( !ready ); });
        }
    }
}

// ---------- input validation ----------

constexpr int MAX_INSTANCES = 1000;   // keeps thread count sane
constexpr int MAX_CLEAR_TIME = 15;    // spec: t2 <= 15

// strict integer parse: rejects "", "12abc", "1.5", "abc" and anything outside int range
std::optional<int> parse_int(const std::string& token){
    long long value = 0;
    const char* first = token.data();
    const char* last = token.data() + token.size();
    if (!token.empty() && *first == '+') ++first;   // from_chars doesn't accept a leading '+'
    auto [ptr, ec] = std::from_chars(first, last, value);
    if (ec != std::errc{} || ptr != last || first == last) return std::nullopt;
    if (value < std::numeric_limits<int>::min() || value > std::numeric_limits<int>::max()) return std::nullopt;
    return static_cast<int>(value);
}

struct Config {
    int instances = 0;
    int num_tank = 0;
    int num_healer = 0;
    int num_dps = 0;
    int t1 = 0;
    int t2 = 0;
};

// reads "key value" lines; blank lines and lines starting with '#' are ignored.
// prints every problem it finds (not just the first) and returns nullopt on any error.
std::optional<Config> load_config(const std::string& path){
    std::ifstream file(path);
    if (!file.is_open()){
        std::cerr << "error: could not open '" << path << "'\n";
        return std::nullopt;
    }

    Config cfg;
    const std::vector<std::pair<std::string, int*>> fields = {
        {"instances", &cfg.instances}, {"num_tank", &cfg.num_tank}, {"num_healer", &cfg.num_healer},
        {"num_dps", &cfg.num_dps}, {"t1", &cfg.t1}, {"t2", &cfg.t2},
    };
    std::vector<bool> seen(fields.size(), false);
    bool ok = true;
    auto fail = [&](int line_no, const std::string& msg){
        std::cerr << path << ":" << line_no << ": error: " << msg << "\n";
        ok = false;
    };

    std::string line;
    int line_no = 0;
    while (std::getline(file, line)){
        ++line_no;
        std::istringstream ss(line);
        std::string key, value, extra;
        if (!(ss >> key) || key[0] == '#') continue;

        auto it = std::ranges::find(fields, key, &std::pair<std::string, int*>::first);
        if (it == fields.end()){
            fail(line_no, "unknown key '" + key + "'");
            continue;
        }
        const auto idx = static_cast<size_t>(it - fields.begin());
        if (seen[idx]){
            fail(line_no, "'" + key + "' is set more than once");
            continue;
        }
        seen[idx] = true;   // counts as present even if the value below is bad, so it isn't also reported as missing
        if (!(ss >> value)){
            fail(line_no, "missing value for '" + key + "'");
            continue;
        }
        if (ss >> extra){
            fail(line_no, "unexpected extra text '" + extra + "' after '" + key + " " + value + "'");
            continue;
        }
        auto parsed = parse_int(value);
        if (!parsed){
            fail(line_no, "'" + value + "' is not a valid whole number for '" + key + "' (or is too large)");
            continue;
        }
        *it->second = *parsed;
    }

    for (size_t i = 0; i < fields.size(); ++i){
        if (!seen[i]){
            std::cerr << path << ": error: missing required key '" << fields[i].first << "'\n";
            ok = false;
        }
    }
    if (!ok) return std::nullopt;

    // range checks (only meaningful once every key is present)
    auto check = [&](bool cond, const std::string& msg){
        if (!cond){
            std::cerr << path << ": error: " << msg << "\n";
            ok = false;
        }
    };
    check(cfg.instances >= 1, std::format("instances must be at least 1 (got {})", cfg.instances));
    check(cfg.instances <= MAX_INSTANCES, std::format("instances must be at most {} (got {})", MAX_INSTANCES, cfg.instances));
    check(cfg.num_tank >= 0, std::format("num_tank cannot be negative (got {})", cfg.num_tank));
    check(cfg.num_healer >= 0, std::format("num_healer cannot be negative (got {})", cfg.num_healer));
    check(cfg.num_dps >= 0, std::format("num_dps cannot be negative (got {})", cfg.num_dps));
    check(cfg.t1 >= 0, std::format("t1 cannot be negative (got {})", cfg.t1));
    check(cfg.t2 <= MAX_CLEAR_TIME, std::format("t2 must be at most {} (got {})", MAX_CLEAR_TIME, cfg.t2));
    check(cfg.t1 <= cfg.t2, std::format("t1 ({}) cannot be greater than t2 ({})", cfg.t1, cfg.t2));
    if (!ok) return std::nullopt;
    return cfg;
}

int main(int argc, char* argv[]){
    if (argc > 2){
        std::cerr << "usage: " << argv[0] << " [config file]   (defaults to config.txt)\n";
        return 1;
    }
    const std::string path = argc == 2 ? argv[1] : "config.txt";
    auto cfg = load_config(path);
    if (!cfg){
        std::cerr << "please fix the config file and try again.\n";
        return 1;
    }

    const int instances = cfg->instances;
    int num_tank = cfg->num_tank;
    int num_healer = cfg->num_healer;
    int num_dps = cfg->num_dps;
    const int t1 = cfg->t1;
    const int t2 = cfg->t2;

    const int possible_parties = std::min({num_tank, num_healer, num_dps / 3});
    std::cout << "=== LFG Dungeon Queue ===\n";
    std::cout << std::format("Instances: {} | Queue: {} tank, {} healer, {} dps | Clear time: {}-{}s\n",
                             instances, num_tank, num_healer, num_dps, t1, t2);
    std::cout << std::format("Full parties that can be formed: {}\n", possible_parties);
    if (possible_parties == 0){
        std::cout << "note: not enough players for a full party (needs 1 tank, 1 healer, 3 dps).\n";
    }
    std::cout << "\n";

    // "bootstrapping"
    parties_served.assign(instances, 0);
    time_served.assign(instances, 0);
    std::vector<uint8_t> working_list(instances, 0);
    std::vector<std::thread> thread_list;
    thread_list.reserve(instances + 1);

    for(int i = 0; i < instances; ++i){
        thread_list.emplace_back(instance_worker, i, std::ref(num_tank), std::ref(num_healer), std::ref(num_dps), t1, t2, std::ref(working_list));
    }
    thread_list.emplace_back(signaling_worker, instances, std::ref(working_list), std::ref(num_tank), std::ref(num_healer), std::ref(num_dps));
    for(auto& t : thread_list){
        if (t.joinable()) t.join();
    }

    // output results
    std::cout << "\n=== Summary ===\n";
    std::cout << std::format("{:<10} {:>15} {:>18}\n", "Instance", "Parties served", "Total time served");
    int total_time = 0;
    for (int i = 0; i < instances; ++i){
        std::cout << std::format("{:<10} {:>15} {:>17}s\n", i, parties_served[i], time_served[i]);
        total_time += time_served[i];
    }
    std::cout << std::format("{:<10} {:>15} {:>17}s\n", "Total", num_served, total_time);
    std::cout << std::format("\nPlayers left in queue: {} tank, {} healer, {} dps\n", num_tank, num_healer, num_dps);
    if constexpr (DEBUG) std::cout << "total signaler iterations " << signal_iter << "\n";

    return 0;
}
