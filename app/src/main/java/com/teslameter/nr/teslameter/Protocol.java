package com.teslameter.nr.teslameter;

/**
 * Created by nenad on 5.7.17..
 */

public class Protocol {
    public Protocol() {
        this.shouldExit = false;
    }

    public void open() {
        this.protocolOpen();

        this.readerTask = new MyRunnable(this) {
            @Override
            public void run() {
                while (!this.protocol.shouldExit) {
                    int character = this.protocol.protocolRdByte();

                    if (character < 0) {
                        this.protocol.shouldExit = true;
                        break;
                    } else {
                        this.protocol.process_input(character);
                    }
                }
            }
        };
        this.readerThread = new Thread(this.readerTask);
        this.readerThread.start();
    }

    public void close() {
        protocolClose();
    }

    private abstract class MyRunnable implements Runnable {
        public Protocol protocol;

        public MyRunnable(Protocol protocol) {
            this.protocol = protocol;
        }
    };

    private MyRunnable readerTask;
    private Thread readerThread;
    private volatile boolean shouldExit;

    private void process_input(int character) {

    }
    // Protocol library functions
    private native int protocolOpen();
    private native int protocolClose();
    private native int protocolRdByte();
    private native int protocolWrBuf(int [] buf);

    // Used to load the 'protocol' library on application startup.
    static {
        System.loadLibrary("protocol");
    }
}
