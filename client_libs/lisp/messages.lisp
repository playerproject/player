; helpers and structs for message formats

; the kind of object that we'll pass about for client control
(defstruct playerclient
  stream subscriptions)

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

(defstruct player-devresp
  device
  index
  access)

(defun player-devresp-format () (list uint16 uint16 uint16 uint8))
(defun player-make-devresp (arglist)
  (make-player-devresp :device (cadr arglist) 
                       :index (caddr arglist) 
                       :access (cadddr arglist)))

(defun player-sonar-data-format () 
  (list uint16 uint16 uint16 uint16
        uint16 uint16 uint16 uint16
        uint16 uint16 uint16 uint16
        uint16 uint16 uint16 uint16))

