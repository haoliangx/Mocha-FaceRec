package com.example.testpreview;

import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.net.Socket;
import java.util.ArrayList;
import java.util.List;
import android.annotation.SuppressLint;
import android.content.Context;
import android.graphics.Canvas;
import android.graphics.ImageFormat;
import android.graphics.Matrix;
import android.graphics.Paint;
import android.graphics.Rect;
import android.graphics.RectF;
import android.graphics.YuvImage;
import android.hardware.Camera;
import android.hardware.Camera.Face;
import android.hardware.Camera.FaceDetectionListener;
import android.hardware.Camera.Parameters;
import android.os.AsyncTask;
import android.os.Handler;
import android.util.Log;
import android.view.SurfaceHolder;
import android.view.SurfaceView;


@SuppressLint("ViewConstructor")
public class CameraPreview extends SurfaceView implements SurfaceHolder.Callback, Camera.PreviewCallback, FaceDetectionListener {
	
    public Socket sock;
	private TCPClient mTcpClient;	
	static int index = 0;
	private long startnow_local;
	private long endnow_local;
	
	private long temp;
	
	public List<packet_stamp> packet_list = new ArrayList<packet_stamp>();
	
	
	private String ip_address;
	private Integer port_num;
	
	private int PreviewSizeWidth;
	private int PreviewSizeHeight;
	private Camera myCamera;
	private Matrix matrix = new Matrix();
	private List<RecognizedFace> myFaces = new ArrayList<RecognizedFace>();
	private Rect my_rect;
	
	protected final Paint myRecPaint = new Paint();
	protected final Paint myTextPaint = new Paint();
	
	public class packet_stamp{
		int packet_index;
		long send_time;
		
		public packet_stamp(int index, long time){
			packet_index = index;
			send_time = time;
		}
	}
	
	public class connectTask extends AsyncTask<String,String,TCPClient> {

		@Override
		protected TCPClient doInBackground(String... arg0) {
		
			mTcpClient = new TCPClient(ip_address, port_num);		
			sock = mTcpClient.run(); 	
			return null;
		}
    }
	
	
	/* Class for recognized faces */
	public class RecognizedFace {
		public int id;
		public RectF faceCoordinates;
		public String name;
		public double confidence;
		public int score;
		
		public RecognizedFace() {
			id = 0;
			faceCoordinates = new RectF();
			name = "UNKNOWN";
			confidence = 0.0;
			score = 0;
		}
	}
	
	/* Constructor */
	public CameraPreview(Context context, int PreviewlayoutWidth, int PreviewlayoutHeight) {
		super(context);
		
		/* Set Preview Size */
		PreviewSizeWidth = PreviewlayoutWidth;
		PreviewSizeHeight = PreviewlayoutHeight;
		
		/* Set paint style for bounding box and the text*/
		myRecPaint.setARGB(100, 200, 0, 0);
		myRecPaint.setStyle(Paint.Style.STROKE);
		myRecPaint.setStrokeWidth(5);
		
		myTextPaint.setARGB(100, 200, 0, 0);
		myTextPaint.setStyle(Paint.Style.FILL);
		myTextPaint.setTextSize(32);
		
		matrix.setScale(1, 1);
		matrix.postRotate(0);
		matrix.postScale(PreviewSizeWidth / 2000f, PreviewSizeHeight / 2000f);
		matrix.postTranslate(PreviewSizeWidth / 2f, PreviewSizeHeight / 2f);
		
		my_rect = new Rect(0,0,PreviewSizeWidth, PreviewSizeHeight);
		
		/* Set this will draw */
		this.setWillNotDraw(false);
		
	
	}
	
	/* Function for drawing bounding box and the text for each face */
	@Override
	protected void onDraw(Canvas canvas) {
		super.onDraw(canvas);
		for (RecognizedFace currentFace : myFaces) {
			canvas.drawRect(currentFace.faceCoordinates, myRecPaint);
			canvas.drawText(currentFace.name + ' ' + currentFace.confidence , currentFace.faceCoordinates.left, currentFace.faceCoordinates.top, myTextPaint);
		}
	}
	
	/* Callback function for each frame from camera preview */
	@Override
	public void onPreviewFrame(byte[] data, Camera camera) {
		startnow_local = android.os.SystemClock.uptimeMillis();	
		Log.d("fps", " "+(android.os.SystemClock.uptimeMillis()-temp));
		temp = android.os.SystemClock.uptimeMillis();
		ByteArrayOutputStream imgStream = new ByteArrayOutputStream();
		//Rect rect = new Rect(0, 0, 0, 0);
		YuvImage myYuvImage = new YuvImage(data, ImageFormat.NV21, PreviewSizeWidth, PreviewSizeHeight, null);
		//for (RecognizedFace currentFace : myFaces) {
			//currentFace.faceCoordinates.round(rect);
			//myYuvImage.compressToJpeg(rect, 80, imgStream);
			myYuvImage.compressToJpeg(my_rect, 96, imgStream);
			
			byte[] imageBytes = imgStream.toByteArray();
			Log.i("SIZE", "Size of Final "+imageBytes.length);
			//ATTENTION: ALL THE PACKET TYPE IS DEFINED IN THE MOBILE SIDE, WHEN THE MOCHA PACKET IS FORMED, IT IS TAGGED WITH A PACKET TYPE
			//THE DATA SEND FROM THE MOBILE IS EITHER RAW IMAGE OR FACE IMAGE, HOWEVER, WHEN YOU SET THE DATA TYPE YOU SHOULD CONSIDER WHEN THE DATA HITS THE CLOUD, IF THE DATA HITS THE CLOUD IS VECTOR, YOU SHOULD SET THE TYPE TO VECTOR			
			MochaPacket mocha = new MochaPacket(1, index, imageBytes);
			DispatchMochaTask task = new DispatchMochaTask(mocha, new DispatchMochaTaskCallback() {
				@Override
				public void taskComplete(String result) {
					/*for (RecognizedFace currentFace : myFaces) {
						if(currentFace.id == result.id) {
							//TODO
						} else {
							continue;
						}
							
					}*/
				}
				
			});
			task.dispatch();
			endnow_local = android.os.SystemClock.uptimeMillis();
			Log.i("return", "local time: "+(endnow_local-startnow_local)+ " ms");
			
			//Local Test
			//currentFace.name = "LOCAL TEST";
			//currentFace.confidence = 89.1;
			//this.invalidate();
		//}
	}
	
	@Override
	public void onFaceDetection(Face[] faces, Camera camera) {
		Log.d("FaceDetection", faces.length + " Faces detected");
		myFaces.removeAll(myFaces);
		for (Face currentFace : faces) {
			if(currentFace.score > 50) {
				RecognizedFace myFace = new RecognizedFace();
				myFace.id = currentFace.id;
				myFace.faceCoordinates.set(currentFace.rect);
				/* Re-map the coordinates to preview space */
				matrix.mapRect(myFace.faceCoordinates);
				myFace.score = currentFace.score;
				myFaces.add(myFace);
			} else {
				continue;
			}
		}
		/* sleep for a while to prevent overwriting */
		try {
			Thread.sleep(20);
		} catch(InterruptedException e) {
			;
		}
	}
	
	@Override
	public void surfaceChanged(SurfaceHolder arg0, int arg1, int arg2, int arg3) {
		myCamera.startPreview();
		//myCamera.startFaceDetection();
	}
	
	@Override
	public void surfaceCreated(SurfaceHolder arg0) {
		myCamera = Camera.open();
		try {
		    myCamera.setPreviewDisplay(arg0);
		    myCamera.setPreviewCallback(this);
		    myCamera.setFaceDetectionListener(this);
		    Parameters p = myCamera.getParameters();
		    p.setPreviewSize(PreviewSizeWidth, PreviewSizeHeight);
		    myCamera.setParameters(p);
		    List<Camera.Size> t = p.getSupportedPreviewSizes();
		    for(Camera.Size mt:t) {
		    	Log.d("supportedsize","width = " + mt.width + " height = " + mt.height);
		    }
		    Log.d("supportedsize", "supported number of faces = " + p.getMaxNumDetectedFaces());
		    	
		} catch (IOException e) {
		    myCamera.release();
		    myCamera = null;
		}
	}
	
	public Socket connect(String ip_address, int port_num) {
		this.ip_address = ip_address;
		this.port_num = port_num;
		new connectTask().execute("");
		return sock;
	}
	
	@Override
	public void surfaceDestroyed(SurfaceHolder arg0) {
		myCamera.stopFaceDetection();
		myCamera.stopPreview();
		myCamera.setFaceDetectionListener(null);
		myCamera.setPreviewCallback(null);
		myCamera.release();
		myCamera = null;
	}
	
	interface DispatchMochaTaskCallback {
		void taskComplete(String result);
	}
	
	/* Multi-threading  class for MOCHA*/
	public class DispatchMochaTask extends Thread {
		
		private DispatchMochaTaskCallback callback;
		private MochaPacket packet;
		private Handler handler;
		
		/*public class FaceRecognizeResult {
			public int id;
			public String name;
			public double confidence;
			
			public FaceRecognizeResult() {
				this.id = -1;
				this.name = "UNKNOWN";
				this.confidence = 0.0;
			}
		}*/
		
		public DispatchMochaTask(MochaPacket packet, DispatchMochaTaskCallback callback) {
			this.callback = callback;
			this.packet = packet;
			handler = new Handler();
		}
		
		public void dispatch() {
			this.start();
		}
		
		@Override
		public void run() {
			//final FaceRecognizeResult result = new FaceRecognizeResult();
			final String result = new String();
			try {
				if(sock == null){
					Log.e("Callback", "socket not avaiable");
				}
				else{	
					sock.getOutputStream().write(packet.toByteArrayOutputStream().toByteArray());
					packet_stamp my_packet = new packet_stamp(index, android.os.SystemClock.uptimeMillis());
					packet_list.add(my_packet);				
					index = index+1;
					Log.i("stream", "Send success"+packet.toByteArrayOutputStream().toByteArray().length);
					//wait for receive
/*
					Log.i("return", "before write result");


							EditText myText = (EditText) findViewById(R.id.result_box);
							myText.setText("hello");
							Log.i("return", "write result");
*/
				}
			} catch (Exception e) {
				Log.e("return", "I/Oerror: " + e.toString());
			} finally {
				handler.post(new Runnable() {
					@Override
					public void run() {
						callback.taskComplete(result);	
					}
				});
			}
		}
	}
} 