// Taskweft HTN planner + HRR holographic memory — godot-sandbox RISC-V ELF guest.
// Godot host loads a domain JSON string; guest plans and returns JSON.
// HRR algebra exposed for host-side similarity search and state encoding.
// Build with: scons (see SConstruct) or CMake (see CMakeLists.txt).
//
// CPPPATH must include multiplayer-fabric-taskweft/standalone/ so these
// header-only includes resolve without path prefixes.
#include <api.hpp>
#include "tw_loader.hpp"
#include "tw_planner.hpp"
#include "tw_hrr.hpp"

static TwLoader::TwLoaded g_loaded;

// Load a JSON-LD domain (full file contents as a string from the host).
static Variant api_load_domain(String json) {
    std::string s(json);
    g_loaded = TwLoader::load_json(s);
    return Nil;
}

// Plan using the pre-loaded domain's default task list. Returns plan JSON.
static Variant api_plan() {
    auto plan = tw_plan(g_loaded.state, g_loaded.tasks, g_loaded.domain);
    std::string result = plan ? TwLoader::plan_to_json(*plan) : "null";
    return String(result);
}

// Combined load + plan in one call — avoids a round-trip vmcall from the host.
// Returns plan JSON, or "null" if planning fails.
static Variant api_plan_domain(String json) {
    std::string s(json);
    TwLoader::TwLoaded loaded = TwLoader::load_json(s);
    auto plan = tw_plan(loaded.state, loaded.tasks, loaded.domain);
    std::string result = plan ? TwLoader::plan_to_json(*plan) : "null";
    return String(result);
}

// Plan an explicit task list (JSON array of arrays). Returns plan JSON.
static Variant api_plan_tasks(String tasks_json) {
    std::string s(tasks_json);
    TwValue tasks_val = TwLoader::parse_json_str(s);
    std::vector<TwTask> tasks;
    if (tasks_val.is_array()) {
        for (auto &t : tasks_val.as_array()) {
            if (!t.is_array() || t.as_array().empty()) continue;
            TwCall call;
            call.name = t.as_array()[0].as_string();
            for (size_t i = 1; i < t.as_array().size(); ++i)
                call.args.push_back(t.as_array()[i]);
            tasks.push_back(std::move(call));
        }
    }
    auto plan = tw_plan(g_loaded.state, tasks, g_loaded.domain);
    std::string result = plan ? TwLoader::plan_to_json(*plan) : "null";
    return String(result);
}

// ---- HRR holographic memory API -------------------------------------------

// Encode a word → phase vector serialised as JSON float array.
static Variant api_hrr_encode_atom(String word, int dim) {
    auto phases = TwHRR::encode_atom(std::string(word), dim);
    std::string result = "[";
    for (size_t i = 0; i < phases.size(); ++i) {
        if (i) result += ",";
        char buf[32];
        snprintf(buf, sizeof(buf), "%.8f", phases[i]);
        result += buf;
    }
    result += "]";
    return String(result);
}

// similarity(a_json, b_json) → float. Inputs are JSON float arrays.
static Variant api_hrr_similarity(String a_json, String b_json) {
    auto parse_vec = [](const std::string &s) -> TwHRR::PhaseVec {
        TwValue v = TwLoader::parse_json_str(s);
        TwHRR::PhaseVec phases;
        if (v.is_array()) {
            for (auto &x : v.as_array())
                phases.push_back(x.as_number());
        }
        return phases;
    };
    auto a = parse_vec(std::string(a_json));
    auto b = parse_vec(std::string(b_json));
    return (float)TwHRR::similarity(a, b);
}

// encode_text(text, dim) → JSON float array
static Variant api_hrr_encode_text(String text, int dim) {
    auto phases = TwHRR::encode_text(std::string(text), dim);
    std::string result = "[";
    for (size_t i = 0; i < phases.size(); ++i) {
        if (i) result += ",";
        char buf[32];
        snprintf(buf, sizeof(buf), "%.8f", phases[i]);
        result += buf;
    }
    result += "]";
    return String(result);
}

int main() {
    ADD_API_FUNCTION(api_load_domain,      "void",   "String json");
    ADD_API_FUNCTION(api_plan,             "String", "");
    ADD_API_FUNCTION(api_plan_domain,      "String", "String json");
    ADD_API_FUNCTION(api_plan_tasks,       "String", "String tasks_json");
    ADD_API_FUNCTION(api_hrr_encode_atom,  "String", "String word, int dim");
    ADD_API_FUNCTION(api_hrr_encode_text,  "String", "String text, int dim");
    ADD_API_FUNCTION(api_hrr_similarity,   "float",  "String a_json, String b_json");
    halt();
}
