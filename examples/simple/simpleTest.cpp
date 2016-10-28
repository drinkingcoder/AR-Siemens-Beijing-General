/*
*  simpleTest.c
*
*  gsub-based example code to demonstrate use of ARToolKit.
*
*  Press '?' while running for help on available key commands.
*
*  Disclaimer: IMPORTANT:  This Daqri software is supplied to you by Daqri
*  LLC ("Daqri") in consideration of your agreement to the following
*  terms, and your use, installation, modification or redistribution of
*  this Daqri software constitutes acceptance of these terms.  If you do
*  not agree with these terms, please do not use, install, modify or
*  redistribute this Daqri software.
*
*  In consideration of your agreement to abide by the following terms, and
*  subject to these terms, Daqri grants you a personal, non-exclusive
*  license, under Daqri's copyrights in this original Daqri software (the
*  "Daqri Software"), to use, reproduce, modify and redistribute the Daqri
*  Software, with or without modifications, in source and/or binary forms;
*  provided that if you redistribute the Daqri Software in its entirety and
*  without modifications, you must retain this notice and the following
*  text and disclaimers in all such redistributions of the Daqri Software.
*  Neither the name, trademarks, service marks or logos of Daqri LLC may
*  be used to endorse or promote products derived from the Daqri Software
*  without specific prior written permission from Daqri.  Except as
*  expressly stated in this notice, no other rights or licenses, express or
*  implied, are granted by Daqri herein, including but not limited to any
*  patent rights that may be infringed by your derivative works or by other
*  works in which the Daqri Software may be incorporated.
*
*  The Daqri Software is provided by Daqri on an "AS IS" basis.  DAQRI
*  MAKES NO WARRANTIES, EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION
*  THE IMPLIED WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY AND FITNESS
*  FOR A PARTICULAR PURPOSE, REGARDING THE DAQRI SOFTWARE OR ITS USE AND
*  OPERATION ALONE OR IN COMBINATION WITH YOUR PRODUCTS.
*
*  IN NO EVENT SHALL DAQRI BE LIABLE FOR ANY SPECIAL, INDIRECT, INCIDENTAL
*  OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
*  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
*  INTERRUPTION) ARISING IN ANY WAY OUT OF THE USE, REPRODUCTION,
*  MODIFICATION AND/OR DISTRIBUTION OF THE DAQRI SOFTWARE, HOWEVER CAUSED
*  AND WHETHER UNDER THEORY OF CONTRACT, TORT (INCLUDING NEGLIGENCE),
*  STRICT LIABILITY OR OTHERWISE, EVEN IF DAQRI HAS BEEN ADVISED OF THE
*  POSSIBILITY OF SUCH DAMAGE.
*
*  Copyright 2015 Daqri LLC. All Rights Reserved.
*  Copyright 2002-2015 ARToolworks, Inc. All Rights Reserved.
*
*  Author(s): Hirokazu Kato, Philip Lamb.
*
*/



#ifdef _WIN32
#define _WINSOCKAPI_
#  include <windows.h>
#endif

#include <chrono>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
using std::string;
#ifndef __APPLE__
#  include <GL/gl.h>
#  include <GL/glut.h>
#else
#  include <OpenGL/gl.h>
#  include <GLUT/glut.h>
#endif

#include <AR/ar.h>
#include <AR/gsub.h>
#include <AR/video.h>
#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>
#include <vector>
#include "connection.h"
#include "car.h"
#include <sstream>
#include <iostream>
#include <map>

#define MYLOG(format, ...)

using std::stringstream;
using namespace std::chrono;
using namespace std;

#define             CPARA_NAME       "Data/camera_para.dat"
#define             VPARA_NAME       "Data/cameraSetting-%08x%08x.dat"
#define             PATT_NAME        "Data/hiro.patt"
#define             PATT_PATH        "Data/mydata"
#define             PATT_NUM         36
#define				MAX_MARKER_NUM	100
#define				PLANAR_ROWS 16
#define				PLANAR_COLS 8


typedef enum MarkerType {
	NONE,BLOCK05,BLOCK06,BLOCK20,ROADPIN,CARGEN,BUSGEN,WAYGEN,
	LCROSS, TCROSS, XCROSS, LIGHTLCROSS, LIGHTTCROSS, LIGHTXCROSS, PARK, STATION, ROTATE,
	M11, M12, M31, M32, M33, M34, M41, M42, M43, M51, M52, M53, M54, M55, M56, M57, M58, M551,M552,M61,
	NUM
} MarkerType;

typedef enum Orientation {
	UP, RIGHT, DOWN, LEFT
} Orientation;

typedef struct Marker {
	int x, y;
	MarkerType type;
	Orientation orientation;
} Marker;

typedef struct UnrestrictedMarker {
	BOOL flag;
	long int liveCount;
	const char* name;
} UnrestrictedMarker;

ARHandle           *arHandle;
ARPattHandle       *arPattHandle;
AR3DHandle         *ar3DHandle;
ARGViewportHandle  *vp;
int                 xsize, ysize;
int                 flipMode = 0;
int                 patt_id;
int                 patt_ids[PATT_NUM + 1];
double              patt_width = 80.0;
int                 fpsCount = 0;
char                fps[256];
char                errValue[256];
int                 distF = 0;
int                 contF = 0;
ARParamLT          *gCparamLT = NULL;

map<MarkerType, UnrestrictedMarker*> unRestrictedMarkerMap;
vector<MarkerType> unRestrictedMarkerSet;
//client socket
Server server;

static void   init(int argc, char *argv[]);
static void   keyFunc(unsigned char key, int x, int y);
static void   cleanup(void);
static void   mainLoop(void);
static void   draw(ARdouble trans[3][4]);
static void	  drawRed(ARdouble trans[3][4]);
Orientation computeOrientation(ARdouble trans[3][4]);
void LoadPatt();
//void resetGL();

ARdouble LUTrans[3][4] = {
	{ 1, 0, 0, -568 },
	{ 0, 1, 0, -341 },
	{ 0, 0, 1, 1347 }
};

ARdouble LDTrans[3][4] = {
	{ 1, 0, 0, -595 },
	{ 0, 1, 0, 411 },
	{ 0, 0, 1, 1444 }
};

ARdouble RUTrans[3][4] = {
	{ 1, 0, 0, 590 },
	{ 0, 1, 0, -343 },
	{ 0, 0, 1, 1332 }
};

ARdouble RDTrans[3][4] = {
	{ 1, 0, 0, 614 },
	{ 0, 1, 0, 408 },
	{ 0, 0, 1, 1419 }
};


void LoadPatt()
{
	char PattPath[] = PATT_PATH;
	char PattName[100];
	char DatName[100];
	char PattPathName[100];
	memset(DatName, 0, sizeof(DatName));
	memset(PattPathName, 0, sizeof(PattPathName));
	memset(PattName, 0, sizeof(PattName));
	sprintf(DatName, "%s/marker.dat", PattPath);
	MYLOG("DatName = %s\n", DatName);
	FILE* fp = fopen(DatName, "r");
	if (fp == NULL) {
		ARLOGe("marker can't read!\n");
		system("pause");
		exit(0);
	}
	for (int i = 1; i <= PATT_NUM; i++) {
		fscanf(fp, "%s", PattName);
		MYLOG("PattName = %s\n", PattName);
		sprintf(PattPathName, "%s/%s", PATT_PATH, PattName);
		if ((patt_ids[i] = arPattLoad(arPattHandle, PattPathName)) < 0) {
			sprintf(PattName, "pattern[%d] load error !!\n", i);
			ARLOGe(PattName);
			system("pause");
			exit(0);
		}
	}
}

int main(int argc, char *argv[])
{
	glutInit(&argc, argv);
	init(argc, argv);

	// socket
	/*client.InitSocket();
	client.Connect("127.0.0.1", "56025");*/
	/*carServer.SetAddr("127.0.0.1", "56025");

	carGenerator.InitCrosses();
	while (true)
		testLoop();*/


	//server socket
	server.SetAddr("192.168.1.2", "56025");

	preMs = duration_cast<milliseconds>(system_clock::now().time_since_epoch());
	argSetDispFunc(mainLoop, 1);
	argSetKeyFunc(keyFunc);
	//arSetLabelingThreshMode(arHandle,AR_LABELING_THRESH_MODE_AUTO_ADAPTIVE);
	arSetLabelingThresh(arHandle,85);
	fpsCount = 0;
	fps[0] = '\0';
	arUtilTimerReset();
	argMainLoop();

	//client.Close();
	server.Close();

	return (0);
}

static void keyFunc(unsigned char key, int x, int y)
{
	int   value;
	//system("pause");
	switch (key) {
	case 0x1b:
		cleanup();
		exit(0);
		break;
	case '1':
	case '-':
		arGetLabelingThresh(arHandle, &value);
		value -= 5;
		if (value < 0) value = 0;
		arSetLabelingThresh(arHandle, value);
		printf("thresh = %d\n", value);
		//system("pause");
		break;
	case '2':
	case '+':
		arGetLabelingThresh(arHandle, &value);
		value += 5;
		if (value > 255) value = 255;
		arSetLabelingThresh(arHandle, value);
		printf("thresh = %d\n", value);
		//system("pause");
		break;
	case 'd':
	case 'D':
		arGetDebugMode(arHandle, &value);
		value = 1 - value;
		arSetDebugMode(arHandle, value);
		break;
	case 'h':
	case 'H':
		if (flipMode & AR_GL_FLIP_H) flipMode = flipMode & AR_GL_FLIP_V;
		else                         flipMode = flipMode | AR_GL_FLIP_H;
		argViewportSetFlipMode(vp, flipMode);
		break;
	case 'v':
	case 'V':
		if (flipMode & AR_GL_FLIP_V) flipMode = flipMode & AR_GL_FLIP_H;
		else                         flipMode = flipMode | AR_GL_FLIP_V;
		argViewportSetFlipMode(vp, flipMode);
		break;
	case ' ':
		distF = 1 - distF;
		if (distF) {
			argViewportSetDistortionMode(vp, AR_GL_DISTORTION_COMPENSATE_ENABLE);
		}
		else {
			argViewportSetDistortionMode(vp, AR_GL_DISTORTION_COMPENSATE_DISABLE);
		}
		break;
	case 'c':
		contF = 1 - contF;
		break;
	case '?':
	case '/':
		ARLOG("Keys:\n");
		ARLOG(" [esc]         Quit demo.\n");
		ARLOG(" - and +       Adjust threshhold.\n");
		ARLOG(" d             Activate / deactivate debug mode.\n");
		ARLOG(" h and v       Toggle horizontal / vertical flip mode.\n");
		ARLOG(" [space]       Toggle distortion compensation.\n");
		ARLOG(" ? or /        Show this help.\n");
		ARLOG("\nAdditionally, the ARVideo library supplied the following help text:\n");
		arVideoDispOption();
		break;
	default:
		break;
	}
}


void transferToJson(rapidjson::Document& doc, int markernum, Marker* markers)
{
	curMs = duration_cast<milliseconds>(system_clock::now().time_since_epoch());
	std::chrono::duration<double, std::milli> fp_ms = curMs - preMs;
	carGenerator.crosses.clear();
	//block size
	rapidjson::Document::AllocatorType& allocator = doc.GetAllocator();
	rapidjson::Value width(32);
	rapidjson::Value height(16);
	rapidjson::Value blockSize;
	blockSize.SetObject();
	blockSize.AddMember("width", width, allocator);
	blockSize.AddMember("height", height, allocator);
	doc.AddMember("blockSize", blockSize, allocator);

	/*
	doc.AddMember("rotate", rotateFlag, allocator);
	doc.AddMember("roadPin", roadPinFlag, allocator);
	doc.AddMember("carGen", carGenFlag, allocator);
	*/
	for (MarkerType x : unRestrictedMarkerSet)
	{
		rapidjson::Value strValue(rapidjson::kStringType);
		strValue.SetString(unRestrictedMarkerMap[x]->name,allocator);
		doc.AddMember(strValue, unRestrictedMarkerMap[x]->flag, allocator);
	}

	rapidjson::Value mkArray(rapidjson::kArrayType);
	for (int i = 0; i < markernum; i++)
	{
		rapidjson::Value mk(rapidjson::kObjectType);
		rapidjson::Value span(rapidjson::kObjectType);
		rapidjson::Value spanX(3);
		rapidjson::Value spanY(2);
		rapidjson::Value x(markers[i].x);
		rapidjson::Value y(markers[i].y);
		rapidjson::Value type((int)(markers[i].type));
		rapidjson::Value orientation((int)(markers[i].orientation + 1));

		mk.AddMember("x", x, allocator);
		mk.AddMember("y", y, allocator);
		//mk.AddMember("type", type, allocator);
		mk.AddMember("type", "BLOCK", allocator);
		switch (markers[i].type)
		{
			/*
		case BLOCK01:mk.AddMember("id", "1", allocator); break;
		case BLOCK02:mk.AddMember("id", "2", allocator); break;
		case BLOCK03:mk.AddMember("id", "3", allocator); break;
		case BLOCK04:mk.AddMember("id", "4", allocator); break;
		case BLOCK05:mk.AddMember("id", "5", allocator); break;
		case BLOCK06:mk.AddMember("id", "6", allocator); break;
		case BLOCK07:mk.AddMember("id", "7", allocator); break;
		case BLOCK08:mk.AddMember("id", "8", allocator); break;
		case BLOCK09:mk.AddMember("id", "9", allocator); break;
		case BLOCK11:mk.AddMember("id", "11", allocator); break;
		case BLOCK12:mk.AddMember("id", "12", allocator); break;
		case BLOCK13:mk.AddMember("id", "13", allocator); break;
		case BLOCK14:mk.AddMember("id", "14", allocator); break;
		case BLOCK15:mk.AddMember("id", "15", allocator); break;
		case BLOCK16:mk.AddMember("id", "16", allocator); break;
		case BLOCK17:mk.AddMember("id", "17", allocator); break;
		case BLOCK18:mk.AddMember("id", "18", allocator); break;
		case BLOCK19:mk.AddMember("id", "19", allocator); break;
		case BLOCK20:mk.AddMember("id", "20", allocator); break;
		*/
		case BLOCK05:mk.AddMember("id", "5", allocator); break;
		case BLOCK06:mk.AddMember("id", "6", allocator); break;
		case BLOCK20:mk.AddMember("id", "20", allocator); break;
		case TCROSS:
		{
			mk.AddMember("id", "tcross", allocator);
			Cross cross;
			cross.position[0] = markers[i].x;
			cross.position[1] = markers[i].y;
			cross.orientation = markers[i].orientation + 1;
			cross.type = "tcross";
			carGenerator.crosses.push_back(cross);
			break;
		}
		case LCROSS:
		{
			mk.AddMember("id", "lcross", allocator);
			Cross cross;
			cross.position[0] = markers[i].x;
			cross.position[1] = markers[i].y;
			cross.orientation = markers[i].orientation + 1;
			cross.type = "lcross";
			carGenerator.crosses.push_back(cross);
			break;
		}
		case XCROSS:
		{
			mk.AddMember("id", "xcross", allocator);
			Cross cross;
			cross.position[0] = markers[i].x;
			cross.position[1] = markers[i].y;
			cross.orientation = markers[i].orientation + 1;
			cross.type = "xcross";
			carGenerator.crosses.push_back(cross);
			break;
		}
		case LIGHTTCROSS:
		{
			mk.AddMember("id", "tcross_greenlight", allocator);
			Cross cross;
			cross.position[0] = markers[i].x;
			cross.position[1] = markers[i].y;
			cross.orientation = markers[i].orientation + 1;
			cross.type = "tcross_greenlight";
			carGenerator.crosses.push_back(cross);
			break;
		}
		case LIGHTLCROSS:
		{
			mk.AddMember("id", "lcross_greenlight", allocator);
			Cross cross;
			cross.position[0] = markers[i].x;
			cross.position[1] = markers[i].y;
			cross.orientation = markers[i].orientation + 1;
			cross.type = "lcross_greenlight";
			carGenerator.crosses.push_back(cross);
			break;
		}
		case LIGHTXCROSS:
		{
			mk.AddMember("id", "xcross_greenlight", allocator);
			Cross cross;
			cross.position[0] = markers[i].x;
			cross.position[1] = markers[i].y;
			cross.orientation = markers[i].orientation + 1;
			cross.type = "xcross_greenlight";
			carGenerator.crosses.push_back(cross);
			break;
		}
		case STATION:mk.AddMember("id", "station", allocator); break;
		case PARK:mk.AddMember("id", "park", allocator); break;
		case M11:mk.AddMember("id", "oldChargingStation", allocator); break;
		case M12:mk.AddMember("id", "newChargingStation", allocator); break;
		case M31:mk.AddMember("id", "largeChargingStation", allocator); break;
		case M32:mk.AddMember("id", "container", allocator); break;
		case M33:mk.AddMember("id", "holder", allocator); break;
		case M34:mk.AddMember("id", "electricConnector", allocator); break;
		case M41:mk.AddMember("id", "MV", allocator); break;
		case M42:mk.AddMember("id", "transformer", allocator); break;
		case M43:mk.AddMember("id", "charge", allocator); break;
		case M51:mk.AddMember("id", "largeBus", allocator); break;
		case M52:mk.AddMember("id", "underpan", allocator); break;
		case M53:mk.AddMember("id", "underpanDC", allocator); break;
		case M54:mk.AddMember("id", "underpanDCCom", allocator); break;
		case M55:mk.AddMember("id", "underpanDCComTrack", allocator); break;
		case M56:mk.AddMember("id", "DC", allocator); break;
		case M57:mk.AddMember("id", "track", allocator); break;
		case M58:mk.AddMember("id", "com", allocator); break;
		case M551:mk.AddMember("id", "underpanDCComTrack1", allocator); break;
		case M552:mk.AddMember("id", "underpanDCComTrack2", allocator); break;
		case M61:mk.AddMember("id", "busWithChargingStation", allocator); break;
		default:
			mk.AddMember("id", "4", allocator); break;
			break;
		}
		//mk.AddMember("id", "building01", allocator);
		mk.AddMember("orientation", orientation, allocator);
		span.AddMember("x", spanX, allocator);
		span.AddMember("y", spanY, allocator);
		mk.AddMember("span", span, allocator);

		mkArray.PushBack(mk, allocator);

		MYLOG("%d\t%d\n", markers[i].x, markers[i].y);
		switch (markers[i].orientation)
		{
		case UP:MYLOG("up\n"); break;
		case DOWN:MYLOG("down\n"); break;
		case LEFT:MYLOG("left\n"); break;
		case RIGHT:MYLOG("right\n"); break;
		default:
			break;
		}
	}
	MYLOG("marker number = %d\n", markernum);
	doc.AddMember("entry", mkArray, allocator);

	//cars
	rapidjson::Value cars(rapidjson::kArrayType);
	if (fp_ms >= std::chrono::duration<double, std::milli>(2000.0f))
	{
	//	printf("car gen\n");
		carGenerator.GenerateCars(cars, allocator);
		preMs = curMs;
	}
	doc.AddMember("cars", cars, allocator);
}

string printJson(rapidjson::Document& doc)
{
	stringstream ss;
	ss.str("");
	rapidjson::StringBuffer strbuf;
	rapidjson::Writer<rapidjson::StringBuffer> writer(strbuf);
	doc.Accept(writer);

	const char* jstr = strbuf.GetString();
	ss << jstr;
	//MYLOG("%s\n", jstr);
	return ss.str();
}

Orientation computeOrientation(ARdouble trans[3][4])
{
	ARdouble x[3] = { 1, 0, 0 };
	ARdouble resx[3] = { 0, 0, 0 };
	for (int i = 0; i < 3; i++)
		for (int j = 0; j < 3; j++)
			resx[i] += trans[i][j] * x[j];
	//resx[0] = resx[0] / (sqrt(resx[0] * resx[0] + resx[1] * resx[1]));
	//resx[1] = resx[1] / (sqrt(resx[0] * resx[0] + resx[1] * resx[1]));

	if (resx[0] >= 0 && resx[0] - abs(resx[1]) >= 0) return Orientation::UP;
	else if (resx[1] >= 0 && abs(resx[0]) - resx[1] < 0) return Orientation::LEFT;
	else if (resx[0] < 0 && abs(resx[0]) - abs(resx[1]) >= 0) return Orientation::DOWN;
	else return Orientation::RIGHT;

}

static void mainLoop(void)
{
	static int      contF2 = 0;
	static ARdouble patt_trans[3][4];
	static ARUint8 *dataPtr = NULL;
	static long int liveCount[PLANAR_ROWS][PLANAR_COLS];
	static MarkerType markerType[PLANAR_ROWS][PLANAR_COLS];
	static Orientation markerOrientation[PLANAR_ROWS][PLANAR_COLS];
	static int initFlag = 1;
	static int x, y;
	static MarkerType type;
	static Orientation orientation;
	ARMarkerInfo   *markerInfo;
	int             markerNum;
	ARdouble        err;
	int             imageProcMode;
	int             debugMode;
	int             j;
	Marker				markers[MAX_MARKER_NUM];
	int recognizedMarkerNum = 0;
	/*
	static long int rotatemarker = 0;
	static long int roadPinMarker = 0;
	static long int carGenMarker = 0;
	*/

	/*first visit*/
	if (initFlag == 1) {
		for (int i = 0; i < PLANAR_ROWS; i++)
			for (int j = 0; j < PLANAR_COLS; j++)
			{
				markerType[i][j] = NONE;
				liveCount[i][j] = 0;
			}
		initFlag = 0;
	}

	/* grab a video frame */
	if ((dataPtr = (ARUint8 *)arVideoGetImage()) == NULL) {
		arUtilSleep(2);
		return;
	}

	argDrawMode2D(vp);
	arGetDebugMode(arHandle, &debugMode);
	if (debugMode == 0) {
		argDrawImage(dataPtr);
	}
	else {
		arGetImageProcMode(arHandle, &imageProcMode);
		if (imageProcMode == AR_IMAGE_PROC_FRAME_IMAGE) {
			argDrawImage(arHandle->labelInfo.bwImage);
		}
		else {
			argDrawImageHalf(arHandle->labelInfo.bwImage);
		}
	}
	drawRed(LUTrans);
	drawRed(LDTrans);
	drawRed(RUTrans);
	drawRed(RDTrans);
	//  MYLOG("xsize = %d, ysize = %d\n",arHandle->xsize,arHandle->ysize);
	/* detect the markers in the video frame */
	if (arDetectMarker(arHandle, dataPtr) < 0) {
		cleanup();
		exit(0);
	}

	if (fpsCount % 60 == 0) {
		sprintf(fps, "%f[fps]", 60.0 / arUtilTimer());
		arUtilTimerReset();
	}
	fpsCount++;
	glColor3f(0.0f, 1.0f, 0.0f);
	argDrawStringsByIdealPos(fps, 10, ysize - 30);

	markerNum = arGetMarkerNum(arHandle);
	/*
	if (markerNum == 0) {
		argSwapBuffers();
		return;
	}
	*/

	/* check for object visibility */
	markerInfo = arGetMarker(arHandle);
	// resetGL();
	rapidjson::Document doc;
	doc.SetObject();
	//MYLOG("begin!\n");
	int num = 0;
	for (int j = 0; j<markerNum; j++)
	{
		for (int i = 1; i <= PATT_NUM; i++) {
			// ARLOG("ID=%d, CF = %f\n", markerInfo[j].id, markerInfo[j].cf);
			if (patt_ids[i] == markerInfo[j].id) {
				//  if( k == -1 ) {
				if (markerInfo[j].cf >= 0.7) {
					num++;
					//           MYLOG("line = %d\n",arHandle->markerInfo2[j].coord_num/4);
					err = arGetTransMatSquare(ar3DHandle, &(markerInfo[j]), patt_width, patt_trans);
					sprintf(errValue, "err = %f", err);  
					glColor3f(0.0f, 1.0f, 0.0f);
					argDrawStringsByIdealPos(fps, 10, ysize - 30);
					argDrawStringsByIdealPos(errValue, 10, ysize - 60);
					//           ARLOG("err = %f\n", err);



					/*work out the coordinate&&orientation of the marker on the planar*/
					//MYLOG("%d\t%d\t%d\n",(int)(patt_trans[0][3]),(int)(patt_trans[1][3]),(int)(patt_trans[2][3]));
					//MYLOG("%f\t%f\n", patt_trans[0][3] / patt_trans[2][3], patt_trans[1][3] / patt_trans[2][3]);
					//	printf("%f\t%f\n", patt_trans[0][3] / patt_trans[2][3], patt_trans[1][3]/patt_trans[2][3]);

					static double	xlu = -0.4, ylu = -0.27,
						xld = -0.4, yld = 0.3,
						xru = 0.44, yru = -0.27,
						xrd = 0.44, yrd = 0.3;

					static double	yu, yd;
					static double	x0, y0;
					static double	lenLeft = yld - ylu, lenRight = yrd - yru;

					static double	len;
					static double deltaxl = 0.026, deltaxr = 0.034;
					static double accx = 0, xnum = 0, accy = 0, accz = 0;
					x0 = patt_trans[0][3] / patt_trans[2][3];
					y0 = patt_trans[1][3] / patt_trans[2][3];

					accx += x0;
					accy += y0;
					accz += patt_trans[2][3];
					xnum += 1;
					//	printf("zmean = %lf\n", accz/xnum);
					//	printf("current z = %lf\n", patt_trans[2][3]);



					//printf("ymean = %lf\n", accy / xnum);
					//MYLOG("x0 = %lf, y0 = %lf\n", x0, y0);
					//xu = (xru - x0) / (xru - xlu) * 31;
					//xd = (xrd - x0) / (xrd - xld) * 31;
					//MYLOG("xu = %lf, xd = %lf\n", xu, xd);
					//x = (int)((yrd - y0) / (yrd - yru)*xd + (y0 - yru) / (yrd - yru)*xu + 0.5);

					//out of the plane
					static double xlen = 16.0, ylen = 8.0;

					if (x0 < xlu - (xru - xlu) / (xlen - 1) / 2.0)	continue;
					if (patt_trans[2][3] <= 1300.0 || patt_trans[2][3] >= 1500.0) continue;
					//else printf("out of plane!\n");
					x = (int)((x0 - (xlu)) / ((xru)-(xlu)) * (xlen - 1) + 0.5);
					//	len = x / (xlen - 1)*lenRight + ((xlen - 1) - x) / (xlen - 1)*lenLeft;
					//x = x + 6;
					//	yu = yrd - len;
					//MYLOG("yu = %lf\n",yu);
					MYLOG("!!!!!! =  %lf\n", (x0 - (xlu)) / ((xru)-(xlu)));
					//printf("x =  %d\n", x);

					y = (int)((y0 - yru) / (yrd - yru)*(ylen - 1) + 0.5);

					/*
					if( patt_trans[0][3] > 0 ) x = (int)(patt_trans[0][3] / patt_trans[2][3] / 0.03) + 1;
					else x = (int)(patt_trans[0][3] / patt_trans[2][3] / 0.03) - 1;
					if( patt_trans[1][3] > 0 )	y = (int)(patt_trans[1][3] / patt_trans[2][3] / 0.048) + 1;
					else y = (int)(patt_trans[1][3] / patt_trans[2][3] / 0.048) - 1;
					x = x + PLANAR_ROWS / 2;
					y = y + PLANAR_COLS / 2;
					*/

					//MYLOG("x = %d, y = %d\n", x, y);
					if (x >= PLANAR_ROWS || x < 0) {
						MYLOG("x = %d out of range!\n", x);
						continue;
					}
					if (y >= PLANAR_COLS || y < 0) {
						MYLOG("y = %d out of range!\n", y);
						continue;
					}
					if (patt_ids[i] >= MarkerType::NUM) type = MarkerType::NONE;
					else type = (MarkerType)(patt_ids[i] + 1);
					orientation = computeOrientation(patt_trans);

					/*fill the marker into the cell && make it be reconized robust*/
					//			MYLOG("type = %d\n", (int)type);
					//			MYLOG("markerType[x][y] = %d\n", markerType[x][y]);
					if (type == NONE) continue;
					static BOOL unRestrictedMarkerFindFlag;
					unRestrictedMarkerFindFlag = false;
					for (MarkerType x : unRestrictedMarkerSet)
						if (type == x)
						{
							unRestrictedMarkerMap[x]->liveCount |= 0x1;
							unRestrictedMarkerFindFlag = true;
							break;
						}
					if (unRestrictedMarkerFindFlag) continue;
					/*
					if (type == ROTATE)
					{
						rotatemarker |= 0x1;
						continue;
					}
					if (type == ROADPIN)
					{
						roadPinMarker |= 0x1;
						continue;
					}
					if (type == CARGEN)
					{
						carGenMarker |= 0x1;
						continue;
					}
					*/

					if (markerType[x][y] == type && markerOrientation[x][y] == orientation)
					{
						liveCount[x][y] |= 0x1;
						//				markerOrientation[x][y] = orientation;
						//				MYLOG("entry1\n");
					}
					else if (markerType[x][y] == NONE)
					{
						markerOrientation[x][y] = orientation;
						markerType[x][y] = type;
						liveCount[x][y] |= 0x1;
						//				MYLOG("entry2");
					}
					draw(patt_trans);

				}
				//     } //else if( markerInfo[j].cf > markerInfo[k].cf ) k = j;
			}
		}
		//draw(patt_trans);
	}
	//MYLOG("num = %d\n",num);
	//MYLOG("end!\n");
	/* live count increase*/
	for (int i = 0; i < PLANAR_ROWS; i++)
		for (int j = 0; j < PLANAR_COLS; j++)
		{
			liveCount[i][j] <<= 1;
			liveCount[i][j] &= 0xffffff;
			if (liveCount[i][j] == 0) markerType[i][j] = NONE;
		}
	for (MarkerType x : unRestrictedMarkerSet)
	{
		unRestrictedMarkerMap[x]->liveCount <<= 1;
		unRestrictedMarkerMap[x]->liveCount &= 0xffffff;
		if (unRestrictedMarkerMap[x]->liveCount == 0) unRestrictedMarkerMap[x]->flag = false;
		else unRestrictedMarkerMap[x]->flag = true; 
	}
	/*
	rotatemarker <<= 1;
	rotatemarker &= 0xffffff;
	if (rotatemarker == 0) rotateFlag = false;
	else rotateFlag = true;

	roadPinMarker <<= 1;
	roadPinMarker &= 0xffffff;
	if(roadPinMarker == 0) roadPinFlag = false;
	else roadPinFlag = true;

	carGenMarker <<= 1;
	carGenMarker &= 0xffffff;
	if (carGenMarker == 0) carGenFlag = false;
	else carGenFlag = true;
	*/
	/*record the reconized markers*/
	for (int i = 0; i < PLANAR_ROWS; i++)
		for (int j = 0; j < PLANAR_COLS; j++)
		{
			if (markerType[i][j] == NONE) continue;

			markers[recognizedMarkerNum].x = i;
			markers[recognizedMarkerNum].y = j;
			markers[recognizedMarkerNum].type = markerType[i][j];
			markers[recognizedMarkerNum].orientation = markerOrientation[i][j];
			/*
			switch (markers[recognizedMarkerNum].orientation)
			{
			case Orientation::UP: MYLOG("UP\n"); break;
			case Orientation::LEFT: MYLOG("LEFT\n"); break;
			case Orientation::RIGHT: MYLOG("RIGHT\n"); break;
			case Orientation::DOWN: MYLOG("DOWN\n"); break;
			default: MYLOG("Orientation Err!\n"); break;
			}
			switch (markers[recognizedMarkerNum].type)
			{
			case TAXI:MYLOG("taxi\n"); break;
			case BUS:MYLOG("bus\n"); break;
			case TCROSS:MYLOG("tcross\n"); break;
			default: MYLOG("Marker type haven't been imported!\n"); break;
			}
			*/
			recognizedMarkerNum++;
			//MYLOG("%d %d\n", i, j);
		}
	//MYLOG("reconized markers = %d\n", recognizedMarkerNum);
	transferToJson(doc, recognizedMarkerNum, markers);
	static string oldjson;
	string json = printJson(doc);
	//	if (oldjson != json) system("pause");
	cout << json << endl;
	oldjson = json;

	server.Send(json);

	argSwapBuffers();
}

static void   init(int argc, char *argv[])
{
	ARParam         cparam;
	ARGViewport     viewport;
	char            vconf[512];
	AR_PIXEL_FORMAT pixFormat;
	ARUint32        id0, id1;
	int             i;

	if (argc == 1) vconf[0] = '\0';
	else {
		strcpy(vconf, argv[1]);
		for (i = 2; i < argc; i++) { strcat(vconf, " "); strcat(vconf, argv[i]); }
	}

	unRestrictedMarkerMap[ROTATE] = (UnrestrictedMarker*)malloc(sizeof(UnrestrictedMarker));
	unRestrictedMarkerMap[CARGEN] = (UnrestrictedMarker*)malloc(sizeof(UnrestrictedMarker));
	unRestrictedMarkerMap[WAYGEN] = (UnrestrictedMarker*)malloc(sizeof(UnrestrictedMarker));
	unRestrictedMarkerMap[ROADPIN] = (UnrestrictedMarker*)malloc(sizeof(UnrestrictedMarker));
	unRestrictedMarkerMap[BUSGEN] = (UnrestrictedMarker*)malloc(sizeof(UnrestrictedMarker));
	unRestrictedMarkerSet.push_back(ROTATE);
	unRestrictedMarkerSet.push_back(CARGEN);
	unRestrictedMarkerSet.push_back(WAYGEN);
	unRestrictedMarkerSet.push_back(ROADPIN);
	unRestrictedMarkerSet.push_back(BUSGEN);
	unRestrictedMarkerMap[ROTATE]->name = "rotate";
	unRestrictedMarkerMap[CARGEN]->name = "carGen";
	unRestrictedMarkerMap[WAYGEN]->name = "wayGen";
	unRestrictedMarkerMap[ROADPIN]->name = "roadPin";
	unRestrictedMarkerMap[BUSGEN]->name = "busGen";
	/*
	strcpy(unRestrictedMarkerMap[ROTATE]->name,"rotate");
	strcpy(unRestrictedMarkerMap[CARGEN]->name, "carGen");
	strcpy(unRestrictedMarkerMap[WAYGEN]->name, "wayGen");
	strcpy(unRestrictedMarkerMap[ROADPIN]->name, "roadPin");
	strcpy(unRestrictedMarkerMap[BUSGEN]->name, "busGen");
	*/
	for (MarkerType x : unRestrictedMarkerSet)
	{
		unRestrictedMarkerMap[x]->flag = false;
		unRestrictedMarkerMap[x]->liveCount = 0;
	}

	/* open the video path */
	ARLOGi("Using video configuration '%s'.\n", vconf);
	if (arVideoOpen(vconf) < 0) exit(0);
	if (arVideoGetSize(&xsize, &ysize) < 0) exit(0);
	ARLOGi("Image size (x,y) = (%d,%d)\n", xsize, ysize);
	if ((pixFormat = arVideoGetPixelFormat()) < 0) exit(0);
	if (arVideoGetId(&id0, &id1) == 0) {
		ARLOGi("Camera ID = (%08x, %08x)\n", id1, id0);
		sprintf(vconf, VPARA_NAME, id1, id0);
		if (arVideoLoadParam(vconf) < 0) {
			ARLOGe("No camera setting data!!\n");
		}
	}

	/* set the initial camera parameters */
	if (arParamLoad(CPARA_NAME, 1, &cparam) < 0) {
		ARLOGe("Camera parameter load error !!\n");
		exit(0);
	}
	arParamChangeSize(&cparam, xsize, ysize, &cparam);
	ARLOG("*** Camera Parameter ***\n");
	arParamDisp(&cparam);
	if ((gCparamLT = arParamLTCreate(&cparam, AR_PARAM_LT_DEFAULT_OFFSET)) == NULL) {
		ARLOGe("Error: arParamLTCreate.\n");
		exit(-1);
	}

	if ((arHandle = arCreateHandle(gCparamLT)) == NULL) {
		ARLOGe("Error: arCreateHandle.\n");
		exit(0);
	}
	if (arSetPixelFormat(arHandle, pixFormat) < 0) {
		ARLOGe("Error: arSetPixelFormat.\n");
		exit(0);
	}

	if ((ar3DHandle = ar3DCreateHandle(&cparam)) == NULL) {
		ARLOGe("Error: ar3DCreateHandle.\n");
		exit(0);
	}

	if ((arPattHandle = arPattCreateHandle()) == NULL) {
		ARLOGe("Error: arPattCreateHandle.\n");
		exit(0);
	}

	LoadPatt();
	/*
	if( (patt_id=arPattLoad(arPattHandle, PATT_NAME)) < 0 ) {
	ARLOGe("pattern load error !!\n");
	exit(0);
	}
	*/
	arPattAttach(arHandle, arPattHandle);

	/* open the graphics window */
	/*
	int winSizeX, winSizeY;
	argCreateFullWindow();
	argGetScreenSize( &winSizeX, &winSizeY );
	viewport.sx = 0;
	viewport.sy = 0;
	viewport.xsize = winSizeX;
	viewport.ysize = winSizeY;
	*/
	viewport.sx = 0;
	viewport.sy = 0;
	viewport.xsize = xsize;
	viewport.ysize = ysize;
	if ((vp = argCreateViewport(&viewport)) == NULL) exit(0);
	argViewportSetCparam(vp, &cparam);
	argViewportSetPixFormat(vp, pixFormat);
	//argViewportSetDispMethod( vp, AR_GL_DISP_METHOD_GL_DRAW_PIXELS );
	argViewportSetDistortionMode(vp, AR_GL_DISTORTION_COMPENSATE_DISABLE);

	if (arVideoCapStart() != 0) {
		ARLOGe("video capture start error !!\n");
		exit(0);
	}
}

/* cleanup function called when program exits */
static void cleanup(void)
{
	arVideoCapStop();
	argCleanup();
	arPattDetach(arHandle);
	arPattDeleteHandle(arPattHandle);
	ar3DDeleteHandle(&ar3DHandle);
	arDeleteHandle(arHandle);
	arParamLTFree(&gCparamLT);
	arVideoClose();
}

static void draw(ARdouble trans[3][4])
{
	ARdouble  gl_para[16];
	GLfloat   mat_diffuse[] = { 0.0f, 0.0f, 1.0f, 0.0f };
	GLfloat   mat_flash[] = { 1.0f, 1.0f, 1.0f, 0.0f };
	GLfloat   mat_flash_shiny[] = { 50.0f };
	GLfloat   light_position[] = { 100.0f, -200.0f, 200.0f, 0.0f };
	GLfloat   light_ambi[] = { 0.1f, 0.1f, 0.1f, 0.0f };
	GLfloat   light_color[] = { 1.0f, 1.0f, 1.0f, 0.0f };

	argDrawMode3D(vp);
	glClearDepth(1.0);
	glClear(GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);

	/* load the camera transformation matrix */
	argConvGlpara(trans, gl_para);
	glMatrixMode(GL_MODELVIEW);
#ifdef ARDOUBLE_IS_FLOAT
	glLoadMatrixf(gl_para);
#else
	glLoadMatrixd(gl_para);
#endif

	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, 1);
	glLightfv(GL_LIGHT0, GL_POSITION, light_position);
	glLightfv(GL_LIGHT0, GL_AMBIENT, light_ambi);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, light_color);
	glLightfv(GL_LIGHT0, GL_SPECULAR, light_color);
	glMaterialfv(GL_FRONT, GL_SPECULAR, mat_flash);
	glMaterialfv(GL_FRONT, GL_SHININESS, mat_flash_shiny);
	glMaterialfv(GL_FRONT, GL_DIFFUSE, mat_diffuse);
	glMaterialfv(GL_FRONT, GL_AMBIENT, mat_diffuse);

#if 1
	glTranslatef(0.0f, 0.0f, 40.0f);
	glutSolidCube(80.0);
#else
	glTranslatef(0.0f, 0.0f, 20.0f);
	glRotatef(90.0f, 1.0f, 0.0f, 0.0f);
	glutSolidTeapot(40.0);
#endif
	glDisable(GL_LIGHT0);
	glDisable(GL_LIGHTING);

	glDisable(GL_DEPTH_TEST);
}

static void drawRed(ARdouble trans[3][4])
{
	ARdouble  gl_para[16];
	GLfloat   mat_diffuse[] = { 1.0f, 0.0f, 0.0f, 0.0f };
	GLfloat   mat_flash[] = { 1.0f, 1.0f, 1.0f, 0.0f };
	GLfloat   mat_flash_shiny[] = { 50.0f };
	GLfloat   light_position[] = { 100.0f, -200.0f, 200.0f, 0.0f };
	GLfloat   light_ambi[] = { 0.1f, 0.1f, 0.1f, 0.0f };
	GLfloat   light_color[] = { 1.0f, 1.0f, 1.0f, 0.0f };

	argDrawMode3D(vp);
	glClearDepth(1.0);
	glClear(GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);

	/* load the camera transformation matrix */
	argConvGlpara(trans, gl_para);
	glMatrixMode(GL_MODELVIEW);
#ifdef ARDOUBLE_IS_FLOAT
	glLoadMatrixf(gl_para);
#else
	glLoadMatrixd(gl_para);
#endif

	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, 1);
	glLightfv(GL_LIGHT0, GL_POSITION, light_position);
	glLightfv(GL_LIGHT0, GL_AMBIENT, light_ambi);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, light_color);
	glLightfv(GL_LIGHT0, GL_SPECULAR, light_color);
	glMaterialfv(GL_FRONT, GL_SPECULAR, mat_flash);
	glMaterialfv(GL_FRONT, GL_SHININESS, mat_flash_shiny);
	glMaterialfv(GL_FRONT, GL_DIFFUSE, mat_diffuse);
	glMaterialfv(GL_FRONT, GL_AMBIENT, mat_diffuse);

#if 1
	glTranslatef(0.0f, 0.0f, 40.0f);
	glutSolidCube(80.0);
#else
	glTranslatef(0.0f, 0.0f, 20.0f);
	glRotatef(90.0f, 1.0f, 0.0f, 0.0f);
	glutSolidTeapot(40.0);
#endif
	glDisable(GL_LIGHT0);
	glDisable(GL_LIGHTING);

	glDisable(GL_DEPTH_TEST);
}
