#ifndef PLAYERERROR_H
#define PLAYERERROR_H

#include <string>
#include <iostream>

namespace PlayerCc
{

class PlayerError
{
  protected:
    std::string mStr;
    std::string mFun;

  public:
    std::string GetErrorStr() const { return(mStr); };
    std::string GetErrorFun() const { return(mFun); };

    PlayerError(const std::string aFun, const std::string aStr);
    ~PlayerError();
};

}

// this needs to stay out of the namespace, doesn't it?
std::ostream& operator << (std::ostream& os, const PlayerCc::PlayerError& e);

#endif
