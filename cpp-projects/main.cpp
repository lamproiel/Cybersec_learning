#include <iostream>

using namespace std;

int findmin(int *numbers, int leg) {
    int min = *numbers;
    for (int i = 0; i < leg; i++) {
        if (min > *(numbers + i))
            min = *(numbers + i);
    }
    return min;
}

int main() {
    setlocale(LC_ALL, "RU");

    int numbers[] = {15, 3, 42, -7, 8, 20};
    int min = findmin(numbers, 6);
    cout << min << endl;

    return 0;
}
