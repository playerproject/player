; helpers and structs for message formats

; the kind of object that we'll pass about for client control
(defstruct playerclient
  stream subscriptions
  misc
  gripper
  position
  sonar
  laser
  vision
  ptz
  audio
  laserbeacon
  broadcast
  speech
  gps
  occupancy
  truth
  bps)

; a player message header
(defstruct player-msgheader
  stx
  type
  device
  index
  time_sec
  time_usec
  timestamp_sec
  timestamp_usec
  reserved
  size)

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; these are for building payloads that we send out
;
(defun player-make-player-devreq (access device &OPTIONAL (index 0))
  (list *PLAYER-PLAYER-DEV-REQ*  uint16
            device uint16
            index uint16
            access uint8))
(setq *PLAYER-PLAYER-DEV-REQ-SIZE* 7)

(defun player-make-player-datamode (mode) 
  (list *PLAYER-PLAYER-DATAMODE-REQ* uint16 
         mode uint8))
(setq *PLAYER-PLAYER-DATAMODE-REQ-SIZE* 3)

(defun player-make-player-datareq () 
  (list *PLAYER-PLAYER-DATA-REQ* uint16))
(setq *PLAYER-PLAYER-DATA-REQ-SIZE* 2)

(defun player-make-position-command (tv rv)
  (list (cond ((>= tv 0) tv) (T (+ *SHORTMAX* tv))) uint16
        (cond ((>= rv 0) rv) (T (+ *SHORTMAX* rv))) uint16))
(setq *PLAYER-POSITION-COMMAND-SIZE* 4)
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; 
; these are for payloads that we read in
;
(defstruct player-devresp
  device
  index
  access)
(defun player-devresp-format () (list uint16 uint16 uint16 uint8))
(defun player-make-player-devresp (args)
  (make-player-devresp :device (cadr args) 
                       :index (caddr args) 
                       :access (cadddr args)))


(defun player-misc-data-format ()
  (list uint8 uint8 uint8 uint8 uint8))

(defun player-gripper-data-format ()
  (list uint8 uint8))

(defun player-position-data-format ()
  (list int32 int32
        uint16
        int16 int16
        uint16
        uint8))

(defun player-sonar-data-format (&OPTIONAL (count 0)) 
  (cond 
    ((= count 16) '())
    (T (append (list uint16) (player-sonar-data-format (+ count 1))))))

(defun player-laser-data-format (&OPTIONAL (count 0)) 
  (cond 
    ((= count 401) '())
    ((= count 0) 
      (append (list int16 int16 uint16 uint16 uint16)
                 (player-laser-data-format (+ count 1))))
    (T (append (list uint16) (player-laser-data-format (+ count 1))))))

;(defun player-vision-data-format ())

(defun player-ptz-data-format ()
  (list int16 int16 uint16))

;(defun player-audio-data-format ())

;(defun player-laserbeacon-data-format ())

;(defun player-broadcast-data-format ())

;(defun player-speech-data-format ())

(defun player-gps-data-format ()
  (list int32 int32 int32))

;(defun player-bps-data-format ())
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


