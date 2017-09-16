#ifndef __DEBUG_H
#define __DEBUG_H

namespace act
{
    //measure time
	class Timestamp
    {
    public:
        void start() { start_ = clock(); }
        void end() { end_ = clock(); }
        double getTime() const { return static_cast<double>(end_ - start_) / CLOCKS_PER_SEC; }

    private:
        clockid_t start_ = 0;
        clockid_t end_ = 0;
    };

	//trackbar
	struct trackbar
	{
		trackbar() {}
		trackbar(char* _name, int _maxValue) : name(_name), maxValue(_maxValue) {}
		char* name;
		int maxValue = 0;
	};

	int outputValue = 0;
	int slider = 0;
	void callback(int, void*)
	{
		outputValue = slider;
	}

	std::vector<trackbar> trackbarSet;

	//void showTrackbar();

	//void act::Camera::showTrackbar()
	//{
	//	trackbarSet = new trackbar[6];
	//
	//	trackbarSet[0] = trackbar("minB", 255);
	//	trackbarSet[1] = trackbar("maxB", 255);
	//	trackbarSet[2] = trackbar("minG", 255);
	//	trackbarSet[3] = trackbar("maxG", 255);
	//	trackbarSet[4] = trackbar("minR", 255);
	//	trackbarSet[5] = trackbar("maxR", 255);
	//	
	//	cv::namedWindow("TKB");
	//	
	//	for (auto i = 0; i < 6; i++)
	//	{
	//		cv::createTrackbar(trackbarSet[i].name, "TKB", &trackbarSet[i].slider, trackbarSet[i].maxValue, trackbarSet[i].callback);
	//		trackbarSet[i].callback(trackbarSet[i].slider, 0);
	//	}
	//
	//	delete[] trackbarSet;
	//}
};

#endif // !__DEBUG_H
