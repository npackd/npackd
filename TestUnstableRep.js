var npackdcl = "C:\\Program Files (x86)\\NpackdCL\\ncl.exe";

var FSO = new ActiveXObject("Scripting.FileSystemObject");

Array.prototype.contains = function(v) {
    for (var i = 0; i < this.length; i++) {
        if (this[i] == v)
            return true;
    }
    return false;
}

// http://stackoverflow.com/questions/6274339/how-can-i-shuffle-an-array-in-javascript
function shuffle(array) {
    var counter = array.length, temp, index;

    // While there are elements in the array
    while (counter > 0) {
        // Pick a random index
        index = Math.floor(Math.random() * counter);

        // Decrease counter by 1
        counter--;

        // And swap the last element with it
        temp = array[counter];
        array[counter] = array[index];
        array[index] = temp;
    }

    return array;
}

function exec(cmd) {
    var result = exec2("cmd.exe /c " + cmd + " 2>&1");
    return result[0];
}

/**
 * @param package_ full package name
 * @param version version number
 * @return path to the specified package or "" if not installed
 */
function getPath(package_, version) {
    var res = exec2("cmd.exe /c \"" + npackdcl + "\" path -p " + package_ +
            " -v " + version + " 2>&1");
    var lines = res[1];
    if (lines.length > 0)
        return lines[0];
    else
        return "";
}

/**
 * Executes the specified command, prints its output on the default output.
 *
 * @param cmd this command should be executed
 * @return [exit code, [output line 1, output line2, ...]]
 */
function exec2(cmd) {
    var shell = WScript.CreateObject("WScript.Shell");
    var p = shell.exec(cmd);
    var output = [];
    while (!p.StdOut.AtEndOfStream) {
        var line = p.StdOut.ReadLine();
        WScript.Echo(line);
        output.push(line);
    }

    return [p.ExitCode, output];
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
function apiNotify(package_, version, install, success) {
    download("https://npackd.appspot.com/api/notify?package=" +
            package_ + "&version=" + version +
            "&password=" + password +
            "&install=" + (install ? "1" : "0") + 
            "&success=" + (success ? "1" : "0"));
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
function apiTag(package_, version, tag, set) {
    return download("https://npackd.appspot.com/api/tag?package=" +
            package_ + "&version=" + version +
            "&password=" + password +
            "&name=" + tag + 
            "&value=" + (set ? "1" : "0"));
}

/**
 * Processes one package version.
 *
 * @param package_ package name
 * @param version version number
 * @return true if the test was successful
 */
function process(package_, version) {
    var ec = exec("\"" + npackdcl + "\" add --package="+package_
                + " --version=" + version + " -t 600");
    if (ec !== 0) {
        WScript.Echo("npackdcl.exe add failed");
        apiNotify(package_, version, true, false);
        apiTag(package_, version, "test-failed", true);
		
        var log = package_ + "-" + version + "-install.log";
        exec("cmd.exe /C \"" + npackdcl + "\" add -d --package="+ package_
					+ " --version=" + version + " -t 600 > " + log + " 2>&1");
        exec("appveyor PushArtifact " + log);
		
        return false;
    }
    apiNotify(package_, version, true, true);

    var path = getPath(package_, version);
    WScript.Echo("where=" + path);
    if (path !== "") {
        var tree = package_ + "-" + version + "-tree.txt";
        exec2("cmd.exe /c tree \"" + path + "\" /F > " + tree + " 2>&1");
        exec("appveyor PushArtifact " + tree);

        exec("cmd.exe /c dir \"" + path + "\"");

        var msilist = package_ + "-" + version + "-msilist.txt";
        exec2("cmd.exe /c \"C:\\Program Files (x86)\\CLU\\clu.exe\" list-msi > " + msilist + " 2>&1");
        exec("appveyor PushArtifact " + msilist);

        var info = package_ + "-" + version + "-info.txt";
        exec("cmd.exe /C \"" + npackdcl + "\" info --package="+ package_
					+ " --version=" + version + " > " + info + " 2>&1");
        exec("appveyor PushArtifact " + info);

        var list = package_ + "-" + version + "-list.txt";
        exec("cmd.exe /C \"" + npackdcl + "\" list > " + list + " 2>&1");
        exec("appveyor PushArtifact " + list);

        var proglist = package_ + "-" + version + "-proglist.txt";
        exec2("cmd.exe /c \"C:\\Program Files (x86)\\Sysinternals_suite\\psinfo.exe\" -s /accepteula > " + proglist + " 2>&1");
        exec("appveyor PushArtifact " + proglist);
    }

    var ec = exec("\"" + npackdcl + "\" remove -e=ck --package="+package_
                + " --version=" + version + " -t 600");
    if (ec !== 0) {
        WScript.Echo("npackdcl.exe remove failed");
        apiNotify(package_, version, false, false);
        apiTag(package_, version, "test-failed", true);

        var log = package_ + "-" + version + "-uninstall.log";
        exec("cmd.exe /C \"" + npackdcl + 
                "\" remove -d -e=ck --package=" + package_
                + " --version=" + version + " -t 600 > " + log + " 2>&1");
        exec("appveyor PushArtifact " + log);
		
        return false;
    }
    apiNotify(package_, version, false, true);
    apiTag(package_, version, "test-failed", false);
    return true;
}

function countPackageFiles(Folder) {
    var NF = 2;
    var n = 0;

    for (var objEnum = new Enumerator(Folder.Files); !objEnum.atEnd(); objEnum.moveNext()) {
        n++;
        if (n > NF)
            break;
    }


    if (n <= NF) {
        for (var objEnum = new Enumerator(Folder.SubFolders); !objEnum.atEnd(); objEnum.moveNext()) {
            if (n > NF)
                break;

            var d = objEnum.item();
            if (d.Name.toLowerCase() !== ".npackd") {
                n += countPackageFiles(d);
            }
        }
    }

    return n;
}

/**
 * Downloads a file.
 *
 * @param url URL
 * @return true if the download was OK
 */
function download(url) {
    var Object = WScript.CreateObject('MSXML2.XMLHTTP');

    Object.Open('GET', url, false);
    Object.Send();

    return Object.Status == 200;
}

/**
 * @param a first version as a String
 * @param b first version as a String
 */
function compareVersions(a, b) {
	a = a.split(".");
	b = b.split(".");
	
	var len = Math.max(a.length, b.length);
	
	var r = 0;
	
	for (var i = 0; i < len; i++) {
		var ai = 0;
		var bi = 0;
		
		if (i < a.length)
			ai = parseInt(a[i]);
		if (i < b.length)
			bi = parseInt(b[i]);

		// WScript.Echo("comparing " + ai + " and " + bi);
		
		if (ai < bi) {
			r = -1;
			break;
		} else if (ai > bi) {
			r = 1;
			break;
		}
	}
	
	return r;
}

/**
 * @param onlyNewest true = only test the newest versions
 */
function processURL(url, password, onlyNewest) {
    var start = new Date();
    
    var xDoc = new ActiveXObject("MSXML2.DOMDocument.6.0");
    xDoc.async = false;
    xDoc.setProperty("SelectionLanguage", "XPath");
    WScript.Echo("Loading " + url);
    if (xDoc.load(url)) {
        var pvs_ = xDoc.selectNodes("//version");

        // copy the nodes into a real array
        var pvs = [];
        for (var i = 0; i < pvs_.length; i++) {
            pvs.push(pvs_[i]);
        }

		// only retain newest versions for each package
		if (onlyNewest) {
			WScript.Echo("Only testing the newest versions out of " + pvs.length);
			var newest = {};
			for (var i = 0; i < pvs.length; i++) {
				var pvi = pvs[i];
				var pvip = pvi.getAttribute("package");
				var pvj = newest[pvip];
				
				if (((typeof pvj) === "undefined") ||
						compareVersions(pvi.getAttribute("name"), 
						pvj.getAttribute("name")) > 0) {
					newest[pvip] = pvi;
				}
			}
			
			pvs = [];
			for (var key in newest) {
				pvs.push(newest[key]);
				
				/*
				WScript.Echo("Newest: " + newest[key].getAttribute("package") + 
						" " +
						newest[key].getAttribute("name"));
				*/
			}

			WScript.Echo("Only the newest versions: " + pvs.length);
		}
		
        shuffle(pvs);

        // WScript.Echo(pvs.length + " versions found");
        var failed = [];

        for (var i = 0; i < pvs.length; i++) {
            var pv = pvs[i];
            var package_ = pv.getAttribute("package");
            var version = pv.getAttribute("name");

            WScript.Echo(package_ + " " + version);
            if (!process(package_, version)) {
                failed.push(package_ + "@" + version);
            } else {
                if (apiTag(package_, version, "untested", false)) {
                    WScript.Echo(package_ + " " + version + " was marked as tested");
                } else {
                    WScript.Echo("Failed to mark " + package_ + " " + version + " as tested");
                }
            }
            WScript.Echo("==================================================================");

	    if ((new Date()).getTime() - start.getTime() > 45 * 60 * 1000) {
		break;
	    }
        }

        if (failed.length > 0) {
            WScript.Echo(failed.length + " packages failed:");
            for (var i = 0; i < failed.length; i++) {
                WScript.Echo(failed[i]);
            }
            return 1;
        } else {
            WScript.Echo("All packages are OK in " + url);
        }
    } else {
        WScript.Echo("Error loading XML");
        return 1;
    }

    return 0;
}

var arguments = WScript.Arguments;
var password = arguments.Named.Item("password");
var githubToken = arguments.Named.Item("github_token");
//  WScript.Echo("password=" + password);

// so that the password is not printed
var shell = WScript.CreateObject("WScript.Shell");
var oExec = shell.Exec("\"C:\\Program Files\\Git\\usr\\bin\\git\" push https://tim-lebedkov:" + githubToken +
     "@github.com/tim-lebedkov/npackd.git");
while (oExec.Status == 0) {
     WScript.Sleep(100);
}
WScript.Echo("git push returned " + oExec.Status);

var ec = exec("\"" + npackdcl + "\" detect");
if (ec !== 0) {
    WScript.Echo("npackdcl.exe detect failed: " + ec);
    WScript.Quit(1);
}

exec("\"" + npackdcl + "\" help");

processURL("https://npackd.appspot.com/rep/recent-xml?tag=untested", 
		password, false);

var reps = ["stable", "stable64", "libs"];

// the stable repository is about 3900 KiB
// and should be tested more often
var index = Math.floor(Math.random() * 4000);
if (index < 3000)
	index = 0;
else if (index < 3900)
	index = 1;
else
	index = 2;

// 9 of 10 times only check the newest versions
var newest = Math.random() < 0.9;

processURL("http://npackd.appspot.com/rep/xml?tag=" + reps[index], password, 
		newest);

