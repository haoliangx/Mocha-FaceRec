package com.example.testpreview;

import java.io.ByteArrayOutputStream;
import java.io.DataOutputStream;
import java.io.OutputStream;

import android.util.Log;

public class MochaPacket {
	
	private byte[] greeting = new byte[6];
	private int type;
	private int length;
	private int index;
	private int device_id;
	private byte[] payload;
	OutputStream out;
	
	public MochaPacket(int type, int index, byte[] payload){
		greeting[0] = 'M';
		greeting[1] = 'O';
		greeting[2] = 'C';
		greeting[3] = 'H';
		greeting[4] = 'A';
		greeting[5] = '\0';
		this.type = type;
		this.index = index;
		this.payload = payload;
		device_id = 0;
		length = 6+4+4+4+4+payload.length;
		
	}
	
	public ByteArrayOutputStream toByteArrayOutputStream(){
		ByteArrayOutputStream out = new ByteArrayOutputStream();
		DataOutputStream dos = new DataOutputStream(out);
		try{
			dos.write(greeting, 0, 6); 
			dos.writeInt(type);
			dos.writeInt(index);
			dos.writeInt(length);
			dos.writeInt(device_id);
			dos.write(payload);
			//dos.flush();
			Log.i("stream", "length="+length+"  size="+out.toByteArray().length);
			Log.i("stream", "type="+type+"  index="+index+" device_id= "+device_id);
			Log.i("index", "send index "+index);
		
		}catch (Exception e) {
			 Log.e("stream", "Byte array Error");
		 }
		
		return out;
	}
}
