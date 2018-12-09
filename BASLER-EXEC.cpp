/*
Control Basler camera using pylon SDK and use OpenCV library
*/

#include <iostream>
#include <cstring>
#include <fstream>
#include <stdexcept>
#include <stdio.h>
#include <string>
#include <cstddef>
#include <unistd.h>

// Include Serial Communication
#include <wiringPi.h>
#include <wiringSerial.h>

// Include Basler API
#include <pylon/PylonIncludes.h>
#include <pylon/gige/BaslerGigEInstantCamera.h>
#include "include/SampleImageCreator.h"

// Include JSON
#include "include/json.hpp"

// Include files OpenCV
#include <opencv2/opencv.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>

struct INFO{
	std::string DATE;
	std::string LAT;
	std::string LNG;
	std::string BIN;
	std::string JPG;
	std::string SN;
	std::string MODEL;
	std::string BATT;
};


uint32_t imageWidth;
uint32_t imageHeight;
cv::Mat Img, roiImg;

bool enableGPS(int fd, bool onoff);
INFO GetGPS(int fd);
int CameraConfig();
INFO CameraFrame(INFO gps);
void MatWrite(const std::string& filename, const cv::Mat& mat);
cv::Mat MatRead(const std::string& filename);
std::string exec(std::string cmd);


// Namespace for using pylon objects.
using namespace GenApi;
using namespace Pylon;
using namespace std;
using json = nlohmann::json ;

// The name of the pylon feature stream file.
std::string version = "1.0";
const char Fileconfig[] = "ConfigCamera.pfs";
std::string path = "/media/usb-drive/";

const int ledPin = 13; // FLASH LED pin 13,
const int butPin = 26; // TRIGGER button pin 26,

int main(int argc, char* argv[]){
    // The exit code of the sample application.

  int fd;
  wiringPiSetupGpio(); // Initialize wiringPi -- using Broadcom pin numbers
  pinMode(ledPin, OUTPUT);     // Set regular LED as output
  pinMode(butPin, INPUT);      // Set button as INPUT
  pullUpDnControl(butPin, PUD_UP); // Enable pull-up resistor on button

  while ((fd = serialOpen ("/dev/ttyS0", 115200)) < 1)
  {
	printf ("Connecting to serial port:\n") ;
	delay (1000) ;
  }

  printf ("\nConnect port successful !\n::ttyS0 to 115200 baud::\n");

  /* Habilitar GPS via UART  */
  enableGPS(fd, true);

  /* Configurar Camara  */
  CameraConfig();

  std::cout << "\n --- TRIGGER: Trigger Mode ---" << endl;

  while(1)
  {
    if (digitalRead(butPin)) // Button is released if this returns 1
    {
			std::cout << "Trigger adquisition OFF" << std::endl;
			digitalWrite(ledPin, LOW);
	    delay(2500);
    }
    else // If digitalRead returns 0, button is pressed
    {

		delay(100);
		std::cout << "Trigger adquisition ON" << std::endl;

		/* OBTENER GPS: DATE, LAT y LNG  */
		INFO gps = GetGPS(fd);
		delay(100);

		std::cout << "DATE: " << gps.DATE << std::endl;
		std::cout << "Latitude: " << gps.LAT << std::endl;
		std::cout << "Longitude: " << gps.LNG << std::endl;

		digitalWrite(ledPin, HIGH); // Turn LED ON

		// ESTRUCTURA JSON PARA METADATOS
		INFO MD5 =CameraFrame(gps);
		delay(100);

		std::cout << "LED OFF" << std::endl;
        digitalWrite(ledPin, LOW); // Turn LED FLASH ON, via RELE

		// ESTRUCTURA JSON PARA METADATOS
		json js;
		js["id"] = gps.DATE;
		js["SWVersion"] = version;
		js["image"]["MD5"]["JPG"] = MD5.JPG;
		js["image"]["MD5"]["BIN"] = MD5.BIN;
		js["image"]["LAT"] = gps.LAT;
		js["image"]["LNG"] = gps.LNG;
		js["device"]["SN"] = MD5.SN;
		js["device"]["Model"] = MD5.MODEL;

		// GUARDAR JSON
		std::cout << "\n --- JSON: DATA---"  << std::endl;
		std::ofstream ofs;
		ofs.open (path + gps.DATE + ".json", std::ofstream::out | std::ofstream::app);
		ofs << js;
		ofs.close();
		cout << "JSON info Saved"<< endl;

		delay(500);

    }
  }

  return 0;

}

bool enableGPS(int fd, bool onoff){

  if (onoff){
	printf ("\n --- GPS: ENABLE---\n") ;
    serialPuts(fd,"AT+CGNSPWR=1\r") ;
    delay (100);
  }  else if (!onoff){
	printf ("\nGPS:DISABLE\n") ;
    serialPuts(fd, "AT+CGPS=0\r") ;
    delay (100) ;
	}

  while (serialDataAvail (fd))
  {
	printf ("%c", (char)serialGetchar (fd)) ;
	fflush (stdout) ;
  }

}

INFO GetGPS(int fd){
  char GPS[300];

  /* Obtener GPS mediante AT+CGNSINF  */
  printf ("\n --- GPS:Get GPS --- \n") ;
  fflush (stdout) ;
  serialPuts(fd, "AT+CGNSINF\r") ;
  delay (100) ;

  int k=0;
  while (serialDataAvail (fd))
  {
	GPS[k] = (char)serialGetchar(fd);
	fflush (stdout);
	k++;
  }

  delay(100);

  int j=0;
  std::string date;
  std::string lat;
  std::string lon;
  char *token = std::strtok(GPS, ",");
  while (token)
  {
    token = std::strtok(NULL, ",");
	if (j==1){
		date = token; //.find(".");
	}
	if (j==2){
		lat = token;
	}
	if (j==3){
		lon = token;
	}
	j++;
  }

  date = date.substr(0,14);
  INFO gps;
  gps.DATE = date;
  gps.LAT =  lat;
  gps.LNG = lon;

  return gps;
}

int CameraConfig(){

	std::cout << "\n --- CAMERA: CONFIG---"  << std::endl;

	int exitCode = 0;
	PylonInitialize();

	try
	{

		CTlFactory& tlFactory = CTlFactory::GetInstance();

		// Get all attached devices and exit application if no device is found.
		DeviceInfoList_t devices;
		if ( tlFactory.EnumerateDevices(devices) == 0 ){
            throw RUNTIME_EXCEPTION( "No camera present.");
        }

		int sensorid=-1;
		for ( size_t i = 0; i < devices.size(); ++i){
			if (devices[i].GetModelName().compare("acA4096-11gc")==0){
				sensorid=i;
				break;
			}
		}

		CInstantCamera camera(tlFactory.CreateDevice( devices[ sensorid ]));
		cout << "Using device " << camera.GetDeviceInfo().GetModelName() << endl;

		//Open the camera and save config.
		camera.Open();
		cout << "Loading file config to camera's..."<< endl;
		CFeaturePersistence::Load(Fileconfig, &camera.GetNodeMap(), true );
		cout << "Saved Config \n"<< endl;
		camera.Close();

	}
	catch (const GenericException &e)
    {
        // Error handling
        cerr << "An exception occurred." << endl
        << e.GetDescription() << endl;
        exitCode = 1;
    }

	// Releases all pylon resources.
    PylonTerminate();
    return exitCode;

}

INFO CameraFrame(INFO gps){

	//INFO gps
	std::cout << "\n --- CAMERA: FRAME---"  << std::endl;

	INFO MD5;

	int exitCode = 0;
	PylonInitialize();

	try
	{

		CGrabResultPtr ptrGrabResult;
		CTlFactory& tlFactory = CTlFactory::GetInstance();

		//Get all attached devices and exit application if no device is found.
		DeviceInfoList_t devices;
		if ( tlFactory.EnumerateDevices(devices) == 0 ){
            throw RUNTIME_EXCEPTION( "No camera present.");
        }

		int sensorid=-1;
		for ( size_t i = 0; i < devices.size(); ++i){
			if (devices[i].GetModelName().compare("acA4096-11gc")==0){
				sensorid=i;
				break;
			}
		}

		CInstantCamera camera(tlFactory.CreateDevice(devices[ sensorid ]));
		cout << "Using device " << camera.GetDeviceInfo().GetModelName() << endl;
		MD5.MODEL = camera.GetDeviceInfo().GetModelName();
		MD5.SN = camera.GetDeviceInfo().GetSerialNumber();
		camera.GrabOne(5000, ptrGrabResult);


		if (ptrGrabResult->GrabSucceeded())
		{
			//CImagePersistence::Save(ImageFileFormat_Tiff, "GrabbedImage.tiff", ptrGrabResult);
			imageWidth = ptrGrabResult->GetWidth();
			imageHeight = ptrGrabResult->GetHeight();

			uint16_t *pImageBuffer = (uint16_t *)ptrGrabResult->GetBuffer();
			cv::Mat ImgBayer16UC1(imageHeight, imageWidth, CV_16UC1, pImageBuffer, cv::Mat::AUTO_STEP);
			cv::Mat Img16UC3(imageHeight, imageWidth, CV_16UC3);
			cv::cvtColor(ImgBayer16UC1,Img16UC3,cv::COLOR_BayerBG2BGR);
			Img16UC3.convertTo(Img, CV_8UC3, 1.0/16);

			cv::imwrite(path + gps.DATE + ".jpg", Img);
			cout << "JPG frame Saved"<< endl;
			MD5.JPG = exec("md5sum " + path +  gps.DATE + ".jpg");
			MD5.JPG = MD5.JPG.substr(0,32);

			MatWrite(path + gps.DATE + ".bin", Img);
			cout << "BIN frame Saved"<< endl;
			MD5.BIN = exec("md5sum " + path + gps.DATE + ".bin");
			MD5.BIN = MD5.BIN.substr(0,32);

		}
	}
	catch (const GenericException &e)
    {
        // Error handling
        cerr << "An exception occurred." << endl
        << e.GetDescription() << endl;
        exitCode = 1;
    }

	// Releases all pylon resources.
    PylonTerminate();

	return MD5;

}

void MatWrite(const std::string& filename, const cv::Mat& mat){
	std::ofstream fs(filename, std::fstream::binary);

	// Header
	int type = mat.type();
	int channels = mat.channels();
	fs.write((char*)&mat.rows, sizeof(int));    // rows
	fs.write((char*)&mat.cols, sizeof(int));    // cols
	fs.write((char*)&type, sizeof(int));        // type
	fs.write((char*)&channels, sizeof(int));    // channels
												// Data
	if (mat.isContinuous())
	{
		fs.write(mat.ptr<char>(0), (mat.dataend - mat.datastart));
	}
	else
	{
		int rowsz = CV_ELEM_SIZE(type) * mat.cols;
		for (int r = 0; r < mat.rows; ++r)
		{
			fs.write(mat.ptr<char>(r), rowsz);
		}
	}
}

cv::Mat MatRead(const std::string& filename){
	std::ifstream fs(filename, std::fstream::binary);

	// Header
	int rows, cols, type, channels;
	fs.read((char*)&rows, sizeof(int));         // rows
	fs.read((char*)&cols, sizeof(int));         // cols
	fs.read((char*)&type, sizeof(int));         // type
	fs.read((char*)&channels, sizeof(int));     // channels

												// Data
	cv::Mat mat(rows, cols, type);
	fs.read((char*)mat.data, CV_ELEM_SIZE(type) * rows * cols);

	cv::imwrite("bin2jpg.jpg", mat);
	return mat;
}

std::string exec(std::string cmd) {
    char buffer[128];
    std::string result = "";
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) throw std::runtime_error("popen() failed!");
    try {
        while (!feof(pipe)) {
            if (fgets(buffer, 128, pipe) != NULL)
                result += buffer;
        }
    } catch (...) {
        pclose(pipe);
        throw;
    }
    pclose(pipe);
    return result;
}
