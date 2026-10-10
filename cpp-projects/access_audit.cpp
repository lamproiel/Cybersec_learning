#include <iostream>

bool isuser(int userid);
int acc(int accounts[], int size);

int main() {

  int users[7] = {-1, 0, 999, 1001, 1002, 1003, 1004};
  int res = acc(users, 7);
  std::cout << "всего аномалий обнаружено: " << res << std::endl;

  return 0;
}

bool isuser(int userid) {
  if (userid == 0 || userid == 999 || userid < 0)
    return true;
  else
    return false;
}

int acc(int accounts[], int size) {
  int res = 0;
  for (int i = 0; i < size; i++) {
    if (isuser(accounts[i])) {
      std::cout << "обнаружен подозрительный аккаунт: UID " << accounts[i]
                << std::endl;
      res++;
    } else
      std::cout << "обычный пользователь: " << accounts[i] << std::endl;
  }
  return res;
}
