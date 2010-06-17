package com.googlecode.wpm;

import java.awt.BorderLayout;
import java.beans.PropertyChangeEvent;
import java.beans.PropertyChangeListener;
import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.net.Proxy;
import java.net.ProxySelector;
import java.net.URISyntaxException;
import java.net.URL;
import java.net.URLConnection;
import java.util.List;
import javax.swing.BorderFactory;
import javax.swing.JFrame;
import javax.swing.JOptionPane;
import javax.swing.JTabbedPane;
import javax.swing.SwingUtilities;
import javax.swing.UIManager;
import javax.swing.border.Border;

/**
 * wpm
 */
class App {
    private static JFrame mainFrame;

    /**
     * Rethrows an exception as an I/O exception
     *
     * @param ex cause
     * @throws IOException ex
     */
    private static void rethrowAsIOException(URISyntaxException ex) 
            throws IOException {
        throw (IOException) new IOException().initCause(ex);
    }

    /**
     * Opens an URL using all the defined proxies.
     * 
     * @param url an URL
     * @return stream
     */
    public static InputStream readURL(URL url) throws IOException {
        InputStream r = null;
        try {
            List<Proxy> proxies = ProxySelector.getDefault().
                    select(url.toURI());
            URLConnection urlc = url.openConnection(proxies.get(0));
            r = urlc.getInputStream();
        } catch (URISyntaxException ex) {
            App.rethrowAsIOException(ex);
        }
        return r;
    }

    /**
     * @return program files directory
     */
    static File getProgramFilesDir() {
        return new File(System.getenv("ProgramFiles"));
    }

    public static void main(String[] args) {
        try {
            UIManager.setLookAndFeel(UIManager.getSystemLookAndFeelClassName());
        } catch (Throwable ex) {
            App.unexpectedWarn(ex);
        }

        if (Repository.getLocation() == null) {
            App.informUser(new Exception(
                    "The package repository is not set up. " +
                    "Please configure it on the Settings tab."));
        }

        mainFrame = new JFrame("Windows Package Manager");
        mainFrame.setSize(500, 400);
        mainFrame.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);

        final MainPanel mp = new MainPanel();

        SettingsPanel sp = new SettingsPanel();
        sp.addPropertyChangeListener("repositoryLocation",
                new PropertyChangeListener() {
            @Override
            public void propertyChange(PropertyChangeEvent evt) {
                mp.reloadRepository();
            }
        });

        JTabbedPane tp = new JTabbedPane();
        tp.addTab("Packages", mp);
        tp.addTab("Settings", sp);

        final Border border = BorderFactory.createEmptyBorder(11, 11, 12, 12);
        mp.setBorder(border);
        sp.setBorder(border);

        mainFrame.getContentPane().setLayout(new BorderLayout());
        mainFrame.getContentPane().add(tp, BorderLayout.CENTER);
        mainFrame.setVisible(true);
        SwingUtilities.invokeLater(new Runnable() {
            @Override
            public void run() {
                mp.reloadRepository();
            }
        });
    }

    /**
     * Informs a user about an error (not an internal program error).
     *
     * @param t an error
     */
    static void informUser(Throwable t) {
        t.printStackTrace();
        if (t instanceof RuntimeException) {
            if (t.getCause() != null)
                t = t.getCause();
        }
        JOptionPane.showMessageDialog(mainFrame, t.getMessage(), "Error",
                JOptionPane.ERROR_MESSAGE);
    }

    /**
     * Rethrows an exception as a RuntimeException
     *
     * @param ex an exception
     */
    static void rethrowAsRuntimeException(Throwable ex) {
        throw (RuntimeException) new RuntimeException(ex);
    }

    /**
     * Handles an unexpected exception that does not affect the program
     * execution.
     * 
     * @param e an exception
     */
    static void unexpectedWarn(Throwable e) {
        e.printStackTrace();
    }
}
