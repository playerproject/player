#Copyright (c) 2009, Toby Collett
#All rights reserved.
#
#Redistribution and use in source and binary forms, with or without 
#modification, are permitted provided that the following conditions are met:
#
#    * Redistributions of source code must retain the above copyright notice, 
#      this list of conditions and the following disclaimer.
#    * Redistributions in binary form must reproduce the above copyright notice,
#      this list of conditions and the following disclaimer in the documentation
#      and/or other materials provided with the distribution.
#    * Neither the name of the Player Project nor the names of its contributors 
#      may be used to endorse or promote products derived from this software 
#      without specific prior written permission.
#
#THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
#ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED 
#WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE 
#DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
#ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES 
#(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; 
#LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON 
#ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
#(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS 
#SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.


"""
Manual integration test for passthrough. Requires player server and blackboard driver, and python client.

The test sets up two blackboards, and connects to them via a passthrough driver with multiple clients.

First a direct connection is established and some bootstrap data is placed into both blackboards.

Then three connections are established via the passthrough.
The first two two the first blackboard and the third to the second bloackboard.

This allows for testing of independant connections that should all recieve different data

To run tests simply run the .cfg with player and then run this script.

"""

import playerc, sys, time, pprint

harness_client = playerc.playerc_client(None,"localhost",6665)
harness_client.connect()
harness_client.set_replace_rule(-1, -1, playerc.PLAYER_MSGTYPE_DATA, -1, 0)

def read_if_waiting(c):
    if c.peek(1):
        c.read()

print "Bootstrapping first BB"
harness_bb1 = playerc.playerc_blackboard(harness_client,0)
harness_bb1.subscribe(playerc.PLAYER_OPEN_MODE)
harness_bb1.SetInt("Client1","BB1",11)
harness_bb1.SetInt("Client2","BB1",12)
harness_bb1.SetInt("Shared","BB1",13)

harness_bb1.SubscribeToKey("Client1","BB1")
harness_bb1.SubscribeToKey("Client2","BB1")
harness_bb1.SubscribeToKey("Shared","BB1")

print "Bootstrapping second BB"
harness_bb2 = playerc.playerc_blackboard(harness_client,1)
harness_bb2.subscribe(playerc.PLAYER_OPEN_MODE)
harness_bb2.SetInt("Client1","BB2",21)
harness_bb2.SetInt("Client2","BB2",22)
harness_bb2.SetInt("Shared","BB2",23)

harness_bb2.SubscribeToKey("Client1","BB2")
harness_bb2.SubscribeToKey("Client2","BB2")
harness_bb2.SubscribeToKey("Shared","BB2")


print "Setting up test clients"
client1 = playerc.playerc_client(None,"localhost",6665)
client1.connect()
client1.set_replace_rule(-1, -1, playerc.PLAYER_MSGTYPE_DATA, -1, 0)

client2 = playerc.playerc_client(None,"localhost",6665)
client2.connect()
client2.set_replace_rule(-1, -1, playerc.PLAYER_MSGTYPE_DATA, -1, 0)

bb1_1 = playerc.playerc_blackboard(client1,10)
bb1_1.subscribe(playerc.PLAYER_OPEN_MODE)
bb2_1 = playerc.playerc_blackboard(client1,11)
bb2_1.subscribe(playerc.PLAYER_OPEN_MODE)

bb1_2 = playerc.playerc_blackboard(client2,10)
bb1_2.subscribe(playerc.PLAYER_OPEN_MODE)

print "Subscribing to keys"
bb1_1.SubscribeToKey("Client1","BB1")
bb1_1.SubscribeToKey("Shared","BB1")

bb2_1.SubscribeToKey("Client1","BB2")
bb2_1.SubscribeToKey("Shared","BB2")

bb1_2.SubscribeToKey("Client2","BB1")
bb1_2.SubscribeToKey("Shared","BB1")

print "comparing initial data"
d1_1=bb1_1.GetDict()
d2_1=bb2_1.GetDict()
d1_2=bb1_2.GetDict()

if len(d1_1["BB1"].keys()) != 2:
    print len(d1_1.keys())
    print d1_1
    raise Exception("Wrong number of entries")
if d1_1["BB1"]["Client1"]["data"] != 11:
    raise Exception("Bad harness data")
if d1_1["BB1"]["Shared"]["data"] != 13:
    raise Exception("Bad harness data")

if len(d2_1["BB2"].keys()) != 2:
    raise Exception("Wrong number of entries")
if d2_1["BB2"]["Client1"]["data"] != 21:
    raise Exception("Bad harness data")
if d2_1["BB2"]["Shared"]["data"] != 23:
    raise Exception("Bad harness data")

if len(d1_2["BB1"].keys()) != 2:
    raise Exception("Wrong number of entries")
if d1_2["BB1"]["Client2"]["data"] != 12:
    raise Exception("Bad harness data")
if d1_2["BB1"]["Shared"]["data"] != 13:
    raise Exception("Bad harness data")

print "Updating data"
harness_bb1.SetInt("Client1","BB1",111)
harness_bb1.SetInt("Client2","BB1",112)
harness_bb1.SetInt("Shared","BB1",113)
harness_bb2.SetInt("Shared","BB2",223)

print "Read some messages"

for x in range(1,30):
    read_if_waiting(client1)
    read_if_waiting(client2)
    read_if_waiting(harness_client)

print "Check results"
d1_1=bb1_1.GetDict()
d2_1=bb2_1.GetDict()
d1_2=bb1_2.GetDict()
dh_1=harness_bb1.GetDict()
dh_2=harness_bb2.GetDict()

if len(d1_1["BB1"].keys()) != 2:
    raise Exception("Wrong number of entries")
if d1_1["BB1"]["Client1"]["data"] != 111:
    raise Exception("Bad data")
if d1_1["BB1"]["Shared"]["data"] != 113:
    pprint.pprint({"dh_1":dh_1,"d1_1":d1_1, "d1_2":d1_2})
    raise Exception("Bad data %d != %d" % (d1_1["BB1"]["Shared"]["data"], 113))

if len(d2_1["BB2"].keys()) != 2:
    raise Exception("Wrong number of entries")
if d2_1["BB2"]["Client1"]["data"] != 21:
    raise Exception("Bad data")
if d2_1["BB2"]["Shared"]["data"] != 223:
    raise Exception("Bad data")

if len(d1_2["BB1"].keys()) != 2:
    raise Exception("Wrong number of entries")
if d1_2["BB1"]["Client2"]["data"] != 112:
    pprint.pprint({"dh_1":dh_1,"d1_1":d1_1, "d1_2":d1_2})
    pprint.pprint(harness_bb1.GetEntry("Client2","BB1"))
    raise Exception("Bad data %d != %d" % (d1_2["BB1"]["Client2"]["data"], 112))
if d1_2["BB1"]["Shared"]["data"] != 113:
    raise Exception("Bad data")

client1.disconnect()
client2.disconnect()
harness_client.disconnect()
