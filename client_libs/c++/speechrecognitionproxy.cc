/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) 2000-2003
 *     Brian Gerkey, Kasper Stoy, Richard Vaughan, & Andrew Howard
 *                      
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */
/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) 2003
 *     Brian Gerkey, Kasper Stoy, Richard Vaughan, & Andrew Howard
 *                      
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/*
 * $Id$
 *
 * client-side speech device 
 */

#include <playerclient.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>

SpeechRecognitionProxy::SpeechRecognitionProxy (PlayerClient *pc, unsigned short index, unsigned char access)
: ClientProxy( pc, PLAYER_SPEECH_RECOGNITION_CODE, index, access)
{
}

// Destructor
SpeechRecognitionProxy::~SpeechRecognitionProxy()
{
}

// interface that all proxies must provide
void SpeechRecognitionProxy::FillData (player_msghdr_t hdr, const char *buffer)
{
  int i,j;
  int startIndex = 0;

  player_speech_recognition_data_t *data = (player_speech_recognition_data_t*)buffer;

  if(hdr.size != sizeof(player_speech_recognition_data_t))
  {
    if(player_debug_level(-1) >= 1)
      fprintf(stderr,"WARNING: expected %d bytes of speech recognition data, but "
              "received %d. Unexpected results may ensue.\n",
              sizeof(player_speech_recognition_data_t),hdr.size);
  }

  this->wordCount = 0;

  printf ("Text[%s] Length[%d]\n",data->text, strlen(data->text));

  // Split the text string into words
  for (i=0; i<strlen(data->text); i++)
  {
    // If space, then reached a word boundary. So create a new word
    if (data->text[i] == ' ' && i > startIndex)
    {
      // Copy the word
      for (j=startIndex; j<i; j++)
        this->words[this->wordCount][j-startIndex] = data->text[j];

      // Add string termination character
      this->words[this->wordCount][i-startIndex] = '\0';

      //printf("Word[%s]\n",this->words[this->wordCount]);

      startIndex = i+1;
      this->wordCount++;
    }
  }
}

void SpeechRecognitionProxy::Clear()
{
  this->wordCount = 0;

  for (int i=0; i<20; i++)
  {
    bzero(this->words[i],30);
  }
}
