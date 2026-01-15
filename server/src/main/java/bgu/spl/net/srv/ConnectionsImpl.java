package bgu.spl.net.srv;

import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.atomic.AtomicInteger;

public class ConnectionsImpl<T> implements Connections<T> {
    //FIELDS
    //map unique userID which given by this class to each client represented by connection handler
    private ConcurrentHashMap<Integer,ConnectionHandler<T>> mapUserIDTtoConnectionHandler;
    //the id saved in this field will be the next available unique one:
    private AtomicInteger idCounter;
    //map each channel to hashmap contains subscribers: User ID & Subscription ID for each subscriber
    private ConcurrentHashMap<String,ConcurrentHashMap<Integer,Integer>> mapChannels;
    //map each user to list of subscriptions , for each subscription subscriptionID, channel name <userID,<subID,channelName>>
    private ConcurrentHashMap<Integer,ConcurrentHashMap<Integer,String>> mapUserToChannels;

    //CONSTRUCTOR
    //called when server is starting aka server.serve
    public ConnectionsImpl(){
        mapUserIDTtoConnectionHandler = new ConcurrentHashMap<>();
        idCounter = new AtomicInteger(0);
        mapChannels = new ConcurrentHashMap<>();
        mapUserToChannels = new ConcurrentHashMap<>();
    }

    //Register new Connection Handler, return the id when done
    //called when new connection handler is created
    public int connect(ConnectionHandler<T> ch){
        int tempID = idCounter.getAndIncrement();
        mapUserIDTtoConnectionHandler.put(tempID,ch);
        return tempID;
    }

    //gets channel name, user ID, subscription ID
    //if the channel already exists map the user to it's hashmap, if not put channel into mapChannels and then map user
    //called when user subscribe
    public void subscribe(String channel,Integer userID, Integer subscriptionID){
        ConcurrentHashMap<Integer,Integer> subscribers = new ConcurrentHashMap<>();
        //if channel exists it doesnt add new channel
        //if absent it adds the empty clients map i created
        mapChannels.putIfAbsent(channel, subscribers);
        //no we know channel is in mapChannels so we can add user details to its clients map
        mapChannels.get(channel).put(userID, subscriptionID);  

        //mapUserToChannels part:
        ConcurrentHashMap<Integer,String> channels = new ConcurrentHashMap<>();
        mapUserToChannels.putIfAbsent(userID,channels);
        mapUserToChannels.get(userID).put(subscriptionID, channel);
    }

    //called when user unsubscribe
    public void unsubscribe(Integer userID, Integer subscriptionID){
        ConcurrentHashMap<Integer,String> currentChannels = mapUserToChannels.get(userID);
        if(currentChannels!=null){
            String channel = currentChannels.get(subscriptionID);
            currentChannels.remove(subscriptionID);
            if(channel!=null){
                ConcurrentHashMap<Integer,Integer> currentSubs = mapChannels.get(channel);
                if(currentSubs!=null){
                    currentSubs.remove(userID);
                }
            }
        }
    }


    //returns map of <USERID, sub ID> for specific channel
    public ConcurrentHashMap<Integer,Integer> getSubscribers(String channel){
        return mapChannels.get(channel);
    }


    //ABSTRACT METHODS:
    //sends a message T to client represented by the given connectionId.
    public boolean send(int connectionId, T msg){
        ConnectionHandler<T> ch = mapUserIDTtoConnectionHandler.get(connectionId);
        if(ch!=null){
            ch.send(msg);
            return true;
        }
        return false;
    }


    //Sends a message T to clients subscribed to channel
    public void send(String channel, T msg){
        ConcurrentHashMap<Integer,Integer> subscribers = mapChannels.get(channel);
        if(subscribers!=null){
            for(Map.Entry<Integer,Integer> pair : subscribers.entrySet()){
                send(pair.getKey(),msg);
            }
        }
    }

    //Removes an active client connectionId from the map
    public void disconnect(int connectionId){
        //first data structure
        mapUserIDTtoConnectionHandler.remove(connectionId);
        //subscription remove
        //channels that the user is subscribe to
        ConcurrentHashMap<Integer,String> channels = mapUserToChannels.get(connectionId);
        //unsubscribe from all channels - delete from second data structure
        if(channels!=null){
            for(Map.Entry<Integer,String> channel : channels.entrySet()){
                int subID = channel.getKey();
                unsubscribe(connectionId, subID);
            }
        }
        //delete user from third data structure
        mapUserToChannels.remove(connectionId);
    }
}
