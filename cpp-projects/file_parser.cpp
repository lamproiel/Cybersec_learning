#include <fstream>
#include <iostream>
#include <string>

int main() {

    std::ifstream filepasw("/etc/passwd");
    if (filepasw.is_open()) {
        char line[256];
        for (int i = 0; i < 10; i++) {
            filepasw.getline(line, 256);
            std::cout << "User [" << i << "]: " << line << std::endl;
        }
        filepasw.close();
    }

    return 0;
}
