#include <libplayerc++/playerc++.h>
#include <iostream>
#include <unistd.h>

std::string  gHostname(PlayerCc::PLAYER_HOSTNAME);
uint         gPort(PlayerCc::PLAYER_PORTNUM);
uint         gIndex(0);
uint         gDebug(0);
uint         gFrequency(10); // Hz
uint         gDataMode(PLAYER_DATAMODE_PUSH_NEW);

void
print_usage(int argc, char** argv)
{
  using namespace std;
  cerr << "USAGE:  " << *argv << " [options]" << endl << endl;
  cerr << "Where [options] can be:" << endl;
  cerr << "  -h <hostname>  : hostname to connect to (default: "
       << PlayerCc::PLAYER_HOSTNAME << ")" << endl;
  cerr << "  -p <port>      : port where Player will listen (default: "
       << PlayerCc::PLAYER_PORTNUM << ")" << endl;
  cerr << "  -i <index>     : device index"
       << endl;
  cerr << "  -d <level>     : debug message level (0 = none -- 9 = all)"
       << endl;
  cerr << "  -u <rate>      : set server update rate to <rate> in Hz"
       << endl;
  cerr << "  -m <datamode>  : set server data delivery mode"
       << endl;
  cerr << "                      PLAYER_DATAMODE_PUSH_ALL = "
       << PLAYER_DATAMODE_PUSH_ALL << endl;
  cerr << "                      PLAYER_DATAMODE_PULL_ALL = "
       << PLAYER_DATAMODE_PULL_ALL << endl;
  cerr << "                      PLAYER_DATAMODE_PUSH_NEW = "
       << PLAYER_DATAMODE_PUSH_NEW << endl;
  cerr << "                      PLAYER_DATAMODE_PULL_NEW = "
       << PLAYER_DATAMODE_PULL_NEW << endl;
  cerr << "                      PLAYER_DATAMODE_ASYNC    = "
       << PLAYER_DATAMODE_ASYNC << endl;
}

int
parse_args(int argc, char** argv)
{
  const char* optflags = "h:p:i:d:u:m:";
  int ch;

  while(-1 != (ch = getopt(argc, argv, optflags)))
  {
    switch(ch)
    {
      /* case values must match long_options */
      case 'h':
          gHostname = optarg;
          break;
      case 'p':
          gPort = atoi(optarg);
          break;
      case 'i':
          gIndex = atoi(optarg);
          break;
      case 'd':
          gDebug = atoi(optarg);
          break;
      case 'u':
          gFrequency = atoi(optarg);
          break;
      case 'm':
          gDataMode = atoi(optarg);
          break;
      case '?':
      case ':':
      default:
        print_usage(argc, argv);
        return (-1);
    }
  }

  return (0);
}
