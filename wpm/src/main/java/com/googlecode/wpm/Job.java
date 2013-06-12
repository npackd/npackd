package com.googlecode.wpm;

import java.beans.PropertyChangeEvent;
import java.beans.PropertyChangeListener;
import java.beans.PropertyChangeSupport;

/**
 * A long running job.
 */
public class Job {
    private int progress;
    private PropertyChangeSupport propertyChangeSupport =
            new PropertyChangeSupport(this);
    private String hint;
    public static final String PROP_HINT = "hint";
    public static final String PROP_PROGRESS = "progress";

    /**
     * @return a job that is a part of this one
     */
    public Job createSubJob() {
        final Job job = new Job();
        job.addPropertyChangeListener(new PropertyChangeListener() {
            private String start;

            @Override
            public void propertyChange(PropertyChangeEvent evt) {
                if (start == null)
                    start = Job.this.getHint();
                Job.this.setHint(start + "/" + job.getHint());
            }
        });
        return job;
    }

    /**
     * Get the value of hint
     *
     * @return the value of hint
     */
    public String getHint() {
        return hint;
    }

    /**
     * Set the value of hint
     *
     * @param hint new value of hint
     */
    public void setHint(String hint) {
        String oldHint = this.hint;
        this.hint = hint;
        propertyChangeSupport.firePropertyChange(PROP_HINT, oldHint, hint);
    }

    /**
     * Get the value of progress
     *
     * @return the value of progress
     */
    public int getProgress() {
        return progress;
    }

    /**
     * Set the value of progress
     *
     * @param progress new value of progress
     */
    public void setProgress(int progress) {
        this.progress = progress;
    }

    /**
     * Add PropertyChangeListener.
     *
     * @param listener
     */
    public void addPropertyChangeListener(PropertyChangeListener listener) {
        propertyChangeSupport.addPropertyChangeListener(listener);
    }

    /**
     * Remove PropertyChangeListener.
     *
     * @param listener
     */
    public void removePropertyChangeListener(PropertyChangeListener listener) {
        propertyChangeSupport.removePropertyChangeListener(listener);
    }
}
