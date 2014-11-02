#include "opencv2/core/core.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/contrib/contrib.hpp"
#include "opencv2/objdetect/objdetect.hpp"
#include "opencv2/gpu/gpu.hpp"

#include <iostream>
#include <fstream>
#include <sstream>

#include <Windows.h>
#include <math.h>

using namespace std;
using namespace cv;
using namespace gpu;

#include "facerec.h"

int faceRecLocal(Mat &testFace, face_t &ret_val, int init)
{
	static bool init_flag = false;
	static Ptr<FaceRecognizer> myFaceRecognizer;
	static vector<string> names;
	static int components;
	static int trainfaces;
	static int height;
	static int width;
	static double average;
	static int counter;
	double my_confi;

	int predicted_label;
	double distance_square;
	double confidence;

	if((init == 1 && init_flag == false) || init == 2)
	{
		databaseInit(myFaceRecognizer, names, height, width);
		components = myFaceRecognizer->getInt("ncomponents");
		trainfaces = (int)names.size();
		init_flag = true;
		average = 0;
		counter = 0;

		return 0;
	}

	if(init_flag == false)
	{
		CV_Error(CV_StsInternal, "Face Recoginizer not initialized");
		return -1;
	}

	imagePreProcess(testFace, height, width);

	//myFaceRecognizer -> predict(testFace, predicted_label, distance_square);
	double timing_start;
	timing_start = GetTickCount();

	Mat projection = myFaceRecognizer -> project(testFace);

	cout << "Projection Time : " << GetTickCount() - timing_start << "ms" << endl;

	timing_start = GetTickCount();

	myFaceRecognizer->search(projection, predicted_label, distance_square);

	cout << "Search Time : " << GetTickCount() - timing_start << "ms" << endl;

	//confidence = 1.0f - sqrt(distance_square/(float)(trainfaces * components)) / 255.0f;
	cout<<"**********************************my confi "<<1-sqrt(distance_square/(255*255*100))<<endl;
	confidence = distance_square;
	average += confidence;
	cout<<" AVERAGE "<<average<<endl;
	cout << trainfaces<< " " << components << " " << predicted_label <<" " << distance_square <<endl;
	ret_val.name = names[predicted_label];
	ret_val.confidence = double2string(confidence);
	ret_val.error_no = 0;

	return 0;
}

int faceRecCloudlet(Mat &testFace, Mat &projection, int init)
{
	static bool init_flag = false;
	static HANDLE init_mutex = CreateMutex(NULL, FALSE, NULL);
	static vector<string> names;
	static int height;
	static int width;

	static Ptr<FaceRecognizer> myFaceRecognizer;

	if(init == 1)
	{
		databaseInit(myFaceRecognizer, names, height, width);
		init_flag = true;
		return 0;
	}

	if(init_flag ==false)
	{
		CV_Error(CV_StsInternal, "Face Recoginizer not initialized");
		return -1;
	}

	imagePreProcess(testFace, height, width);

	double timing_start;
	timing_start = GetTickCount();

	projection = myFaceRecognizer -> project(testFace);

	cout << "Projection Time : " << GetTickCount() - timing_start << "ms" << endl;

	return 0;
}

int faceRecCloud(Mat &projection, face_t &ret_val, int init)
{
	static bool init_flag = false;
	static HANDLE init_mutex = CreateMutex(NULL, FALSE, NULL);
	static vector<string> names;

	static Ptr<FaceRecognizer> myFaceRecognizer;

	static int components;
	static int trainfaces;
	static int height;
	static int width;

	int predicted_label = -1;
	double distance_square = 0.0;
	double confidence;

	if(init == 1)
	{
		databaseInit(myFaceRecognizer, names, height, width);
		components = myFaceRecognizer->getInt("ncomponents");
		trainfaces = (int)names.size();
		init_flag = true;
		return 0;
	}

	if(init_flag == false)
	{
		CV_Error(CV_StsInternal, "Face Recoginizer not initialized");
		return -1;
	}

	double timing_start;
	timing_start = GetTickCount();

	myFaceRecognizer->search(projection, predicted_label, distance_square);

	cout << "Search Time : " << GetTickCount() - timing_start << "ms" << endl;

	confidence = 1.0f - sqrt(distance_square/(float)(trainfaces * components)) / 255.0f;

	ret_val.name = names[predicted_label];
	ret_val.confidence = double2string(confidence);
	ret_val.error_no = 0;

	return 0;
}

void databaseInit(Ptr<FaceRecognizer> &myFaceRecognizer, vector<string> &names, int &height, int &width)
{
	vector<Mat> images;
	vector<int> labels;

	try {
		read_csv(images, labels, ';');
		std::ifstream file(DATABASE_NAMES, ifstream::in);
		string line, path, classlabel;
		while (getline(file, line)) {
			names.push_back(line);
		}
	} catch (cv::Exception& e) {
		cerr << "Error opening file \"" << DATABASE_INDEX << "\". Reason: " << e.msg << endl;
		return;
	}

	height = (images[0]).size.p[0];
	width = (images[0]).size.p[1];

	//myFaceRecognizer = createEigenFaceRecognizer((int)(images.size()/8), DBL_MAX);
	myFaceRecognizer = createEigenFaceRecognizer((int)(images.size()), DBL_MAX);
	//myFaceRecognizer = createEigenFaceRecognizer((int)NEIGENVECTORS, DBL_MAX);

	ifstream file(DATABASE_FILE);
/*
	if(file) {
		cout << "Init from xml file" << endl;
		myFaceRecognizer->load(string(DATABASE_FILE));
	} else {
*/
		cout<<"Init from scratch"<<endl;
		myFaceRecognizer -> train(images, labels);
		myFaceRecognizer -> save(string(DATABASE_FILE));
		Mat eigenvalues = myFaceRecognizer -> getMat("eigenvalues");
		cout << "eigenvalue size" << eigenvalues.size() << endl;
		ofstream file2("matrix");
		file2 << eigenvalues;
		
	//}
}

void imagePreProcess(Mat &testFace, int height, int width)
{
	resize(testFace, testFace, Size(width, height));
	if(testFace.channels() > 1)
		cvtColor(testFace, testFace, CV_RGB2GRAY);
	equalizeHist(testFace, testFace);
}

string double2string(double n)
{
	char tmp[1024];
	sprintf(tmp, "%.4f", n);
	return string(tmp);
}

void read_csv(vector<Mat>& images, vector<int>& labels, char separator) {
    std::ifstream file(DATABASE_INDEX, ifstream::in);
    if (!file)
        CV_Error(CV_StsBadArg, "No valid input file was given, please check the given filename.");
    string line, path, classlabel;
    while (getline(file, line)) {
        stringstream liness(line);
        getline(liness, path, separator);
        getline(liness, classlabel);
        if(!path.empty() && !classlabel.empty()) {
			Mat my_img = imread(path, 0); //grayscale
			equalizeHist(my_img, my_img);
            images.push_back(my_img); 
            labels.push_back(atoi(classlabel.c_str()));
        }
    }
}

int faceDetect(Mat &image, vector<Mat> &faces, CascadeClassifier cascade, int init)
{

	static string cascade_path = string(CASCADE_PATH);

	vector< Rect_<int> > det_faces;
	Mat gray;


	LARGE_INTEGER li;
	QueryPerformanceCounter(&li);
	__int64 CounterStart = li.QuadPart;

	cvtColor(image, gray, CV_BGR2GRAY);
	equalizeHist(gray, gray);

	//LARGE_INTEGER li;
	QueryPerformanceCounter(&li);
    CounterStart = li.QuadPart;

	cascade.detectMultiScale(gray, det_faces,1.1,2, CV_HAAR_FIND_BIGGEST_OBJECT | CV_HAAR_DO_ROUGH_SEARCH |CV_HAAR_SCALE_IMAGE, Size(30,30));

	QueryPerformanceCounter(&li);
	cout<<"Detect time is "<<double(li.QuadPart-CounterStart)/PCFreq<<endl;

	for(int i = 0; i < det_faces.size(); i++)
	{
		Rect face_i = det_faces[i];
		Mat face = gray(face_i);
		resize(face, face, Size(100, 100));
		faces.push_back(face); 
	}
	return 0;
}

int faceDetect_GPU(Mat &image, vector<Mat> &faces, int init)
{
	static CascadeClassifier_GPU cascade_gpu;
	static HANDLE init_mutex;

	if(init == 1)
	{
		init_mutex = CreateMutex(NULL, FALSE, NULL);
		cascade_gpu.load(string(CASCADE_PATH));
		return 0;
	}

	Mat gray;
	cvtColor(image, gray, CV_BGR2GRAY);
	equalizeHist(gray, gray);

	GpuMat image_gpu(gray);
	GpuMat objbuf;
	
	WaitForSingleObject(init_mutex, INFINITE);

	double start  = GetTickCount();
	int detections_number = cascade_gpu.detectMultiScale(image_gpu, objbuf, 1.1, 3);
	cout << "Face Detect GPU Time : " << GetTickCount()-start << "ms" << endl;

	ReleaseMutex(init_mutex);

	Mat obj_host;
	objbuf.colRange(0, detections_number).download(obj_host);
	Rect* det_faces = obj_host.ptr<Rect>();
	for(int i = 0; i < detections_number; ++i)
	{
		Mat face = gray(det_faces[i]);
		faces.push_back(face); 
	}
	return 0;
}
