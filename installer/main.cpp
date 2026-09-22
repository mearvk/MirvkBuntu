#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>

namespace fs = std::filesystem;

#ifndef MIRVKBUNTU_VERSION
#define MIRVKBUNTU_VERSION "dev"
#endif

static void usage(const char* argv0) {
    std::cout << "MirvkBuntu Installer " << MIRVKBUNTU_VERSION << "\n"
              << "Usage: " << argv0 << " [--prefix PATH] [--source PATH] [--dry-run]\n";
}

static bool is_root() {
#ifdef _WIN32
    return true;
#else
    const char* user = std::getenv("USER");
    return user == nullptr || std::string(user) == "root";
#endif
}

int main(int argc, char** argv) {
    fs::path prefix;
#ifdef _WIN32
    prefix = fs::path(std::getenv("ProgramFiles") ? std::getenv("ProgramFiles") : "C:/Program Files") / "MirvkBuntu";
#elif __APPLE__
    prefix = "/Applications/MirvkBuntu";
#else
    prefix = "/opt/mirvkbuntu";
#endif

    fs::path source = fs::current_path();
    bool dry_run = false;

    for (int i = 1; i < argc; ++i) {
        std::string arg(argv[i]);
        if (arg == "--help" || arg == "-h") { usage(argv[0]); return 0; }
        else if (arg == "--dry-run") { dry_run = true; }
        else if (arg == "--prefix" && i + 1 < argc) { prefix = argv[++i]; }
        else if (arg == "--source" && i + 1 < argc) { source = argv[++i]; }
        else { std::cerr << "Unknown or incomplete option: " << arg << "\n"; usage(argv[0]); return 2; }
    }

    std::cout << "MirvkBuntu Installer " << MIRVKBUNTU_VERSION << "\n";
    std::cout << "Source: " << source << "\n";
    std::cout << "Install prefix: " << prefix << "\n";

#ifndef _WIN32
    if (!is_root() && prefix.string().rfind("/opt/", 0) == 0) {
        std::cerr << "Installing to " << prefix << " normally requires administrator privileges.\n";
        std::cerr << "Use sudo or choose a writable --prefix.\n"; return 3;
    }
#endif

    if (dry_run) { std::cout << "Dry run: no files were changed.\n"; return 0; }
    std::error_code ec; fs::create_directories(prefix, ec);
    if (ec) { std::cerr << "Unable to create install prefix: " << ec.message() << "\n"; return 4; }
    std::cout << "Installer framework initialized successfully.\n";
    std::cout << "The MirvkBuntu ISO build remains responsible for assembling the operating-system image.\n";
    return 0;
}