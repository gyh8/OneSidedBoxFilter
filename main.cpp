#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <iostream>
#include <fstream>
#include <assert.h>
#include <time.h>
#include <cmath>
#include <string> 
/* Please cite the following paper if you use this code
@ARTICLE{osbf,
  author={Gong, Yuanhao},
  journal={IEEE Access}, 
  title={OSBF: One-Sided Box Filter for Edge-preserving Image Processing}, 
  year={2025},
  volume={13},
  number={},
  pages={10289-10298},
  doi={10.1109/ACCESS.2025.3555434}}
 */

using namespace cv;
using namespace std;

//default iteration number and radius
int ItNum = 10;
int R = 1;

float signedMin(float arr[]) {
	int min_abs = ((int&)(arr[0]) & 0x7FFFFFFF);
	int index = 0;
	for (int i = 1; i < 4; i++) {
		int current_abs = ((int&)(arr[i]) & 0x7FFFFFFF);
		if (current_abs < min_abs) {
			min_abs = current_abs;
			index = i;
		}
	}
	return arr[index];
}

int main(int argc, char** argv)
{
	if (argc < 2 || argc>4)
	{
		std::cout << "usage: osbf.exe inputImage window_radius iteration_number" << endl;
		return -1;
	}

    Mat image_orig=imread(argv[1], IMREAD_COLOR); //read the image
    if (argc > 2) R = atoi(argv[2]); //read the radius
	if (argc > 3) ItNum = atoi(argv[3]); //read the iteration number

    Mat image;//padded image
	copyMakeBorder(image_orig, image, R, 0, R, 0, BORDER_REPLICATE);

	const Size kernel_size = Size(R + 1, R + 1);
	const int M = image.rows;
	const int N = image.cols;

	//turn image into float
	Mat current = Mat::zeros(image.size(), CV_32FC3);
	image.convertTo(current, CV_32FC3);
	
	//the classical box filter
	Mat corner = Mat::zeros(image.size(), CV_32FC3);
	clock_t start, end; 
	start = clock();
	boxFilter(current, corner, -1, kernel_size, Point(0, 0), true, BORDER_REPLICATE);
	end = clock();
	std::cout << "Original Box Filter runs " << end - start << " ms (one iteration)." << endl;

	//one-sided box filter
	Vec3f *p, *p_pre, *p_current;
	
	start = clock();
	for (int it = 0; it < ItNum; it++)
	{
		//right bottom corner window filter
		boxFilter(current, corner, -1, kernel_size, Point(0, 0), true, BORDER_REPLICATE);
		for (int i = R; i < M; i++)
		{
			p = corner.ptr<Vec3f>(i);
			p_pre = corner.ptr<Vec3f>(i - R);
			p_current = current.ptr<Vec3f>(i);
			for (int j = R; j < N; j++)
				for (int ch = 0; ch < 3; ch++)
				{
					//find the signed minimal distance
					float dist[4],dist2[4];
					float c = p_current[j][ch];
					dist[0] = p[j - R][ch] - c; //left bottom window
					dist[1] = p[j][ch] - c;     //right bottom window
					dist[2] = p_pre[j - R][ch] - c;//left up window
					dist[3] = p_pre[j][ch] - c; //right up window

					dist2[0] = dist[0] + dist[1]; //bottom half window, scaled by 2
					dist2[1] = dist[2] + dist[3]; //up half window
					dist2[2] = dist[0] + dist[2]; //left half window
					dist2[3] = dist[1] + dist[3]; //right half window
					//update with the signed min distance
					float d1= signedMin(dist);
					float d2 = signedMin(dist2);
					if (((int&)(d1) & 0x7FFFFFFF) *2 < ((int&)(d2) & 0x7FFFFFFF))
						p_current[j][ch] += d1;
					else
						p_current[j][ch] += d2 / 2;
				}
		}
	}
	end = clock();
	std::cout << "One-sided Box Filter new runs " << (end - start) / ItNum << " ms/Iteraion." << endl;

	//save result
	Mat result = Mat::zeros(image.size(), CV_8UC3);
	current.convertTo(result, CV_8UC3);
	Rect rect(R, R, image_orig.cols,image_orig.rows);

	Mat image_final = result(rect);
	string name = string("result_R") + to_string(R)+string("_IT")+to_string(ItNum)+string(".png");
	cv::imwrite(name, image_final);
	return 0;
}


