;
; various utilities that are nice to have
;

; get the nth elt of ls.  (does CL have something for this?)
(defun lindex (n ls)
  (cond
    ((or (> n (length ls)) (< n 0)) NIL)
    ((zerop n) (car ls))
    (T (lindex (- n 1) (cdr ls)))))

