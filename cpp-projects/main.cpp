#include <iostream>

using namespace std;

int main() {
    setlocale(LC_ALL, "RU");

    int a[3] = {1, 2, 3};
    int *ptr = a;
    cout << *(ptr + 2) << endl;
    cout << *(ptr + 3) << endl;

    return 0;
}
