;
; simple sonar-based obstacle avoidance
;

(load "playerclient.lisp")

(setq client (player-connect "localhost"))
(player-open-sonar client)
(player-open-position client)
(player-set-motorpower client 1)
(setq mindist 400)

(let ((avoidcount 0))
  (loop
    (player-read client)
    (let ((sonar (playerclient-sonar client)))
      (cond
        ((= avoidcount 0)
          (cond
            ((or (< (lindex 2 sonar) mindist)
                 (< (lindex 3 sonar) mindist)
                 (< (lindex 4 sonar) mindist)
                 (< (lindex 5 sonar) mindist))
              (cond
                ((< (+ (lindex 0 sonar) (lindex 1 sonar))
                    (+ (lindex 6 sonar) (lindex 7 sonar)))
                  (player-setspeed client 0 -30))
                (T
                  (player-setspeed client 0 30)))
              (setq avoidcount 20))
            (T
              (player-setspeed client 150 0))))
        (T (setq avoidcount (- avoidcount 1)))))))

