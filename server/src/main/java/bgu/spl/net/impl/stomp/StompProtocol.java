package bgu.spl.net.impl.stomp;

import java.util.HashMap;
import bgu.spl.net.impl.data.LoginStatus;
import bgu.spl.net.api.StompMessagingProtocol;
import bgu.spl.net.impl.data.Database;
import bgu.spl.net.srv.Connections;
import bgu.spl.net.srv.ConnectionsImpl;

public class StompProtocol implements StompMessagingProtocol<String> {
    // =================================FIELDS===========================================================================
    // reference for connections which was created by server in order to be able to
    // do stuff protocol has to do:
    Connections<String> connections = new ConnectionsImpl<>();
    // userID:
    int connectionId = 0;
    boolean shouldTerminate = false;

    // =================================ABSTRACT===========================================================================
    // Used to initiate the current client protocol with it's personal connection ID
    // and the connections implementation
    // we use this when first client reach to server
    public void start(int connectionId, Connections<String> connections) {
        this.connections = connections;
        this.connectionId = connectionId;
    }

    public void process(String message) {
        // using splitClientFrame helper function to split msg into frame, see Frame class which represent frame
        // if we got null an error was sent so stop
        Frame frame = splitClientFrame(message);
        if (frame == null) {
            return;
        }
        
        //from this part each client frame has helper function, see  helper functions below
        if (frame.getCommand().equals("CONNECT")) {
              connectFrame(frame); 
              return;   
        }
        if(frame.getCommand().equals("SEND")){
            sendFrame(frame);
            return;
        }
         if(frame.getCommand().equals("SUBSCRIBE")){
            subscribeFrame(frame);
            return;
        }
         if(frame.getCommand().equals("UNSUBSCRIBE")){
            unsubscribeFrame(frame);
            return;
        }
         if(frame.getCommand().equals("DISCONNECT")){
            disconnectFrame(frame);
            return;
        }
        //if we got till here we didnt return and the command is not equal to any of what the STOMP allow
        sendErrorFrame(new Frame("ERROR", "message", "Unknown command", ""));
    }


    // @return true if the connection should be terminated
    public boolean shouldTerminate() {
        return shouldTerminate;
    }


    // =============================================HELPER======================================================================
    // receive receiptHeader, Messafe header which describes shortly the error and
    // body which has more details and send error frame
    // if recipt header == null dont include it in the frame
    // @pre: body!=null
    public void sendErrorFrame(Frame errorFrame) {
        connections.send(connectionId, errorFrame.toString());
        shouldTerminate = true;
    }

   
    // gets a String msg and returns frame. split msg into frame contains
    // command,headers,body
    // see Frame class
    // in this part I sent error frames for structure and overall errors,
    // errors related to specific command I sent in specific part.
    // if an errorFrame was sent return null
    public Frame splitClientFrame(String message) {
        Frame resultFrame = new Frame();
        // command:
        if (message.length() == 0) {
            Frame errorFrame = new Frame("ERROR","message","empty frame","empty frame");
            sendErrorFrame(errorFrame);
            return null;
        }
        String[] msgInfo = message.split("\n");
        resultFrame.setCommand(msgInfo[0]);
        // headers:
        // split headers from msg
        int headerStart = message.indexOf('\n');
        int headerEnd = message.indexOf("\n\n");
        if (headerStart == -1 || headerEnd == -1 || headerEnd < headerStart) {
            Frame errorFrame = new Frame("ERROR","message","malformed frame", "invalid frame structure");
            sendErrorFrame(errorFrame);
            return null;
        }
        if (headerEnd > headerStart) { //zero headers are allowed
            String headers = message.substring(headerStart + 1, headerEnd);
            // map headers bc they say the order has no meaning
            String[] headersInfo = headers.split("\n");
            for (int i = 0; i < headersInfo.length; i++) {
                String currentHeader = headersInfo[i];
                int splitIndex = currentHeader.indexOf(':');
                if (splitIndex == -1) {
                    Frame errorFrame = new Frame("ERROR","message","malformed frame", "invalid frame structure");
                    sendErrorFrame(errorFrame);
                    return null;
                }
                String name = currentHeader.substring(0, splitIndex);
                String value = currentHeader.substring(splitIndex + 1);
                resultFrame.putHeader(name, value);
            }
        }
        // msg:
        int msgStart = headerEnd + 2;
        resultFrame.setBody(message.substring(msgStart));
        return resultFrame;
    }


    //get connect frame client instance and handle it as STOMP should
    public void connectFrame(Frame frame){
        String version = frame.getHeaders().get("accept-version");
        if (version == null || !version.equals("1.2")) {
            Frame errorFrame = new Frame("ERROR", "message", "unsupported version", "supported version is 1.2");
            sendErrorFrame(errorFrame);
            return; 
        }
        String host = frame.getHeaders().get("host");
        if (host == null || !host.equals("stomp.cs.bgu.ac.il")) {
            Frame errorFrame = new Frame("ERROR", "message", "invalid host", "host must be stomp.cs.bgu.ac.il");
            sendErrorFrame(errorFrame);
            return;
        }
        if (!frame.getHeaders().containsKey("login") || !frame.getHeaders().containsKey("passcode")) {
            Frame errorFrame = new Frame("ERROR", "message", "malformed frame", "missing login or passcode headers");
            sendErrorFrame(errorFrame);
            return;
        }
        String username = frame.getHeaders().get("login");
        String password = frame.getHeaders().get("passcode");
        Database db = Database.getInstance();
        LoginStatus status = db.login(connectionId, username, password);
        if(status == LoginStatus.CLIENT_ALREADY_CONNECTED){
            Frame errorFrame = new Frame("ERROR","message","client already connected","");
            sendErrorFrame(errorFrame);
            return;
        }
        if(status == LoginStatus.ALREADY_LOGGED_IN){
            Frame errorFrame = new Frame("ERROR","message","already logged in","");
            sendErrorFrame(errorFrame);
            return;
        }
        if(status == LoginStatus.WRONG_PASSWORD){
            Frame errorFrame = new Frame("ERROR","message","wrong password","");
            sendErrorFrame(errorFrame);
            return;
        }
        if(status == LoginStatus.ADDED_NEW_USER || status == LoginStatus.LOGGED_IN_SUCCESSFULLY){
            Frame connectedFrame = new Frame("CONNECTED","version","1.2","");
            connections.send(connectionId,connectedFrame.toString());
        }
    }

    //get send frame client instance and handle it as STOMP should
    public void sendFrame(Frame frame){
    }

    //get subscribre frame client instance and handle it as STOMP should
    public void subscribeFrame(Frame frame){
    }


    //get unsubscribre frame client instance and handle it as STOMP should
    public void unsubscribeFrame(Frame frame){
    }
    //get disconnect frame client instance and handle it as STOMP should
     public void disconnectFrame(Frame frame){
    }

}
