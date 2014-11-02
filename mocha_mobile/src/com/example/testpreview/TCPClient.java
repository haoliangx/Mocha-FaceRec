package com.example.testpreview;

import java.net.InetAddress;
import java.net.Socket;

import android.util.Log;

public class TCPClient {
	
	public  String SERVERIP;
	public  Integer SERVERPORT;
	
	//private Resources resources;
	//private ByteArrayOutputStream stream;
	 
	public TCPClient(String ip_address, Integer port_num){
		SERVERIP = ip_address;
		SERVERPORT = port_num;
	}
	
	public Socket run() {
		
		 Socket socket = null;
	     Log.i("TCP", "IP:"+SERVERIP.toString()+"Portnum: "+SERVERPORT.toString());
		 try {
			 InetAddress serverAddr = InetAddress.getByName(SERVERIP);	 
			 
			 socket = new Socket(serverAddr, SERVERPORT);	 			 
		 } catch (Exception e) {
			 Log.e("TCP", "Error");
		 }
		 
		 return socket;
	 }

}
