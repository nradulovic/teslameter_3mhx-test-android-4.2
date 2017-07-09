package com.teslameter.nr.teslameter;
import android.util.Log;

/**
 * Created by nenad on 5.7.17..
 */


/*
 * import static android.content.ContentValues.TAG;
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
                    int[] response;
                    int character = this.protocol.protocolRdByte();
                    int status;

                    if (character < 0) {
                        Log.e(Protocol.TAG, String.format("protocolRdByte() = %d", character));
                        this.protocol.shouldExit = true;
                        break;
                    } else {
                        response = this.protocol.process_input(character);
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
    private static final String TAG = "Protocol";

    private int [] process_input(int character) {
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
