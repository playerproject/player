;
; my first cut at a Common Lisp client for Player. 
;

(load "constants.lisp")

; connects, prints banner, returns socket
(defun player-connect (&OPTIONAL (port *PLAYER-PORTNUM*))
  (let ((sock (socket-connect port))
        (banner (make-string *PLAYER-BANNERLEN*)))
    (read-char-sequence banner sock)
    (setf (stream-element-type sock) '(unsigned-byte 8))
    (print banner)
    sock))

(defun player-disconnect (sock) (close sock))

; write a message header to the server
(defun player-write-header (sock type size device &OPTIONAL (index 0))
  (write-integer *PLAYER-STX* sock '(unsigned-byte 16) :big)
  (write-integer type sock uint16 :big)
  (write-integer device sock uint16 :big)
  (write-integer index sock uint16 :big)
  (write-integer 0 sock uint32 :big)
  (write-integer 0 sock uint32 :big)
  (write-integer 0 sock uint32 :big)
  (write-integer 0 sock uint32 :big)
  (write-integer 0 sock uint32 :big)
  (write-integer size sock uint32 :big))
(defun player-write-cmd-header (sock size device &OPTIONAL (index 0))
  (player-write-header sock *PLAYER-MSGTYPE-CMD* size device index))
(defun player-write-req-header (sock size device &OPTIONAL (index 0))
  (player-write-header sock *PLAYER-MSGTYPE-REQ* size device index))

(defun player-request-device-access (sock access device &OPTIONAL (index 0))
  (player-write-req-header sock 7 *PLAYER-PLAYER-CODE*)
  (write-integer *PLAYER-PLAYER-DEV-REQ* sock uint16 :big)
  (write-integer device sock uint16 :big)
  (write-integer index sock uint16 :big)
  (write-byte access sock))

(setq sock (player-connect))

