package main

import (
	"bytes"
	"encoding/json"
	"encoding/xml"
	"errors"
	"fmt"
	"io"
	"io/ioutil"
	"math/rand"
	"net/http"
	"net/url"
	"os"
	"os/exec"
	"strconv"
	"strings"
	"syscall"
	"time"
)

var changeData = false

type Settings struct {
	curl        string
	githubToken string
	password    string
	git         string
	npackdcl    string
}

func indexOf(slice []string, item string) int {
	for i, _ := range slice {
		if slice[i] == item {
			return i
		}
	}
	return -1
}

// http://stackoverflow.com/questions/6274339/how-can-i-shuffle-an-array-in-javascript
func shuffle(array []string) {
	counter := len(array)

	// While there are elements in the array
	for counter > 0 {
		// Pick a random index
		index := rand.Intn(counter)

		// Decrease counter by 1
		counter--

		// And swap the last element with it
		temp := array[counter]
		array[counter] = array[index]
		array[index] = temp
	}
}

func shufflePackageVersions(array []PackageVersion) {
	counter := len(array)

	// While there are elements in the array
	for counter > 0 {
		// Pick a random index
		index := rand.Intn(counter)

		// Decrease counter by 1
		counter--

		// And swap the last element with it
		temp := array[counter]
		array[counter] = array[index]
		array[index] = temp
	}
}

type Package struct {
	Name     string `xml:"name,attr"`
	Category []string
}

type PackageVersion struct {
	Name    string `xml:"name,attr"`
	Package string `xml:"package,attr"`
	Url     string `xml:"url"`
}

type Repository struct {
	PackageVersion []PackageVersion `xml:"version"`
	Package        []Package        `xml:"package"`
}

func uploadAllToGithub(settings *Settings, url string, releaseID int) error {
	fmt.Println("Re-uploading packages in " + url)

	fmt.Println("Loading " + url)
	bytes, err, _ := download(url)
	if err != nil {
		return err
	}

	v := Repository{}
	err = xml.Unmarshal(bytes.Bytes(), &v)
	if err != nil {
		return err
	}

	packages := make(map[string]bool)
	for i := 0; i < len(v.Package); i++ {
		p := v.Package[i]

		if indexOf(p.Category, "reupload") >= 0 {
			packages[v.Package[i].Name] = true
		}
	}

	for i := 0; i < len(v.PackageVersion); i++ {
		var pv = v.PackageVersion[i]
		var package_ = pv.Package
		var version = pv.Name
		var url = pv.Url

		_, ok := packages[package_]

		if ok && strings.Index(url,
			"https://github.com/tim-lebedkov/packages/releases/download/") != 0 &&
			url != "" {
			fmt.Println("https://www.npackd.org/p/" + package_ + "/" + version)

			newURL, err := uploadToGithub(settings, url, package_, version, releaseID)
			if err != nil {
				fmt.Println(err.Error())
			} else {
				err, _ = apiSetURL(settings, package_, version, newURL)
				if err == nil {
					fmt.Println(package_ + " " + version + " changed URL")
				} else {
					fmt.Println("Failed to change URL for " + package_ + " " + version)
				}
			}
			fmt.Println("------------------------------------------------------------------")
		}
	}
	fmt.Println("==================================================================")

	return nil
}

func uploadToGithub(settings *Settings, from string, package_ string, version string, releaseID int) (string, error) {
	fmt.Println("Re-uploading " + from + " to Github")
	var mime = "application/vnd.microsoft.portable-executable" // "application/octet-stream";

	var p = strings.LastIndex(from, "/")
	var file = package_ + "-" + version + "-" + from[p+1:]

	var cmd = "\"" + settings.curl + "\" -f -L --connect-timeout 30 --max-time 900 " + from + " --output " + file
	fmt.Println(cmd)
	var result, _ = exec2(settings.curl, cmd)
	if result != 0 {
		return "", errors.New("Cannot download the file")
	}

	var url = "https://uploads.github.com/repos/tim-lebedkov/packages/releases/" +
		strconv.Itoa(releaseID) + "/assets?name=" + file
	var downloadURL = "https://github.com/tim-lebedkov/packages/releases/download/2019_Q1/" + file
	// fmt.Println("Uploading to " + url);
	fmt.Println("Download from " + downloadURL)

	if changeData {
		result, _ = exec2(settings.curl, "\""+settings.curl+"\" -f -H \"Authorization: token "+settings.githubToken+"\""+
			" -H \"Content-Type: "+mime+"\""+
			" --data-binary @"+file+" \""+url+"\"")
		if result != 0 {
			return "", errors.New("Cannot upload the file to Github")
		}
	}

	return downloadURL, nil
}

func exec_(program string, cmd string) int {
	fullcmd := "cmd.exe /s /c \"" + cmd + " 2>&1\""
	exitCode, _ := exec2("cmd.exe", fullcmd)
	return exitCode
}

/**
 * @param package_ full package name
 * @param version version number or null for "newest"
 * @return path to the specified package or "" if not installed
 */
func getPath(settings *Settings, package_ string, version string) string {
	cmd := "\"" + settings.npackdcl + "\" path -p " + package_
	if version != "" {
		cmd = cmd + " -v " + version
	}

	fullcmd := "cmd.exe /s /c \"" + cmd + " 2>&1\""

	_, lines := exec2("cmd.exe", fullcmd)
	if len(lines) > 0 {
		return lines[0]
	} else {
		return ""
	}

	return ""
}

/**
 * Executes the specified command, prints its output on the default output.
 *
 * @param cmd this command should be executed
 * @return [exit code, [output line 1, output line2, ...]]
 */
func exec2(program string, command string) (exitCode int, output []string) {
	fmt.Println(command)

	cmd := exec.Command(program)
	cmd.SysProcAttr = &syscall.SysProcAttr{}
	cmd.SysProcAttr.CmdLine = command

	out, err := cmd.CombinedOutput()

	lines := strings.Split(string(out), "\n")
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

/**
 * Notifies the application at https://npackd.appspot.com about an installation
 * or an uninstallation.
 *
 * @param package_ package name
 * @param version version number
 * @param install true if it was an installation, false if it was an
 *     un-installation
 * @param success true if the operation was successful
 */
func apiNotify(settings *Settings,
	package_ string, version string, install bool, success bool) {
	url := "https://npackd.appspot.com/api/notify?package=" +
		package_ + "&version=" + version +
		"&password=" + settings.password +
		"&install="
	if install {
		url = url + "1"
	} else {
		url = url + "0"
	}

	url = url + "&success="
	if success {
		url = url + "1"
	} else {
		url = url + "0"
	}

	if changeData {
		download(url)
	}
}

/**
 * Adds or removes a tag for a package version at https://npackd.appspot.com .
 *
 * @param package_ package name
 * @param version version number
 * @param tag the name of the tag
 * @param set true if the tag should be added, false if the tag should be
 *     removed
 * @return true if the call to the web service succeeded, false otherwise
 */
func apiTag(settings *Settings, package_ string, version string, tag string,
	set bool) error {
	url := "https://npackd.appspot.com/api/tag?package=" +
		package_ + "&version=" + version +
		"&password=" + settings.password +
		"&name=" + tag +
		"&value="
	if set {
		url = url + "1"
	} else {
		url = url + "0"
	}

	if changeData {
		_, err, _ := download(url)
		return err
	} else {
		return nil
	}
}

/**
 * Changes the URL for a package version at https://npackd.appspot.com .
 *
 * @param package_ package name
 * @param version version number
 * @param url new URL
 * @return true if the call to the web service succeeded, false otherwise
 */
func apiSetURL(settings *Settings, package_ string, version string, url_ string) (error, int) {
	a := "https://npackd.appspot.com/api/set-url?package=" +
		package_ + "&version=" + version +
		"&password=" + settings.password +
		"&url=" + url.QueryEscape(url_)

	if changeData {
		_, err, statusCode := download(a)
		return err, statusCode
	} else {
		return nil, 0
	}
}

/**
 * Processes one package version.
 *
 * @param package_ package name
 * @param version version number
 * @return true if the test was successful
 */
func process(settings *Settings, package_ string, version string) bool {
	ec := exec_(settings.npackdcl, "\""+settings.npackdcl+"\" add --package="+package_+
		" --version="+version+" -t 600")
	if ec != 0 {
		fmt.Println("npackdcl.exe add failed")
		apiNotify(settings, package_, version, true, false)
		apiTag(settings, package_, version, "test-failed", true)

		log := package_ + "-" + version + "-install.log"
		exec_("cmd.exe", "cmd.exe /C \""+settings.npackdcl+"\" add -d --package="+package_+
			" --version="+version+" -t 600 > "+log+" 2>&1")
		exec_("appveyor", "appveyor PushArtifact "+log)

		return false
	}
	apiNotify(settings, package_, version, true, true)

	var path = getPath(settings, package_, version)
	fmt.Println("where=" + path)
	if path != "" {
		var tree = package_ + "-" + version + "-tree.txt"
		exec2("cmd.exe", "cmd.exe /c tree \""+path+"\" /F > "+tree+" 2>&1")
		exec_("appveyor", "appveyor PushArtifact "+tree)

		exec_("cmd.exe", "cmd.exe /c dir \""+path+"\"")

		var msilist = package_ + "-" + version + "-msilist.txt"
		exec2("cmd.exe", "cmd.exe /c \"C:\\Program Files (x86)\\CLU\\clu.exe\" list-msi > "+msilist+" 2>&1")
		exec_("appveyor", "appveyor PushArtifact "+msilist)

		var info = package_ + "-" + version + "-info.txt"
		exec_("cmd.exe", "cmd.exe /C \""+settings.npackdcl+"\" info --package="+package_+
			" --version="+version+" > "+info+" 2>&1")
		exec_("appveyor", "appveyor PushArtifact "+info)

		var list = package_ + "-" + version + "-list.txt"
		exec_("cmd.exe", "cmd.exe /C \""+settings.npackdcl+"\" list > "+list+" 2>&1")
		exec_("appveyor", "appveyor PushArtifact "+list)

		var proglist = package_ + "-" + version + "-proglist.txt"
		exec2("cmd.exe", "cmd.exe /c \"C:\\Program Files (x86)\\Sysinternals_suite\\psinfo.exe\" -s /accepteula > "+proglist+" 2>&1")
		exec_("appveyor", "appveyor PushArtifact "+proglist)
	}

	ec = exec_(settings.npackdcl, "\""+settings.npackdcl+"\" remove -e=ck --package="+package_+
		" --version="+version+" -t 600")
	if ec != 0 {
		fmt.Println("npackdcl.exe remove failed")
		apiNotify(settings, package_, version, false, false)
		apiTag(settings, package_, version, "test-failed", true)

		var log = package_ + "-" + version + "-uninstall.log"
		exec_("cmd.exe", "cmd.exe /C \""+settings.npackdcl+
			"\" remove -d -e=ck --package="+package_+
			" --version="+version+" -t 600 > "+log+" 2>&1")
		exec_("appveyor", "appveyor PushArtifact "+log)

		return false
	}
	apiNotify(settings, package_, version, false, true)
	apiTag(settings, package_, version, "test-failed", false)
	return true
}

// Max returns the larger of x or y.
func Max(x, y int) int {
	if x < y {
		return y
	}
	return x
}

/**
 * @param a first version as a String
 * @param b first version as a String
 */
func compareVersions(a string, b string) int {
	aparts := strings.Split(a, ".")
	bparts := strings.Split(b, ".")

	var mlen = Max(len(aparts), len(bparts))

	var r = 0

	for i := 0; i < mlen; i++ {
		var ai = 0
		var bi = 0

		if i < len(aparts) {
			ai, _ = strconv.Atoi(aparts[i])
		}
		if i < len(bparts) {
			bi, _ = strconv.Atoi(bparts[i])
		}

		// fmt.Println("comparing " + ai + " and " + bi);

		if ai < bi {
			r = -1
			break
		} else if ai > bi {
			r = 1
			break
		}
	}

	return r
}

/**
 * @param onlyNewest true = only test the newest versions
 */
func processURL(url string, settings *Settings, onlyNewest bool) error {
	var start = time.Now()

	bytes, err, _ := download(url)
	if err != nil {
		return err
	}

	// parse the repository XML
	v := Repository{}
	err = xml.Unmarshal(bytes.Bytes(), &v)
	if err != nil {
		return err
	}

	var pvs = v.PackageVersion

	// only retain newest versions for each package
	if onlyNewest {
		fmt.Println("Only testing the newest versions out of " + strconv.Itoa(len(pvs)))
		var newest = make(map[string]PackageVersion)
		for i := 0; i < len(pvs); i++ {
			var pvi = pvs[i]
			var pvip = pvi.Package
			var pvj, found = newest[pvip]

			if !found || compareVersions(pvi.Name, pvj.Name) > 0 {
				newest[pvip] = pvi
			}
		}

		pvs = nil
		for _, value := range newest {
			pvs = append(pvs, value)

			/*
			   fmt.Println("Newest: " + newest[key].getAttribute("package") +
			   " " +
			   newest[key].getAttribute("name"))
			*/
		}

		fmt.Println("Only the newest versions: " + strconv.Itoa(len(pvs)))
	}

	shufflePackageVersions(pvs)

	// fmt.Println(pvs.length + " versions found")
	var failed []string = nil

	// packages with the download size over 1 GiB
	var bigPackages = []string{"mevislab", "mevislab64", "nifi", "com.nokia.QtMinGWInstaller",
		"urban-terror", "com.microsoft.VisualStudioExpressCD",
		"com.microsoft.VisualCSharpExpress", "com.microsoft.VisualCPPExpress",
		"win10-edge-virtualbox", "win7-ie11-virtualbox"}

	for i := range pvs {
		var pv = pvs[i]
		var package_ = pv.Package
		var version = pv.Name

		fmt.Println(package_ + " " + version)
		fmt.Println("https://www.npackd.org/p/" + package_ + "/" + version)

		if indexOf(bigPackages, package_) >= 0 {
			fmt.Println(package_ + " " + version + " ignored because of the download size")
		} else if !process(settings, package_, version) {
			failed = append(failed, package_+"@"+version)
		} else {
			if apiTag(settings, package_, version, "untested", false) == nil {
				fmt.Println(package_ + " " + version + " was marked as tested")
			} else {
				fmt.Println("Failed to mark " + package_ + " " + version + " as tested")
			}
		}
		fmt.Println("==================================================================")

		if time.Since(start).Minutes() > 45 {
			break
		}
	}

	if len(failed) > 0 {
		fmt.Println(strconv.Itoa(len(failed)) + " packages failed:")
		for i := 0; i < len(failed); i++ {
			fmt.Println(failed[i])
		}
		return errors.New("Some packages failed")
	} else {
		fmt.Println("All packages are OK in " + url)
	}

	return nil
}

func downloadRepos(settings *Settings) {
	// download the newest repository files and commit them to the project
	exec_(settings.git, "\""+settings.git+"\" checkout master")

	exec_(settings.curl, "\""+settings.curl+"\" -f -o repository\\RepUnstable.xml "+
		"https://www.npackd.org/rep/xml?tag=unstable")
	exec_(settings.curl, "\""+settings.curl+"\" -f -o repository\\Rep.xml "+
		"https://www.npackd.org/rep/xml?tag=stable")
	exec_(settings.curl, "\""+settings.curl+"\" -f -o repository\\Rep64.xml "+
		"https://www.npackd.org/rep/xml?tag=stable64")
	exec_(settings.curl, "\""+settings.curl+"\" -f -o repository\\Libs.xml "+
		"https://www.npackd.org/rep/xml?tag=libs")

	exec_(settings.git, "\""+settings.git+"\" config user.email \"tim.lebedkov@gmail.com\"")
	exec_(settings.git, "\""+settings.git+"\" config user.name \"tim-lebedkov\"")
	exec_(settings.git, "\""+settings.git+"\" commit -a -m \"Automatic data transfer from https://www.npackd.org\"")

	if changeData {
		exec_(settings.git, "\""+settings.git+"\" push https://tim-lebedkov:"+settings.githubToken+
			"@github.com/tim-lebedkov/npackd.git")
	}
}

func download(url string) (*bytes.Buffer, error, int) {
	fmt.Println("Downloading " + url)

	// Get the data
	resp, err := http.Get(url)
	if err != nil {
		return nil, err, 0
	}
	defer resp.Body.Close()

	// Check server response
	if resp.StatusCode != http.StatusOK {
		return nil, fmt.Errorf("bad status: %s", resp.Status), resp.StatusCode
	}

	b := new(bytes.Buffer)

	// Write the body to file
	_, err = io.Copy(b, resp.Body)
	if err != nil {
		return nil, err, resp.StatusCode
	}

	return b, nil, resp.StatusCode
}

type Release struct {
	Tag_name string
	Id       int
}

func createSettings() Settings {
	var settings Settings
	// settings.password = os.Getenv("PASSWORD")
	settings.password = "abc"
	settings.githubToken = os.Getenv("github_token")
	settings.curl = getPath(&settings, "se.haxx.curl.CURL64", "") + "\\bin\\curl.exe"
	settings.git = "C:\\Program Files\\Git\\cmd\\git.exe"
	settings.npackdcl = "C:\\Program Files (x86)\\NpackdCL\\ncl.exe"

	return settings
}

func process2() error {
	var settings Settings = createSettings()

	downloadRepos(&settings)

	reps := []string{"stable", "stable64", "libs"}

	b, err, _ := download("https://api.github.com/repos/tim-lebedkov/packages/releases")
	if err != nil {
		return err
	}

	var releases []Release

	err = json.Unmarshal(b.Bytes(), &releases)
	if err != nil {
		return err
	}

	releaseID := 0
	for _, r := range releases {
		if r.Tag_name == "2019_Q1" {
			releaseID = r.Id
			break
		}
	}

	if releaseID == 0 {
		return errors.New("Cannot find the release")
	}

	fmt.Printf("Found release ID: %d\n", releaseID)

	// print curl version
	exec2(settings.curl, "\""+settings.curl+"\" --version")

	for _, rep := range reps {
		uploadAllToGithub(&settings, "https://npackd.appspot.com/rep/xml?tag="+rep, releaseID)
	}

	processURL("https://npackd.appspot.com/rep/recent-xml?tag=untested",
		&settings, false)

	// the stable repository is about 3900 KiB
	// and should be tested more often
	index := rand.Intn(4000)
	if index < 3000 {
		index = 0
	} else if index < 3900 {
		index = 1
	} else {
		index = 2
	}

	// 9 of 10 times only check the newest versions
	var newest = rand.Float32() < 0.9

	processURL("https://npackd.appspot.com/rep/xml?tag="+reps[index],
		&settings, newest)
	return nil
}

// correct URLs on npackd.org from an XML file
func correctURLs() error {
	var settings Settings = createSettings()

	// read Rep.xml
	dat, err := ioutil.ReadFile("repository/Rep.xml")
	if err != nil {
		return err
	}

	// parse the repository XML
	v := Repository{}
	err = xml.Unmarshal(dat, &v)
	if err != nil {
		return err
	}

	// set URL for every package version
	for _, pv := range v.PackageVersion {
		if pv.Url != "" {
			err, statusCode := apiSetURL(&settings, pv.Package, pv.Name, pv.Url)
			fmt.Println("Setting URL for " + pv.Package + " " + pv.Name + " to " + pv.Url)
			if err != nil && statusCode != http.StatusNotFound {
				return err
			}
		}
	}

	return nil
}

func main() {
	err := process2()

	// err := correctURLs()

	if err != nil {
		fmt.Println(err.Error())
		os.Exit(1)
	}
}
