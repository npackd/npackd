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
// settings: settings
// url: repository URL
// releaseID: ID of a Github project release
func uploadAllToGithub(settings *Settings, url string, releaseID int) error {
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

			newURL, err := uploadToGithub(settings, url, p, version, releaseID)
			if err != nil {
				fmt.Println(err.Error())
			} else {
				_, err = apiSetURL(settings, p, version, newURL)
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
func uploadToGithub(settings *Settings, from string, fullPackage string,
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
func createGithubReleaseAsset(settings *Settings, user string, project string, from string, file string, mime string,
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
func deleteGithubReleaseAsset(settings *Settings, user string, project string, assetID int) error {
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

func exec3(program string, params string, showParameters bool) int {
	exitCode, _ := exec2("", "cmd.exe", "/s /c \""+program+"\" "+params+" 2>&1\"", showParameters)
	return exitCode
}

/**
 * @param packageName full package name
 * @param version version number or null for "newest"
 * @return path to the specified package or "" if not installed
 */
func getPath(settings *Settings, packageName string, version string) string {
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
func apiNotify(settings *Settings,
	packageName string, version string, install bool, success bool) {
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
func apiTag(settings *Settings, packageName string, version string, tag string,
	set bool) error {
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
func apiSetURL(settings *Settings, packageName string, version string, newURL string) (int, error) {
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
func processPackageVersion(settings *Settings, packageName string, version string) bool {
	ec, _ := exec2("", settings.npackdcl, "add --package="+packageName+
		" --version="+version+" -t 600", true)
	if ec != 0 {
		fmt.Println("npackdcl.exe add failed")
		apiNotify(settings, packageName, version, true, false)
		apiTag(settings, packageName, version, "test-failed", true)

		log := packageName + "-" + version + "-install.log"
		exec2("", "cmd.exe", "/C \""+settings.npackdcl+"\" add -d --package="+packageName+
			" --version="+version+" -t 600 > "+log+" 2>&1", true)
		exec2("", "appveyor", "PushArtifact "+log, true)

		return false
	}
	apiNotify(settings, packageName, version, true, true)

	var path = getPath(settings, packageName, version)
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
		apiNotify(settings, packageName, version, false, false)
		apiTag(settings, packageName, version, "test-failed", true)

		var log = packageName + "-" + version + "-uninstall.log"
		exec2("", "cmd.exe", "/C \""+settings.npackdcl+
			"\" remove -d -e=ck --package="+packageName+
			" --version="+version+" -t 600 > "+log+" 2>&1", true)
		exec2("", "appveyor", "PushArtifact "+log, true)

		return false
	}
	apiNotify(settings, packageName, version, false, true)
	apiTag(settings, packageName, version, "test-failed", false)
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
		var p = pv.Package
		var version = pv.Name

		fmt.Println(p + " " + version)
		fmt.Println("https://www.npackd.org/p/" + p + "/" + version)

		if indexOf(bigPackages, p) >= 0 {
			fmt.Println(p + " " + version + " ignored because of the download size")
		} else if !processPackageVersion(settings, p, version) {
			failed = append(failed, p+"@"+version)
		} else {
			if apiTag(settings, p, version, "untested", false) == nil {
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
//
// settings: settings
func updatePackagesProject(settings *Settings) error {
	settings.packagesTag = time.Now().Format("2006_01")

	ec, _ := exec2("", settings.git,
		"clone https://github.com/tim-lebedkov/packages.git C:\\projects\\packages", true)
	if ec != 0 {
		return errors.New("Cannot clone the \"packages\" project")
	}

	// ignore the exit code here as the tag may already exist
	exec2("C:\\projects\\packages", settings.git, "tag -a "+
		settings.packagesTag+" -m "+settings.packagesTag, true)

	ec, _ = exec2("C:\\projects\\packages", settings.git,
		"push https://tim-lebedkov:"+settings.githubToken+
			"@github.com/tim-lebedkov/packages.git --tags", false)
	if ec != 0 {
		return errors.New("Cannot push the \"packages\" project")
	}

	return nil
}

// Uploads repositories from npackd.org to
// https://github.com/tim-lebedkov/npackd/releases/tag/v1
//
// settings: settings
func uploadReposToGithub(settings *Settings) error {
	files := []string{"repository/Rep.xml", "repository/Libs.xml", "repository/Rep64.xml", "repository/RepUnstable.xml"}
	targetFiles := []string{"stable.xml", "libs.xml", "stable64.xml", "unstable.xml"}

	releases, err := getReleases("tim-lebedkov", "npackd")
	if err != nil {
		return err
	}

	releaseID := findRelease(releases, "v1")
	if releaseID <= 0 {
		return errors.New("Release not found")
	}

	assets, err := getReleaseAssets("tim-lebedkov", "npackd", releaseID)
	if err != nil {
		return err
	}

	for i := range files {
		assetID := findAsset(assets, targetFiles[i])
		if assetID > 0 {
			println(targetFiles[i] + " deleting..." + strconv.Itoa(assetID))
			deleteGithubReleaseAsset(settings, "tim-lebedkov", "npackd", assetID)
		} else {
			println(targetFiles[i] + " not found")
		}

		err := createGithubReleaseAsset(settings, "tim-lebedkov", "npackd", files[i], targetFiles[i], "application/xml", releaseID)
		if err != nil {
			return err
		}
	}

	return nil
}

func downloadRepos(settings *Settings) {
	// download the newest repository files and commit them to the project
	exec2("", settings.git, "checkout master", true)

	exec2("", settings.curl, "-f -o repository/RepUnstable.xml "+
		"https://www.npackd.org/rep/xml?tag=unstable&create=true", true)

	exec2("", settings.curl, "-f -o repository/Rep.xml "+
		"https://www.npackd.org/rep/xml?tag=stable&create=true", true)

	exec2("", settings.curl, "-f -o repository/Rep64.xml "+
		"https://www.npackd.org/rep/xml?tag=stable64&create=true", true)

	exec2("", settings.curl, "-f -o repository/Libs.xml "+
		"https://www.npackd.org/rep/xml?tag=libs&create=true", true)

	exec2("", settings.git, "config --global user.email \"tim.lebedkov@gmail.com\"", true)
	exec2("", settings.git, "config --global user.name \"tim-lebedkov\"", true)
	exec2("", settings.git, "commit -a -m \"Automatic data transfer from https://www.npackd.org\"", true)

	exec2("", settings.git, "push https://tim-lebedkov:"+settings.githubToken+
		"@github.com/tim-lebedkov/npackd.git", false)
}

func createRelease(settings *Settings) error {
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

func createSettings() Settings {
	var settings Settings
	settings.password = os.Getenv("PASSWORD")
	settings.githubToken = os.Getenv("github_token")
	settings.npackdcl = "C:\\Program Files\\NpackdCL\\ncl.exe"
	settings.curl = getPath(&settings, "se.haxx.curl.CURL64", "")
	if settings.curl == "" {
		settings.curl = "curl"
	} else {
		settings.curl = settings.curl + "\\bin\\curl.exe"
	}
	settings.git = "C:\\Program Files\\Git\\cmd\\git.exe"

	fmt.Println("curl: " + settings.curl)

	return settings
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

func downloadRepositories() error {
	var settings Settings = createSettings()

	downloadRepos(&settings)

	err := uploadReposToGithub(&settings)
	if err != nil {
		return err
	}

	err = updatePackagesProject(&settings)
	if err != nil {
		return err
	}

	releases, err := getReleases("tim-lebedkov", "packages")
	if err != nil {
		return err
	}

	releaseID := findRelease(releases, settings.packagesTag)
	if releaseID == 0 {
		err = createRelease(&settings)
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
		uploadAllToGithub(&settings, "https://npackd.appspot.com/rep/xml?tag="+rep, releaseID)
	}

	return nil
}

// unsafe zone. Here we run code from external sites.
func testPackages() error {
	var settings Settings = createSettings()

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

	reps := []string{"stable", "stable64", "libs"}

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

				statusCode, err := apiSetURL(&settings, pv.Package, pv.Name, url)
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

// Download binaries from Github to a directory:
// go run TestUnstableRep.go TestUnstableRep_linux.go -command download-binaries -target /target/directory
//
// Correct URLs for packages at npackd.org:
// PASSWORD=xxxx go run TestUnstableRep.go TestUnstableRep_linux.go -command correct-urls
//
// Download repositories from npackd.org to github.com/tim-lebedkov/npackd:
// github_token=xxxxx PASSWORD=xxxx go run TestUnstableRep.go TestUnstableRep_linux.go -command download-repositories
//
// Test packages on AppVeyor:
// github_token=xxxxx PASSWORD=xxxx go run TestUnstableRep.go TestUnstableRep_linux.go
func main() {
	var err error = nil

	flag.Parse()

	if *command == "download-binaries" {
		err = downloadBinaries(*target)
	} else if *command == "correct-urls" {
		err = correctURLs()
	} else if *command == "download-repositories" {
		err = downloadRepositories()
	} else {
		err = testPackages()
	}

	if err != nil {
		fmt.Println(err.Error())
		os.Exit(1)
	}
}
