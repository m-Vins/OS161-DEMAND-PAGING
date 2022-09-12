import pexpect
import sys
import os
import shutil
import os.path

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
    "sort",
    "matmult",
    "hugematmult1",
    "hugematmult2",
    "ctest"
]

tests = [
    "at",
    "at2",
    "bt",
    "km1",
    "km2",
    "km3 1000",
    "km4"
]

def getprompt(proc, prompt):
	which = proc.expect_exact([
			prompt,
			"panic: ",		# panic message
			"sys161: No progress in ", # sys161 deadman print
			"sys161: Elapsed ",	# sys161 shutdown print
			pexpect.EOF,
			pexpect.TIMEOUT
		    ],
            timeout=3600
        ) #large timeout due to ctest program which takes more time
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
    proc.sendline(f'cd {os.getenv("HOME",default=None)}/os161/root;sys161 kernel')

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

def kill_instance(proc):
    proc.close()


def run_program(proc, test):
    return run_cmd(proc,"p testbin/" + test)


def run_cmd(proc,cmd):
    print("Running command \"" + cmd + "\"")

    proc.sendline(cmd)
    msg = getprompt(proc, "OS/161 kernel [? for menu]: ")
    if msg == None:
        return proc.before
    else:
        return None


def extract_execution_time(output):
    return output.split("\n")[-2].strip().split(" ")[2][:6]


def run_program_tests(f, ram_size):
    passed_programs = []

    for program in programs:
        proc = open_instance()
        program_output = run_program(proc, program)

        if program_output:
            execution_time = extract_execution_time(program_output)
            results = close_instance(proc)
            passed_programs.append(program)
            f.write("|" + program + "|" + ram_size + "|" + execution_time + "|" + "|".join(results) + "|\n")
        else:
            kill_instance(proc)
            f.write("|" + program + "|" + ram_size + "|-|" + "|".join(["-" for s in stats]) + "|\n")

    return passed_programs


def main():
    passed_tests = []

    # Create sys161.conf backup
    if not os.path.exists("../root/sys161.conf.backup"):
        shutil.copyfile("../root/sys161.conf", "../root/sys161.conf.backup")
    else:
        shutil.copyfile("../root/sys161.conf.backup", "../root/sys161.conf")

    # Init .md file
    f = open("testresults.md", "w")

    # Program tests header
    f.write("# Programs\n")
    f.write("| Program name | Ram size | Execution time | " + " | ".join(stats) + "|\n")
    f.write("|".join(["-" for _ in range(len(stats) + 3)]) + "\n")

    # Program tests
    passed_programs = run_program_tests(f, "512K")

    # Change ram size
    fin = open("../root/sys161.conf.backup", "rt")
    fout = open("../root/sys161.conf", "wt")
    for line in fin:
        fout.write(line.replace('31	mainboard  ramsize=512K  cpus=1', '31	mainboard  ramsize=4M  cpus=1'))
    fin.close()
    fout.close()

    run_program_tests(f, "4M")
    f.write("\n")

    # Reset ram size
    shutil.copyfile("../root/sys161.conf.backup", "../root/sys161.conf")

    # Generic tests header
    f.write("# Tests\n")
    f.write("|Test name|Passed|\n")
    f.write("|-|-|\n")

    for test in tests:
        proc = open_instance()
        success = run_cmd(proc, test)
        if success:
            passed_tests.append(test)
            close_instance(proc)
            f.write("|" + test+"|" + "True" + "|\n")
        else:
            kill_instance(proc)
            f.write("|" + test+"|" + "False" + "|\n")
    f.write("\n")
   

    # Stability test header
    f.write("# Stability test\n")
    f.write("|" + " | ".join(stats) + "|\n")
    f.write("|" + "|".join(["-" for s in stats]) + "|\n")

    # Stability test
    proc = open_instance()
    failures = 0
    for _ in range(10):
        for program in passed_programs:
            success = run_program(proc, program)
            if not success:
                failures += 1
        for test in passed_tests:
            success = run_cmd(proc, test)
            if not success:
                failures += 1
    results = close_instance(proc)
    f.write("|" + "|".join(results) + "|\n")

    print("\n")
    print("Failures: " + str(failures))

    f.close()


if __name__ == '__main__':
    main()
