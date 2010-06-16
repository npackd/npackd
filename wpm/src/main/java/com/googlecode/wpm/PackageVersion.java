package com.googlecode.wpm;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.URL;
import java.util.zip.ZipEntry;
import java.util.zip.ZipInputStream;

/**
 * A version of a package.
 */
public class PackageVersion {
    /** name of the package */
    public String package_;

    /** package version */
    public int[] version;

    /** download URL */
    public URL url;

    /** is this package version installed? */
    private boolean installed;
    
    private boolean installedValid;

    /**
     * @return true if this package version is already installed
     */
    public boolean isInstalled() {
        if (!installedValid) {
            File d = getDirectory();
            installed = d.exists() && d.isDirectory();
            installedValid = true;
        }
        return installed;
    }

    /**
     * Installs this package version.
     *
     * @param job a job for this method
     */
    public void install(Job job) throws IOException {
        if (!installed) {
            File tmp = File.createTempFile("wpm", ".zip");
            try {
                FileOutputStream os = new FileOutputStream(tmp);
                try {
                    job.setHint("Starting the download");
                    InputStream is = url.openStream();
                    try {
                        job.setHint("Downloading");
                        copy(is, os, job.createSubJob());
                    } finally {
                        try {
                            is.close();
                        } catch (IOException e) {
                            App.unexpectedWarn(e);
                        }
                    }
                } finally {
                    try {
                        os.close();
                    } catch (IOException e) {
                        App.unexpectedWarn(e);
                    }
                }

                job.setHint("Unpacking");
                unzip(new FileInputStream(tmp), job.createSubJob());
                this.installed = true;
                this.installedValid = true;
            } catch (IOException e) {
                try {
                    uninstall();
                } catch (Throwable t) {
                    App.unexpectedWarn(t);
                }
                throw e;
            }
        }
    }

    @Override
    public String toString() {
        return package_ + ' ' + getVersionAsString() +
                (installed ? " [installed]" : "");
    }

    /**
     * @return version
     */
    public String getVersionAsString() {
        StringBuilder sb = new StringBuilder();
        for (int i = 0; i < version.length; i++) {
            if (i != 0)
                sb.append('.');
            sb.append(version[i]);
        }
        return sb.toString();
    }

    /**
     * Resets the internal flag so that the next time {@link #isInstalled()}
     * is called it does not use the internal cache.
     */
    public void refreshInstalled() {
        this.installedValid = false;
    }

    /**
     * @return directory where this package version is installed
     */
    public File getDirectory() {
        return new File(App.getProgramFilesDir(), "wpm\\" +
                this.package_ + '-' +
                this.getVersionAsString());
    }

    /**
     * Unzips a package version.
     *
     * @param source .zip (will be closed)
     * @param job a Job for this method
     * @throws IOException if something goes wrong
     */
    private void unzip(InputStream source, Job job) throws IOException {
        try {
            File projectRoot = getDirectory();
            job.setHint("Opening ZIP file");
            ZipInputStream str = new ZipInputStream(source);
            ZipEntry entry;
            int last = 0;
            int total = 0;
            while ((entry = str.getNextEntry()) != null) {
                total++;
                if (total - last > 100) {
                    job.setHint(total + " files extracted");
                    last = total;
                }
                final File file = new File(projectRoot, entry.getName());
                if (entry.isDirectory()) {
                    if (!file.mkdirs())
                        throw new IOException("Cannot create directory: " +
                                file);
                } else {
                    OutputStream out = new FileOutputStream(file);
                    try {
                        copy(str, out, job.createSubJob());
                    } finally {
                        try {
                            out.close();
                        } catch (IOException e) {
                            App.unexpectedWarn(e);
                        }
                    }
                }
            }
        } finally {
            try {
                source.close();
            } catch (IOException e) {
                App.unexpectedWarn(e);
            }
        }
    }

    /**
     * Copies one stream to another.
     *
     * @param is input
     * @param os output
     * @param job a Job for this method
     * @throws IOException if something goes wrong
     */
    private static void copy(InputStream is, OutputStream os, Job job)
            throws IOException {
        byte[] buf = new byte[1024 * 1024];
        int read;
        long last = 0;
        long total = 0;
        while ((read = is.read(buf)) != -1) {
            os.write(buf, 0, read);
            total += read;
            if (total - last > 1024 * 1024) {
                job.setHint(total + " bytes copied");
                last = total;
            }
        }
    }

    /**
     * Uninstallation
     */
    public void uninstall() throws IOException {
        delete(getDirectory());
        this.installed = false;
        this.installedValid = true;
    }

    /**
     * Deletes a file or a directory.
     * 
     * @param f a file or a directory
     * @throws IOException if something goes wrong
     */
    private void delete(File f) throws IOException {
        if (f.isDirectory()) {
            File[] ch = f.listFiles();
            if (ch != null) {
                for (File chf: ch)
                    delete(chf);
            }
        }
        if (!f.delete())
            throw new IOException("Cannot delete a file: " + f);
    }
}
