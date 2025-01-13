#include <jni.h>
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/video/tracking.hpp>
#include <vector>
#include <numeric>

constexpr int FEATURE_STEP = 5;

cv::Mat grey_m1, grey_m2;
std::vector<cv::Point2f> p1, p2;
std::vector<uchar> status;
std::vector<float> err;
cv::TermCriteria criteria((cv::TermCriteria::COUNT) + (cv::TermCriteria::EPS), 10, 0.03);

extern "C" JNIEXPORT void JNICALL
Java_com_example_android_1img_1processing_MainActivity_calcOpticalFlow(JNIEnv*, jobject, jlong matAddr, jfloatArray resultAddr) {
    static bool is_first = true;
    static long cnt = 0;

    cv::Mat* grey_m;
    cv::Mat* prev_m;
    std::vector<cv::Point2f>* p;
    std::vector<cv::Point2f>* prev_p;
    std::vector<float> fine_x;
    std::vector<float> fine_y;

    cv::Mat& m = *(cv::Mat*)matAddr;
    (float*)& result = *(float*)resultAddr;

    if (cnt % 2)
    {
        grey_m = &grey_m1;
        prev_m = &grey_m2;
        p = &p1;
        prev_p = &p2;
    }
    else
    {
        grey_m = &grey_m2;
        prev_m = &grey_m1;
        p = &p2;
        prev_p = &p1;
    }
    cv::cvtColor(m, *grey_m, cv::COLOR_RGBA2GRAY);

    if(is_first)
    {
        cv::goodFeaturesToTrack(*grey_m, *p, 100, 0.3, 7, cv::Mat(), 7, false, 0.04);
        is_first = false;
    }
    else
    {
        if (!(cnt % FEATURE_STEP))
            cv::goodFeaturesToTrack(*prev_m, *prev_p, 100, 0.3, 7, cv::Mat(), 7, false, 0.04);
        cv::calcOpticalFlowPyrLK(*prev_m, *grey_m, *prev_p, *p, status, err, cv::Size(15,15), 2, criteria);

        for(uint i = 0; i < prev_p->size(); i++)
        {
            if(status[i])
            {
                cv::circle(m, (*prev_p)[i], 5, cv::Scalar(255, 0, 0));
                cv::circle(m, (*p)[i], 5, cv::Scalar(0, 255, 0));
                fine_x.push_back((*p)[i].x - (*prev_p)[i].x);
                fine_y.push_back((*p)[i].y - (*prev_p)[i].y);
                restd::accumulate(fine_x.begin(), fine_x.end(), 0);
            }
        }

    }

    cnt++;
    return 0;
}

extern "C" JNIEXPORT void JNICALL
Java_com_example_android_1img_1processing_MainActivity_release(JNIEnv*, jobject) {
    grey_m1.release();
    grey_m2.release();
}