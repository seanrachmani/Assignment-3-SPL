package bgu.spl.net.impl.stomp;

import bgu.spl.net.srv.BaseServer;
import bgu.spl.net.srv.Reactor;

public class StompServer {

    public static void main(String[] args) {
        if (args.length < 2) {
            System.out.println("Usage: StompServer <port> <tpc/reactor>");
            return;
        }

        int port;
        try {
            port = args[0];
        } catch (NumberFormatException e) {
            System.out.println("Invalid port: " + args[0]);
            return;
        }

        String mode = args[1];

        if (mode.equalsIgnoreCase("tpc")) {
            new BaseServer(
                    port,
                    () -> new StompProtocol(),
                    () -> new StompEncoderDecoder()
            ).serve();

        } else if (mode.equalsIgnoreCase("reactor")) {
            new Reactor(
                    8, 
                    port,
                    () -> new StompProtocol(),
                    () -> new StompEncoderDecoder()
            ).serve();

        } else {
            System.out.println("Unknown mode: " + mode + ". Use 'tpc' or 'reactor'.");
        }
    }
}