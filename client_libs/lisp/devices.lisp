;
; this file contains all the device-specific stuff
;

; generic read func to get all current sensor data and put it in the
; client object
(defun player-read (client &OPTIONAL (subs (playerclient-subscriptions client)))
  (cond
    ((car subs)
      (cond
        ((equal subs (playerclient-subscriptions client)) 
          (player-datareq client)))
      (let* ((sock (playerclient-stream client))
             (hdr (player-read-header sock))
             (device (player-msgheader-device hdr)))
        (cond
          ((= device *PLAYER-MISC-CODE*)
            (setf (playerclient-misc client)
              (player-read-payload sock (player-misc-data-format))))
          ((= device *PLAYER-GRIPPER-CODE*)
            (setf (playerclient-gripper client)
              (player-read-payload sock (player-gripper-data-format))))
          ((= device *PLAYER-POSITION-CODE*)
            (setf (playerclient-position client)
              (player-read-payload sock (player-position-data-format))))
          ((= device *PLAYER-SONAR-CODE*)
            (setf (playerclient-sonar client)
              (player-read-payload sock (player-sonar-data-format))))
          ((= device *PLAYER-LASER-CODE*)
            (setf (playerclient-laser client)
              (player-read-payload sock (player-laser-data-format))))
          ((= device *PLAYER-VISION-CODE*)
            (print "Vision device not yet supported"))
          ((= device *PLAYER-PTZ-CODE*)
            (setf (playerclient-ptz client)
              (player-read-payload sock (player-ptz-data-format))))
          ((= device *PLAYER-AUDIO-CODE*)
            (print "Audio device not yet supported"))
          ((= device *PLAYER-LASERBEACON-CODE*)
            (print "Laserbeacon device not yet supported"))
          ((= device *PLAYER-BROADCAST-CODE*)
            (print "Broadcast device not yet supported"))
          ((= device *PLAYER-SPEECH-CODE*)
            (print "Speech device not yet supported"))
          ((= device *PLAYER-GPS-CODE*)
            (setf (playerclient-gps client)
              (player-read-payload sock (player-gps-data-format))))
          ((= device *PLAYER-BPS-CODE*)
            (print "BPS device not yet supported"))
          (T (print "unknwown device") (print device))))
      (player-read client (cdr subs)))
   (t t)))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; config requests to the Player device
;
(defun player-datamode-reqrep (client)
  (player-request client
    (player-make-player-datamode *PLAYER-PLAYER-DATAMODE-REQREP*)
    *PLAYER-PLAYER-DATAMODE-REQ-SIZE* *PLAYER-PLAYER-CODE* 0 T))

(defun player-datareq (client)
  (player-request client
    (player-make-player-datareq)
    *PLAYER-PLAYER-DATA-REQ-SIZE* *PLAYER-PLAYER-CODE* 0 T))
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; some helpers for opening devices
;
(defun player-open-misc (client &OPTIONAL (index 0))
  (player-request-device-access client 
    *PLAYER-READ-MODE* *PLAYER-MISC-CODE* index))

(defun player-open-gripper (client &OPTIONAL (index 0))
  (player-request-device-access client 
    *PLAYER-ALL-MODE* *PLAYER-GRIPPER-CODE* index))

(defun player-open-position (client &OPTIONAL (index 0))
  (player-request-device-access client 
    *PLAYER-ALL-MODE* *PLAYER-POSITION-CODE* index))

(defun player-open-sonar (client &OPTIONAL (index 0))
  (player-request-device-access client 
    *PLAYER-READ-MODE* *PLAYER-SONAR-CODE* index))

(defun player-open-laser (client &OPTIONAL (index 0))
  (player-request-device-access client 
    *PLAYER-READ-MODE* *PLAYER-LASER-CODE* index))

(defun player-open-vision (client &OPTIONAL (index 0))
  (player-request-device-access client 
    *PLAYER-READ-MODE* *PLAYER-VISION-CODE* index))

(defun player-open-ptz (client &OPTIONAL (index 0))
  (player-request-device-access client 
    *PLAYER-ALL-MODE* *PLAYER-PTZ-CODE* index))

(defun player-open-audio (client &OPTIONAL (index 0))
  (player-request-device-access client 
    *PLAYER-ALL-MODE* *PLAYER-AUDIO-CODE* index))

(defun player-open-laserbeacon (client &OPTIONAL (index 0))
  (player-request-device-access client 
    *PLAYER-READ-MODE* *PLAYER-LASERBEACON-CODE* index))

(defun player-open-broadcast (client &OPTIONAL (index 0))
  (player-request-device-access client 
    *PLAYER-ALL-MODE* *PLAYER-BROADCAST-CODE* index))

(defun player-open-speech (client &OPTIONAL (index 0))
  (player-request-device-access client 
    *PLAYER-WRITE-MODE* *PLAYER-SPEECH-CODE* index))

(defun player-open-gps (client &OPTIONAL (index 0))
  (player-request-device-access client 
    *PLAYER-READ-MODE* *PLAYER-GPS-CODE* index))

(defun player-open-bps (client &OPTIONAL (index 0))
  (player-request-device-access client 
    *PLAYER-READ-MODE* *PLAYER-BPS-CODE* index))
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; some helpers for closing devices
;
(defun player-close-misc (client &OPTIONAL (index 0))
  (player-request-device-access client 
    *PLAYER-CLOSE-MODE* *PLAYER-MISC-CODE* index))

(defun player-close-gripper (client &OPTIONAL (index 0))
  (player-request-device-access client 
    *PLAYER-CLOSE-MODE* *PLAYER-GRIPPER-CODE* index))

(defun player-close-position (client &OPTIONAL (index 0))
  (player-request-device-access client 
    *PLAYER-CLOSE-MODE* *PLAYER-POSITION-CODE* index))

(defun player-close-sonar (client &OPTIONAL (index 0))
  (player-request-device-access client 
    *PLAYER-CLOSE-MODE* *PLAYER-SONAR-CODE* index))

(defun player-close-laser (client &OPTIONAL (index 0))
  (player-request-device-access client 
    *PLAYER-CLOSE-MODE* *PLAYER-LASER-CODE* index))

(defun player-close-vision (client &OPTIONAL (index 0))
  (player-request-device-access client 
    *PLAYER-CLOSE-MODE* *PLAYER-VISION-CODE* index))

(defun player-close-ptz (client &OPTIONAL (index 0))
  (player-request-device-access client 
    *PLAYER-CLOSE-MODE* *PLAYER-PTZ-CODE* index))

(defun player-close-audio (client &OPTIONAL (index 0))
  (player-request-device-access client 
    *PLAYER-CLOSE-MODE* *PLAYER-AUDIO-CODE* index))

(defun player-close-laserbeacon (client &OPTIONAL (index 0))
  (player-request-device-access client 
    *PLAYER-CLOSE-MODE* *PLAYER-LASERBEACON-CODE* index))

(defun player-close-broadcast (client &OPTIONAL (index 0))
  (player-request-device-access client 
    *PLAYER-CLOSE-MODE* *PLAYER-BROADCAST-CODE* index))

(defun player-close-speech (client &OPTIONAL (index 0))
  (player-request-device-access client 
    *PLAYER-CLOSE-MODE* *PLAYER-SPEECH-CODE* index))

(defun player-close-gps (client &OPTIONAL (index 0))
  (player-request-device-access client 
    *PLAYER-CLOSE-MODE* *PLAYER-GPS-CODE* index))

(defun player-close-bps (client &OPTIONAL (index 0))
  (player-request-device-access client 
    *PLAYER-CLOSE-MODE* *PLAYER-BPS-CODE* index))
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; helpers for sending commands
;
(defun player-setspeed (client tv rv &OPTIONAL (index 0))
  (player-write-command client
    (player-make-position-command tv rv) *PLAYER-POSITION-COMMAND-SIZE*
    *PLAYER-POSITION-CODE* index))

