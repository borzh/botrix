(define (problem bots)

	(:domain
		bots-domain
	)

	(:objects
		bot1 bot2 - bot

		area-respawn1 area-respawn2 area-crossbow area-button5 area-button4 area-button3 area-physcannon area-button1 area-shoot_button area-climb2 area-up1 area-climb3 area-up2 area-goal - waypoint
		door1 door2 door3 door4 door5 door6 door7 door8 door9 door10 - door

		button1 button2 button3 button4 button5 button6 button7 button8 button9 button10 - button
		weapon_crossbow1 weapon_physcannon2 - weapon

		box1 - box
	)

	(:init
		(sniper-weapon weapon_crossbow1)
		(weapon-at weapon_crossbow1 area-crossbow)

		(physcannon weapon_physcannon2)
		(weapon-at weapon_physcannon2 area-physcannon)

		(at bot1 area-respawn1) (empty bot1)
		(at bot2 area-respawn2) (empty bot2)

		(button-at button1 area-button1)
		(button-at button2 area-physcannon)
		(button-at button3 area-button3)
		(button-at button4 area-button4)
		(button-at button5 area-button5)
		(button-at button6 area-crossbow)
		(button-at button7 area-goal)
		(button-at button8 area-respawn2)
		(button-at button9 area-shoot_button)

		(between door1 area-button1 area-respawn2)
		(between door2 area-respawn2 area-physcannon)
		(between door3 area-button3 area-respawn2)
		(between door4 area-button4 area-button3)
		(between door5 area-button4 area-respawn1)
		(between door6 area-button5 area-respawn1)
		(between door7 area-respawn1 area-crossbow)
		(between door8 area-shoot_button area-button4)
		(between door9 area-climb3 area-goal)
		(between door10 area-climb2 area-shoot_button)

		(box-at box1 area-respawn1)

		(can-shoot button10 area-shoot_button)
		(can-climb-two area-climb2 area-up1)
		(can-move area-up1 area-climb2)
		(can-move area-up1 area-climb3)
		(can-climb-two area-climb3 area-up1)
		(can-climb-three area-climb3 area-up2)
		(can-move area-up2 area-climb3)
		(can-move area-up2 area-goal)

		; Auto-generated buttons configuration.
        (toggle button1 door9 door3)
        (toggle button10 door4 door7)
        (toggle button2 door2 door5)
        (toggle button3 door8 door4)
        (toggle button4 door1 door8)
        (toggle button5 door4 door7)
        (toggle button6 door7 door10)
        (toggle button7 door9 door6)
        (toggle button8 door3 door10)
        (toggle button9 door2 door6)

	)

	(:goal
		(and
			(at bot1 area-goal) (at bot2 area-goal)
		)
	)
)
