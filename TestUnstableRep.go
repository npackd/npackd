package main

import (
	"unicode/utf8"
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
	"regexp"
	"runtime"
	"strconv"
	"strings"
	"time"
	"crypto/sha256"
	"mime/multipart"
	"sync"	
	"bufio"
/*	"golang.org/x/oauth2"
	"golang.org/x/oauth2/jwt"
	"golang.org/x/oauth2/google"
	*/
)

var changeData = false

// Settings is for global program settings
type Settings struct {
	githubToken   string
	password      string
	git           string
	npackdcl      string
	packagesTag   string
	virusTotalKey string // x-apikey
	googleID	  string
	googleSecret string
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
	Name          string   `xml:"name,attr"`
	Category      []string `xml:"category"`
	Tag           []string `xml:"tag"`
	DiscoveryPage string   `xml:"_discovery-page"`
	DiscoveryRE   string   `xml:"_discovery-re"`
	DiscoveryURLPattern string `xml:"_discovery-url-pattern"`
}

// PackageVersion is an Npackd package version
type PackageVersion struct {
	Name    string `xml:"name,attr"`
	Package string `xml:"package,attr"`
	URL     string `xml:"url"`
	SHA1    string `xml:"sha1"`
	HashSum string `xml:"hash-sum"`
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

	u, err := url.Parse(from)
	if err != nil {
		return "", err
	}

	var p = strings.LastIndex(u.Path, "/")
	var file = fullPackage + "-" + version + "-" + u.Path[p+1:]

	err = downloadToFile(from, file)
	if err != nil {
		return "", errors.New("Cannot download the file")
	}

	var url = "https://uploads.github.com/repos/tim-lebedkov/packages/releases/" +
		strconv.Itoa(releaseID) + "/assets?name=" + file
	var downloadURL = "https://github.com/tim-lebedkov/packages/releases/download/" + settings.packagesTag + "/" + file
	// fmt.Println("Uploading to " + url);
	fmt.Println("Download from " + downloadURL)

	var mime = "application/vnd.microsoft.portable-executable" // "application/octet-stream";
	err = postFile(url, mime, file)
	if err != nil {
		return "", err
	}

	return downloadURL, nil
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
 * packageName full package name
 * version version number or null for "newest"
 * path to the specified package or "" if not installed
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
 * package_ package name
 * version version number
 * install true if it was an installation, false if it was an
 *     un-installation
 * success true if the operation was successful
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
 * package_ package name
 * version version number
 * tag the name of the tag
 * set true if the tag should be added, false if the tag should be
 *     removed
 * returns true if the call to the web service succeeded, false otherwise
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
 * package_ package name
 * version version number
 * url new URL
 * returns true if the call to the web service succeeded, false otherwise
 */
func apiSetURL(packageName string, version string, newURL string) (int, error) {
	a := "https://npackd.appspot.com/api/set-url?package=" +
		url.QueryEscape(packageName) + "&version=" + url.QueryEscape(version) +
		"&password=" + url.QueryEscape(settings.password) +
		"&url=" + url.QueryEscape(newURL)

	_, statusCode, err := download(a, false)
	return statusCode, err
}

/**
 * Changes the URL for a package version at https://npackd.appspot.com .
 *
 * package_ package name
 * version version number
 * url new URL
 * returns true if the call to the web service succeeded, false otherwise
 */
 func apiDelete(packageName string, version string) error {
	a := "https://npackd.appspot.com/api/set-url?package=" +
		url.QueryEscape(packageName) + "&version=" + url.QueryEscape(version) +
		"&password=" + url.QueryEscape(settings.password)

	// Create client
	client := &http.Client{}

	// Create request
	req, err := http.NewRequest("DELETE", a, nil)
	if err != nil {
		return err
	}

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
	//fmt.Println("response Body : ", string(data))

	if resp.StatusCode/100 != 2 {
		return errors.New("HTTP status: " + strconv.Itoa(resp.StatusCode))
	}

	return nil
}

/**
 * Changes the URL for a package version at https://npackd.appspot.com .
 *
 * package_ package name
 * version version number
 * url new URL
 * tags: set these tags
 * returns: error
 */
func apiSetURLAndHashSum(packageName string, version string, newURL string, newHashSum string, tags []string) error {
	a := "https://npackd.appspot.com/api/set-url?package=" +
		url.QueryEscape(packageName) + "&version=" + url.QueryEscape(version) +
		"&password=" + url.QueryEscape(settings.password) +
		"&url=" + url.QueryEscape(newURL) +
		"&hash-sum=" + url.QueryEscape(newHashSum)

	for _, tag := range(tags) {
		a = a + "&tag=" + url.QueryEscape(tag)
	}

	_, statusCode, err := download(a, false)
	if statusCode != 200 {
		return errors.New("Invalid HTTP code")
	}

	return err
}

/**
 * Changes the URL for a package version at https://npackd.appspot.com .
 *
 * package_ package name
 * version version number
 * url new URL
 * returns: error
 */
func apiCopyPackageVersion(packageName string, version string, newVersion string) error {
	a := "https://npackd.appspot.com/api/copy?package=" +
		packageName + "&from=" + version +
		"&to=" + newVersion + "&password=" + settings.password

	_, statusCode, err := download(a, false)
	if statusCode != 200 {
		return errors.New("Invalid HTTP status code")
	}

	return err
}

/**
 * Processes one package version.
 *
 * package_ package name
 * version version number
 * returns: true if the test was successful
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
		var ai int
		if i < len(a) {
			ai = a[i]
		}
		var bi int
		if i < len(b) {
			bi = b[i]
		}
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
 * onlyNewest true = only test the newest versions
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
		"clone https://github.com/tim-lebedkov/packages.git "+dir, true)
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

func postFile(url string, mime string, path string) error {
	file, err := os.Open(path)
	if err != nil {
		return err
	}
	defer file.Close()

	fi, err := file.Stat()
	if err != nil {
	  	return err
	}

	// Create request
	req, err := http.NewRequest("POST", url, file)
	if err != nil {
		return err
	}

	req.Header.Add("Authorization", "token "+settings.githubToken)
	req.ContentLength = fi.Size()
	req.Header.Set("Content-Type", mime)

	client := &http.Client{}
	resp, err := client.Do(req)
	if err != nil {
		return err
	}

    defer resp.Body.Close()

	data, err := ioutil.ReadAll(resp.Body)

	if resp.StatusCode/100 != 2 {
		fmt.Println(string(data))
		return errors.New("HTTP status " + strconv.Itoa(resp.StatusCode))
	}

    if err != nil {
		fmt.Println(string(data))
        return err
    }
	
	return nil
}

func uploadFileMultipart(url string, path string) (*http.Response, error) {
	f, err := os.OpenFile(path, os.O_RDONLY, 0644)
	if err != nil {
		return nil, err
	}

	// Reduce number of syscalls when reading from disk.
	bufferedFileReader := bufio.NewReader(f)
	defer f.Close()

	// Create a pipe for writing from the file and reading to
	// the request concurrently.
	bodyReader, bodyWriter := io.Pipe()
	formWriter := multipart.NewWriter(bodyWriter)

	// Store the first write error in writeErr.
	var (
		writeErr error
		errOnce  sync.Once
	)
	setErr := func(err error) {
		if err != nil {
			errOnce.Do(func() { writeErr = err })
		}
	}
	go func() {
		partWriter, err := formWriter.CreateFormFile("file", path)
		setErr(err)
		_, err = io.Copy(partWriter, bufferedFileReader)
		setErr(err)
		setErr(formWriter.Close())
		setErr(bodyWriter.Close())
	}()

	req, err := http.NewRequest(http.MethodPost, url, bodyReader)
	if err != nil {
		return nil, err
	}
	req.Header.Add("Content-Type", formWriter.FormDataContentType())

	// This operation will block until both the formWriter
	// and bodyWriter have been closed by the goroutine,
	// or in the event of a HTTP error.
	resp, err := http.DefaultClient.Do(req)

	if writeErr != nil {
		return nil, writeErr
	}

	return resp, err
}

// Download to a file
//
// url: URL for HTTP GET
// path: output file
// Returns: error message
func downloadToFile(url, path string) error {
	fmt.Println("Downloading " + url + " to " + path)

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
	f, err := os.OpenFile(path, os.O_RDWR|os.O_CREATE|os.O_TRUNC, 0644)
	if err != nil {
		return err
	}
	defer f.Close()

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
	settings.githubToken = os.Getenv("GITHUB_TOKEN")
	settings.googleID = os.Getenv("GOOGLE_ID")
	settings.googleSecret = os.Getenv("GOOGLE_SECRET")
	if runtime.GOOS == "windows" {
		settings.npackdcl = "C:\\Program Files\\NpackdCL\\ncl.exe"
		settings.git = "C:\\Program Files\\Git\\cmd\\git.exe"
	} else {
		settings.git = "git"
	}
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
	for i, p := range parts {
		v, err := strconv.Atoi(p)
		if err != nil {
			return nil, err
		}
		res[i] = v
	}

	return res, nil
}

func maxVersion(a []PackageVersion) *PackageVersion {
	var m []int = []int{}
	var res *PackageVersion = nil
	for _, pv := range a {
		v, _ := parseVersion(pv.Name)
		if compareVersions(v, m) > 0 {
			m = v
			res = &pv
		}
	}

	return res
}

// Download an URL and compute the SHA-256 of the data.
//
// address: URL
// return: (SHA-256, error)
func downloadAndHash(address string) ([]byte, error) {
	file, err := ioutil.TempFile("", "prefix")
	if err != nil {
		return nil, err
	}

	defer os.Remove(file.Name())

	err = downloadToFile(address, file.Name())
	if err != nil {
		return nil, err
	}

	f, err := os.Open(file.Name())
	if err != nil {
		return nil, err
	}
	defer f.Close()

	h := sha256.New()
	if _, err := io.Copy(h, f); err != nil {
		return nil, err
	}

	return h.Sum(nil), nil
}

// packageName: this package will be processed
// returns: error or nil
func detect(packageName string) error {
	// now we download the data from the same package, but also with
	// additional fields for discovery
	bytes, _, err := download("https://www.npackd.org/rep/recent-xml?extra=true&package="+packageName, true)
	if err != nil {
		return err
	}

	// parse the repository XML
	rep := Repository{}
	err = xml.Unmarshal(bytes.Bytes(), &rep)
	if err != nil {
		return err
	}

	packageVersions := getPackageVersions(&rep, packageName)
	if len(packageVersions) == 0 {
		return errors.New("No versions found")
	}

	p := getPackage(&rep, packageName)
	if p.DiscoveryPage == "" {
		return errors.New("No discovery page")
	}

	re, err := regexp.Compile(p.DiscoveryRE)
	if err != nil {
		return err
	}

	data, _, err := download(p.DiscoveryPage, true)
	if err != nil {
		return err
	}

	buf := data.Bytes()
	f := re.FindSubmatch(buf)
	if f == nil {
		return errors.New("No match found for the regular expression")
	}
	if len(f) < 2 {
		return errors.New("No first sub-group is found for the regular expression")
	}

	match := string(f[0])

	s := string(f[1])
	s = strings.Replace(s, "-", ".", -1);
	s = strings.Replace(s, "+", ".", -1);
	s = strings.Replace(s, "_", ".", -1);

	// process version numbers like 2.0.6b
	if (len(s) > 0) {
		c, _ := utf8.DecodeLastRuneInString(s)
		if (c >= 'a') && (c <= 'z') {
			s = s[0:len(s) - 1] + "." + strconv.Itoa(int(c) - 'a' + 1);
		}
	}

	newVersion, err := parseVersion(s)
	if err != nil {
		return err
	}

	newVersion = normalizeVersion(newVersion)

	pv := maxVersion(packageVersions)
	version, err := parseVersion(pv.Name)
	if err != nil {
		return err
	}

	if compareVersions(newVersion, version) <= 0 {
		return errors.New("No new version found")
	}

	fmt.Println("Found new version " + versionToString(newVersion))

	err = apiCopyPackageVersion(p.Name, versionToString(version), versionToString(newVersion))
	if err != nil {
		return err
	}

	// only change the URL and hash sum if the tag "same-url" is not present
	if indexOf(p.Tag, "same-url") < 0 {
		// create the new download URL from the template
		downloadURL := pv.URL
		if (len(p.DiscoveryURLPattern) > 0) {
			downloadURL = strings.Replace(p.DiscoveryURLPattern, "${match}", match, -1)
			downloadURL = strings.Replace(downloadURL, "${version}", versionToString(newVersion), -1)

			for i := 0; i < 5; i++ {
				var repl string
				if len(newVersion) > i {
					repl = strconv.Itoa(newVersion[i])
				} else {
					repl = "0"
				}
				downloadURL = strings.Replace(downloadURL, "${v" + strconv.Itoa(i) + "}", repl, -1)
			}
		}

		// compute the check sum
		hashSum := pv.HashSum
		if len(hashSum) > 0 && downloadURL != pv.URL {
			hash, err := downloadAndHash(downloadURL)

			// even if the download fails, we should still update the URL
			if err == nil {
				hashSum = fmt.Sprintf("%x", hash)
			} else {
				fmt.Println("Error downloading/computing hash sum: " + err.Error())
			}
		}

		fmt.Println("Set URL=" + downloadURL + " and hash sum=" + hashSum)
		err = apiSetURLAndHashSum(p.Name, versionToString(newVersion), downloadURL, hashSum, []string{"untested"})
		if err != nil {
			return err
		}
	} else {
		err = apiDelete(p.Name, versionToString(version))
		if err != nil {
			return err
		}
	}

	return nil
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

func getPackage(rep *Repository, packageName string) *Package {
	for _, p := range rep.Package {
		if p.Name == packageName {
			return &p
		}
	}
	return nil
}

// normalize a version number
// version: a non-normalized version number
// return: normalized version number
func normalizeVersion(version []int) []int {
	res := version
	for len(res) > 1 && res[len(res) - 1] == 0 {
		res = res[:len(res)-1]
	}
	return res
}

func versionToString(version []int) string {
	res := ""
	for i, v := range version {
		if i != 0 {
			res = res + "."
		}
		res = res + strconv.Itoa(v)
	}
	return res
}

func detectNewVersions() error {
	fmt.Println("Checking for new package versions")

	reps := []string {"repository/stable.xml", "repository/stable64.xml", "repository/libs.xml"}

	for _, rep := range(reps) {
		dat, err := ioutil.ReadFile(rep)
		if err != nil {
			return err
		}

		// parse the repository XML
		rep := Repository{}
		err = xml.Unmarshal(dat, &rep)
		if err != nil {
			return err
		}

		for i := 0; i < len(rep.Package); i++ {
			p := rep.Package[i]

			fmt.Println("https://www.npackd.org/p/" + p.Name)

			err = detect(p.Name)
			if err != nil {
				fmt.Println(err.Error())
			}
		}
	}

	return nil
}

func maintenance() error {
	err := downloadRepos()
	if err != nil {
		return err
	}

	err = uploadBinariesToGithub()
	if err != nil {
		return err
	}

	err = detectNewVersions()
	if err != nil {
		return err
	}

	return nil
}

// unsafe zone. Here we run code from external sites.
func testPackages() error {
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
func correctURLs() error {
	reps := []string {"repository/stable.xml", "repository/stable64.xml", "repository/unstable.xml", "repository/libs.xml"}

	for _, rep := range(reps) {
		dat, err := ioutil.ReadFile(rep)
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

				prefix := "https://sourceforge.net/projects/"
				if strings.HasPrefix(url, prefix) {
					s := url[len(prefix):]
					parts := strings.Split(s, "/")
					if len(parts) > 1 && parts[1] != "files" {
						url = "https://sourceforge.net/projects/" + parts[0] + "/files/" + s[len(parts[0])+1:]
					}
				}

				prefix = "https://ayera.dl.sourceforge.net/project/"
				if strings.HasPrefix(url, prefix) {
					s := url[len(prefix):]
					parts := strings.Split(s, "/")
					if len(parts) > 1 && parts[1] != "files" {
						url = "https://sourceforge.net/projects/" + parts[0] + "/files/" + s[len(parts[0])+1:]
					}
				}

				prefix = "https://sourceforge.net/projects/"
				if strings.HasPrefix(url, prefix) {
					s := url[len(prefix):]
					parts := strings.Split(s, "/")
					if len(parts) > 1 && parts[1] != "files" {
						url = "https://sourceforge.net/projects/" + parts[0] + "/files/" + s[len(parts[0])+1:]
					}
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
	}

	return nil
}

func saveToFile(data io.Reader, filename string) error {
	// open file
	f, err := os.OpenFile(filename, os.O_RDWR|os.O_CREATE|os.O_TRUNC, 0644)
	if err != nil {
		return err
	}
	defer f.Close()

	// Write the body to file
	_, err = io.Copy(f, data)
	if err != nil {
		return err
	}

	return nil
}

// go get golang.org/x/oauth2
func login() error {
	/*
// Your credentials should be obtained from the Google
	// Developer Console (https://console.developers.google.com).
	conf := &jwt.Config{
		Email: "appveyor@npackd.iam.gserviceaccount.com",
		// The contents of your RSA private key or your PEM file
		// that contains a private key.
		// If you have a p12 file instead, you
		// can use `openssl` to export the private key into a pem file.
		//
		//    $ openssl pkcs12 -in key.p12 -passin pass:notasecret -out key.pem -nodes
		//
		// The field only supports PEM containers with no passphrase.
		// The openssl command will convert p12 keys to passphrase-less PEM containers.
		PrivateKey: []byte("-----BEGIN PRIVATE KEY-----\n.........\n-----END PRIVATE KEY-----\n"),
		Scopes: []string{
			"https://www.googleapis.com/auth/userinfo.email",
		},
		TokenURL: google.JWTTokenURL,
		// If you would like to impersonate a user, you can
		// create a transport with a subject. The following GET
		// request will be made on the behalf of user@example.com.
		// Optional.
		//Subject: "tim.lebedkov@gmail.com",
	}

	// Initiate an http.Client, the following GET request will be
	// authorized and authenticated on the behalf of user@example.com.
	client := conf.Client(oauth2.NoContext)
	resp, err := client.Get("https://www.npackd.org/api/notify?package=org.7-zip.SevenZIP64&version=9.20&install=1&success=1")
	if err != nil {
		return err
	}
	defer resp.Body.Close()

	saveToFile(resp.Body, "output.html")

	// Display Results
	//fmt.Println("response Status : ", resp.Status)
	//fmt.Println("response Headers : ", resp.Header)
	//fmt.Println("response Body : ", string(respBody))

	if resp.StatusCode/100 != 2 {
		return errors.New("HTTP status " + strconv.Itoa(resp.StatusCode))
	}
*/
	return nil
}

var command = flag.String("command", "test-packages", "the action that should be performed")
var target = flag.String("target", "", "directory where the downloaded binaries are stored")

// Download binaries from Github to a directory:
// go run TestUnstableRep.go TestUnstableRep_linux.go -command download-binaries -target /target/directory
//
// Correct URLs for packages at npackd.org:
// PASSWORD=xxx go run TestUnstableRep.go TestUnstableRep_linux.go -command correct-urls
//
// Download repositories from npackd.org to github.com/npackd/npackd, re-upload packages to github.com/tim-lebedkov/packages:
// PASSWORD=xxx GITHUB_TOKEN=xxx go run TestUnstableRep.go TestUnstableRep_linux.go -command maintenance
//
// Test packages on AppVeyor:
// PASSWORD=xxx go run TestUnstableRep.go TestUnstableRep_linux.go
func main() {
	var err error = nil

	flag.Parse()

	rand.Seed(time.Now().UnixNano())

	createSettings()

	if *command == "login" {
		err = login()
	} else if *command == "download-binaries" {
		err = downloadBinaries(*target)
	} else if *command == "upload-binaries" {
		err = uploadBinariesToGithub()
	} else if *command == "correct-urls" {
		err = correctURLs()
	} else if *command == "maintenance" {
		err = maintenance()
	} else {
		err = testPackages()
	}

	if err != nil {
		fmt.Println(err.Error())
		os.Exit(1)
	}
}
