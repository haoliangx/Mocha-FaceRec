#ifndef FACE_RECOGNITION
#define FACE_RECOGNITION

#include <string>
#include <vector>
#include <cv.h>

#include <cvaux.h>

#include <highgui.h>
#include "opencv2/core/core.hpp"
#include "opencv2/gpu/gpu.hpp"

extern double PCFreq;

#define NEIGENVECTORS 100
#define DATABASE_INDEX "result.txt"
#define DATABASE_NAMES "names.txt"
#define DATABASE_FILE "facerec.dat"
#define CASCADE_PATH /*"lbpcascade_frontalface.xml"*/"haarcascade_frontalface_alt.xml"

typedef struct _face_t
{
	std::string name;
	std::string confidence;
	int error_no;
}face_t;

int faceDetect(cv::Mat &image, std::vector<cv::Mat> &faces, cv::CascadeClassifier, int init = 0);

int faceDetect_GPU(cv::Mat &image, std::vector<cv::Mat> &faces, int init = 0);

//input:image out: result
int faceRecLocal(cv::Mat &testFace, face_t &ret_val, int init = 0);
//input:image out: vector
int faceRecCloudlet(cv::Mat &testFace, cv::Mat &projection, int init = 0);
//input: vecotr output:result
int faceRecCloud(cv::Mat &projection, face_t &ret_val, int init = 0);

/*********************************************************************/

void databaseInit(cv::Ptr<cv::FaceRecognizer> &myFaceRecognizer, std::vector<std::string> &names, int &height, int &width);
void imagePreProcess(cv::Mat &testFace, int height, int width);
void read_csv(std::vector<cv::Mat>& images, std::vector<int>& labels, char separator);
std::string double2string(double n);
#endif