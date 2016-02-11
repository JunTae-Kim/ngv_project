#include <opencv2/opencv.hpp>
#include <iostream>
#include <opencv/cv.hpp>
#include <opencv2/highgui.hpp>
#include <math.h>
#include <stdio.h>

#define WIDTH 320
#define HEIGHT 240

using namespace cv;
using namespace std;

void morphology(Mat edge, Mat mor, int mask1, int mask2);
void roi(Mat frame, Mat img, int x1, int y1, int x2, int y2, int channel);
int hough(Mat frame, Mat img, vector<Vec2f> lines, int threshold, int t_point_x);
int colordetect(Mat frame, Mat hsv, int flag);
void steering(Mat frame, int angle);
void binaryfilter(Mat frame);

int main()
{
	int64 t1, t2;

	int roi1_x1 = 40;
	int roi1_y1 = HEIGHT / 2 + 10;
	int roi1_x2 = WIDTH - 40;
	int roi1_y2 = HEIGHT;		//line

	int roi2_x1 = 40;
	int roi2_y1 = 60;
	int roi2_x2 = WIDTH - 40;
	int roi2_y2 = HEIGHT  / 2 + 10;		//traffic signal

	int hough_threshold = 30;
	int edge_down_th = 150;
	int edge_up_th = 250;

	//int mask1 = 40;
	//int mask2 = 25;
	int traffic_flag = 0;

	int t_point_x = 0;				// before steering angle

	VideoCapture video(0);

	video.set(CV_CAP_PROP_FORMAT, CV_8UC3);
	video.set(CV_CAP_PROP_FRAME_WIDTH, WIDTH);
	video.set(CV_CAP_PROP_FRAME_HEIGHT, HEIGHT);

	if (!video.isOpened()) {
		cerr << "Camera open failed!" << endl;
	}

	Mat frame, grayimg, edgeimg, blurimg, hsvimg, morimg;
	Rect rt1(roi1_x1,roi1_y1,roi1_x2-roi1_x1,roi1_y2-roi1_y1);
	Rect rt2(roi2_x1,roi2_y1,roi2_x2-roi2_x1,roi2_y2-roi2_y1);

	vector<Vec2f> lines;

	while(1)
	{
		t1 = getTickCount();

		video >> frame;

		cvtColor(frame, grayimg, CV_BGR2GRAY);		//transform grayscale
		cvtColor(frame, hsvimg, CV_BGR2HSV);		//transform hsv color space
		binaryfilter(grayimg);	
		GaussianBlur(grayimg, blurimg, Size(3,3), 0, 0);
		Canny(blurimg, edgeimg, edge_down_th, edge_up_th);
		roi(frame, edgeimg, roi1_x1, roi1_y1, roi1_x2, roi1_y2, 1);
		roi(frame, hsvimg, roi2_x1, roi2_y1, roi2_x2, roi2_y2, 3);
		//morphology(edgeimg, morimg, mask1, mask2);
		t_point_x = hough(frame, edgeimg, lines, hough_threshold, t_point_x);
		//hough(frame, morimg, lines, hough_threshold);
		traffic_flag = colordetect(frame, hsvimg, traffic_flag);

		rectangle(frame, rt1, Scalar(100,155,50),2);		//line roi
		rectangle(frame, rt2, Scalar(200,55,50),2);		//traffic light roi

		t2 = getTickCount();
		cout << "It took " << (t2-t1)*1000/getTickFrequency() << " ms." << endl;

		imshow("frame", frame);
		imshow("edgeimg", edgeimg);
		imshow("hsvimg", hsvimg);

		int k = waitKey(1);
		if (k == 27) break;
	}
	return 0;
}

void morphology(Mat edge, Mat mor, int mask1, int mask2)
{
	Mat closeimg;

	int element_shape = MORPH_RECT;
	Mat element1 = getStructuringElement(element_shape, Size(mask1,mask1));
	Mat element2 = getStructuringElement(element_shape, Size(mask2,mask2));

	morphologyEx(edge, closeimg, MORPH_CLOSE, element1);
	erode(closeimg, mor, element2);
}

void binaryfilter(Mat frame)
{
	int binary_threshold = 20;

	for (int y=0; y<HEIGHT; y++){
		for (int x=0; x<WIDTH; x++){
			if (frame.at<uchar>(y,x) < binary_threshold)
			{
				frame.at<uchar>(y,x) = 0;
			}
		}
	}
}

void roi(Mat frame, Mat img, int x1, int y1, int x2, int y2, int channel)
{
	Rect rt1(x1,y1,x2-x1,y2-y1);

	if (channel == 1)
	{
		for (int y=0; y<HEIGHT; y++){
			for (int x=0; x<WIDTH; x++){
				if (y>=y1 && y<=y2 && x>=x1 && x<=x2);
				else img.at<uchar>(y,x) = 0;
			}
		}
	}
	else if (channel == 3)
	{
		for (int y=0; y<HEIGHT; y++){
			for (int x=0; x<WIDTH; x++){
				if (y>=y1 && y<=y2 && x>=x1 && x<=x2);
				else
				{
					img.at<Vec3b>(y,x)[0] = 0;
					img.at<Vec3b>(y,x)[1] = 0;
					img.at<Vec3b>(y,x)[2] = 0;
				}
			}
		}
	}
}

int hough(Mat frame, Mat img, vector<Vec2f> lines, int threshold, int t_point_x)
{
	Point pt1, pt2;
	Point down_crosspoint, up_crosspoint, temppoint;

	float line_rho1[200] = {0,};
	float line_rho2[200] = {0,};
	float line_theta1[200] = {0,};
	float line_theta2[200] = {0,};
	int line_count1 = 0;
	int line_count2 = 0;
	int stop_count = 0;

	float rho = 0;
	float theta = 0;

	float test_rho1 = 0;		//add1
	float test_theta1 = 0;		//add1
	float test_rho2 = 0;		//add1
	float test_theta2 = 0;		//add1
	int outline_cnt = 0;
	int outline_range = 40;

	int length = 800;
	int offset = 50;
	float angle = 0;

	down_crosspoint.x = (int)WIDTH / 2;
	down_crosspoint.y = (int)HEIGHT - 10;	

	HoughLines(img, lines, 1, CV_PI / 180, threshold, 0, 0);

	for (size_t i = 0; i < lines.size(); i++)
	{
		rho = lines[i][0], theta = lines[i][1];

		if (theta<=1.15 && theta >= 0) 				//right line
		{
			//printf("left line : %f\t %f\n", rho, theta);
			line_rho1[line_count1] = rho;
			line_theta1[line_count1] = theta;

			line_count1++;
			if (line_count1 >= 99)	line_count1 = 99;
		}

		else if (theta >= 2.0  && theta <= 3.14)	//left line
		{
			//printf("right line : %f\t %f\n", rho, theta);
			line_rho2[line_count2] = rho;
			line_theta2[line_count2] = theta;
			line_count2++;
			if (line_count2 >= 99) line_count2 = 99;
		}
		else if (theta >=1.4 && theta <= 1.6)
		{
			stop_count++;
		}
	}

	if (line_count1 != 0 || line_count2 != 0)
	{
		rho = 0;
		theta = 0;

		test_rho1 = 0;		//add1
		test_theta1 = 0;	//add1
		test_rho2 = 0;		//add1
		test_theta2 = 0;	//add1

		if (line_count1 >= line_count2)
		{
			for (int i=0; i<line_count1; i++)
			{
				rho = rho + line_rho1[i];
				theta = theta + line_theta1[i];
			}
			
			for (int i =0; i<line_count2; i++)
			{
				rho = rho - line_rho2[i];
				theta = theta + 3 - line_theta2[i];
			}
			
			rho = rho / (line_count1+line_count2);
			theta = theta / (line_count1+line_count2);

			for (int i=0; i<line_count1; i++)		//add1
			{
				if (line_rho1[i]<rho - outline_range || line_rho1[i]>rho + outline_range)
				{
					test_rho1 += line_rho1[i];
					test_theta1 += line_theta1[i];
					outline_cnt++;
				}
			}
			for (int i=0; i<line_count2; i++)
			{
				if (line_rho2[i]<-rho - outline_range || line_rho2[i]>-rho + outline_range)
				{
					test_rho2 -= line_rho2[i];
					test_theta2 += 3 - line_theta2[i];
					outline_cnt++;
				}
			}

			rho = ((rho * (float)(line_count1 + line_count2)) - test_rho1 - test_rho2)/(float)(line_count1 + line_count2 - outline_cnt);
			theta = ((theta * (float)(line_count1 + line_count2)) - test_theta1 - test_theta2)/(float)(line_count1 + line_count2 - outline_cnt);

		}
		else
		{
			
			for (int i=0; i<line_count1; i++)
			{
				rho = rho - line_rho1[i];
				theta = theta + 3 + line_theta1[i];
			}
			
			for (int i=0; i<line_count2; i++)
			{
				rho = rho + line_rho2[i];
				theta = theta + line_theta2[i];
			}
			rho = rho / (line_count1+line_count2);
			theta = theta / (line_count1+line_count2);

			for (int i=0; i<line_count1; i++)		//add1
			{
				if (line_rho1[i]<-rho - outline_range || line_rho1[i]>-rho + outline_range)
				{
					test_rho1 -= line_rho1[i];
					test_theta1 += 3 + line_theta1[i];
					outline_cnt++;
				}
			}
			for (int i=0; i<line_count2; i++)
			{
				if (line_rho2[i]<rho - outline_range || line_rho2[i]>rho + outline_range)
				{
					test_rho2 += line_rho2[i];
					test_theta2 += line_theta2[i];
					outline_cnt++;
				}
			}

			rho = ((rho * (float)(line_count1 + line_count2)) - test_rho1 - test_rho2)/(float)(line_count1 + line_count2 - outline_cnt);
			theta = ((theta * (float)(line_count1 + line_count2)) - test_theta1 - test_theta2)/(float)(line_count1 + line_count2 - outline_cnt);

		}
	
		line_count1 = 0;
		line_count2 = 0;

		if (stop_count >= 10)
		{
			putText(frame, "STOP", Point(WIDTH/2-20, 30), 2, 1, Scalar(0,0,255));
			//stop motor
		}

		double a = cos(theta), b = sin(theta);
		double x = a*rho, y = b*rho;

		pt1.x = cvRound(x - length * (-b));
		pt1.y = cvRound(y - length * (a));
		pt2.x = cvRound(x + length * (-b));
		pt2.y = cvRound(y + length * (a));

		float gradient = (float)(pt2.y - pt1.y) / (float)(pt2.x - pt1.x);
		float intercept = pt2.y - gradient * pt2.x;

        up_crosspoint.x = (int)(intercept / -gradient);
        up_crosspoint.y = (int)(gradient * up_crosspoint.x + intercept);

        temppoint.y = (int)(HEIGHT / 2 + offset);
//        temppoint.x = (int)(((float)(up_crosspoint.x-down_crosspoint.x)/(float)(up_crosspoint.y - down_crosspoint.y))*(float)(temppoint.y - down_crosspoint.y)+(float)down_crosspoint.x);
        temppoint.x = (int)(((float)(up_crosspoint.x-pt2.x)/(float)(up_crosspoint.y - pt2.y))*(float)(temppoint.y - pt2.y)+(float)pt2.x);

        angle = atan((float)(temppoint.x - down_crosspoint.x)/(float)(down_crosspoint.y - temppoint.y)) * 57.3;

        printf("angle : %f\n", angle);

        // steering angle setting

        steering(frame, angle);

        line(frame, down_crosspoint, temppoint, Scalar(0,0,0), 4, CV_AA);

        line(frame, pt1, pt2, Scalar(255,20,147), 2, CV_AA);  //detect line

		return temppoint.x;
	}
	else
	{
        temppoint.y = (int)(HEIGHT / 2 + offset);
        temppoint.x = t_point_x;

        angle = atan((float)(temppoint.x - down_crosspoint.x)/(float)(down_crosspoint.y - temppoint.y)) * 57.3;
		steering(frame, angle);

		line(frame, down_crosspoint, temppoint, Scalar(0,0,0), 4, CV_AA);
		
		return t_point_x;
	}
}

int colordetect(Mat frame, Mat hsv, int flag)
{
	int max1 = 500, max2 = 500, max3 = 500;
	int left, top, width, height;
	int idx1 = 0, idx2 = 0, idx3 = 0;

	int LowH1 = 170, HighH1 = 179;			//red
	int LowH2 = 24, HighH2 = 33;			//yellow
	int LowH3 = 75, HighH3 = 85;			//green
	int LowS = 50, HighS = 255;
	int LowV = 30, HighV = 255;

	int traffic_threshold = 500;
	int tag = 0;

	Mat img_labels, stats, centroids;
	Mat img_binary1, img_binary2, img_binary3;

	int element_shape = MORPH_RECT;
	Mat element = getStructuringElement(element_shape, Size(5,5));

	inRange(hsv, Scalar(LowH1,LowS,LowV), Scalar(HighH1, HighS, HighV), img_binary1);
	inRange(hsv, Scalar(LowH2,LowS,LowV), Scalar(HighH2, HighS, HighV), img_binary2);
	inRange(hsv, Scalar(LowH3,LowS+50,LowV), Scalar(HighH3, HighS, HighV), img_binary3);

	morphologyEx(img_binary1, img_binary1, MORPH_CLOSE, element);
	morphologyEx(img_binary2, img_binary2, MORPH_CLOSE, element);
	morphologyEx(img_binary3, img_binary3, MORPH_CLOSE, element);

////////////////////////////////////////////////////////////////////////////////////////////////////

	int numOfLables = connectedComponentsWithStats(img_binary1, img_labels, stats, centroids, 8, CV_32S);

	for(int i = 1; i<numOfLables; i++)					//Red signal
	{
		int area = stats.at<int>(i, CC_STAT_AREA);
		if(max1 < area)
		{
			max1 = area;
			idx1 = i;
		}
	}
	left = stats.at<int>(idx1, CC_STAT_LEFT);
	top = stats.at<int>(idx1, CC_STAT_TOP);
	width = stats.at<int>(idx1, CC_STAT_WIDTH);
	height = stats.at<int>(idx1, CC_STAT_HEIGHT);

	if (max1 > traffic_threshold)
	{
		putText(frame, "Red", Point(WIDTH/2+40, HEIGHT / 2 - 30), 1, 1, Scalar(0,0,255));
		rectangle(frame, Point(left, top), Point(left+width, top+height), Scalar(0,0,255), 2);
		flag++;
		tag = 1;
	}

////////////////////////////////////////////////////////////////////////////////////////////////////
	if (flag >= 10)
	{
		numOfLables = connectedComponentsWithStats(img_binary2, img_labels, stats, centroids, 8, CV_32S);

		for(int i = 1; i<numOfLables; i++)					//Yellow signal
		{
			int area = stats.at<int>(i, CC_STAT_AREA);
			if(max2 < area)
			{
				max2 = area;
				idx2 = i;
			}
		}

		left = stats.at<int>(idx2, CC_STAT_LEFT);
		top = stats.at<int>(idx2, CC_STAT_TOP);
		width = stats.at<int>(idx2, CC_STAT_WIDTH);


		if (max2 > traffic_threshold)
		{
			putText(frame, "Yellow", Point(WIDTH/2+40, HEIGHT / 2 - 30), 1, 1, Scalar(0,0,255));
			rectangle(frame, Point(left, top), Point(left+width, top+height), Scalar(0,255,255), 2);
		}

////////////////////////////////////////////////////////////////////////////////////////////////////
	
		numOfLables = connectedComponentsWithStats(img_binary3, img_labels, stats, centroids, 8, CV_32S);

		for(int i = 1; i<numOfLables; i++)					//Green signal
		{
			int area = stats.at<int>(i, CC_STAT_AREA);
			if(max3 < area)
			{
				max3 = area;
				idx3 = i;
			}
		}

		left = stats.at<int>(idx3, CC_STAT_LEFT);
		top = stats.at<int>(idx3, CC_STAT_TOP);
		width = stats.at<int>(idx3, CC_STAT_WIDTH);
		height = stats.at<int>(idx3, CC_STAT_HEIGHT);

		if (max3 > traffic_threshold)
		{
			putText(frame, "Green", Point(WIDTH/2+40, HEIGHT / 2 - 30), 1, 1, Scalar(0,0,255));
			rectangle(frame, Point(left, top), Point(left+width, top+height), Scalar(0,255,0), 2);
		}
	}
	else if (flag < 10 && tag == 0) flag = 0;

	//printf("flag = %d", flag);
	return flag;
}

void steering(Mat frame, int angle)
{
    string text;

    if (angle < -40)
    {
        text = "left4";
        putText(frame, text, Point(WIDTH/2+40, HEIGHT-10), 1, 1, Scalar(0,0,255));
    }
    else if (angle >= -40 && angle < -30)
    {
        text = "left3";
        putText(frame, text, Point(WIDTH/2+40, HEIGHT-10), 1, 1, Scalar(0,0,255));
    }
    else if (angle >= -30 && angle < -20)
    {
        text = "left2";
        putText(frame, text, Point(WIDTH/2+40, HEIGHT-10), 1, 1, Scalar(0,0,255));
    }
    else if (angle >=-20 && angle < -10)
    {
        text = "left1";
        putText(frame, text, Point(WIDTH/2+40, HEIGHT-10), 1, 1, Scalar(0,0,255));
    }
    else if (angle >= -10 && angle < 10)
    {
        text = "straight";
        putText(frame, text, Point(WIDTH/2+40, HEIGHT-10), 1, 1, Scalar(0,0,255));
    }
    else if (angle >= 10 && angle < 20)
    {
        text = "right1";
        putText(frame, text, Point(WIDTH/2+40, HEIGHT-10), 1, 1, Scalar(0,0,255));
    }
    else if (angle >= 20 && angle < 30)
    {
        text = "right2";
        putText(frame, text, Point(WIDTH/2+40, HEIGHT-10), 1, 1, Scalar(0,0,255));
    }
    else if (angle >= 30 && angle < 40)
    {
        text = "right3";
        putText(frame, text, Point(WIDTH/2+40, HEIGHT-10), 1, 1, Scalar(0,0,255));
    }
    else if (angle >= 40)
    {
        text = "right4";
        putText(frame, text, Point(WIDTH/2+40, HEIGHT-10), 1, 1, Scalar(0,0,255));
    }
}