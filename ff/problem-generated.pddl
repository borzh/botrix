(define (problem bots)

	(:domain
		bots-domain
	)

	(:objects
		bot0 bot1 - bot

		default area-respawn1 area-respawn2 area-crossbow area-button5 area-button4 area-button3 area-gravity-gun area-button1 area-shoot-button area-wall1 area-high area-wall2 area-goal - area
		door3 door4 door5 door6 door7 door9 - door

		button3 button8 button9 - button
		gravity-gun crossbow - weapon

		box0 - box
	)

	(:init
		(physcannon gravity-gun)
		(sniper-weapon crossbow)
		(at bot0 area-respawn2) (empty bot0)

		(at bot1 area-respawn1) (empty bot1) (has bot1 gravity-gun)

		(box-at box0 area-respawn1)

		(button-at button3 area-button4)
		(button-at button8 area-shoot-button)
		(can-shoot button9 area-shoot-button)

		(between door4 area-button4 area-respawn1)
		(can-move area-button4 area-respawn1)
		(can-move area-respawn1 area-button4)
		(between door7 area-shoot-button area-button4)
		(can-move area-shoot-button area-button4)
		(can-move area-button4 area-shoot-button)
		(between door9 area-wall1 area-shoot-button)
		(can-move area-wall1 area-shoot-button)
		(can-move area-shoot-button area-wall1)

		(wall area-wall1 area-high)
	)

	(:goal
			( exists (?box - box) (box-at ?box area-wall1) )
	)
)
