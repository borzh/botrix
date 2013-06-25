#!/usr/bin/python


import os, sys, random


def write_file(input_file, output_file, buttons, doors, between, toggles):
    inp = open(input_file, "r")
    outp = open(output_file, "w")
    for line in inp:
        outp.write(line)
        if "; Auto-generated buttons configuration." in line:
            for button in sorted( toggles.keys() ):
                door1, door2 = toggles[button]
                outp.write("        (toggle " + button + " " + door1 + " " + door2 + ")\n")
#        elif "; Auto-generated doors configuration." in line:
#            out.write(line)

def main():
    doors = list()
    buttons = list()
    between = dict()

    objects = False
    init = False
    num = 0

    input_file = sys.argv[1]
    output_file = "result-problem.pddl"
    output_file_3 = "result-problem-3.pddl"

    # Read problem file.
    f = open(input_file, "r")
    for line in f:
        num += 1
        if ":objects" in line:
            objects = True
        if ":init" in line:
            init = True

        if objects:
            if ")" in line:
                objects = False

            if "- door" in line:
                args = line.split("- door")
                values = args[0].split(" ")
                for i in range( len(values) ):
                    door = values[i].strip()
                    if door != "":
                        doors.append(door)

            if "- button" in line:
                args = line.split("- button")
                values = args[0].split(" ")
                for i in range( len(values) ):
                    button = values[i].strip()
                    if button != "":
                        buttons.append(button)

        if init:
            if ":goal" in line:
                init = False

            if "(between " in line:
                betweens = line.split("(between ")
                for i in range( 1, len(betweens) ):
                    index = betweens[i].find(")")
                    args = betweens[i][:index].split(" ")
                    if len(args) == 3:
                        between[args[0]] = args[1], args[2]
                    else:
                        print "Error at line ", num, ", between must take 3 arguments"
    f.close()

    # Primero probamos con todas las puertas cerradas.
    total_buttons = len(buttons)
    size = total_buttons*2
    maxSteps = 0
    maxSteps3 = 0
    ff_output = "/tmp/ff_output"
    combination = [0] * size

    # Ejecutar N veces.
    N = 10000
    for run in range(N):
        done = False
        while not done:
            for i in range(size):
                combination[i] = random.randint(0, total_buttons-1)
            sys.stdout.write("\r" + repr(combination))

            toggles = dict()
            for j in range(total_buttons):
                toggles[ buttons[j] ] = doors[combination[j*2]], doors[combination[j*2+1]]

            sys.stdout.write("\r" + repr(combination))
            sys.stdout.flush

            write_file(input_file, "/tmp/tmp.pddl", buttons, doors, betweens, toggles)

            # Ahora corremos ff
            os.system("ff -o domain.pddl -f /tmp/tmp.pddl >" + ff_output)

            done = True
            steps = 0
            startCount = False
            climbThree = False
            f = open(ff_output, "r")
            for line in f:
                if startCount:
                    steps += 1
                    if "CLIMB-THREE" in line:
                        climbThree = True;
                if "ff: found legal plan as follows" in line:
                    startCount = True
                if "time spent" in line:
                    startCount = False
                if "No plan will solve it" in line:
                    done = False
            f.close()

            steps -= 4
            if done:
                if climbThree:
                    sys.stdout.write(": " + repr(steps) + " steps of max3 " + repr(maxSteps3) + "\n");
                    if steps > maxSteps3:
                        maxSteps3 = steps
                        os.system("mv /tmp/tmp.pddl " + output_file_3)
                else:
                    sys.stdout.write(": " + repr(steps) + " steps of max " + repr(maxSteps));
                    if steps > maxSteps:
                        maxSteps = steps
                        os.system("mv /tmp/tmp.pddl " + output_file)

    print "\nFound plan with " + repr(maxSteps) + " steps"
    os.system("rm " + output_file)

if __name__ == "__main__":
    if len(sys.argv) == 2:
        main()
    else:
        print "Please specify problem file name"
