/*
 * $Id$
 *
 * a test for the C++ FiducialFinder proxy
 */

#include "playerclient.h"
#include "test.h"
#include <unistd.h>

int
test_fiducial(PlayerClient* client, int index)
{
  unsigned char access;
  FiducialProxy fp(client,index,'c');

  printf("device [fiducialfinder] index [%d]\n", index);

  TEST("subscribing (read)");
  if((fp.ChangeAccess(PLAYER_READ_MODE,&access) < 0) ||
     (access != PLAYER_READ_MODE))
  {
    FAIL();
    printf("DRIVER: %s\n", fp.driver_name);
    return -1;
  }
  PASS();
  printf("DRIVER: %s\n", fp.driver_name);

  // wait for P2OS to start up
  for(int i=0; i < 10; i++)
    client->Read();

  fp.Print();

  TEST("getting geometry" );
  if( fp.PrintGeometry() < 0 )
    {
      FAIL();
      return -1;
    }
  else
    PASS();

  TEST("getting field of view (FOV)" );
  puts( "" );
  if( fp.PrintFOV() < 0 )
    {
      FAIL();
      //return -1;
    }
  else PASS();

  double original_min_range  = fp.min_range;
  double original_max_range  = fp.max_range;
  double original_view_angle = fp.view_angle;
  
  double goal_min_range = 1.0;
  double goal_max_range = 10.0;
  double goal_view_angle = M_PI/2.0;
  
  TEST("setting field of view" );
    printf( "(%.2f, %.2f, %.2f) ", 
	    goal_min_range, goal_max_range, goal_view_angle );

  if( fp.SetFOV( goal_min_range, goal_max_range, goal_view_angle) < 0 )
    {
      FAIL();
      //return -1;
    }
  else
    {
      if( !(fp.min_range == goal_min_range && 
	    fp.max_range == goal_max_range && 
	    fp.view_angle == goal_view_angle ))
	{
	  puts( "\nwarning: resulting FOV differs from requested values" );
	  
	  if( fp.min_range != goal_min_range )
	    printf( "FOV min range %.2f doesn't match requested value %.2f\n",
		    fp.min_range, goal_min_range  );
	  
	  if( fp.max_range != goal_max_range )
	    printf( "FOV max range %.2f doesn't match requested value %.2f\n",
		    fp.max_range, goal_max_range );
	  
	  if( fp.view_angle != goal_view_angle )
	    printf( "FOV min range %.2f doesn't match requested value %.2f\n",
		    fp.view_angle, goal_view_angle );
	}
      PASS();
    }

    // wait for a few cycles so we can see the change
    for(int i=0; i < 10; i++)
      client->Read();
    
    TEST("resetting original field of view" );
    if( fp.SetFOV( original_min_range,original_max_range,original_view_angle) 
	< 0 )
      {
	FAIL();
	//return -1;
      }
    else
      {
	if( !(fp.min_range == original_min_range && 
	      fp.max_range == original_max_range && 
	      fp.view_angle == original_view_angle ))
	  {
	    puts( "\nwarning: resulting FOV differs from original values" );
	    
	    if( fp.min_range != original_min_range )
	      printf( "FOV min range %.2f doesn't match original value %.2f\n",
		      fp.min_range, original_min_range  );
	    
	    if( fp.max_range != original_max_range )
	      printf( "FOV max range %.2f doesn't match original value %.2f\n",
		      fp.max_range, original_max_range );
	    
	    if( fp.view_angle != original_view_angle )
	      printf( "FOV min range %.2f doesn't match original value %.2f\n",
		      fp.view_angle, original_view_angle );
	  }
	PASS();
      }
    
    // wait for a few cycles so we can see the change
    for(int i=0; i < 10; i++)
      client->Read();
    
    // attempt to send a message
    TEST("broadcasting a message");
    
    player_fiducial_msg_t msg;
    memset( &msg, 0, sizeof(msg) );
    msg.target_id = -1; // broadcast
    snprintf( (char*)&msg.bytes, PLAYER_FIDUCIAL_MAX_MSG_LEN, 
	      "broadcast message" );
    msg.len = (uint8_t)strlen((char*)&msg.bytes);
    
    if( fp.SendMessage(&msg,true) < 0 )
      {
	FAIL();
	puts( "Messaging not supported" );
      }
    else
      PASS();

    // send a message to each detected fiducial in turn
    for( int i=0; i<fp.count; i++ )
      {
	// wait for a few cycles so we can see the messages happen
	for(int j=0; j < 3; j++)
	  client->Read();
	
	msg.target_id = fp.beacons[i].id;
	snprintf( (char*)&msg.bytes, PLAYER_FIDUCIAL_MAX_MSG_LEN, 
		  "hello %d", msg.target_id );
	msg.len = (uint8_t)strlen((char*)&msg.bytes);
	
	// attempt to send a message
	TEST("sending addressed message");

	printf( "\"%s\" to %d ...", msg.bytes, msg.target_id ); 
	
	if( fp.SendMessage(&msg,true) < 0 )
	  {
	    FAIL();
	    puts( "Fail. Messaging probably not supported" );
	    break;
	  }
	else
	  PASS();
      }
    
        // attempt to read a message
    TEST("reading a message");
    
    player_fiducial_msg_t recv;
    while( fp.RecvMessage(&recv, true) == 0 )
      {
	printf( "Message received: " );

	for( int i=0; i<recv.len; i++ )
	  putchar( recv.bytes[i] );

	puts( "" );
      }

    puts( "No message available." );

    PASS();

    TEST("unsubscribing");
    if((fp.ChangeAccess(PLAYER_CLOSE_MODE,&access) < 0) ||
       (access != PLAYER_CLOSE_MODE))
      {
	FAIL();
	return -1;
      }
    
    PASS();
    
    return(0);
}

