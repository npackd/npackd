package com.googlecode.wpm;

import java.io.IOException;
import java.io.InputStream;
import java.net.MalformedURLException;
import java.net.URL;
import java.util.ArrayList;
import java.util.List;
import java.util.prefs.Preferences;
import javax.xml.parsers.DocumentBuilder;
import javax.xml.parsers.DocumentBuilderFactory;
import javax.xml.parsers.ParserConfigurationException;
import org.w3c.dom.Document;
import org.w3c.dom.Element;
import org.w3c.dom.NodeList;
import org.xml.sax.SAXException;

/**
 * A repository with packages.
 */
public class Repository {
    private static int[] parseVersion(String version) {
        String[] p = version.split("\\.");
        int[] r = new int[p.length];
        for (int i = 0; i < r.length; i++) {
            r[i] = Integer.parseInt(p[i]);
        }
        return r;
    }

    /**
     * @return URL of the repository XML or null if unknown
     */
    public static URL getLocation() {
        Preferences p = Preferences.userNodeForPackage(Repository.class);
        String s = p.get("repositoryURL", null);
        if (s == null)
            return null;
        else {
            try {
                return new URL(s);
            } catch (MalformedURLException ex) {
                App.unexpectedWarn(ex);
                return null;
            }
        }
    }

    /**
     * @param url new repository XML location or null if unknown
     */
    public static void setLocation(URL url) {
        Preferences p = Preferences.userNodeForPackage(Repository.class);
        if (url == null)
            p.remove("repositoryURL");
        else
            p.put("repositoryURL", url.toExternalForm());
    }

    /** available package versions */
    public List<PackageVersion> packageVersions =
            new ArrayList<PackageVersion>();

    /**
     * Loads a repository from an XML file.
     *
     * @param url URL of the repository
     * @return the repository
     * @throws IOException if something goes wrong
     */
    public static Repository load(URL url) throws IOException {
        try {
            InputStream is = url.openStream();
            DocumentBuilder db = DocumentBuilderFactory.newInstance().newDocumentBuilder();
            Document d = db.parse(is);
            Element root = d.getDocumentElement();
            Repository r = new Repository();
            NodeList elements = root.getElementsByTagName("version");
            for (int j = 0; j < elements.getLength(); j++) {
                Element v = (Element) elements.item(j);
                PackageVersion pv = new PackageVersion();
                pv.package_ = v.getAttribute("package");
                pv.version = parseVersion(v.getAttribute("name"));
                final String nodeValue = v.getElementsByTagName("url").
                        item(0).getFirstChild().getNodeValue();
                pv.url = new URL(
                        nodeValue);
                r.packageVersions.add(pv);
            }
            return r;
        } catch (SAXException ex) {
            throw (IOException) new IOException().initCause(ex);
        } catch (ParserConfigurationException ex) {
            throw (IOException) new IOException().initCause(ex);
        }
    }

    /**
     * Updates the "installed" status of all package versions from this
     * repository.
     */
    public void updateInstalledStatus() {
        for (PackageVersion pm: this.packageVersions) {
            pm.refreshInstalled();
            pm.isInstalled();
        }
    }
}
