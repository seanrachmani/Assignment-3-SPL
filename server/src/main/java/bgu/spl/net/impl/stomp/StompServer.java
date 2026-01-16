package bgu.spl.net.impl.stomp;

import java.nio.channels.Selector;
import java.util.concurrent.ConcurrentLinkedQueue;
import java.util.function.Supplier;

import bgu.spl.net.api.MessageEncoderDecoder;
import bgu.spl.net.api.MessagingProtocol;
import bgu.spl.net.srv.Server;

public class StompServer <T> implements Server<T> {

    private final int port;
    //messeging protocol
    //message encoder decoder
    //threads
    // boolean reactor or 
    private Thread selectorThread;
    private final ConcurrentLinkedQueue<Runnable> selectorTasks = new ConcurrentLinkedQueue<>();

    public Reactor(
            int numThreads,
            int port,
            Supplier<MessagingProtocol<T>> protocolFactory,
            Supplier<MessageEncoderDecoder<T>> readerFactory) {

        this.pool = new ActorThreadPool(numThreads);
        this.port = port;
        this.protocolFactory = protocolFactory;
        this.readerFactory = readerFactory;
    }

    public static void main(String[] args) {
        // TODO: implement this
    }
}