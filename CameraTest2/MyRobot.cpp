#include "nivision.h"
#include "Vision/MonoImage.h"
#include <algorithm>
#include <math.h>
#include "WPILib.h"
#include "Vision/ColorImage.h"
//#include "Dashboard.h"


// These parameters set Rectangle finding in the NI imaq (Image Aquisition) library.
// Refer to the CVI Function Reference PDF document installed with LabVIEW for
// additional information.
class RobotDemo : public SimpleRobot
{
	RobotDrive myRobot; // robot drive system
	Joystick stick; // only joystick
	Victor vic;

	float distance(float heightpixel) {
		float fov = 54;
		float widthtarget = 2 ;
		float yresolution = 360;
		float distance = widthtarget*yresolution/heightpixel/tan(fov*3.14159265/180);

		return (distance);
	}
public:
	RobotDemo(void):
		myRobot(2, 3),	// these must be initialized in the same order
		stick(1),		// as they are declared above.
		vic(1)
	{
		myRobot.SetExpiration(0.1);
	}

	void OperatorControl(void)
	{
		myRobot.SetSafetyEnabled(false);
		AxisCamera &camera = AxisCamera::GetInstance("10.28.53.11");
		camera.WriteResolution(AxisCamera::kResolution_320x240);
		camera.WriteRotation(AxisCamera::kRotation_180);//Flip image upside-down.
		camera.WriteCompression(80);//Compresses the image(?)
		camera.WriteBrightness(50);//Sets the brightness to 80 on a scale from 0-100
		DriverStationLCD *screen = DriverStationLCD::GetInstance();
		int count = 0;
		while(IsOperatorControl())
		{
			screen->UpdateLCD();
			count++;
			HSLImage* imgpointer;		//declares a new hue saturation lum image
			imgpointer = camera.GetImage();  //grabs an image to initialize that image
			//imaqCreateImage(IMAQ_IMAGE_U8,) 
			//imaqCast(NULL, imgpointer, IMAQ_IMAGE_U8, NULL, -1);
			BinaryImage* binImg = NULL;	//declares a new binary image


			//ThresholdHSL changes our regular image into a binary image.
			/*hueLow Low value for hue  
			hueHigh High value for hue  
			saturationLow Low value for saturation  
			saturationHigh High value for saturation  
			luminenceLow Low value for luminence  
			luminenceHigh High value for luminence   
			 */
			binImg = imgpointer->ThresholdHSL(1, 255, 1, 255, 230, 255);

			//Saves the image to a temp directory
			binImg->Write("/tmp/thresh.jpg");

			//Writes the binary image to disk

			imaqWriteFile(binImg->GetImaqImage(), "/tmp/thresh2.jpg", NULL);

			delete imgpointer;
			Image* Convex = imaqCreateImage(IMAQ_IMAGE_U8, 0);
			int returnvalue;
			returnvalue = imaqConvexHull(Convex, binImg->GetImaqImage(), TRUE);

	//		imaqWriteFile(Convex, "/tmp/convex.jpg", NULL); 

			delete binImg;
	//		screen->PrintfLine(DriverStationLCD::kUser_Line4,"convex is %d",returnvalue);
			screen->UpdateLCD();
			float lookuptable[256];
			lookuptable[0] = 0;
			lookuptable[1] = 65535;

			//Converts image to 16 bit, then back to 8 bit.
			Image* cast = imaqCreateImage(IMAQ_IMAGE_U16, 0);
			imaqCast(cast, Convex, IMAQ_IMAGE_U16, lookuptable, 0);
			imaqDispose(Convex);
			Image* bitcast = imaqCreateImage(IMAQ_IMAGE_U8, 0);
			imaqCast(bitcast, cast, IMAQ_IMAGE_U8, NULL, 0);
			imaqDispose(cast);
	//		screen->PrintfLine(DriverStationLCD::kUser_Line3,"det %d", imaqGetLastError());//Notifies the user
			imaqWriteFile(bitcast, "/tmp/bitcast.jpg", NULL);

			//Image* SuperSize = im
			//imaqCreateImage(IMAQ_IMAGE_U8, 2240);
			//int returnvalue2;
			//returnvalue2 = imaqSizeFilter(SuperSize, Convex, TRUE, 1, IMAQ_KEEP_LARGE, NULL); 
			//screen->UpdateLCD();
			//imaqDispose (Convex);
//			ROI *roi;
//			Rect rectangle;
//			rectangle.top = 0;
//			rectangle.left = 0;
//			rectangle.width = 320;
//			rectangle.height = 120;
//			imaqAddRectContour(roi,rectangle);

			static RectangleDescriptor rectangleDescriptor = 
			{
				30, 	// minWidth (All values are estimated)
				200, 	// maxWidth
				20, 	// minHeight
				200		// maxHeight
			};

			static CurveOptions curveOptions = //extraction mode specifies what the VI identifies curves in the image. curve options are all the  
			{	
				IMAQ_NORMAL_IMAGE,	// extractionMode
				75, 				// threshold
				IMAQ_NORMAL, 		// filterSize
				25, 				// minLength
				15, 				// rowStepSize 
				15, 				// columnStepSize
				10, 				// maxEndPointGap
				FALSE,				// onlyClosed
				FALSE				// subpixelAccuracy
			};
			static ShapeDetectionOptions shapeOptions = 
			{		
				IMAQ_GEOMETRIC_MATCH_ROTATION_INVARIANT,	// mode
				NULL,			// angle ranges
				0,				// num angle ranges
				{75, 125},		// scale range
				300				// minMatchScore
			};

			int matches = 0;
			//double highscore = 0;
			//int highestindex = -1;
			float difference = 0;
			float time = 0;
			float average = 0;
			double y = 0;


			//The big important line of code that does important stuff
			RectangleMatch* recmatch = imaqDetectRectangles(bitcast, &rectangleDescriptor, &curveOptions, &shapeOptions, NULL, &matches);

//			DashboardDataSender *dds;
//			dds = new DashboardDataSender;

			//screen->PrintfLine(DriverStationLCD::kUser_Line3,"det %d", imaqGetLastError());
			//screen->PrintfLine(DriverStationLCD::kUser_Line4,"Matches: %d",matches);//Notifies the user
			screen->PrintfLine(DriverStationLCD::kUser_Line1,"Matches: %i",matches);//Notifies the user
			screen->UpdateLCD();

			/*for(int i = 0; i < matches; i++)
			{
				//screen->PrintfLine((DriverStationLCD::Line)(i+1),"%i,%i,%i",recmatch[i].height,recmatch[i].width,recmatch[i].score);
				if(recmatch[i].score > highscore)
				{
					highscore = recmatch[i].score;
					highestindex = i;
				}
				screen->PrintfLine(DriverStationLCD::kUser_Line1,"score %i, i %i", recmatch[i].score, i);
				screen->UpdateLCD();
			}*/
			average = (recmatch->corner[1].x + recmatch->corner[3].x)/2;
			y = (472/(recmatch->height-4));
			screen->PrintfLine(DriverStationLCD::kUser_Line5,"%f",average);
			screen->PrintfLine(DriverStationLCD::kUser_Line3," %f",distance(recmatch->height));
			//screen->PrintfLine(DriverStationLCD::kUser_Line5,"%f,%f",recmatch->height, y);
			screen->PrintfLine(DriverStationLCD::kUser_Line6,"%f,%f",ceil(recmatch->corner[1].x),ceil(recmatch->corner[3].x));
			screen->UpdateLCD();
			//screen->PrintfLine(DriverStationLCD::kUser_Line6,"%d, %d, %d, %d",recmatch->corner[1].x,recmatch->corner[2].x,recmatch->corner[3].x,recmatch->corner[4].x);
			//screen->PrintfLine(DriverStationLCD::kUser_Line5,"%d, %d",recmatch->height,recmatch->width);
			if(average == 0)
			{
				screen->PrintfLine(DriverStationLCD::kUser_Line1,"Image Not Found");
			}
			else if(average < 145)
			{
				difference = (160 - average);
				time = difference*0.0062;
				vic.Set(0.1);
				Wait(time);
				vic.Set(0.0);
				screen->PrintfLine(DriverStationLCD::kUser_Line1,"Less!");
				screen->UpdateLCD();
			}
			else if (average > 175)
			{
				difference = (average - 160);
				time = difference*0.0062;
				vic.Set(-0.1);
				Wait(time);
				vic.Set(0.0);
				screen->PrintfLine(DriverStationLCD::kUser_Line1,"More!");
				screen->UpdateLCD();
			}
			else
			{
				vic.Set(0);
				screen->PrintfLine(DriverStationLCD::kUser_Line1,"Perfection!");
				screen->UpdateLCD();
			}
			screen->PrintfLine(DriverStationLCD::kUser_Line2,"Time: %f",time);
			screen->PrintfLine(DriverStationLCD::kUser_Line4,"difference %f",difference);

		//	screen->PrintfLine(DriverStationLCD::kUser_Line2,"Score %i, Index %i", highscore, highestindex);
	/*		if(matches > 0)
			{
				int counter = 0;
				while (counter < 20)	
				{
					average = (recmatch[highestindex].corner[1].x + recmatch[highestindex].corner[2].x + recmatch[highestindex].corner[3].x +recmatch[highestindex].corner[4].x)/4;
					average2 = (recmatch[highestindex].corner[1].y + recmatch[highestindex].corner[2].y + recmatch[highestindex].corner[3].y +recmatch[highestindex].corner[4].y)/4;
				//	screen->PrintfLine(DriverStationLCD::kUser_Line5,"Center = %f, %f",average, average2);
					screen->UpdateLCD();
					if (average > 170)
					{
						vic.Set(-0.1);
						//screen->PrintfLine(DriverStationLCD::kUser_Line1,"Left %f",vic.Get());
						//screen->UpdateLCD();		
					}
					else if (average < 150)
					{
						vic.Set(0.1);
						//screen->PrintfLine(DriverStationLCD::kUser_Line1,"Right %f",vic.Get());
						//screen->UpdateLCD();
					}
					else
					{
						counter = 20;	
						vic.Set(0);
				//		screen->PrintfLine(DriverStationLCD::kUser_Line6,"Not Mallory! %f",vic.Get());
						screen->UpdateLCD();
					}
					counter++a;
				}
				*/
	//			imaqWriteFile(cast, "/tmp/linedetect.jpg", NULL);

//			}
/*			else
			{
				vic.Set(0);
			//	screen->PrintfLine(DriverStationLCD::kUser_Line6,"Not Mallory! %f",vic.Get());
				screen->UpdateLCD();
			}*/
			imaqDispose(bitcast);
			imaqDispose(recmatch);
			//imaqDispose(roi);
			vic.Set(0);
		}

	}
};

START_ROBOT_CLASS(RobotDemo);

