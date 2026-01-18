package bgu.spl.net.impl.stomp;

import java.util.function.Supplier;

import bgu.spl.net.api.MessageEncoderDecoder;
import bgu.spl.net.api.StompMessagingProtocol;
import bgu.spl.net.srv.Server;

public class StompServer {

    public static void main(String[] args) {
        if(args.length < 2){
            throw new IllegalArgumentException("no port or server method specified");
        }
        int port = Integer.parseInt(args[0]);
        String method = args[1];
        //lambada for supplieres get method impl
        Supplier<StompMessagingProtocol<String>> protocolFactory = () -> new StompProtocol();
        Supplier<MessageEncoderDecoder<String>> encDecFactory = () -> new StompEncoderDecoder();

        if(method.equals("tpc")){
            Server.threadPerClient(port, protocolFactory, encDecFactory).serve();
        }
        else{
            if(method.equals("reactor")){
                int nthreads = Runtime.getRuntime().availableProcessors();
                Server.reactor(nthreads, port, protocolFactory,encDecFactory).serve(); 
            }
            else{
                System.out.println("invalid port or server method");
                return;
            }
        }
        //try
    }

}


