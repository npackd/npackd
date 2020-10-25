package main

import (
	"bytes"
	"encoding/json"
	"encoding/xml"
	"errors"
	"flag"
	"fmt"
	"io"
	"io/ioutil"
	"math/rand"
	"net/http"
	"net/url"
	"os"
	"strconv"
	"strings"
	"time"
	"regexp"
	"runtime"
)

var changeData = false

// Settings is for global program settings
type Settings struct {
	curl          string
	githubToken   string
	password      string
	git           string
	npackdcl      string
	packagesTag   string
	virusTotalKey string // x-apikey
}

var settings Settings

// Release is a Github release
type Release struct {
	TagName string `json:"tag_name"`
	ID      int    `json:"id"`
}

// Asset is a Github release asset
type Asset struct {
	BrowserDownloadURL string `json:"browser_download_url"`
	ID                 int    `json:"id"`
	Name               string `json:"name"`
}

// Package is an Npackd package declaration
type Package struct {
	Name     string   `xml:"name,attr"`
	Category []string `xml:"category"`
	Tag      []string `xml:"tag"`
	DiscoveryPage     string   `xml:"_discovery-page"`
	DiscoveryRE     string   `xml:"_discovery-re"`
}

// PackageVersion is an Npackd package version
type PackageVersion struct {
	Name    string `xml:"name,attr"`
	Package string `xml:"package,attr"`
	URL     string `xml:"url"`
}

// Repository is an Npackd repository XML
type Repository struct {
	PackageVersion []PackageVersion `xml:"version"`
	Package        []Package        `xml:"package"`
}

func indexOf(slice []string, item string) int {
	for i := range slice {
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

// Re-upload from an external page to Github
//
// url: repository URL
// releaseID: ID of a Github project release
func uploadRepositoryBinariesToGithub(url string, releaseID int) error {
	fmt.Println("Re-uploading packages in " + url)

	bytes, _, err := download(url, true)
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

		if indexOf(p.Tag, "reupload") >= 0 {
			packages[p.Name] = true
			fmt.Println("Found package for re-upload:" + p.Name)
		}
	}

	n := 0
	for i := 0; i < len(v.PackageVersion); i++ {
		var pv = v.PackageVersion[i]
		var p = pv.Package
		var version = pv.Name
		var url = pv.URL

		_, ok := packages[p]

		if ok && strings.Index(url,
			"https://github.com/tim-lebedkov/packages/releases/download/") != 0 &&
			url != "" {
			fmt.Println("https://www.npackd.org/p/" + p + "/" + version)

			newURL, err := uploadToGithub(url, p, version, releaseID)
			if err != nil {
				fmt.Println(err.Error())
			} else {
				_, err = apiSetURL(p, version, newURL)
				if err == nil {
					fmt.Println(p + " " + version + " changed URL")
				} else {
					fmt.Println("Failed to change URL for " + p + " " + version)
				}
			}
			fmt.Println("------------------------------------------------------------------")
			n = n + 1
		}

		if n > 2 {
			break
		}
	}
	fmt.Println("==================================================================")

	return nil
}

// from: URL
func uploadToGithub(from string, fullPackage string,
	version string, releaseID int) (string, error) {
	fmt.Println("Re-uploading " + from + " to Github")
	var mime = "application/vnd.microsoft.portable-executable" // "application/octet-stream";

	u, err := url.Parse(from)
	if err != nil {
		return "", err
	}

	var p = strings.LastIndex(u.Path, "/")
	var file = fullPackage + "-" + version + "-" + u.Path[p+1:]

	var cmd = "-f -L --connect-timeout 30 --max-time 900 " + from + " --output " + file
	fmt.Println(cmd)
	var result, _ = exec2("", settings.curl, cmd, true)
	if result != 0 {
		return "", errors.New("Cannot download the file")
	}

	var url = "https://uploads.github.com/repos/tim-lebedkov/packages/releases/" +
		strconv.Itoa(releaseID) + "/assets?name=" + file
	var downloadURL = "https://github.com/tim-lebedkov/packages/releases/download/" + settings.packagesTag + "/" + file
	// fmt.Println("Uploading to " + url);
	fmt.Println("Download from " + downloadURL)

	result, _ = exec2("", settings.curl, "-f -H \"Authorization: token "+settings.githubToken+"\""+
		" -H \"Content-Type: "+mime+"\""+
		" --data-binary @"+file+" \""+url+"\"", false)
	if result != 0 {
		return "", errors.New("Cannot upload the file to Github")
	}

	return downloadURL, nil
}

// Uploads a file to Github
//
// user: Github user
// project: Github project
// from: source file name
// file: target file name
// mime: MIME type of the file
// releaseID: ID of a Github release
func createGithubReleaseAsset(user string, project string, from string, file string, mime string,
	releaseID int) error {
	var url = "https://uploads.github.com/repos/" + user + "/" + project + "/releases/" +
		strconv.Itoa(releaseID) + "/assets?name=" + file

	result, _ := exec2("", settings.curl, "-f -H \"Authorization: token "+settings.githubToken+"\""+
		" -H \"Content-Type: "+mime+"\""+
		" --data-binary @"+from+" \""+url+"\"", false)
	if result != 0 {
		return errors.New("Cannot upload the file to Github")
	}

	return nil
}

// Delete a Github release asset.
//
// user: Github user
// project: Github project
// from: source file name
// file: target file name
// mime: MIME type of the file
// releaseID: ID of a Github release
func deleteGithubReleaseAsset(user string, project string, assetID int) error {
	var url = "https://api.github.com/repos/" + user + "/" + project + "/releases/assets/" +
		strconv.Itoa(assetID)

	// Create client
	client := &http.Client{}

	// Create request
	req, err := http.NewRequest("DELETE", url, nil)
	if err != nil {
		return err
	}

	req.Header.Add("Authorization", "token "+settings.githubToken)

	// Fetch Request
	resp, err := client.Do(req)
	if err != nil {
		return err
	}
	defer resp.Body.Close()

	// Read Response Body
	_, err = ioutil.ReadAll(resp.Body)
	if err != nil {
		return err
	}

	// Display Results
	//fmt.Println("response Status : ", resp.Status)
	//fmt.Println("response Headers : ", resp.Header)
	//fmt.Println("response Body : ", string(respBody))

	if resp.StatusCode/100 != 2 {
		return errors.New("Cannot delete a Github release asset")
	}

	return nil
}

/**
 * @param packageName full package name
 * @param version version number or null for "newest"
 * @return path to the specified package or "" if not installed
 */
func getPath(packageName string, version string) string {
	params := "path -p " + packageName
	if version != "" {
		params = params + " -v " + version
	}

	_, lines := exec2("", settings.npackdcl, params, true)
	if len(lines) > 0 {
		return lines[0]
	}
	return ""
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
func apiNotify(packageName string, version string, install bool, success bool) {
	url := "https://npackd.appspot.com/api/notify?package=" +
		packageName + "&version=" + version +
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

	download(url, false)
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
func apiTag(packageName string, version string, tag string, set bool) error {
	url := "https://npackd.appspot.com/api/tag?package=" +
		packageName + "&version=" + version +
		"&password=" + settings.password +
		"&name=" + tag +
		"&value="
	if set {
		url = url + "1"
	} else {
		url = url + "0"
	}

	_, _, err := download(url, false)
	return err
}

/**
 * Changes the URL for a package version at https://npackd.appspot.com .
 *
 * @param package_ package name
 * @param version version number
 * @param url new URL
 * @return true if the call to the web service succeeded, false otherwise
 */
func apiSetURL(packageName string, version string, newURL string) (int, error) {
	a := "https://npackd.appspot.com/api/set-url?package=" +
		packageName + "&version=" + version +
		"&password=" + settings.password +
		"&url=" + url.QueryEscape(newURL)

	_, statusCode, err := download(a, false)
	return statusCode, err
}

/**
 * Processes one package version.
 *
 * @param package_ package name
 * @param version version number
 * @return true if the test was successful
 */
func processPackageVersion(packageName string, version string) bool {
	ec, _ := exec2("", settings.npackdcl, "add --package="+packageName+
		" --version="+version+" -t 600", true)
	if ec != 0 {
		fmt.Println("npackdcl.exe add failed")
		apiNotify(packageName, version, true, false)
		apiTag(packageName, version, "test-failed", true)

		log := packageName + "-" + version + "-install.log"
		exec2("", "cmd.exe", "/C \""+settings.npackdcl+"\" add -d --package="+packageName+
			" --version="+version+" -t 600 > "+log+" 2>&1", true)
		exec2("", "appveyor", "PushArtifact "+log, true)

		return false
	}
	apiNotify(packageName, version, true, true)

	var path = getPath(packageName, version)
	fmt.Println("where=" + path)
	if path != "" {
		var tree = packageName + "-" + version + "-tree.txt"
		exec2("", "cmd.exe", "/c tree \""+path+"\" /F > "+tree+" 2>&1", true)
		exec2("", "appveyor", "PushArtifact "+tree, true)

		exec2("", "cmd.exe", "/c dir \""+path+"\"", true)

		var msilist = packageName + "-" + version + "-msilist.txt"
		exec2("", "cmd.exe", "/c \"C:\\Program Files (x86)\\CLU\\clu.exe\" list-msi > "+msilist+" 2>&1", true)
		exec2("", "appveyor", "PushArtifact "+msilist, true)

		var info = packageName + "-" + version + "-info.txt"
		exec2("", "cmd.exe", "/C \""+settings.npackdcl+"\" info --package="+packageName+
			" --version="+version+" > "+info+" 2>&1", true)
		exec2("", "appveyor", "PushArtifact "+info, true)

		var list = packageName + "-" + version + "-list.txt"
		exec2("", "cmd.exe", "/C \""+settings.npackdcl+"\" list > "+list+" 2>&1", true)
		exec2("", "appveyor", "PushArtifact "+list, true)

		var proglist = packageName + "-" + version + "-proglist.txt"
		exec2("", "cmd.exe", "/c \"C:\\Program Files (x86)\\Sysinternals_suite\\psinfo.exe\" -s /accepteula > "+proglist+" 2>&1", true)
		exec2("", "appveyor", "PushArtifact "+proglist, true)
	}

	ec, _ = exec2("", settings.npackdcl, "remove -e=ck --package="+packageName+
		" --version="+version+" -t 600", true)
	if ec != 0 {
		fmt.Println("npackdcl.exe remove failed")
		apiNotify(packageName, version, false, false)
		apiTag(packageName, version, "test-failed", true)

		var log = packageName + "-" + version + "-uninstall.log"
		exec2("", "cmd.exe", "/C \""+settings.npackdcl+
			"\" remove -d -e=ck --package="+packageName+
			" --version="+version+" -t 600 > "+log+" 2>&1", true)
		exec2("", "appveyor", "PushArtifact "+log, true)

		return false
	}
	apiNotify(packageName, version, false, true)
	apiTag(packageName, version, "test-failed", false)
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
 * a: first version
 * b: first version
 */
func compareVersions(a []int, b []int) int {
	var mlen = Max(len(a), len(b))

	var r = 0

	for i := 0; i < mlen; i++ {
		var ai = a[i]
		var bi = b[i]
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
func processURL(url string, onlyNewest bool) error {
	var start = time.Now()

	bytes, _, err := download(url, true)
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

			pviv, _ := parseVersion(pvi.Name)
			pvjv, _ := parseVersion(pvj.Name)

			if !found || compareVersions(pviv, pvjv) > 0 {
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
		var p = pv.Package
		var version = pv.Name

		fmt.Println(p + " " + version)
		fmt.Println("https://www.npackd.org/p/" + p + "/" + version)

		if indexOf(bigPackages, p) >= 0 {
			fmt.Println(p + " " + version + " ignored because of the download size")
		} else if !processPackageVersion(p, version) {
			failed = append(failed, p+"@"+version)
		} else {
			if apiTag(p, version, "untested", false) == nil {
				fmt.Println(p + " " + version + " was marked as tested")
			} else {
				fmt.Println("Failed to mark " + p + " " + version + " as tested")
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
	}

	fmt.Println("All packages are OK in " + url)

	return nil
}

// Creates a tag "YYYY_MM" in https://github.com/tim-lebedkov/packages
func updatePackagesProject() error {
	settings.packagesTag = time.Now().Format("2006_01")

	dir, err := ioutil.TempDir("", "prefix")
	if err != nil {
		return err
	}
	defer os.RemoveAll(dir)
	
	ec, _ := exec2("", settings.git,
		"clone https://github.com/tim-lebedkov/packages.git " + dir, true)
	if ec != 0 {
		return errors.New("Cannot clone the \"packages\" project")
	}

	// ignore the exit code here as the tag may already exist
	exec2(dir, settings.git, "tag -a "+
		settings.packagesTag+" -m "+settings.packagesTag, true)

	ec, _ = exec2(dir, settings.git,
		"push https://tim-lebedkov:"+settings.githubToken+
			"@github.com/tim-lebedkov/packages.git --tags", false)
	if ec != 0 {
		return errors.New("Cannot push the \"packages\" project")
	}

	return nil
}

func downloadRepos() error {
	// download the newest repository files and commit them to the project
	ec, _ := exec2("", settings.git, "checkout master", true)
	if ec != 0 {
		return errors.New("Program execution failed")
	}

	reps := []string{"unstable", "stable", "stable64", "libs"}
	for _, s := range reps {
		err := downloadToFile("https://www.npackd.org/rep/xml?tag="+s+"&create=true",
			"repository/"+s+".xml")
		if err != nil {
			return err
		}

		err = downloadToFile("https://www.npackd.org/rep/zip?tag="+s+"&create=true",
			"repository/"+s+".zip")
		if err != nil {
			return err
		}
	}

	ec, _ = exec2("", settings.git, "config --global user.email \"tim.lebedkov@gmail.com\"", true)
	if ec != 0 {
		return errors.New("Program execution failed")
	}

	ec, _ = exec2("", settings.git, "config --global user.name \"tim-lebedkov\"", true)
	if ec != 0 {
		return errors.New("Program execution failed")
	}

	// ignore the exit code here as there may be no changes to commit
	exec2("", settings.git, "commit -a -m \"Automatic data transfer from https://www.npackd.org\"", true)

	ec, _ = exec2("", settings.git, "push https://tim-lebedkov:"+settings.githubToken+
		"@github.com/npackd/npackd.git", false)
	if ec != 0 {
		return errors.New("Program execution failed")
	}

	return nil
}

func createRelease() error {
	body := `{
  "tag_name": "` + settings.packagesTag + `",
  "target_commitish": "master",
  "name": "` + settings.packagesTag + `",
  "body": "Description of the release",
  "draft": false,
  "prerelease": false
}`
	req, err := http.NewRequest("POST",
		"https://api.github.com/repos/tim-lebedkov/packages/releases",
		strings.NewReader(body))
	if err != nil {
		return err
	}
	req.Header.Set("Content-Type", "application/json; charset=utf-8")
	req.Header.Set("Authorization", "token "+settings.githubToken)
	resp, err := http.DefaultClient.Do(req)
	if err != nil {
		return err
	}
	defer resp.Body.Close()

	// Check server response
	if resp.StatusCode < 200 || resp.StatusCode >= 300 {
		return fmt.Errorf("bad status: %s", resp.Status)
	}

	b := new(bytes.Buffer)

	// Write the body to file
	_, err = io.Copy(b, resp.Body)
	if err != nil {
		return err
	}

	return nil
}

func download(address string, showParameters bool) (*bytes.Buffer, int, error) {
	if showParameters {
		fmt.Println("Downloading " + address)
	} else {
		u, err := url.Parse(address)
		if err != nil {
			return nil, 0, err
		}
		fmt.Println("Downloading " + u.Scheme + "://" + u.Host + u.EscapedPath() + "?<<<parameters hidden>>>")
	}

	// Get the data
	resp, err := http.Get(address)
	if err != nil {
		return nil, 0, err
	}
	defer resp.Body.Close()

	// Check server response
	if resp.StatusCode != http.StatusOK {
		return nil, resp.StatusCode, fmt.Errorf("bad status: %s", resp.Status)
	}

	b := new(bytes.Buffer)

	// Write the body to file
	_, err = io.Copy(b, resp.Body)
	if err != nil {
		return nil, resp.StatusCode, err
	}

	return b, resp.StatusCode, nil
}

// Download to a file
//
// url: URL for HTTP GET
// path: output file
// Returns: error message
func downloadToFile(url, path string) error {
	fmt.Println("Downloading " + url)

	// Get the data
	resp, err := http.Get(url)
	if err != nil {
		return err
	}
	defer resp.Body.Close()

	// Check server response
	if resp.StatusCode != http.StatusOK {
		return fmt.Errorf("bad status: %s", resp.Status)
	}

	// open file
	f, err := os.OpenFile(path, os.O_RDWR|os.O_CREATE, 0644)
	if err != nil {
		return err
	}

	// Write the body to file
	_, err = io.Copy(f, resp.Body)
	if err != nil {
		return err
	}

	return nil
}

// fileExists checks if a file exists.
func fileExists(filename string) bool {
    _, err := os.Stat(filename)
    return !os.IsNotExist(err)
}

func createSettings() {
	settings.password = os.Getenv("PASSWORD")
	settings.githubToken = os.Getenv("github_token")
	if runtime.GOOS == "windows" {
		settings.npackdcl = "C:\\Program Files\\NpackdCL\\ncl.exe"
		settings.curl = getPath("se.haxx.curl.CURL64", "")
		settings.git = "C:\\Program Files\\Git\\cmd\\git.exe"
	} else {
		settings.curl = "curl"
		settings.git = "git"
	}

	fmt.Println("curl: " + settings.curl)
}

// List releases for a Github project.
//
// user: Github user name
// project: Github project name
func getReleases(user string, project string) ([]Release, error) {
	b, _, err := download("https://api.github.com/repos/"+user+"/"+project+"/releases", true)
	if err != nil {
		return nil, err
	}

	var releases []Release

	err = json.Unmarshal(b.Bytes(), &releases)
	if err != nil {
		return nil, err
	}

	return releases, nil
}

// List Github release assets
//
// id: release ID
// Returns: (assets, error message)
func getReleaseAssets(user string, project string, id int) ([]Asset, error) {
	b, _, err := download("https://api.github.com/repos/"+user+"/"+project+"/releases/"+
		strconv.Itoa(id)+"/assets", true)
	if err != nil {
		return nil, err
	}

	var releases []Asset

	err = json.Unmarshal(b.Bytes(), &releases)
	if err != nil {
		return nil, err
	}

	return releases, nil

}

func findRelease(releases []Release, tagName string) int {
	releaseID := 0
	for _, r := range releases {
		if r.TagName == tagName {
			releaseID = r.ID
			break
		}
	}

	return releaseID
}

func findAsset(assets []Asset, filename string) int {
	id := 0
	for _, r := range assets {
		if r.Name == filename {
			id = r.ID
			break
		}
	}

	return id
}

// download all binaries from https://github.com/tim-lebedkov/packages
// dir: target directory
func downloadBinaries(dir string) error {
	releases, err := getReleases("tim-lebedkov", "packages")
	if err != nil {
		return err
	}

	fmt.Println(strconv.Itoa(len(releases)) + " releases found")

	for _, release := range releases {
		fmt.Println("Processing release " + release.TagName)

		assets, err := getReleaseAssets("tim-lebedkov", "packages", release.ID)
		if err != nil {
			return err
		}

		fmt.Println("Found " + strconv.Itoa(len(assets)) +
			" assets in the release " + release.TagName)

		for _, asset := range assets {
			path := dir + "/" + release.TagName + "/" + asset.Name
			if _, err := os.Stat(path); os.IsNotExist(err) {
				bytes, _, err := download(asset.BrowserDownloadURL, true)
				if err != nil {
					return err
				}

				err = os.MkdirAll(dir+"/"+release.TagName, 0777)
				if err != nil {
					return err
				}

				err = ioutil.WriteFile(path, bytes.Bytes(), 0644)
				if err != nil {
					return err
				}
			}
		}
	}

	return nil
}

func uploadBinariesToGithub() error {
	fmt.Println("Re-uploading binaries to github.com")

	err := updatePackagesProject()
	if err != nil {
		return err
	}

	releases, err := getReleases("tim-lebedkov", "packages")
	if err != nil {
		return err
	}

	releaseID := findRelease(releases, settings.packagesTag)
	if releaseID == 0 {
		err = createRelease()
		if err != nil {
			return err
		}
		releaseID = findRelease(releases, settings.packagesTag)
	}

	if releaseID == 0 {
		return errors.New("Cannot find the release")
	}

	fmt.Printf("Found release ID: %d\n", releaseID)

	reps := []string{"stable", "stable64", "libs"}
	for _, rep := range reps {
		err = uploadRepositoryBinariesToGithub("https://npackd.appspot.com/rep/xml?tag="+rep, releaseID)
		if err != nil {
			return err
		}
	}

	return nil
}

func parseVersion(version string) ([]int, error) {
	parts := strings.Split(version, ".")
	if len(parts) == 0 {
		return nil, errors.New("0 parts")
	}

	res := make([]int, len(parts))
	for i, p := range(parts) {
		v, err := strconv.Atoi(p)
		if err != nil {
			return nil, err
		}
		res[i] = v
	}

	return res, nil
}

func maxVersion(a []PackageVersion) []int {
	res := []int{0}
	for _, pv := range(a) {
		v, _ := parseVersion(pv.Name)
		if compareVersions(v, res) > 0 {
			res = v
		}
	}
	return res
}

func checkOneForUpdates(p *Package, maxVersion []int) (*PackageVersion, error) {
	var pv *PackageVersion = nil

	if p.DiscoveryPage != "" {
		re, err := regexp.Compile(p.DiscoveryRE)
		if err != nil {
			return nil, err
		}

		data, _, err := download(p.DiscoveryPage, true)
		if err != nil {
			return nil, err
		}

		buf := data.Bytes()
		f := re.Find(buf)
		if f != nil {
			version := string(f)
			v, err := parseVersion(version)
			if err != nil {
				return nil, err
			}

			if compareVersions(v, maxVersion) > 0 {
				// TODO
			}
		} else {
			fmt.Println("No match found for the regular expression")
		}
	}

	return pv, nil
}

func getPackageVersions(rep *Repository, packageName string) []PackageVersion {
	res := []PackageVersion{}
	for _, pv := range rep.PackageVersion {
		if pv.Package == packageName {
			res = append(res, pv)
		}
	}
	return res
}

func checkForUpdates() error {
	dat, err := ioutil.ReadFile("repository/stable.xml")
	if err != nil {
		return err
	}

	// parse the repository XML
	rep := Repository{}
	err = xml.Unmarshal(dat, &rep)
	if err != nil {
		return err
	}

	for i := 0; i < 10; i++ {
		index := rand.Intn(len(rep.Package))
		p := rep.Package[index]
		packageVersions := getPackageVersions(&rep, p.Name)
		if len(packageVersions) > 0 {
			m := maxVersion(packageVersions)

			v, err := checkOneForUpdates(&p, m)
			if err != nil {
				fmt.Println(err.Error())
			} else {
				if v != nil {
					// TODO
					fmt.Println("Found new version")
				}
			}
		}
	}

	return nil
}

func maintenance(password string, githubToken string) error {
	createSettings()
	settings.password = password
	settings.githubToken = githubToken

	err := downloadRepos()
	if err != nil {
		return err
	}

	err = uploadBinariesToGithub()
	if err != nil {
		return err
	}

	err = checkForUpdates()
	if err != nil {
		return err
	}

	return nil
}

// unsafe zone. Here we run code from external sites.
func testPackages() error {
	createSettings()

	processURL("https://npackd.appspot.com/rep/recent-xml?tag=untested",
		false)

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

	reps := []string{"stable", "stable64", "libs"}

	processURL("https://npackd.appspot.com/rep/xml?tag="+reps[index],
		newest)

	return nil
}

// correct URLs on npackd.org from an XML file
//
// password: npackd.org password
func correctURLs(password string) error {
	createSettings()
	settings.password = password

	dat, err := ioutil.ReadFile("repository/stable.xml")
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
		if pv.URL != "" {
			url := pv.URL

			prefix := "https://ayera.dl.sourceforge.net/project/"
			if strings.HasPrefix(url, prefix) {
				url = url[len(prefix):]
				parts := strings.Split(url, "/")
				url = url[len(parts[0])+1:]
				url = "https://sourceforge.net/projects/" + parts[0] + "/files/" + url
			}

			if url != pv.URL {
				fmt.Println("Changing URL for " + pv.Package + " " + pv.Name)
				fmt.Println("    from " + pv.URL)
				fmt.Println("    to " + url)

				statusCode, err := apiSetURL(pv.Package, pv.Name, url)
				if err != nil && statusCode != http.StatusNotFound {
					return err
				}
			}
		}
	}

	return nil
}

var command = flag.String("command", "test-packages", "the action that should be performed")
var target = flag.String("target", "", "directory where the downloaded binaries are stored")
var password = flag.String("password", "", "npackd.org password")
var githubToken = flag.String("github-token", "", "github.org token")

// Download binaries from Github to a directory:
// go run TestUnstableRep.go TestUnstableRep_linux.go -command download-binaries -target /target/directory
//
// Correct URLs for packages at npackd.org:
// go run TestUnstableRep.go TestUnstableRep_linux.go -command correct-urls -password PASSWORD
//
// Download repositories from npackd.org to github.com/npackd/npackd, re-upload packages to github.com/tim-lebedkov/packages:
// go run TestUnstableRep.go TestUnstableRep_linux.go -command maintenance -password PASSWORD -github-token GITHUB_TOKEN
//
// Test packages on AppVeyor:
// go run TestUnstableRep.go TestUnstableRep_linux.go -password PASSWORD
func main() {
	var err error = nil

	flag.Parse()

	rand.Seed(time.Now().UnixNano())

	if *command == "download-binaries" {
		err = downloadBinaries(*target)
	} else if *command == "correct-urls" {
		err = correctURLs(*password)
	} else if *command == "maintenance" {
		err = maintenance(*password, *githubToken)
	} else {
		err = testPackages()
	}

	if err != nil {
		fmt.Println(err.Error())
		os.Exit(1)
	}
}
