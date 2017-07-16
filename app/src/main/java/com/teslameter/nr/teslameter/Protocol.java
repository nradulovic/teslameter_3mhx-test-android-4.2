package com.teslameter.nr.teslameter;
import android.util.Log;

import java.io.IOException;

/**
 * Created by nenad on 5.7.17..
 */


/*
 * import static android.content.ContentValues.TAG;
 */



public class Protocol {
    public Protocol(CdiManager cdiManager) {
        this.shouldExit = false;
        this.cdiManager = cdiManager;
    }

    public void open() throws IOException {
        int status;
        status = this.protocolOpen();

        if (status != 0) {
            Log.e(Protocol.TAG, String.format("protocolOpen() = %d", status));
            throw new IOException();
        }
    }
    public void process_events() {
        this.readerTask = new MyRunnable(this) {
            @Override
            public void run() {
                int status;

                status = this.protocol.protocolWrBuf(new int[] {78, 69, 83, 65});

                if (status != 0) {
                    Log.e(Protocol.TAG, String.format("protocolWrBuff('hello') = %d", status));
                }

                while (!this.protocol.shouldExit) {
                    status = this.protocol.protocolWrBuf(new int[] {78, 69, 83, 65});

                    if (status != 0) {
                        Log.e(Protocol.TAG, String.format("protocolWrBuff('hello') = %d", status));
                    }
                    try {
                        Thread.sleep(2000);
                    } catch (InterruptedException e) {
                        e.printStackTrace();
                    }
                }
                while (!this.protocol.shouldExit) {
                    int[] response;
                    int character = this.protocol.protocolRdByte();

                    if (character < 0) {
                        Log.e(Protocol.TAG, String.format("protocolRdByte() = %d", character));
                        this.protocol.shouldExit = true;
                        break;
                    } else {
                        response = this.protocol.process_input(this.protocol, character);
                    }

                    if (response.length > 0) {
                        status = this.protocol.protocolWrBuf(response);

                        if (status != 0) {
                            Log.e(Protocol.TAG, String.format("protocolWrBuf() = %d", status));
                        }
                    }
                }
            }
        };
        this.readerThread = new Thread(this.readerTask);
        this.readerThread.start();
    }

    public void close() {
        this.shouldExit = true;
        try {
            this.readerThread.join();
        } catch (InterruptedException e) {
            e.printStackTrace();
        }
        protocolClose();
    }

    private abstract class MyRunnable implements Runnable {
        public Protocol protocol;

        public MyRunnable(Protocol protocol) {
            this.protocol = protocol;
        }
    };
    private CdiManager cdiManager;
    private MyRunnable readerTask;
    private Thread readerThread;
    private volatile boolean shouldExit;
    private static final String TAG = "Protocol";

    private int[] process_input(Protocol protocol, int character) {
        int[] retval = {1, 2, 3};
        return retval;
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
