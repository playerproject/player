#include <libplayercore/playercore.h>

#include "UvcInterface.h"

class CameraUvc;

#ifndef CAMERAUVC_H_
#define CAMERAUVC_H_

class CameraUvc : public ThreadedDriver
{
	public:
		CameraUvc(ConfigFile* cf, int section);
    ~CameraUvc();
		int MainSetup();
		void MainQuit();

		int ProcessMessage(QueuePointer &resp_queue, player_msghdr *hdr, void *data);
	private:
		virtual void Main();

		UvcInterface *ui;

		player_camera_data_t data;	// Data to send to client (through the server)
};

#endif /*CAMERAUVC_H_*/
