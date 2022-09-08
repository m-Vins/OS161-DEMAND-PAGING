import pexpect
import sys


def getprompt(proc, prompt):
	which = proc.expect_exact([
			prompt,
			"panic: ",		# panic message
			"sys161: No progress in ", # sys161 deadman print
			"sys161: Elapsed ",	# sys161 shutdown print
			pexpect.EOF,
			pexpect.TIMEOUT
		])
	if which == 0:
		# got the prompt
		return None
	if which == 1:
		proc.expect_exact([pexpect.EOF, pexpect.TIMEOUT])
		return "panic"
	if which == 2:
		proc.expect_exact([pexpect.EOF, pexpect.TIMEOUT])
		return "progress timeout"
	if which == 3:
		proc.expect_exact([pexpect.EOF, pexpect.TIMEOUT])
		return "unexpected shutdown"
	if which == 4:
		return "unexpected end of input"
	if which == 5:
		return "top-level timeout"
	return "runtest: Internal error: pexpect returned out-of-range result"
# end getprompt


def open_instance():
    proc = pexpect.spawn("bash", ignore_sighup=False, encoding='utf-8')
    #proc.logfile_read = sys.stdout
    proc.sendline("cd /home/os161user/os161/root;sys161 kernel")

    msg = getprompt(proc, "OS/161 kernel [? for menu]: ")
    if msg:
        print("ERROR STARTING OS")
        sys.exit(-1)

    return proc


def close_instance(proc):
    proc.sendline("q")
    msg = getprompt(proc, "The system is halted.")
    if msg:
        return None

    output = proc.before
    test_result_rows = [o.strip() for o in output.split("\n")][-12:-2]

    proc.close()
    return [r.split(" ")[-1] for r in test_result_rows]


def run_program(proc, test):
    print("Running program " + test)

    proc.sendline("p testbin/" + test)
    msg = getprompt(proc, "OS/161 kernel [? for menu]: ")
    if msg:
        return False
    else:
        return True


def main():
    stats = [
        "TLB Faults",
        "TLB Faults with Free",
        "TLB Faults with Replace",
        "TLB Invalidations",
        "TLB Reloads",
        "Page Faults (Zeroed)",
        "Page Faults (Disk)",
        "Page Faults from ELF",
        "Page Faults from Swapfile",
        "Swapfile Writes"
    ]

    programs = [
        "palin",
        "huge",
        "sort"
    ]

    tests = []

    f = open("testresults.md", "w")

    # Program tests header
    f.write("# Programs\n")
    f.write("Program name | " + " | ".join(stats) + "\n")
    f.write("|".join(["-" for _ in range(len(stats) + 1)]) + "\n")

    # Program tests
    for program in programs:
        proc = open_instance()
        success = run_program(proc, program)
        results = close_instance(proc)

        if success:
            f.write(program+"|" + "|".join(results) + "\n")
        else:
            f.write(program+"|" + "|".join(["-" for s in stats]) + "\n")
    f.write("\n")
"""
    # Generic tests header
    f.write("# Tests\n")
    f.write("Test name|Passed\n")
    f.write("-|-\n")

    proc = open_instance()
    for test in tests:
        pass # TODO
    close_instance(proc)
    f.write("\n")

    # Stability test header
    f.write("# Stability test\n")
    f.write(" | ".join(stats) + "\n")
    f.write("|".join(["-" for s in stats]) + "\n")
"""
    # Stability test
    proc = open_instance()
    failures = 0
    for _ in range(20):
        for program in programs:
            success = run_program(proc, program)
            if not success:
                failures += 1
    results = close_instance(proc)
    f.write("|".join(results) + "\n")

    print("\n")
    print("Failures: " + str(failures))

    f.close()


if __name__ == '__main__':
    main()
