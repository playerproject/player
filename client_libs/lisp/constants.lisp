(setq *PLAYER-PORTNUM* 6665)
(setq *PLAYER-BANNERLEN* 32)

(setq *PLAYER-STX* #x5878)

(setq *PLAYER-MSGTYPE-DATA*  1)
(setq *PLAYER-MSGTYPE-CMD*   2)
(setq *PLAYER-MSGTYPE-REQ*   3)
(setq *PLAYER-MSGTYPE-RESP*  4)

(setq *PLAYER-PLAYER-CODE*         1)
(setq *PLAYER-MISC-CODE*           2)
(setq *PLAYER-GRIPPER-CODE*        3)
(setq *PLAYER-POSITION-CODE*       4)
(setq *PLAYER-SONAR-CODE*          5)
(setq *PLAYER-LASER-CODE*          6)
(setq *PLAYER-VISION-CODE*         7)
(setq *PLAYER-PTZ-CODE*            8)
(setq *PLAYER-AUDIO-CODE*          9)
(setq *PLAYER-LASERBEACON-CODE*   10)
(setq *PLAYER-BROADCAST-CODE*     11)
(setq *PLAYER-SPEECH-CODE*        12)
(setq *PLAYER-GPS-CODE*           13) 
(setq *PLAYER-OCCUPANCY-CODE*     14)
(setq *PLAYER-TRUTH-CODE*         15)
(setq *PLAYER-BPS-CODE*           16)

(setq *PLAYER-PLAYER-DEV-REQ*     1)
(setq *PLAYER-PLAYER-DATA-REQ*     2)
(setq *PLAYER-PLAYER-DATAMODE-REQ* 3)
(setq *PLAYER-PLAYER-DATAFREQ-REQ* 4)
(setq *PLAYER-PLAYER-AUTH-REQ*     5)

(setq *PLAYER-PLAYER-DATAMODE-CONTINUOUS* 0)
(setq *PLAYER-PLAYER-DATAMODE-REQREP* 1)

(setq *PLAYER-READ-MODE* #x72)
(setq *PLAYER-WRITE-MODE* #x77)
(setq *PLAYER-ALL-MODE* #x61)
(setq *PLAYER-CLOSE-MODE* #x63)
(setq *PLAYER-ERROR-MODE* #x65)

; shorter (and more familiar) names for binary types
(setq uint8 '(unsigned-byte 8))
(setq uint16 '(unsigned-byte 16))
(setq uint32 '(unsigned-byte 32))
(setq int8 '(signed-byte 8))
(setq int16 '(signed-byte 16))
(setq int32 '(signed-byte 32))

(setq *SHORTMAX* 65536)
(setq *HALF-SHORTMAX* (/ *SHORTMAX* 2))
(setq *HALF-INTMAX* 2147483648)

