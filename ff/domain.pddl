(define (domain bots-domain)

	(:requirements :equality :typing :adl)

	(:types bot, waypoint, weapon, door, button, box)

    (:predicates
        (physcannon ?weapon - weapon)                     ; Arma de gravedad
        (sniper-weapon ?weapon - weapon)                  ; Arma francotirador

        (can-move ?from ?to - waypoint)
        (can-shoot ?button - button ?waypoint - waypoint) ; Disparar al boton
        (wall ?from ?to - waypoint)                       ; Una pared.

        ;(visited ?bot - bot ?area - waypoint)

        (at ?bot - bot ?waypoint - waypoint)
		(box-at ?box - box ?waypoint - waypoint)
		(button-at ?button - button ?waypoint - waypoint)
        (weapon-at ?weapon - weapon ?waypoint - waypoint)

        (has ?bot - bot ?weapon - weapon)
		(carry ?bot - bot ?box - box)
		(empty ?bot - bot)

        (between ?door - door ?waypoint1 ?waypoint2 - waypoint) ; Puertas que separan 2 waypoints
		(toggle ?button - button ?door1 ?door2 - door)          ; Boton cambia estados de 2 puertas
    )

	;---------------------------------------------------------------------------
	; Bot se mueve entre waypoints adjacentes.
	;---------------------------------------------------------------------------
    (:action move
        :parameters
            (?bot - bot
             ?to ?from - waypoint)

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
             ?box - box
             ?waypoint - waypoint
             ?weapon - weapon)

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
    ; Bot agarra la caja con el arma desde arriba de la pared.
    ;---------------------------------------------------------------------------
    (:action carry-box-far
        :parameters
            (?bot - bot
             ?box - box
             ?weapon - weapon
             ?waypoint-bot ?waypoint-box - waypoint)

        :precondition
            (and
                (at ?bot ?waypoint-bot)
                (box-at ?box ?waypoint-box)
                (has ?bot ?weapon)
				(physcannon ?weapon)
                (empty ?bot)
				(wall ?waypoint-box ?waypoint-bot)
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
            (?bot - bot
             ?box - box
             ?from ?to - waypoint)

        :precondition
            (and
                (at ?bot ?from)
                (box-at ?box ?from)
                (wall ?from ?to)
                (empty ?bot)
            )

        :effect
            (and
                (not (at ?bot ?from))
                (at ?bot ?to)
            )
    )

	;---------------------------------------------------------------------------
	; Bot salta desde una pared.
	;---------------------------------------------------------------------------
    (:action fall
        :parameters
            (?bot - bot
             ?to ?from - waypoint)

        :precondition
            (and
                (at ?bot ?from)
                (wall ?to ?from)
            )

        :effect
            (and
                (at ?bot ?to)
                (not (at ?bot ?from))
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
                (empty ?bot)
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
                (empty ?bot)
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


