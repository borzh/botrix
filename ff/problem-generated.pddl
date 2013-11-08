(define (problem bots)

	(:domain
		bots-domain
	)

	(:objects
		bot1 bot2 - bot

		default area-respawn1 area-respawn2 area-crossbow area-button5 area-button4 area-button3 area-gravity-gun area-button1 area-shoot-button area-wall1 area-high area-wall2 area-goal - area
		door0 door1 door2 door4 door5 door6 - door

		button7 - button
		gravity-gun crossbow - weapon

		box0 - box
	)

	(:init
		(physcannon gravity-gun)
		(sniper-weapon crossbow)
		(at bot1 area-respawn2) (empty bot1)

		(at bot2 area-respawn1) (empty bot2)

		(box-at box0 area-respawn1)

		(button-at button7 area-respawn2)


	)

	(:goal
		(and
			( exists (?bot - bot) (at ?bot area-respawn2) )
		)
	)
)
