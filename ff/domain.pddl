(define (domain bots-domain)

	(:requirements :equality :typing :adl)

	(:types bot, waypoint, weapon, door, button, box)

    (:predicates
        (physcannon ?weapon - weapon)                    ; Arma de gravedad
        (sniper-weapon ?weapon - weapon)                 ; Arma francotirador

        (can-move ?from ?to - waypoint)
        (can-climb-two ?from ?to - waypoint)              ; Subirse a la caja u otro bot
        (can-climb-three ?from ?to - waypoint)            ; Hacer totem de 3
        (can-shoot ?button - button ?waypoint - waypoint) ; Disparar al boton

        (at ?bot ?waypoint)
		(box-at ?box ?waypoint)
		(button-at ?button ?waypoint)
        (weapon-at ?weapon ?waypoint)

        (has ?bot ?weapon)
		(carry ?bot ?box)
		(empty ?bot)

        (between ?door ?waypoint1 ?waypoint2)   ; Puertas que separan 2 waypoints
		(toggle ?button ?door1 ?door2)          ; Boton cambia estados de 2 puertas
    )

	;---------------------------------------------------------------------------
	; Bot se mueve entre waypoints adjacentes.
	;---------------------------------------------------------------------------
    (:action move
        :parameters
            (?bot - bot
             ?from ?to - waypoint)

        :precondition
            (and
                (at ?bot ?from)
                (can-move ?from ?to)
            )

        :effect
            (and
                (at ?bot ?to)
                (not (at ?bot ?from))
            )
    )

	;---------------------------------------------------------------------------
	; Bot agarra un arma que esta cerca de un waypoint.
	;---------------------------------------------------------------------------
    (:action take-weapon
        :parameters
            (?bot - bot
             ?weapon - weapon
             ?waypoint - waypoint)

        :precondition
            (and
                (at ?bot ?waypoint)
                (weapon-at ?weapon ?waypoint)
            )

        :effect
            (and
                (not (weapon-at ?weapon ?waypoint))
                (has ?bot ?weapon)
            )
    )

    ;---------------------------------------------------------------------------
    ; Bot agarra la caja con el arma physcannon.
    ;---------------------------------------------------------------------------
    (:action carry-box
        :parameters
            (?bot - bot
             ?weapon - weapon
             ?box - box
             ?waypoint - waypoint)

        :precondition
            (and
                (at ?bot ?waypoint)
                (box-at ?box ?waypoint)
                (has ?bot ?weapon)
				(physcannon ?weapon)
                (empty ?bot)
            )

        :effect
            (and
                (not (empty ?bot))
                (carry ?bot ?box)
                (not (box-at ?box ?waypoint))
            )
    )

    ;---------------------------------------------------------------------------
    ; Bot agarra la caja con el arma de lejos.
    ;---------------------------------------------------------------------------
    (:action carry-box-far
        :parameters
            (?bot - bot
             ?weapon - weapon
             ?box - box
             ?waypoint-bot ?waypoint-box - waypoint)

        :precondition
            (and
                (at ?bot ?waypoint-bot)
                (box-at ?box ?waypoint-box)
                (has ?bot ?weapon)
				(physcannon ?weapon)
                (empty ?bot)
				(can-move ?waypoint-bot ?waypoint-box)
            )

        :effect
            (and
                (not (empty ?bot))
                (carry ?bot ?box)
                (not (box-at ?box ?waypoint-box))
            )
    )

    ;---------------------------------------------------------------------------
    ; Bot tira la caja que estaba llevando.
    ;---------------------------------------------------------------------------
    (:action drop-box
        :parameters
            (?bot - bot
             ?box - box
             ?waypoint - waypoint)

        :precondition
            (and
                (at ?bot ?waypoint)
                (carry ?bot ?box)
            )

        :effect
            (and
                (box-at ?box ?waypoint)
                (empty ?bot)
                (not (carry ?bot ?box))
            )
    )

    ;---------------------------------------------------------------------------
    ; Bot sube la caja para pasar a otro waypoint.
    ;---------------------------------------------------------------------------
    (:action climb-box
        :parameters
            (?bot1 - bot
             ?box - box
             ?from ?to - waypoint)

        :precondition
            (and
                (at ?bot1 ?from)
                (box-at ?box ?from)
                (can-climb-two ?from ?to)
                (empty ?bot1)
            )

        :effect
            (and
                (not (at ?bot1 ?from))
                (at ?bot1 ?to)
            )
    )

    ;---------------------------------------------------------------------------
    ; Bot sube a otro bot para pasar a otro waypoint.
    ;---------------------------------------------------------------------------
    (:action climb-two
        :parameters
            (?bot1 ?bot2 - bot
             ?from ?to - waypoint)

        :precondition
            (and
            	(not (= ?bot1 ?bot2))
                (at ?bot1 ?from)
                (at ?bot2 ?from)
                (can-climb-two ?from ?to)
                (empty ?bot1)
                (empty ?bot2)
            )

        :effect
            (and
                (not (at ?bot1 ?from))
                (at ?bot1 ?to)
            )
    )

    ;---------------------------------------------------------------------------
    ; Los dos bots suben la caja y uno sube arriba de otro para pasar a otro waypoint.
    ;---------------------------------------------------------------------------
    (:action climb-three
        :parameters
            (?bot1 ?bot2 - bot
             ?box - box
             ?from ?to - waypoint)

        :precondition
            (and
            	(not (= ?bot1 ?bot2))
                (at ?bot1 ?from)
                (at ?bot2 ?from)
                (box-at ?box ?from)
                (can-climb-three ?from ?to)
                (empty ?bot1)
                (empty ?bot2)
            )

        :effect
            (and
                (not (at ?bot1 ?from))
                (at ?bot1 ?to)
            )
    )

    ;---------------------------------------------------------------------------
    ; Bot aprieta boton que cambia estados de dos puertas.
    ;---------------------------------------------------------------------------
    (:action push-button
        :parameters
            (?bot - bot
             ?button - button
             ?door1 ?door2 - door
             ?waypoint - waypoint
             ?door1-waypoint1 ?door1-waypoint2 - waypoint
             ?door2-waypoint1 ?door2-waypoint2 - waypoint)

        :precondition
            (and
                (between ?door1 ?door1-waypoint1 ?door1-waypoint2)
                (between ?door2 ?door2-waypoint1 ?door2-waypoint2)
                (toggle ?button ?door1 ?door2)
                (at ?bot ?waypoint)
                (button-at ?button ?waypoint)
            )

        :effect
            (and
                (when
                    (can-move ?door1-waypoint1 ?door1-waypoint2)
                    (and
                        (not (can-move ?door1-waypoint1 ?door1-waypoint2))
                        (not (can-move ?door1-waypoint2 ?door1-waypoint1))
                    )
                )
                (when
                    (not (can-move ?door1-waypoint1 ?door1-waypoint2))
                    (and
                        (can-move ?door1-waypoint1 ?door1-waypoint2)
                        (can-move ?door1-waypoint2 ?door1-waypoint1)
                    )
                )
                (when
                    (can-move ?door2-waypoint1 ?door2-waypoint2)
                    (and
                        (not (can-move ?door2-waypoint1 ?door2-waypoint2))
                        (not (can-move ?door2-waypoint2 ?door2-waypoint1))
                    )
                )
                (when
                    (not (can-move ?door2-waypoint1 ?door2-waypoint2))
                    (and
                        (can-move ?door2-waypoint1 ?door2-waypoint2)
                        (can-move ?door2-waypoint2 ?door2-waypoint1)
                    )
                )
            )
    )

    ;---------------------------------------------------------------------------
    ; Bot dispara boton que cambia estados de dos puertas.
    ;---------------------------------------------------------------------------
    (:action shoot-button
        :parameters
            (?bot - bot
             ?button - button
             ?door1 ?door2 - door
             ?sniper-weapon - weapon
             ?waypoint - waypoint
             ?door1-waypoint1 ?door1-waypoint2 - waypoint
             ?door2-waypoint1 ?door2-waypoint2 - waypoint)

        :precondition
            (and
                (sniper-weapon ?sniper-weapon)
                (at ?bot ?waypoint)
                (has ?bot ?sniper-weapon)
                (can-shoot ?button ?waypoint)
                (toggle ?button ?door1 ?door2)
                (between ?door1 ?door1-waypoint1 ?door1-waypoint2)
                (between ?door2 ?door2-waypoint1 ?door2-waypoint2)
            )

        :effect
            (and
                (when
                    (can-move ?door1-waypoint1 ?door1-waypoint2)
                    (and
                        (not (can-move ?door1-waypoint1 ?door1-waypoint2))
                        (not (can-move ?door1-waypoint2 ?door1-waypoint1))
                    )
                )
                (when
                    (not (can-move ?door1-waypoint1 ?door1-waypoint2))
                    (and
                        (can-move ?door1-waypoint1 ?door1-waypoint2)
                        (can-move ?door1-waypoint2 ?door1-waypoint1)
                    )
                )
                (when
                    (can-move ?door2-waypoint1 ?door2-waypoint2)
                    (and
                        (not (can-move ?door2-waypoint1 ?door2-waypoint2))
                        (not (can-move ?door2-waypoint2 ?door2-waypoint1))
                    )
                )
                (when
                    (not (can-move ?door2-waypoint1 ?door2-waypoint2))
                    (and
                        (can-move ?door2-waypoint1 ?door2-waypoint2)
                        (can-move ?door2-waypoint2 ?door2-waypoint1)
                    )
                )
            )
    )

)


