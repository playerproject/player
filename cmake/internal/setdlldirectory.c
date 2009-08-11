#define _WIN32_WINNT 0x0502
#include <windows.h>

#ifdef __CLASSIC_C__
int main(){
  int ac;
  char*av[];
#else
int main(int ac, char*av[]){
#endif
  SetDllDirectory("blag");
  return 0;
}

