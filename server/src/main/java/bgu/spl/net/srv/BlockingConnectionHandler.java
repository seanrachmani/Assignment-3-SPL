package bgu.spl.net.srv;

import bgu.spl.net.api.MessageEncoderDecoder;
import bgu.spl.net.api.MessagingProtocol;
import bgu.spl.net.api.StompMessagingProtocol;
import bgu.spl.net.impl.stomp.Frame;
import bgu.spl.net.impl.stomp.StompEncoderDecoder;
import bgu.spl.net.impl.stomp.StompProtocol;

import java.io.BufferedInputStream;
import java.io.BufferedOutputStream;
import java.io.IOException;
import java.net.Socket;

import javax.imageio.IIOException;

public class BlockingConnectionHandler<T> implements Runnable, ConnectionHandler<T> {
    //change:protocol type
    private final StompMessagingProtocol<T> protocol;
    private final MessageEncoderDecoder<T> encdec;
    private final Socket sock;
    private BufferedInputStream in;
    private BufferedOutputStream out;
    private volatile boolean connected = true;
    //add: connection and connection ID for disconencting connectionID in case it needed:
    private Connections<T> connections;
    private int connectionId;
    
    

    //change:protocol parmater(stomp)
    //add: connections paramter which were getting from base server
    public BlockingConnectionHandler(Socket sock, MessageEncoderDecoder<T> reader, StompMessagingProtocol<T> protocol){
        this.sock = sock;
        this.encdec = reader;
        this.protocol = protocol;
    }

    //add:
    //gets connection id and connection object and start stomp prtocol
    public void startProtocol(Connections<T> connections,int connectionId){
        this.connectionId = connectionId;
        this.connections = connections;
        this.protocol.start(connectionId,connections); 
    }

    @Override
    public void run() {
        try { 
            int read;
            in = new BufferedInputStream(sock.getInputStream());
            out = new BufferedOutputStream(sock.getOutputStream());
            //in.read==-1 --> stream is over
            while (!protocol.shouldTerminate() && connected && (read = in.read()) >= 0) {
                T nextMessage = encdec.decodeNextByte((byte) read);
                if (nextMessage != null) {
                    protocol.process(nextMessage);
                }
            }

        } catch (IOException ex) {
            ex.printStackTrace();
        }
        //automatic closing so we need to makr sure we delete the handler from our connections map
        finally{
            if(connections!=null){
                connections.disconnect(connectionId); //this closes the socket
            }
        }
    }

    @Override
    public void close() throws IOException {
        connected = false;
        sock.close();
    }

    @Override
    //send msg to client throgh the socket
    //called by connectionsImp.send
    public void send(T msg) {
        if(msg!=null){
            //encode to bytes
            byte[] bytes = encdec.encode(msg);
            try{
                //sync in case 2 threads trying to send the same user msg:
                synchronized (this){
                //write to output stream
                    out.write(bytes,0,bytes.length);
                    //actual send
                    out.flush();
                }
            }
            catch(IOException e){
                connections.disconnect(connectionId);
            }
        }  
    }
}
