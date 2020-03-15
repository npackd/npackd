package main

import (
	"fmt"
	"os/exec"
	"strings"
	"syscall"
)

// Executes the specified command, prints its output on the default output.
//
// dir: current directory or ""
// cmd: this command should be executed
// showParameters: true = output the parameters,
// 	false = hide the parameters (e.g. for passwords)
// return: [exit code, [output line 1, output line2, ...]]
func exec2(dir string, program string, params string,
	showParameters bool) (exitCode int, output []string) {
	if showParameters {
		fmt.Println("\"" + program + "\" " + params)
	} else {
		fmt.Println("\"" + program + "\" <<<parameters hidden>>>")
	}

	cmd := exec.Command(program)
	cmd.SysProcAttr = &syscall.SysProcAttr{}
	cmd.SysProcAttr.CmdLine = " " + params
	cmd.Dir = dir

	out, err := cmd.CombinedOutput()

	lines := strings.Split(string(out), "\r\n")
	for _, line := range lines {
		fmt.Println(line)
	}

	if err != nil {
		if exitError, ok := err.(*exec.ExitError); ok {
			fmt.Printf("Exit code: %d\n", exitError.ExitCode())
			return exitError.ExitCode(), nil
		}
	}

	return 0, lines
}

