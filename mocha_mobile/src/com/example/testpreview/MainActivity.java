package com.example.testpreview;

import java.io.DataInputStream;
import java.net.Socket;

import com.example.testpreview.CameraPreview.packet_stamp;

import android.os.Bundle;
import android.app.Activity;
import android.util.Log;
import android.view.SurfaceHolder;
import android.view.View;
import android.view.Window;
import android.view.WindowManager;
import android.view.ViewGroup.LayoutParams;
import android.widget.EditText;
import android.widget.FrameLayout;
import android.widget.TextView;

public class MainActivity extends Activity {
	
	private CameraPreview camPreview;
	private SurfaceHolder camHolder;
	
	private TextView result_tv;
	private EditText text_ip;
	private EditText text_port;
	
    private long total_time=0;
    private int total_packet=0;
    
	public  String DEFAULT_SERVERIP = "172.30.165.20";
	public  Integer DEFAULT_SERVERPORT = 1600;
	
	private int width = 800;
    private int height = 480;
    
	private String name;
	private String confidence;
	private Socket sock;

	
	@Override
	public void onCreate(Bundle savedInstanceState) {
	    super.onCreate(savedInstanceState);
	    this.requestWindowFeature(Window.FEATURE_NO_TITLE);
		this.getWindow().setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN, WindowManager.LayoutParams.FLAG_FULLSCREEN);
	    setContentView(R.layout.activity_main);
	    
	    //CameraPreview camPreview = new CameraPreview(this, width, height);
	    //SurfaceHolder camHolder = camPreview.getHolder();
	    
	    camPreview = new CameraPreview(this, width, height);
	    camHolder = camPreview.getHolder();

	    
	}
	
	public void onClickConnnect(View view) {
		text_ip = (EditText)findViewById(R.id.ip_address);
		String ip_address = text_ip.getText().toString();
		if(ip_address.length()==0){
			ip_address = DEFAULT_SERVERIP;
		}
		text_port = (EditText)findViewById(R.id.port_num);
		String port_string = text_port.getText().toString();
		int port_num=DEFAULT_SERVERPORT;
		if(port_string.length()!=0){
			port_num = Integer.parseInt(port_string);
		}
		Log.i("input", "ip "+ip_address+" port "+port_num);
		camPreview.connect(ip_address, port_num);
	}
	
	public void onClickRecognize(View view) {
		result_tv = (TextView)findViewById(R.id.result_text);
		sock = camPreview.sock;
		camHolder.addCallback(camPreview);
	    FrameLayout mainLayout = (FrameLayout) findViewById(R.id.preview);
 	    mainLayout.addView(camPreview, new LayoutParams(width, height));
		//create recv thread
 	    
		Thread recv_thread = new Thread(recv_runnable);

		recv_thread.setDaemon(true);
		recv_thread.start();

	}
	
	private Runnable recv_runnable = new Runnable() {
		public void run() {
			while(sock==null){
				
			}
			
			Log.i("return", "enter");
			
			while(sock !=null){
/* 				TODO:why this will leads to memory leak?
			int avaiable=0;
			try{

				avaiable = sock.getInputStream().available();
			}catch(Exception e){continue;}
*/
				try{
					DataInputStream in = new DataInputStream(sock.getInputStream());
					byte[] greeting_temp = new byte[6];
					in.read(greeting_temp, 0, 6);
					String greeting = new String(greeting_temp, 0, 5);
					//Log.i("return", "mocha continue");
					//if(greeting_temp[0] != 'M') continue;
					//Log.i("return", "greeting "+greeting);
					int type = in.readInt();
					Log.i("return", "type"+type);
					int index = in.readInt();
					Log.i("return", "index "+index);
					Log.i("index", "return index "+index);
					int length = in.readInt();
					Log.i("return", "length "+type);
					int device_id = in.readInt();
					Log.i("return", "device_id "+device_id);
				
					byte[] confidence_temp = new byte[6];
					in.read(confidence_temp, 0, 6);
					confidence = new String(confidence_temp);	
					Log.i("return", "Confidence "+confidence);
				
					int name_length = length-22-6;
					byte[] name_temp = new byte[name_length];
					in.read(name_temp, 0, name_length);
					name = new String(name_temp);
					Log.i("return", "Name "+name);
					
					result_tv.post(new Runnable(){
						public void run(){
							result_tv.setText("Prof."+name+"  "/*+confidence+" "*/+total_time/total_packet+"ms");
						}
					});

					for(packet_stamp p: camPreview.packet_list){
						if(p.packet_index==index){
							long elpased_time = android.os.SystemClock.uptimeMillis()-p.send_time;
							Log.i("return", "frame time for packet "+index+": "+elpased_time+ " ms");
							total_time += elpased_time;
							total_packet+=1;
							Log.i("return", "average time "+ total_time/total_packet+" ms");								
							camPreview.packet_list.remove(p);
							break;
						}
					}

				}catch(Exception e){Log.i("return_error", "continue"); continue;}
			//}				
		  }
		}
	};
}