#ifndef MOTEPROXY_H
#define MOTEPROXY_H

#include <clientproxy.h>
#include <playerclient.h>

#define MAX_RX_BUF_SIZE 1024

class MoteProxy : public ClientProxy
{
    public: MoteProxy(PlayerClient* pc, unsigned short index, 
		      unsigned char access ='c');

    public: int Sendto(int src, int dest, char* msg, int len);

    public: int TransmitRaw(char *msg, uint16_t len);

    public: int RecieveRaw(char* msg, uint16_t len, double *rssi);

    public: int Sendto(int to, char* msg, int len);

    public: int SetStrength(uint8_t str);

    public: int GetStrength(void);

    public: int RecieveFrom(int from, char* msg, int len);

    public: inline float GetRSSI(void);

    // interface that all proxies must provide
    protected: void FillData(player_msghdr_t hdr, const char* buffer);
    
    // interface that all proxies SHOULD provide
    protected: void Print();
    
    private: player_mote_data_t *rx_data;

    private: player_mote_data_t *rx_queue;

    private: player_mote_config_t m_config;

    private: char r_flag;

    private: unsigned char msg_q_index;
};

#endif
