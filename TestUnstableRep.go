package main

import (
	"fmt"
	"bytes"
	"net/http"
	"io"
//	"os/exec"
)

func DownloadFile(url string) (*bytes.Buffer, error) {
	// Get the data
	resp, err := http.Get(url)
	if err != nil {
		return nil, err
	}
	defer resp.Body.Close()

	b := new(bytes.Buffer)
	
	// Write the body to file
	_, err = io.Copy(b, resp.Body)
	if err != nil {
		return nil, err
	}
	
	return b, nil
}

func downloadRepos() {
    // download the newest repository files and commit them to the project
/*    exec.Command("git", "checkout", "master");

    exec("\"" + curl + "\" -f -o repository\\RepUnstable.xml " +
	 "https://www.npackd.org/rep/xml?tag=unstable");
    exec("\"" + curl + "\" -f -o repository\\Rep.xml " +
	 "https://www.npackd.org/rep/xml?tag=stable");
    exec("\"" + curl + "\" -f -o repository\\Rep64.xml " +
	 "https://www.npackd.org/rep/xml?tag=stable64");
    exec("\"" + curl + "\" -f -o repository\\Libs.xml " +
	 "https://www.npackd.org/rep/xml?tag=libs");

    exec("\"" + git + "\" config user.email \"tim.lebedkov@gmail.com\"");
    exec("\"" + git + "\" config user.name \"tim-lebedkov\"");
    exec("\"" + git + "\" commit -a -m \"Automatic data transfer from https://www.npackd.org\"");
    exec("\"" + git + "\" push https://tim-lebedkov:" + githubToken +
	 "@github.com/tim-lebedkov/npackd.git");
*/}

func main() {
	/*
	npackdcl = "C:\\Program Files (x86)\\NpackdCL\\ncl.exe";
	git = "C:\\Program Files\\Git\\cmd\\git.exe";

	FSO = new ActiveXObject("Scripting.FileSystemObject");
	shell = WScript.CreateObject("WScript.Shell");

	arguments = WScript.Arguments;
	password = arguments.Named.Item("password");

	env = shell.Environment("Process");
	githubToken = env("github_token");

	curl = getPath("se.haxx.curl.CURL64", null) + "\\bin\\curl.exe";

	// packages with the download size over 1 GiB
	bigPackages = ["mevislab", "mevislab64", "nifi", "com.nokia.QtMinGWInstaller",
		"urban-terror", "com.microsoft.VisualStudioExpressCD",
		"com.microsoft.VisualCSharpExpress", "com.microsoft.VisualCPPExpress",
		"win10-edge-virtualbox", "win7-ie11-virtualbox"];
        */
	// WScript.Echo("password=" + githubToken);

	downloadRepos();

	// var reps = ["stable", "stable64", "libs"];

	b, err := DownloadFile("https://api.github.com/repos/tim-lebedkov/packages/releases")
	if err != nil {
		fmt.Println(err.Error())
	} else {
		fmt.Println(b.Len())
	}
	fmt.Println("Hello, 世界")
}
