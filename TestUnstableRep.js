//var npackdcl = "C:\\ProgramFiles\\NpackdCL\\npackdcl.exe";
//var npackdcl = "C:\\Program Files\\NpackdCL\\npackdcl.exe";
// var npackdcl = "C:\\ProgramFiles\\NpackdCL-1.19.13\\npackdcl.exe";
var npackdcl = "C:\\Program Files (x86)\\NpackdCL\\ncl.exe";
//var npackdcl = "C:\\ProgramFiles\\NpackdCL-1.20.5\\ncl.exe";

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
 * Processes one package version.
 *
 * @param package_ package name
 * @param version version number
 * @return true if the test was successful
 */
function process(package_, version) {
    var ec = exec("\"" + npackdcl + "\" add --package="+package_
                + " --version=" + version);
    if (ec !== 0) {
        WScript.Echo("npackdcl.exe add failed");
        apiNotify(package_, version, true, false);
		
		exec("\"" + npackdcl + "\" add -d --package="+package_
					+ " --version=" + version);
        return false;
    }
    apiNotify(package_, version, true, true);

    var path = getPath(package_, version);
    WScript.Echo("where=" + path);
    if (path !== "") {
        var tree = package_ + "-" + version + "-tree.txt";
        exec2("cmd.exe /c tree \"" + path + "\" > " + tree + " 2>&1");
        exec("appveyor PushArtifact " + tree);

        exec("cmd.exe /c dir \"" + path + "\"");

        var msilist = package_ + "-" + version + "-msilist.txt";
        exec2("cmd.exe /c \"C:\\Program Files (x86)\\CLU\\clu.exe\" list-msi > " + msilist + " 2>&1");
        exec("appveyor PushArtifact " + msilist);

        var proglist = package_ + "-" + version + "-proglist.txt";
        exec2("cmd.exe /c \"C:\\Program Files (x86)\\Sysinternals suite\\psinfo.exe\" -s /accepteula > " + proglist + " 2>&1");
        exec("appveyor PushArtifact " + proglist);
    }

    var ec = exec("\"" + npackdcl + "\" remove -e=ck --package="+package_
                + " --version=" + version);
    if (ec !== 0) {
        WScript.Echo("npackdcl.exe remove failed");
        apiNotify(package_, version, false, false);

		exec("\"" + npackdcl + "\" remove -d -e=ck --package="+package_
					+ " --version=" + version);
        return false;
    }
    apiNotify(package_, version, false, true);
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

function processURL(url, password) {
    var ignored = ["org.bitbucket.tortoisehg.TortoiseHg"];

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

        shuffle(pvs);

        // WScript.Echo(pvs.length + " versions found");
        var failed = [];

        for (var i = 0; i < pvs.length; i++) {
            var pv = pvs[i];
            var package_ =pv.getAttribute("package");
            var version = pv.getAttribute("name");

            WScript.Echo(package_ + " " + version);
            if (ignored.contains(package_)) {
                WScript.Echo("The package version " + package_ + " " + version + " was ignored.");
            } else if (!process(package_, version)) {
                failed.push(package_ + "@" + version);
            } else {
                if (download("https://npackd.appspot.com/package-version/mark-tested?package=" +
                        package_ + "&version=" + version +
                        "&password=" + password)) {
                    WScript.Echo(package_ + " " + version + " was marked as tested");
                } else {
                    WScript.Echo("Failed to mark " + package_ + " " + version + " as tested");
                }
            }
            WScript.Echo("==================================================================");
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
//  WScript.Echo("password=" + password);

var ec = exec("\"" + npackdcl + "\" detect");
if (ec !== 0) {
    WScript.Echo("npackdcl.exe detect failed: " + ec);
    WScript.Quit(1);
}

processURL("https://npackd.appspot.com/rep/recent-xml?tag=untested", password);

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

processURL("http://npackd.appspot.com/rep/xml?tag=" + reps[index], password);

