#ifndef PLAYERERROR_H
#define PLAYERERROR_H

#include <string>
#include <iostream>

namespace PlayerCc
{
/** @addtogroup player_clientlib_cplusplus libplayerc++ */
/** @{ */

/** @addtogroup player_clientlib_cplusplus_core Core functionality */
/** @{ */

/**
  Playerc++ exception class
 */
class PlayerError
{
  protected:

    /** a string describing the error
     */
    std::string mStr;
    /** a string describing the location of the error in the source
     */
    std::string mFun;

  public:
    /**
     */
    std::string GetErrorStr() const { return(mStr); };
    /**
     */
    std::string GetErrorFun() const { return(mFun); };
    /** default constructor
     */
    PlayerError(const std::string aFun="", const std::string aStr="");
    /**
     */
    ~PlayerError();
};
/** }@ */

/** }@ */

}

// this needs to stay out of the namespace, doesn't it?
std::ostream& operator << (std::ostream& os, const PlayerCc::PlayerError& e);

#endif
