#include <fstream>
#include <iostream>
#include <string>

using namespace std;

int main() {
    setlocale(LC_ALL, "RU");

    int nums[5] = {10, 20, 30, 40, 50};

    ofstream file("numbers.txt", ios_base::out);
    if (file.is_open()) {
        for (int i = 0; i < 5; i++) {
            file << nums[i] << " ";
        }
        file.close();
    }

    ifstream file1("numbers.txt");
    if (file1.is_open()) {
        char res[100];
        file1.getline(res, 100);
        cout << res << endl;
        file1.close();
    }

    return 0;
}
