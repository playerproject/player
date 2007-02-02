/*
 * $Id$
 *
 * a test for the C++ MComProxy
 */

#include "test.h"
#include <unistd.h>
#include <math.h>

int
test_mcom(PlayerClient* client, int index)
{
  unsigned char access;
  MComProxy mcom(client,index,'c');

  printf("device [mcom] index [%d]\n", index);

  TEST("subscribing (read)");
  if((mcom.ChangeAccess(PLAYER_READ_MODE,&access) < 0) ||
     (access != PLAYER_READ_MODE))
  {
    FAIL();
    printf("DRIVER: %s\n", mcom.driver_name);
    return -1;
  }
  PASS();
  printf("DRIVER: %s\n", mcom.driver_name);

  TEST("push");
  char data[MCOM_DATA_LEN];
  memset(data, 0, MCOM_DATA_LEN);
  strcpy(data, "what hath god wrought?");
  if(mcom.Push(1, "test", data) == 0)
    PASS();
  else
  {
    FAIL();
    return(-1);
  }


  TEST("read");
  if(mcom.Read(1, "test") == 0) 
  {
    printf("read test string from mcom: \"%s\"\n", mcom.LastData());
    PASS();
  }
  else
  {
    FAIL();
    return(-1);
  }

  TEST("pop");
  if(mcom.Pop(1, "test") == 0) {
      printf("popped test string from mcom: \"%s\"\n", mcom.LastData());
      PASS();
  } else {
      FAIL();
  }


  TEST("unsubscribing");
  if((mcom.ChangeAccess(PLAYER_CLOSE_MODE,&access) < 0) ||
     (access != PLAYER_CLOSE_MODE))
  {
    FAIL();
    return -1;
  }

  PASS();

  return(0);
}

