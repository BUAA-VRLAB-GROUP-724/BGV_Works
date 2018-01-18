#include "myHeaders.h"



//extern Mat frame;
Mat fgMaskMOG2; //fg mask fg mask generated by MOG2 method
bool flag_selectTarget = false;
Rect trackWindow;
Point centerStart;
vector<int> distances;
Ptr<BackgroundSubtractor> pMOG2; //MOG2 Background subtractor  
VideoWriter writer;
clock_t start_time = clock();
/*
* 读取视频帧，并调用算法
*/
bool RunAlgorithm(){
	VideoCapture capture(video_dir);
	if (!capture.isOpened()){
		cout << "Open " << video_dir << " failed." << endl;
		out << "Open " << video_dir << " failed." << endl;
		return false;
	}
	pMOG2 = createBackgroundSubtractorMOG2(); //MOG2 approach
	while (capture.read(frame)){
		frameCount++;
		cout << "Frame Count : " << frameCount << endl;
		HoverDetection();
	}
	return true;
}
void HoverDetection(){
	pMOG2->apply(frame, fgMaskMOG2);
	//绘制边界
	erode(fgMaskMOG2, fgMaskMOG2, Mat(), cv::Point(-1, -1), 5);
	dilate(fgMaskMOG2, fgMaskMOG2, Mat(), cv::Point(-1, -1), 10);
	if (!flag_selectTarget){
		vector<vector<Point>> contours0, contours;
		vector<Vec4i> hierarchy;
		findContours(fgMaskMOG2, contours0, hierarchy, RETR_TREE, CHAIN_APPROX_SIMPLE);
		contours.resize(contours0.size());
		for (size_t k = 0; k < contours0.size(); k++)
			approxPolyDP(Mat(contours0[k]), contours[k], 3, true);
		Mat frame_contours(frame.rows, frame.cols, CV_8UC3);
		Mat frame_bbox(frame.rows, frame.cols, CV_8UC3);
		for (int idx = 0; idx < contours.size(); idx++){
			Rect bbox = boundingRect(contours[idx]);
			cout << idx << "  " << bbox.area() << endl;
			if (bbox.area() > 3000 && bbox.area() < 10000 && bbox.area() > trackWindow.area()){
				rectangle(frame, bbox, Scalar(255, 255, 0), 3);
				flag_selectTarget = true;
				trackWindow = bbox;
				centerStart.x = bbox.x + bbox.width / 2;
				centerStart.y = bbox.y + bbox.height / 2;
			}

		}
		if (flag_selectTarget)
			writer = VideoWriter("output.avi", CV_FOURCC('M', 'J', 'P', 'G'), 25, frame.size());
	}
	if (flag_selectTarget){
		RotatedRect trackBox = CamShift(fgMaskMOG2, trackWindow,
			TermCriteria(TermCriteria::EPS | TermCriteria::COUNT, 10, 1));
		if (trackWindow.area() <= 1)
		{
			int cols = frame.cols, rows = frame.rows, r = (MIN(cols, rows) + 5) / 6;
			trackWindow = Rect(trackWindow.x - r, trackWindow.y - r,
				trackWindow.x + r, trackWindow.y + r) &
				Rect(0, 0, cols, rows);
		}
		if (distances.size() == 5)
			distances.pop_back();
		cout << "( " << trackBox.center.x << ", " << trackBox.center.y << ")" << endl;
		distances.insert(distances.begin(), CalcPointsDistance(centerStart, trackBox.center));
		bool descend = true;
		cout << distances.size() << "  ";
		for (vector<int>::iterator iter = distances.begin(); iter != distances.end(); iter++){
			cout << static_cast<int>(*iter) << " ";
			if (iter != distances.begin() && *iter <= *(iter - 1))
				descend = false;
		}
		cout << endl;
		clock_t end_time = clock();
		double runTime = static_cast<double>(end_time - start_time) / CLOCKS_PER_SEC * 1000;
		if (runTime > timeThres*1000 && descend){
			cout << "*******************************descend*******************************" << endl;
			if (frameCount % 6 < 3)  //以三帧为间隔闪烁
				ellipse(frame, trackBox, Scalar(0, 0, 255), 3, LINE_AA);
		}
		ellipse(frame, trackBox.center, Size(10, 10), 0, 0, 360, Scalar(255, 0, 0), 3);
	}
	imshow("fgMaskMOG2", fgMaskMOG2);
	ellipse(frame, centerStart, Size(10, 10), 0, 0, 360, Scalar(0, 255, 0), 3);//标记起始点
	imshow("frame", frame);
	writer << frame;
	waitKey(30);
}