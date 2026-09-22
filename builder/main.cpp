#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <sstream>
#include <string>

namespace fs = std::filesystem;

static void usage(const char* p) {
    std::cout
        << "MirvkBuntu Builder 1.1.0\n"
        << "Builds the checked-out MirvkBuntu source into install media.\n\n"
        << "Usage: " << p << " [--limited|--desktop|--slim|--minimal|--all] [--check]\n"
        << "       " << p << " [--limited|--desktop|--slim|--minimal] [--jobs N]\n"
        << "       " << p << " --help\n\n"
        << "MirvkBuntu's repository build scripts remain the source of truth.\n";
}

static int run(const std::string& command) {
    std::cout << "builder: " << command << "\n";
    return std::system(command.c_str());
}

static bool commandExists(const std::string& command) {
#ifdef _WIN32
    return std::system(("where " + command + " >NUL 2>NUL").c_str()) == 0;
#else
    return std::system(("command -v " + command + " >/dev/null 2>&1").c_str()) == 0;
#endif
}

static fs::path findRepo() {
    fs::path p = fs::current_path();
    for (int i = 0; i < 8; ++i) {
        if (fs::exists(p / "build" / "build-desktop.sh") &&
            fs::exists(p / "build" / "common-build.sh") &&
            fs::exists(p / "kernels")) return p;
        if (p == p.root_path()) break;
        p = p.parent_path();
    }
    return {};
}

static std::string shellQuote(const std::string& s) {
    std::string out = "'";
    for (char c : s) {
        if (c == '\'') out += "'\\''";
        else out += c;
    }
    return out + "'";
}

static int runScout(const fs::path& repo) {
    fs::path binary = repo / "build" / "scout" / "mirvkbuntu-scout";
#ifdef _WIN32
    binary += ".exe";
#endif
    if (!fs::exists(binary)) {
        std::cout << "builder: dependency scout is not built; building it first.\\n";
#ifdef _WIN32
        std::string build = "cmake -S " + shellQuote((repo / "scout").string()) +
                            " -B " + shellQuote((repo / "build" / "scout").string()) +
                            " -DCMAKE_BUILD_TYPE=Release && cmake --build " +
                            shellQuote((repo / "build" / "scout").string()) + " --config Release";
#else
        std::string build = "cd " + shellQuote(repo.string()) + " && bash build/scout.sh";
#endif
        int rc = run(build);
        if (rc != 0) return rc;
    }
    return run(shellQuote(binary.string()));
}

static int linuxBuild(const fs::path& repo, const std::string& target, const std::string& jobs) {
    int scout = runScout(repo);
    if (scout != 0) return scout;
    std::ostringstream cmd;
    cmd << "cd " << shellQuote(repo.string()) << " && ";
    if (!jobs.empty()) cmd << "JOBS=" << shellQuote(jobs) << " ";
    cmd << "bash build/build-" << target << ".sh";
    return run(cmd.str());
}

static int windowsBuild(const fs::path& repo, const std::string& target, const std::string& jobs) {
    int scout = runScout(repo);
    if (scout != 0) return scout;
    if (!commandExists("wsl.exe")) {
        std::cerr << "builder: Windows ISO creation requires WSL with a Linux distribution.\n";
        return 20;
    }
    std::string inner = "cd \"$(wslpath -a '" + repo.string() + "')\" && ";
    if (!jobs.empty()) inner += "JOBS=" + jobs + " ";
    inner += "bash build/build-" + target + ".sh";
    return run("wsl.exe bash -lc " + shellQuote(inner));
}

static int macBuild(const fs::path& repo, const std::string& target, const std::string& jobs) {
    int scout = runScout(repo);
    if (scout != 0) return scout;
    std::string runtime;
    if (commandExists("docker")) runtime = "docker";
    else if (commandExists("podman")) runtime = "podman";
    else {
        std::cerr << "builder: macOS ISO creation requires Docker or Podman.\n";
        return 21;
    }

    std::string inner =
        "apt-get update && "
        "DEBIAN_FRONTEND=noninteractive apt-get install -y "
        "bash ca-certificates git build-essential cmake cpack live-build "
        "xorriso squashfs-tools debootstrap dosfstools rsync && ";
    if (!jobs.empty()) inner += "JOBS=" + jobs + " ";
    inner += "bash build/build-" + target + ".sh";

    std::string command = runtime + " run --privileged --rm -v " +
        shellQuote(repo.string() + ":/src") +
        " -w /src ubuntu:24.04 bash -lc " + shellQuote(inner);
    return run(command);
}

static int checkEnvironment(const fs::path& repo) {
    std::cout << "MirvkBuntu Builder environment check\n";
    std::cout << "Repository: " << repo << "\n";
#ifdef _WIN32
    std::cout << "Host: Windows 10+\n";
    std::cout << "WSL: " << (commandExists("wsl.exe") ? "available" : "missing") << "\n";
#elif defined(__APPLE__)
    std::cout << "Host: macOS\n";
    std::cout << "Docker: " << (commandExists("docker") ? "available" : "missing") << "\n";
    std::cout << "Podman: " << (commandExists("podman") ? "available" : "missing") << "\n";
#else
    std::cout << "Host: Linux\n";
    std::cout << "bash: " << (commandExists("bash") ? "available" : "missing") << "\n";
    std::cout << "live-build: " << (commandExists("lb") ? "available" : "missing") << "\n";
    std::cout << "xorriso: " << (commandExists("xorriso") ? "available" : "missing") << "\n";
#endif
    return 0;
}

int main(int argc, char** argv) {
    std::string target = "desktop";
    std::string jobs;
    bool check = false;
    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        if (a == "--help" || a == "-h") { usage(argv[0]); return 0; }
        if (a == "--check") { check = true; continue; }
        if (a == "--limited" || a == "--desktop" || a == "--slim" || a == "--minimal") { target = a.substr(2); continue; }
        if (a == "--all") { target = "all"; continue; }
        if (a == "--jobs" && i + 1 < argc) { jobs = argv[++i]; continue; }
        std::cerr << "builder: unknown or incomplete option: " << a << "\n";
        return 2;
    }

    fs::path repo = findRepo();
    if (repo.empty()) {
        std::cerr << "builder: run this executable from inside a MirvkBuntu checkout.\n";
        return 3;
    }
    if (check) return checkEnvironment(repo);

    auto buildOne = [&](const std::string& t) -> int {
#if defined(_WIN32)
        return windowsBuild(repo, t, jobs);
#elif defined(__APPLE__)
        return macBuild(repo, t, jobs);
#else
        return linuxBuild(repo, t, jobs);
#endif
    };

    if (target == "all") {
        for (const auto& t : {"desktop", "slim", "minimal"}) {
            int rc = buildOne(t);
            if (rc != 0) return rc;
        }
        return 0;
    }
    return buildOne(target);
}
