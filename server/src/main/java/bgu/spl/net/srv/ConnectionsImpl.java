package bgu.spl.net.srv;

import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.atomic.AtomicInteger;

public class ConnectionsImpl<T> implements Connections<T> {
    //FIELDS
    private ConcurrentHashMap<Integer,ConnectionHandler<T>> map;
    //the id saved in this field will be the next available unique one:
    private AtomicInteger idCounter;

    //CONSTRUCTOR
    //called when server is starting aka server.serve
    public ConnectionsImpl(){
        map = new ConcurrentHashMap<>();
        idCounter = new AtomicInteger(0);
    }

    //Register new Connection Handler, return the id when done
    //called when new connection handler is created
    public int register(ConnectionHandler<T> ch){
        int tempID = idCounter.getAndIncrement();
        map.put(tempID,ch);
        return tempID;
    }

    //ABSTRACT METHODS:
    //sends a message T to client represented by the given connectionId.
    public boolean send(int connectionId, T msg){
        ConnectionHandler<T> ch = map.get(connectionId);
        if(ch!=null){
            ch.send(msg);
            return true;
        }
        return false;
    }


    //Sends a message T to clients subscribed to channel
    public void send(String channel, T msg){

    }
}
