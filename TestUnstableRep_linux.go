// +build linux

package main

import (
	"os/exec"
	"syscall"
	"fmt"
	"strings"
)

func parseCommandLine(command string) ([]string, error) {
    var args []string
    state := "start"
    current := ""
    quote := "\""
    escapeNext := true
    for i := 0; i < len(command); i++ {
        c := command[i]

        if state == "quotes" {
            if string(c) != quote {
                current += string(c)
            } else {
                args = append(args, current)
                current = ""
                state = "start"
            }
            continue
        }

        if (escapeNext) {
            current += string(c)
            escapeNext = false
            continue
        }

        if (c == '\\') {
            escapeNext = true
            continue
        }

        if c == '"' || c == '\'' {
            state = "quotes"
            quote = string(c)
            continue
        }

        if state == "arg" {
            if c == ' ' || c == '\t' {
                args = append(args, current)
                current = ""
                state = "start"
            } else {
                current += string(c)
            }
            continue
        }

        if c != ' ' && c != '\t' {
            state = "arg"
            current += string(c)
        }
    }

    if state == "quotes" {
        return []string{}, fmt.Errorf("Unclosed quote in command line: %s", command)
    }

    if current != "" {
        args = append(args, current)
    }

    return args, nil
}

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

	args, err := parseCommandLine(params)
	if err != nil {
		fmt.Println(err.Error())
		exitCode = 1
		return
	}

	cmd := exec.Command(program, args...)
	cmd.SysProcAttr = &syscall.SysProcAttr{}
	// Windows only cmd.SysProcAttr.CmdLine = " " + params
	cmd.Dir = dir

	out, err := cmd.CombinedOutput()

	lines := strings.Split(string(out), "\r\n")
	for _, line := range lines {
		fmt.Println(line)
	}

	if err != nil {
        // try to get the exit code
        if exitError, ok := err.(*exec.ExitError); ok {
            ws := exitError.Sys().(syscall.WaitStatus)
            exitCode = ws.ExitStatus()
        } else {
            // This will happen (in OSX) if `name` is not available in $PATH,
            // in this situation, exit code could not be get, and stderr will be
            // empty string very likely, so we use the default fail code, and format err
            // to string and set to stderr
            fmt.Printf("Could not get exit code for failed program: %v, %v", program, params)
            exitCode = 1
        }
    } else {
        // success, exitCode should be 0 if go is ok
        ws := cmd.ProcessState.Sys().(syscall.WaitStatus)
        exitCode = ws.ExitStatus()
    }

	fmt.Printf("Exit code: %d\n", exitCode)
    
	return exitCode, lines
}

