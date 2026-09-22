#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <set>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>
#include <cctype>

namespace fs = std::filesystem;

static void usage(const char* p) {
    std::cout
        << "MirvkBuntu Dependency Scout 1.0.0\\n"
        << "Scans MirvkBuntu components, package manifests, and APT dependencies before compilation.\\n\\n"
        << "Usage: " << p << " [--check|--download|--refresh] [--jobs N]\\n"
        << "       " << p << " --help\\n\\n"
        << "Default cache: build/cache/packages\\n";
}

static int run(const std::string& command) {
    std::cout << "scout: " << command << "\\n";
    return std::system(command.c_str());
}

static std::string quote(const std::string& s) {
    std::string o="'";
    for(char c:s) o += (c=='\'' ? "'\\''" : std::string(1,c));
    return o+"'";
}

static fs::path findRepo() {
    fs::path p=fs::current_path();
    for(int i=0;i<8;i++){
        if(fs::exists(p/"build"/"CUSTOM_COMPONENTS.txt") &&
           fs::exists(p/"packages"/"basic-packages.txt")) return p;
        if(p==p.root_path()) break;
        p=p.parent_path();
    }
    return {};
}

static std::vector<std::string> packagesFromManifest(const fs::path& repo) {
    std::vector<std::string> out;
    std::ifstream f(repo/"packages"/"basic-packages.txt");
    std::string line;
    while(std::getline(f,line)){
        auto hash=line.find('#'); if(hash!=std::string::npos) line.erase(hash);
        std::istringstream in(line); std::string p;
        while(in>>p) if(!p.empty() && p[0]!='-') out.push_back(p);
    }
    return out;
}

static bool commandExists(const std::string& c) {
#ifdef _WIN32
    return std::system(("where "+c+" >NUL 2>NUL").c_str())==0;
#else
    return std::system(("command -v "+c+" >/dev/null 2>&1").c_str())==0;
#endif
}

static int check(const fs::path& repo) {
    std::cout<<"MirvkBuntu Dependency Scout preflight\\nRepository: "<<repo<<"\\n";
    std::cout<<"Custom registry: "<<(fs::exists(repo/"build"/"CUSTOM_COMPONENTS.txt")?"present":"MISSING")<<"\\n";
    std::cout<<"Package manifest: "<<(fs::exists(repo/"packages"/"basic-packages.txt")?"present":"MISSING")<<"\\n";
#ifdef _WIN32
    std::cout<<"Host: Windows 10+; dependency acquisition uses WSL.\\n";
    std::cout<<"WSL: "<<(commandExists("wsl.exe")?"available":"missing")<<"\\n";
#elif defined(__APPLE__)
    std::cout<<"Host: macOS; dependency acquisition uses Docker/Podman.\\n";
    std::cout<<"Docker: "<<(commandExists("docker")?"available":"missing")<<"\\n";
    std::cout<<"Podman: "<<(commandExists("podman")?"available":"missing")<<"\\n";
#else
    std::cout<<"Host: Linux\\n";
    std::cout<<"apt-cache: "<<(commandExists("apt-cache")?"available":"missing")<<"\\n";
    std::cout<<"apt-get: "<<(commandExists("apt-get")?"available":"missing")<<"\\n";
    std::cout<<"apt: "<<(commandExists("apt")?"available":"missing")<<"\\n";
#endif
    return 0;
}

static int linuxScout(const fs::path& repo, bool download) {
    if(!commandExists("apt-cache") || !commandExists("apt-get")) {
        std::cerr<<"scout: apt-cache and apt-get are required on Linux.\\n"; return 30;
    }
    fs::path cache=repo/"build"/"cache"/"packages";
    fs::path work=repo/"build"/"work";
    fs::create_directories(cache); fs::create_directories(work);

    const auto roots=packagesFromManifest(repo);
    if(roots.empty()){ std::cerr<<"scout: package manifest contains no packages.\\n"; return 31; }

    fs::path requested=work/"dependency-requested.txt";
    fs::path resolved=work/"dependency-resolved.txt";
    std::set<std::string> seen;
    std::vector<std::string> queue=roots;

    std::ofstream req(requested);
    for(const auto& p:roots) req<<p<<"\\n";
    req.close();

    while(!queue.empty()){
        std::string p=queue.back(); queue.pop_back();
        if(seen.count(p)) continue;
        seen.insert(p);
        std::string cmd="apt-cache depends "+quote(p)+" 2>/dev/null";
        FILE* pipe=popen(cmd.c_str(),"r");
        if(!pipe) continue;
        char buf[4096];
        while(fgets(buf,sizeof(buf),pipe)){
            std::string s(buf);
            auto pos=s.find("Depends:");
            if(pos==std::string::npos) pos=s.find("PreDepends:");
            if(pos==std::string::npos) continue;
            s=s.substr(pos+(s.find("PreDepends:")!=std::string::npos?11:8));
            while(!s.empty() && std::isspace(static_cast<unsigned char>(s.front()))) s.erase(s.begin());
            auto bar=s.find(" | "); if(bar!=std::string::npos) s.erase(bar);
            auto colon=s.find(':'); if(colon!=std::string::npos) s.erase(colon);
            auto space=s.find_first_of(" \\t\\r\\n");
            if(space!=std::string::npos) s.erase(space);
            if(!s.empty() && s[0]!='<') queue.push_back(s);
        }
        pclose(pipe);
    }

    std::ofstream out(resolved);
    for(const auto& p:seen) out<<p<<"\\n";
    out.close();

    std::cout<<"scout: requested packages: "<<roots.size()<<"\\n";
    std::cout<<"scout: resolved package names: "<<seen.size()<<"\\n";
    std::cout<<"scout: dependency manifest: "<<resolved<<"\\n";

    if(download){
        for(const auto& p:seen){
            std::string cmd="cd "+quote(cache.string())+" && apt-get download "+quote(p);
            int rc=run(cmd);
            if(rc!=0){ std::cerr<<"scout: failed to download "<<p<<"\\n"; return 32; }
        }
        std::ofstream stamp(work/"dependency-cache.txt");
        stamp<<"MIRVKBUNTU_DEPENDENCY_CACHE="<<cache<<"\\n";
        stamp<<"PACKAGE_COUNT="<<seen.size()<<"\\n";
        stamp.close();
    }
    return 0;
}

static int windowsScout(const fs::path& repo, bool download) {
    if(!commandExists("wsl.exe")) { std::cerr<<"scout: WSL is required on Windows 10+.\\n"; return 40; }
    std::string path="\\"$(wslpath -a '"+repo.string()+"')\\"";
    std::string inner="cd "+path+" && bash build/scout-linux.sh";
    if(!download) inner+=" --check";
    return run("wsl.exe bash -lc "+quote(inner));
}

static int macScout(const fs::path& repo, bool download) {
    std::string runtime;
    if(commandExists("docker")) runtime="docker";
    else if(commandExists("podman")) runtime="podman";
    else { std::cerr<<"scout: Docker or Podman is required on macOS.\\n"; return 41; }
    std::string inner="apt-get update && DEBIAN_FRONTEND=noninteractive apt-get install -y bash ca-certificates apt-utils && bash build/scout-linux.sh";
    if(!download) inner+=" --check";
    return run(runtime+" run --rm -v "+quote(repo.string()+":/src")+" -w /src ubuntu:24.04 bash -lc "+quote(inner));
}

int main(int argc,char** argv){
    bool checkOnly=false, download=true;
    for(int i=1;i<argc;i++){
        std::string a=argv[i];
        if(a=="--help"||a=="-h"){usage(argv[0]);return 0;}
        if(a=="--check"){checkOnly=true;download=false;continue;}
        if(a=="--download"||a=="--refresh"){download=true;continue;}
        if(a=="--jobs"&&i+1<argc){++i;continue;}
        std::cerr<<"scout: unknown option: "<<a<<"\\n"; return 2;
    }
    fs::path repo=findRepo();
    if(repo.empty()){std::cerr<<"scout: run inside a MirvkBuntu checkout.\\n";return 3;}
    if(checkOnly) return check(repo);
#if defined(_WIN32)
    return windowsScout(repo,download);
#elif defined(__APPLE__)
    return macScout(repo,download);
#else
    return linuxScout(repo,download);
#endif
}
