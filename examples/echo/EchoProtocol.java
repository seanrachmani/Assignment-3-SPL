package bgu.spl.net.impl.examples.echo;

import bgu.spl.net.api.MessagingProtocol;
import bgu.spl.net.api.StompMessagingProtocol;
import bgu.spl.net.srv.Connections;

import java.time.LocalDateTime;

public class EchoProtocol implements StompMessagingProtocol<String> {

    private boolean shouldTerminate = false;

    @Override
    public void start(int id, Connections<String> connections){
        return;
    }

    @Override
    public void process(String msg) {
        shouldTerminate = "bye".equals(msg);
        System.out.println("[" + LocalDateTime.now() + "]: " + msg);
        System.out.println(createEcho(msg));
    }

    private String createEcho(String message) {
        String echoPart = message.substring(Math.max(message.length() - 2, 0), message.length());
        return message + " .. " + echoPart + " .. " + echoPart + " ..";
    }

    @Override
    public boolean shouldTerminate() {
        return shouldTerminate;
    }
}
