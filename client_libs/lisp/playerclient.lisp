;
; my first cut at a Common Lisp client for Player. 
;

(load "constants.lisp")
(load "messages.lisp")

; connects, prints banner, returns client object
(defun player-connect (&OPTIONAL (port *PLAYER-PORTNUM*))
  (let ((sock (socket-connect port))
        (banner (make-string *PLAYER-BANNERLEN*)))
    (read-char-sequence banner sock)
    (setf (stream-element-type sock) '(unsigned-byte 8))
    (print banner)
    (let ((client (make-playerclient :stream sock :subscriptions 0)))
      (player-datamode-reqrep client)
      client)))

; closes conn
(defun player-disconnect (client) (close (playerclient-stream sock)))

; write a message header to the server
(defun player-write-header (sock type size device &OPTIONAL index)
  (write-integer *PLAYER-STX* sock uint16 :big)
  (write-integer type sock uint16 :big)
  (write-integer device sock uint16 :big)
  (write-integer index sock uint16 :big)
  (write-integer 0 sock uint32 :big)
  (write-integer 0 sock uint32 :big)
  (write-integer 0 sock uint32 :big)
  (write-integer 0 sock uint32 :big)
  (write-integer 0 sock uint32 :big)
  (write-integer size sock uint32 :big)
  T)

; helpers
(defun player-write-cmd-header (sock size device &OPTIONAL index)
  (player-write-header sock *PLAYER-MSGTYPE-CMD* size device index))
(defun player-write-req-header (sock size device &OPTIONAL index)
  (player-write-header sock *PLAYER-MSGTYPE-REQ* size device index))

; read and return a header
(defun player-read-header (sock)
  (make-player-msgheader
    :stx (read-integer sock uint16 :big)
    :type (read-integer sock uint16 :big)
    :device (read-integer sock uint16 :big)
    :index (read-integer sock uint16 :big)
    :time_sec (read-integer sock uint32 :big)
    :time_usec (read-integer sock uint32 :big)
    :timestamp_sec (read-integer sock uint32 :big)
    :timestamp_usec (read-integer sock uint32 :big)
    :reserved (read-integer sock uint32 :big)
    :size (read-integer sock uint32 :big)))

(defun _player-read-and-match-response (sock device index)
  (let ((hdr (player-read-header sock)))
    (cond
      ((or (not (eq (player-msgheader-type hdr) *PLAYER-MSGTYPE-RESP*))
           (not (eq (player-msgheader-device hdr) device))
           (not (eq (player-msgheader-index hdr) index)))
        (read-byte-sequence 
          (make-sequence 'vector (player-msgheader-size hdr)) sock)
        (_player-read-and-match-response sock device index))
      (T hdr))))

; make any old request: either consume the response, or return the header
(defun player-request (client payload size device index &OPTIONAL (consume T))
  (let ((sock (playerclient-stream client)))
    ; write the header and the request payload
    (player-write-req-header sock size device index)
    (player-write-payload sock payload)
    (let ((resphdr (_player-read-and-match-response sock device index)))
      (cond
        (consume 
          (read-byte-sequence 
            (make-sequence 'vector (player-msgheader-size resphdr)) sock)
         T)
        (T resphdr)))))

; request access to a device.  returns the granted access.
(defun player-request-device-access (client access device &OPTIONAL (index 0))
  (player-request client
    (player-make-devreq-payload access device index)
    *PLAYER-PLAYER-DEV-REQ-SIZE* *PLAYER-PLAYER-CODE* 0 NIL)
  (let ((granted-access
              (player-devresp-access 
                (player-make-devresp 
                  (player-read-payload (playerclient-stream client) 
                    (player-devresp-format))))))
    ; update subscription counter
    (cond
      ((or (= granted-access *PLAYER-READ-MODE*)
           (= granted-access *PLAYER-ALL-MODE*))
       (setf (playerclient-subscriptions client) 
             (+ (playerclient-subscriptions client) 1))))
  granted-access))

(defun player-datamode-reqrep (client)
  (player-request client
    (player-make-datamode-payload *PLAYER-PLAYER-DATAMODE-REQREP*)
    *PLAYER-PLAYER-DATAMODE-REQ-SIZE* *PLAYER-PLAYER-CODE* 0 T))

(defun player-datareq (client)
  (player-request client
    (player-make-datareq-payload)
    *PLAYER-PLAYER-DATA-REQ-SIZE* *PLAYER-PLAYER-CODE* 0 T))

; recursive func to write a generic payload
(defun player-write-payload (sock payload)
  (cond 
    ((not (car payload)) T)
    (T 
      (write-integer (car payload) sock (cadr payload) :big)
      (player-write-payload sock (cddr payload)))))

; same deal, but read payload
;  typelist should be '(type.....type)
(defun player-read-payload (sock typelist)
  (cond
    ((not (car typelist)) '())
    (t 
      (append (list (read-integer sock (car typelist) :big))
                (player-read-payload sock (cdr typelist))))))

; generic read func to get all current sensor data and put it in the
; client object
(defun player-read (client &OPTIONAL (subs (playerclient-subscriptions client)))
  (print subs)
  (cond
    ((> subs 0)
      (cond
        ((= subs (playerclient-subscriptions client)) 
          (player-datareq client)))
      (let* ((sock (playerclient-stream client))
             (hdr (player-read-header sock))
             (device (player-msgheader-device hdr)))
        (cond
          ((= device *PLAYER-SONAR-CODE*)
            (print "SONAR")
            (setf (playerclient-sonar client)
              (player-read-payload sock (player-sonar-data-format))))
          ((= device *PLAYER-LASER-CODE*)
            (print "LASER")
            (setf (playerclient-laser client)
              (player-read-payload sock (player-laser-data-format))))
          (T (print "unknwown device") (print device))))
      (player-read client (- subs 1)))
   (t t)))


(defun player-read-sonar-data (client)
  (let ((sock (playerclient-stream client)))
    (print (player-read-header sock))
    (player-read-payload sock (player-sonar-data-format))))

(setq client (player-connect))
;(player-request-device-access client *PLAYER-READ-MODE* *PLAYER-SONAR-CODE*)

