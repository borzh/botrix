(define (problem bots)

	(:domain
		bots-domain
	)

	(:objects
		bot0 bot1 - bot

		default area-respawn1 area-respawn2 area-crossbow area-button5 area-button4 area-button3 area-gravity-gun area-button1 area-shoot-button area-wall1 area-high area-wall2 area-goal - area
		door0 door1 door2 door3 door4 door5 door6 door7 door8 door9 - door

		button0 button1 button2 button3 button4 button5 button6 button7 button8 button9 - button
		gravity-gun crossbow - weapon

		box0 box1 - box
	)

	(:init
		(physcannon gravity-gun)
		(sniper-weapon crossbow)
		(at bot0 area-goal) (empty bot0) (has bot0 gravity-gun)

		(at bot1 area-respawn2) (empty bot1) (has bot1 crossbow)

		(box-at box0 area-wall1)
		(box-at box1 area-wall2)

		(button-at button0 area-button1)
		(button-at button1 area-gravity-gun)
		(button-at button2 area-button3)
		(button-at button3 area-button4)
		(button-at button4 area-button5)
		(button-at button5 area-crossbow)
		(button-at button6 area-goal)
		(button-at button7 area-respawn2)
		(button-at button8 area-shoot-button)
		(can-shoot button9 area-shoot-button)

		(between door0 area-button1 area-respawn2)
		(between door1 area-respawn2 area-gravity-gun)
		(between door2 area-button3 area-respawn2)
		(can-move area-button3 area-respawn2)
		(can-move area-respawn2 area-button3)
		(between door3 area-button4 area-button3)
		(can-move area-button4 area-button3)
		(can-move area-button3 area-button4)
		(between door4 area-button4 area-respawn1)
		(can-move area-button4 area-respawn1)
		(can-move area-respawn1 area-button4)
		(between door5 area-button5 area-respawn1)
		(can-move area-button5 area-respawn1)
		(can-move area-respawn1 area-button5)
		(between door6 area-respawn1 area-crossbow)
		(between door7 area-shoot-button area-button4)
		(between door8 area-wall2 area-goal)
		(can-move area-wall2 area-goal)
		(can-move area-goal area-wall2)
		(between door9 area-wall1 area-shoot-button)

		(wall area-wall1 area-high)
		(wall area-wall2 area-high)
		(toggle button0 door0 door5)
		(toggle button1 door1 door3)
		(toggle button2 door6 door6)
		(toggle button3 door2 door7)
		(toggle button4 door0 door8)
		(toggle button5 door6 door9)
		(toggle button6 door0 door7)
		(toggle button7 door1 door4)
		(toggle button8 door5 door7)
		(toggle button9 door0 door9)
	)

	(:goal
		(and
			(at bot0 area-goal)
			(at bot1 area-goal)
		)
	)
)
