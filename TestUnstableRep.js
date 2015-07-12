var arguments = WScript.Arguments; 
var password = arguments.Named.Item("password");
WScript.Echo("password=" + password);

//var npackdcl = "C:\\ProgramFiles\\NpackdCL\\npackdcl.exe";
//var npackdcl = "C:\\Program Files\\NpackdCL\\npackdcl.exe";
// var npackdcl = "C:\\ProgramFiles\\NpackdCL-1.19.13\\npackdcl.exe";
var npackdcl = "C:\\Program Files (x86)\\NpackdCL\\ncl.exe";

function exec(cmd) {
	return exec2("cmd.exe /c " + cmd + " 2>&1");
}

/**
 * Executes the specified command, prints its output on the default output.
 *
 * @param cmd this command should be executed
 * @return exit code
 */
function exec2(cmd) {
    var shell = WScript.CreateObject("WScript.Shell");
    var p = shell.exec(cmd);
	while (!p.StdOut.AtEndOfStream) {
		var line = p.StdOut.ReadLine();
		WScript.Echo(line);
	}

    return p.ExitCode;
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
        return false;
    }

    var ec = exec("\"" + npackdcl + "\" remove --package="+package_
                + " --version=" + version);
    if (ec !== 0) {
        WScript.Echo("npackdcl.exe remove failed");
        return false;
    }
    return true;
}

var FSO = new ActiveXObject("Scripting.FileSystemObject");
var NF = 2;

function countPackageFiles(Folder) {
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

var ec = exec("\"" + npackdcl + "\" detect");
if (ec !== 0) {
    WScript.Echo("npackdcl.exe detect failed: " + ec);
    WScript.Quit(1);
}

var xDoc = new ActiveXObject("MSXML2.DOMDocument.6.0");
xDoc.async = false;
xDoc.setProperty("SelectionLanguage", "XPath");
if (xDoc.load("http://npackd.appspot.com/rep/recent-xml?tag=untested")) {
    var pvs = xDoc.selectNodes("//version");
    // WScript.Echo(pvs.length + " versions found");
    var failed = [];

    for (var i = 0; i < pvs.length; i++) {
        var pv = pvs[i];
        var package_ =pv.getAttribute("package");
        var version = pv.getAttribute("name");
        WScript.Echo(package_ + " " + version);
        if (!process(package_, version)) {
            failed.push(package_ + "@" + version);
        } else {
			if (download("http://npackd.appspot.com/package-version/mark-tested?package=" + 
					package_ + "&version=" + version + 
					"&password=" + password)) {
				WScript.Echo(package_ + " " + version + " was marked as tested");
			} else {
				WScript.Echo("Failed to mark " + package_ + " " + version + " as tested");
			}
		}
    }

    if (failed.length > 0) {
        WScript.Echo(failed.length + " packages failed:");
        for (var i = 0; i < failed.length; i++) {
            WScript.Echo(failed[i]);
        }
        WScript.Quit(1);
    } else {
        WScript.Echo("All packages are OK");
    }
} else {
    WScript.Echo("Error loading XML");
    WScript.Quit(1);
}


