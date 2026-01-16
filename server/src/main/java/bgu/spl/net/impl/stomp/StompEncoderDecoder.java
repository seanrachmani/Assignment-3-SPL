package bgu.spl.net.impl.stomp;

import bgu.spl.net.api.MessageEncoderDecoder;
import java.nio.charset.StandardCharsets;
import java.util.Arrays;

public class StompEncoderDecoder implements MessageEncoderDecoder<String>{

    private byte[] bytes = new byte[1 << 10];
    private int len = 0;

    /**
     * add the next byte to the decoding process
     * @param nextByte the next byte to consider for the currently decoded
     * message
     * @return a message if this byte completes one or null if it doesnt.
     */
    //bytes accepted from client->message for the server to use 
    public String decodeNextByte(byte nextByte){
       if(nextByte == '\u0000'){
            //take bytes from start(0) and take all of them(len)
            String msg = new String(bytes,0,len,StandardCharsets.UTF_8);
            len = 0; //bytes.clear
            return msg;
       } 
       else{
            if(len >=bytes.length){
                bytes = Arrays.copyOf(bytes, len*2);
            }
            bytes[len] = nextByte;
            len ++;
            return null; //we didnt finish yet
       }
    }

    /**
     * encodes the given message to bytes array
     *
     * @param message the message to encode
     * @return the encoded bytes
     */
    //msg from the server --> bytes to send to the client
    public byte[] encode(String message){
        return(message + "\u0000").getBytes(StandardCharsets.UTF_8);
    }

}
