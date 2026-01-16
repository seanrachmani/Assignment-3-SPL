package bgu.spl.net.impl.echo;

import bgu.spl.net.api.MessagingProtocol;
import bgu.spl.net.api.StompMessagingProtocol;
import bgu.spl.net.srv.Connections;

import java.time.LocalDateTime;
import java.util.HashMap;
import java.util.Map;

public class StompProtocol implements StompMessagingProtocol<String> {

    private boolean shouldTerminate = false;
    private int connectionId;
    private Connections<String> connections;



    @Override
    public void start(int connectionId, Connections<String> connections) {
        this.connectionId = connectionId;
        this.connections = connections;
    }

    @Override
    public void process(String message) {
        String[] parts = message.split("\n\n", 2); // Split into [Headers, Body]
        String[] headLines = parts[0].split("\n");
        String command = headLines[0].trim(); 

        if (command.equals("DISCONNECT")) {
            handleDisconnect(headLines);
        }
        if(command.equals("SEND")){
            handleSend(headLines, message);
        }
    }

    @Override
    public boolean shouldTerminate() {
        return shouldTerminate;
    }

    private boolean handleDisconnect(String[] lines) {
        // 1. Find the receipt-id in the headers
        boolean sent = false;
        String receiptId = null;
        for (int i = 1; i < lines.length; i++) {
            if (lines[i].startsWith("receipt:")) {
                receiptId = lines[i].split(":")[1].trim();
                break;
            }
        }

        // 2. If a receipt was requested, send the RECEIPT frame first
        if (receiptId != null) {
            sent = sendRecipt(receiptId);
        }
        // 3. Mark for termination
        shouldTerminate = true;
            
        // 4. Cleanup internal state (subscriptions, etc.)
        connections.disconnect(connectionId);    // Implementation for handling DISCONNECT command if needed
        return sent;
    }

    private void handleSend(String[] headLines, String fullMessage) {
        String destination = null;
        for (String line : headLines) {
            if (line.startsWith("destination:")) {
                destination = line.split(":")[1].trim();
            }
        }

        if (destination != null) {
            // Extract the body (everything after \n\n)
            String body = "";
            int bodyIndex = fullMessage.indexOf("\n\n");
            if (bodyIndex != -1) {
                body = fullMessage.substring(bodyIndex + 2);
            }

            // Construct the MESSAGE frame
            String msgFrame = "MESSAGE\n" +
                            "destination:" + destination + "\n" +
                            "\n" +
                            body + "\u0000";

            connections.send(destination, msgFrame);
        }
    }

    // sends RECEIPT frame to clien
    private boolean sendRecipt(String Id) {
        if (receiptId != null) {
            String receipt= "RECEIPT\n" +
                                "receipt-id:" + Id + "\n" +
                                "\n" +
                                "\u0000";
            
            connections.send(Id, receipt);
            return true;
        }
        return false;
    }
}
