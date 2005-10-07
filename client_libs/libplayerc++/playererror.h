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

    /** error code returned by playerc */
    int mCode;

  public:
    /**
     */
    std::string GetErrorStr() const { return(mStr); };
    /**
     */
    std::string GetErrorFun() const { return(mFun); };
    /**
     */
    int GetErrorCode() const { return(mCode); };

    /** default constructor
     */
    PlayerError(const std::string aFun="",
                const std::string aStr="",
                const int aCode=-1);
    /**
     */
    ~PlayerError();
};
/** }@ */

/** }@ */

}

namespace std
{
std::ostream& operator << (std::ostream& os, const PlayerCc::PlayerError& e);
}

#endif
