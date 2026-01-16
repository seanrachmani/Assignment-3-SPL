package bgu.spl.net.impl.stomp;

import java.util.HashMap;
import java.util.Map;

//this class is made for representing frame by command, headers and body
//header mapped by hashmap <header name, header value>
public class Frame {
    //fields:
    String command;
    HashMap<String,String> headers;
    String body;

    //constructors
    //1.
    //empty
    public Frame(){
        this.command = "";
        this.headers = new HashMap<>();
        this.body = "";
    }

    //2.
    //gets command, 1 header and body 
    public Frame(String command, String headerName, String headerValue,String body){
        this.command = command;
        this.headers = new HashMap<>();
        putHeader(headerName, headerValue);
        this.body = body;
    }

    //getters
   public String getCommand(){
        return command;
   }
   public Map<String, String> getHeaders() {
        return headers;
    }

    public String getBody() {
        return body;
    }


    //setters
    public void setCommand(String command) {
        this.command = command;
    }

    public void setHeaders(HashMap<String, String> headers) {
        this.headers = headers;
    }

    public void setBody(String body) {
        this.body = body;
    }


    //put header in headers map
    public void putHeader(String name,String value){
        headers.put(name, value);
    }

    //toString
    public String toString(){
        String frame="";
        frame = frame + this.command +"\n";
        for(Map.Entry<String,String> header : this.headers.entrySet()){
            frame = frame + header.getKey() + ":" +  header.getValue() + "\n";
        }
        frame = frame + "\n";
        if(this.body!=null){
            frame = frame + this.body ;
        }
        frame = frame + '\u0000';
        return frame;
    }
}
