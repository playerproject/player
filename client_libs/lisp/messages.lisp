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

(defun player-make-devreq-payload (access device &OPTIONAL (index 0))
  (list *PLAYER-PLAYER-DEV-REQ*  uint16
            device uint16
            index uint16
            access uint8))
(setq *PLAYER-PLAYER-DEV-REQ-SIZE* 7)

(defun player-make-datamode-payload (mode) 
  (list *PLAYER-PLAYER-DATAMODE-REQ* uint16 
         mode uint8))
(setq *PLAYER-PLAYER-DATAMODE-REQ-SIZE* 3)

(defun player-make-datareq-payload () 
  (list *PLAYER-PLAYER-DATA-REQ* uint16))
(setq *PLAYER-PLAYER-DATA-REQ-SIZE* 2)

(defstruct player-devresp
  device
  index
  access)
(defun player-devresp-format () (list uint16 uint16 uint16 uint8))
(defun player-make-devresp (arglist)
  (make-player-devresp :device (cadr arglist) 
                       :index (caddr arglist) 
                       :access (cadddr arglist)))


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

