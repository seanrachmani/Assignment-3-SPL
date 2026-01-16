package bgu.spl.net.srv;

import java.util.concurrent.ConcurrentHashMap;

public class ConnectionsImpl<T> implements Connections<T>{

    // A thread-safe map to connect connection IDs to their handlers
    private final ConcurrentHashMap<Integer, ConnectionHandler<T>> activeConnections = new ConcurrentHashMap<>();


    public ConnectionsImpl() {
    }

    public void addConnection(int connectionId, ConnectionHandler<T> handler) {
        try{
            if (!activeConnections.containsKey(connectionId)) {
                activeConnections.put(connectionId, handler);
            }
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    @Override
    public boolean send(int connectionId, T msg) {
        try {
            ConnectionHandler<T> handler = activeConnections.get(connectionId);
            if (handler != null) {
                handler.send(msg);
            }
        } catch (Exception e) {
            e.printStackTrace();
            return false;
        }
        return true;
    }

    @Override
    public void send(String channel, T msg) {
        try {
            for (ConnectionHandler<T> handler : activeConnections.values()) {
                handler.send(msg);
            }
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    @Override
    public void disconnect(int connectionId) {
        activeConnections.remove(connectionId);
    }
    
}
