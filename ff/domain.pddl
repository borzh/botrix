(define (domain bots-domain)

	(:requirements :equality :typing :adl)

	(:types bot, area, weapon, door, button, box)

    (:predicates
        (physcannon ?weapon - weapon)                  ; Arma de gravedad
        (sniper-weapon ?weapon - weapon)               ; Arma francotirador

        (can-move ?from ?to - area)
        (can-shoot ?button - button ?area - area)      ; Disparar al boton
        (wall ?from ?to - area)                        ; Una pared.

        ;(visited ?bot - bot ?area - area)

        (at ?bot - bot ?area - area)
		(box-at ?box - box ?area - area)
		(button-at ?button - button ?area - area)
        (weapon-at ?weapon - weapon ?area - area)

        (has ?bot - bot ?weapon - weapon)
		(carry ?bot - bot ?box - box)
		(empty ?bot - bot)

        (between ?door - door ?area1 ?area2 - area)    ; Puertas que separan 2 areas
		(toggle ?button - button ?door1 ?door2 - door) ; Boton cambia estados de 2 puertas
    )

	;---------------------------------------------------------------------------
	; Bot se mueve entre areas adjacentes.
	;---------------------------------------------------------------------------
    (:action move
        :parameters
            (?bot - bot
             ?to ?from - area)

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
	; Bot salta desde una pared.
	;---------------------------------------------------------------------------
    (:action fall
        :parameters
            (?bot - bot
             ?to ?from - area)

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
	; Bot agarra un arma que esta cerca de un area.
	;---------------------------------------------------------------------------
    (:action take-weapon
        :parameters
            (?bot - bot
             ?weapon - weapon
             ?area - area)

        :precondition
            (and
                (at ?bot ?area)
                (weapon-at ?weapon ?area)
            )

        :effect
            (and
                (not (weapon-at ?weapon ?area))
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
             ?area - area
             ?weapon - weapon)

        :precondition
            (and
                (at ?bot ?area)
                (box-at ?box ?area)
                (has ?bot ?weapon)
				(physcannon ?weapon)
                (empty ?bot)
            )

        :effect
            (and
                (not (empty ?bot))
                (carry ?bot ?box)
                (not (box-at ?box ?area))
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
             ?area-bot ?area-box - area)

        :precondition
            (and
                (at ?bot ?area-bot)
                (box-at ?box ?area-box)
                (has ?bot ?weapon)
				(physcannon ?weapon)
                (empty ?bot)
				(wall ?area-box ?area-bot)
            )

        :effect
            (and
                (not (empty ?bot))
                (carry ?bot ?box)
                (not (box-at ?box ?area-box))
            )
    )

    ;---------------------------------------------------------------------------
    ; Bot tira la caja que estaba llevando.
    ;---------------------------------------------------------------------------
    (:action drop-box
        :parameters
            (?bot - bot
             ?box - box
             ?area - area)

        :precondition
            (and
                (at ?bot ?area)
                (carry ?bot ?box)
            )

        :effect
            (and
                (box-at ?box ?area)
                (empty ?bot)
                (not (carry ?bot ?box))
            )
    )

    ;---------------------------------------------------------------------------
    ; Bot sube la caja para pasar a otro area.
    ;---------------------------------------------------------------------------
    (:action climb-box
        :parameters
            (?bot - bot
             ?box - box
             ?from ?to - area)

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
    ; Bot aprieta boton que cambia estados de dos puertas.
    ;---------------------------------------------------------------------------
    (:action push-button
        :parameters
            (?bot - bot
             ?button - button
             ?door1 ?door2 - door
             ?area - area
             ?door1-area1 ?door1-area2 - area
             ?door2-area1 ?door2-area2 - area)

        :precondition
            (and
                (between ?door1 ?door1-area1 ?door1-area2)
                (between ?door2 ?door2-area1 ?door2-area2)
                (toggle ?button ?door1 ?door2)
                (at ?bot ?area)
                (empty ?bot)
                (button-at ?button ?area)
            )

        :effect
            (and
                (when
                    (can-move ?door1-area1 ?door1-area2)
                    (and
                        (not (can-move ?door1-area1 ?door1-area2))
                        (not (can-move ?door1-area2 ?door1-area1))
                    )
                )
                (when
                    (not (can-move ?door1-area1 ?door1-area2))
                    (and
                        (can-move ?door1-area1 ?door1-area2)
                        (can-move ?door1-area2 ?door1-area1)
                    )
                )
                (when
                    (can-move ?door2-area1 ?door2-area2)
                    (and
                        (not (can-move ?door2-area1 ?door2-area2))
                        (not (can-move ?door2-area2 ?door2-area1))
                    )
                )
                (when
                    (not (can-move ?door2-area1 ?door2-area2))
                    (and
                        (can-move ?door2-area1 ?door2-area2)
                        (can-move ?door2-area2 ?door2-area1)
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
             ?area - area
             ?door1-area1 ?door1-area2 - area
             ?door2-area1 ?door2-area2 - area)

        :precondition
            (and
                (sniper-weapon ?sniper-weapon)
                (at ?bot ?area)
                (has ?bot ?sniper-weapon)
                (empty ?bot)
                (can-shoot ?button ?area)
                (toggle ?button ?door1 ?door2)
                (between ?door1 ?door1-area1 ?door1-area2)
                (between ?door2 ?door2-area1 ?door2-area2)
            )

        :effect
            (and
                (when
                    (can-move ?door1-area1 ?door1-area2)
                    (and
                        (not (can-move ?door1-area1 ?door1-area2))
                        (not (can-move ?door1-area2 ?door1-area1))
                    )
                )
                (when
                    (not (can-move ?door1-area1 ?door1-area2))
                    (and
                        (can-move ?door1-area1 ?door1-area2)
                        (can-move ?door1-area2 ?door1-area1)
                    )
                )
                (when
                    (can-move ?door2-area1 ?door2-area2)
                    (and
                        (not (can-move ?door2-area1 ?door2-area2))
                        (not (can-move ?door2-area2 ?door2-area1))
                    )
                )
                (when
                    (not (can-move ?door2-area1 ?door2-area2))
                    (and
                        (can-move ?door2-area1 ?door2-area2)
                        (can-move ?door2-area2 ?door2-area1)
                    )
                )
            )
    )

)


