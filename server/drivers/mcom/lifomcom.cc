/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) 2000  Brian Gerkey   &  Kasper Stoy
 *                      gerkey@usc.edu    kaspers@robotics.usc.edu
 *
 *  LifoMCom device by Matthew Brewer <mbrewer@andrew.cmu.edu> and 
 *  Reed Hedges <reed@zerohour.net> at the Laboratory for Perceptual 
 *  Robotics, Dept. of Computer Science, University of Massachusetts,
 *  Amherst.
 *
 * This program is free software; you can redistribute it and/or modify
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

#include <string.h> 
#include <sys/types.h>
#include <netinet/in.h>

#include "playercommon.h"

#include "lifomcom.h"

#ifdef MCOM_PLUGIN
// todo dll stuff here
#endif


/*
LifoMCom(int argc, char** argv):CDevice(1,1,20,20),Data(){
}
*/

LifoMCom::LifoMCom(char* interface, ConfigFile* cf, int section) :
  CDevice(sizeof(player_mcom_data_t), 0, 20, 20)
{
}


CDevice* LifoMCom_Init(char* interface, ConfigFile* cf, int section)
{
  if(strcmp(interface, PLAYER_MCOM_STRING))
  {
    PLAYER_ERROR2("the mcom device driver does not support interface \"%s\" (use \"%s\")\n",
                  interface, PLAYER_MCOM_STRING);
    return(NULL);
  }
  else
    return((CDevice*)(new LifoMCom(interface, cf, section)));
}


void LifoMCom_Register(DriverTable* t) {
    t->AddDriver("lifomcom", PLAYER_ALL_MODE, LifoMCom_Init);
}

// called by player with config requests
int LifoMCom::PutConfig(player_device_id_t* device, void* client, void* data, size_t len) {

    assert(len == sizeof(player_mcom_config_t));
    player_mcom_config_t* cfg = (player_mcom_config_t*)data;
    cfg->type = ntohs(cfg->type);

    // arguments to PutReply are: (void* client, ushort replytype, struct timeval* ts, void* data, size_t datalen)
    switch(cfg->command) {
        case PLAYER_MCOM_PUSH_REQ:
            Data.Push(cfg->data, cfg->type, cfg->channel);
            PutReply(client, PLAYER_MSGTYPE_RESP_ACK, NULL, NULL, 0);
            return 0;
        case PLAYER_MCOM_POP_REQ:
            player_mcom_return_t ret;
            ret.data = Data.Pop(cfg->type, cfg->channel);
            if(ret.data.full) {
                ret.type = htons(cfg->type);
                strcpy(ret.channel, cfg->channel);
                PutReply(client, PLAYER_MSGTYPE_RESP_ACK, NULL, &ret ,sizeof(ret));
                return 0;
            } else {
                PutReply(client, PLAYER_MSGTYPE_RESP_NACK, NULL, NULL, 0);
                return 0;
            }
            break;
        case PLAYER_MCOM_READ_REQ:
            ret.data = Data.Read(cfg->type, cfg->channel);
            if(ret.data.full) {
                ret.type = htons(cfg->type);
                strcpy(ret.channel, cfg->channel);
                Unlock();
                PutReply(client, PLAYER_MSGTYPE_RESP_ACK, NULL, &ret ,sizeof(ret));
                return 0;
            } else {
                Unlock();
                PutReply(client, PLAYER_MSGTYPE_RESP_NACK, NULL, NULL, 0);
                return 0;
            }
            break;
        case PLAYER_MCOM_CLEAR_REQ:
            Data.Clear(cfg->type, cfg->channel);
            PutReply(client, PLAYER_MSGTYPE_RESP_ACK, NULL, NULL ,0);
            return 0;
        default:
            printf("Error: message %d to MCOM Device not recognized\n", cfg->command);
            PutReply(client, PLAYER_MSGTYPE_RESP_NACK, NULL, NULL,0);	
            return 0;
    }
    return 0;
}




LifoMCom::Buffer::Buffer() {
    top = 0;
    for(int x = 0 ; x < MCOM_N_BUFS; x++) {
        dat[x].full=0;
        strcpy(dat[x].data,"(EMPTY)");
    }
}

LifoMCom::Buffer::~Buffer(){
}

void LifoMCom::Buffer::Push(player_mcom_data_t newdat) {
    top++;
    if(top>=MCOM_N_BUFS)
        top-=MCOM_N_BUFS;
    dat[top]=newdat;
    dat[top].full=1;
}

player_mcom_data_t LifoMCom::Buffer::Pop(){
    player_mcom_data_t ret;
    ret=dat[top];
    dat[top].full=0;
    strcpy(dat[top].data,"(EMPTY)");
    top-=1;
    if(top<0)
        top+=MCOM_N_BUFS;
    return ret;
}

player_mcom_data_t LifoMCom::Buffer::Read(){
    player_mcom_data_t ret;
    ret=dat[top];
    return ret;
}

void LifoMCom::Buffer::Clear(){
    int s;
    for(s=0;s<MCOM_N_BUFS;s++)
          dat[s].full=0;
}

void LifoMCom::Buffer::print(){
    int x;
    printf("mcom buffer dump of type %i channel %s buffer\n",type,channel);
    for(x=0;x<MCOM_N_BUFS;x++)
        printf("%s :: %i\n",dat[x].data,dat[x].full);
}


LifoMCom::LinkList::LinkList() : top(NULL){
}

LifoMCom::LinkList::~LinkList(){
    Link *p=top;
    Link *next=p;
    while(p!=NULL){
        next=p->next;
        delete p;
        p=next;
    }
}

void LifoMCom::LinkList::Push(player_mcom_data_t d,int type, char channel[MCOM_CHANNEL_LEN]){
    Link * p=top;
    if(p==NULL){
        top=new Link;
        top->next=NULL;
        top->buf.type=type;
        strcpy(top->buf.channel,channel);
        p=top;
    } else {
        while(p->next!=NULL && (p->buf.type!=type || strcmp(p->buf.channel,channel))) {
            p=p->next;
        }
         
        if(p->buf.type!=type || strcmp(p->buf.channel,channel)){
            p->next = new Link;
            p=p->next;
            p->next=NULL;
            p->buf.type=type;
            strcpy(p->buf.channel,channel);
        }
    }
    p->buf.Push(d);
}

player_mcom_data_t LifoMCom::LinkList::Pop(int type, char channel[MCOM_CHANNEL_LEN]){
    Link *p=top;
    Link *last;
    player_mcom_data_t ret;
    strcpy(ret.data,"(EMPTY)");
    ret.full=0;
    if(p==NULL){
        return ret;
    }else{
        while(p->next!=NULL && (p->buf.type!=type || strcmp(p->buf.channel,channel))){
            last=p;
            p=p->next;
        }
        if(p->buf.type!=type || strcmp(p->buf.channel,channel)){
            return ret;
        }else{
            ret = p->buf.Pop();
/*if(ret.full==0){
printf("deleting buffer (it's empty)\n");
if(p==top)
delete top;
else
if(p==top->next){
top->next=top->next->next;
delete p;
}else{
last->next=p->next;
delete p;
}
}*/
        }
    }
    return ret;
}

player_mcom_data_t LifoMCom::LinkList::Read(int type,char channel[MCOM_CHANNEL_LEN]){
    Link * p=top;
    player_mcom_data_t ret;
    ret.full=0;
    strcpy(ret.data, "(EMPTY)");
    if(p==NULL)
        return ret;
    else{
        while(p->next!=NULL && (p->buf.type!=type || strcmp(p->buf.channel,channel))){
            p=p->next;
        }
        if(p->buf.type!=type || strcmp(p->buf.channel,channel))
            return ret;
        else{
            return p->buf.Read();
        }
    }
}

void LifoMCom::LinkList::Clear(int type, char channel[MCOM_CHANNEL_LEN]) {
    Link *p = top;
    Link *last = NULL;
    if(p == NULL)
        return;
    else{
        while(p->next != NULL && (p->buf.type != type || strcmp(p->buf.channel, channel))){
            last = p;
            p = p->next;
        }
        if(p->buf.type != type || strcmp(p->buf.channel, channel))
            return;
        else{
            last->next=p->next->next;
            p->buf.Clear();
            delete p;
        }
    }
}
