#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <filesystem>
#include <cstdlib>
#include <unistd.h>
#include <sys/stat.h>
#include <openssl/sha.h>

namespace fs = std::filesystem;

std::string nixStyleHash(const std::string& packageName) {
    unsigned char hash[SHA_DIGEST_LENGTH];
    SHA1(reinterpret_cast<const unsigned char*>(packageName.c_str()), packageName.size(), hash);

    std::stringstream ss;
    for(int i = 0; i < 5; i++) {  // 5 byte = 10 hex digits (nix benzeri)
        ss << std::hex << (int)hash[i];
    }
    return ss.str();
}

std::string installPackage(const std::string& packageName, const std::string& storeDir) {
    std::string packageHash = nixStyleHash(packageName);
    fs::path packageDir = storeDir + "/" + packageHash;

    if(!fs::exists(packageDir)) {
        std::cout << "Installing " << packageName << " to " << packageDir << std::endl;
        std::string command = "sudo ROOT=" + packageDir.string() + " emerge --buildpkg --ask " + packageName;
        system(command.c_str());
    } else {
        std::cout << packageName << " is already installed at " << packageDir << std::endl;
    }

    return packageDir.string();
}

void createEnvironment(const std::vector<std::string>& packages, const std::string& storeDir, const std::string& envDir) {
    if(!fs::exists(envDir)) {
        fs::create_directories(envDir);
    }

    for(const auto& package : packages) {
        std::string packageDir = installPackage(package, storeDir);
        for(const auto& subDir : {"bin", "lib", "include"}) {
            fs::path srcDir = packageDir + "/" + subDir;
            if(fs::exists(srcDir)) {
                fs::path destDir = envDir + "/" + subDir;
                if(!fs::exists(destDir)) {
                    fs::create_directories(destDir);
                }
                for(const auto& entry : fs::directory_iterator(srcDir)) {
                    fs::path srcFile = entry.path();
                    fs::path destFile = destDir / srcFile.filename();
                    if(!fs::exists(destFile)) {
                        fs::create_symlink(srcFile, destFile);
                    }
                }
            }
        }
    }
}

void activateEnvironment(const std::string& envDir) {
    std::string path = envDir + "/bin:" + std::getenv("PATH");
    std::string ldLibraryPath = envDir + "/lib:" + (std::getenv("LD_LIBRARY_PATH") ? std::getenv("LD_LIBRARY_PATH") : "");
    
    setenv("PATH", path.c_str(), 1);
    setenv("LD_LIBRARY_PATH", ldLibraryPath.c_str(), 1);

    std::cout << "Environment activated." << std::endl;
    system("bash");
}

int main() {
    std::string storeDir = "/nodix/store";
    std::string envDir = "/nodix/env";
    std::string packageListPath = "/path/to/package_list.txt";
    
    std::ifstream packageFile(packageListPath);
    std::vector<std::string> packages;
    std::string line;
    
    while(std::getline(packageFile, line)) {
        packages.push_back(line);
    }
    
    createEnvironment(packages, storeDir, envDir);
    activateEnvironment(envDir);

    return 0;
}
