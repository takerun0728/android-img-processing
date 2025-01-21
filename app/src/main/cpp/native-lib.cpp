#include <jni.h>
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/video/tracking.hpp>
#include <vector>
#include <numeric>
#include <random>

constexpr int FEATURE_STEP = 5;
constexpr double RANSAC_R = 0.25;
constexpr double RANSAC_NT_RATE = 3;
constexpr double RANSAC_NL_RATE = 0.1;
constexpr double RANSAC_RE_NL_RATE = 0.2;
constexpr int RANSAC_MAX_NS = 5;
constexpr double RANSAC_MIN_D = 1.0;
constexpr int MAX_CORNERS = 100;
constexpr int SEED = 0;

cv::Mat grey_m1, grey_m2;
std::vector<cv::Point2f> p1, p2;
std::vector<uchar> status;
std::vector<float> err;
cv::TermCriteria criteria((cv::TermCriteria::COUNT) + (cv::TermCriteria::EPS), 10, 0.03);

extern "C" JNIEXPORT void JNICALL
Java_com_example_android_1img_1processing_MainActivity_calcOpticalFlow(JNIEnv* env, jobject, jlong matAddr, jfloatArray resultAddr) {
    static bool is_first = true;
    static long cnt = 0;
    static auto sq_sum = [](double a, float x){return a + x * x;};
    static std::mt19937 engine(SEED);

    cv::Mat* grey_m;
    cv::Mat* prev_m;
    std::vector<cv::Point2f>* p;
    std::vector<cv::Point2f>* prev_p;
    std::vector<float> fine_x;
    std::vector<float> fine_y;

    cv::Mat& m = *(cv::Mat*)matAddr;
    jfloat* result = (*env).GetFloatArrayElements(resultAddr, 0);


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

    if (!is_first)
    {
        if(m.rows != prev_m->rows || m.cols != prev_m->cols) is_first = true;
    }

    cv::cvtColor(m, *grey_m, cv::COLOR_RGBA2GRAY);

    if(is_first)
    {
        cv::goodFeaturesToTrack(*grey_m, *p, MAX_CORNERS, 0.3, 7, cv::Mat(), 7, false, 0.04);
        is_first = false;
    }
    else
    {
        if (!(cnt % FEATURE_STEP))
            cv::goodFeaturesToTrack(*prev_m, *prev_p, 100, 0.3, 7, cv::Mat(), 7, false, 0.04);
        cv::calcOpticalFlowPyrLK(*prev_m, *grey_m, *prev_p, *p, status, err, cv::Size(15,15), 2, criteria);

        for(uint i = 0; i < prev_p->size(); i++)
        {
            if(status[i]) {
                cv::circle(m, (*prev_p)[i], 5, cv::Scalar(255, 0, 0));
                cv::circle(m, (*p)[i], 5, cv::Scalar(0, 255, 0));
                fine_x.push_back((*p)[i].x - (*prev_p)[i].x);
                fine_y.push_back((*p)[i].y - (*prev_p)[i].y);
            }
        }

        if(!fine_x.empty()) {
            int ns = (int)fmax(fmin(RANSAC_MAX_NS, fine_x.size() * (1 - RANSAC_R) / 2), 1);
            int nt = (int)(ns * RANSAC_NT_RATE);
            double th = result[2] * RANSAC_NL_RATE;

            int max_match_x = -1;
            double max_avg_x = 0;
            int max_match_y = -1;
            double max_avg_y = 0;

            std::vector<int> rand_src(fine_x.size());
            std::iota(rand_src.begin(), rand_src.end(), 0);

            for (int i = 0; i < nt; i++)
            {
                double avg_x = 0;
                double avg_y = 0;
                //Make model
                std::shuffle(rand_src.begin(), rand_src.end(), engine);
                for (int j = 0; j < ns; j++) {
                    avg_x += fine_x[rand_src[j]];
                    avg_y += fine_y[rand_src[j]];
                }
                avg_x /= ns;
                avg_y /= ns;

                double d = fmax(avg_x * RANSAC_NL_RATE, RANSAC_MIN_D);
                double low_x = avg_x - d;
                double high_x = avg_x + d;
                d = fmax(avg_y * RANSAC_NL_RATE, RANSAC_MIN_D);
                double low_y = avg_y - d;
                double high_y = avg_y + d;

                int match_x = 0;
                int match_y = 0;
                for (int j = 0; j < fine_x.size(); j++)
                {
                    if (low_x <= fine_x[j] && fine_x[j] <= high_x) match_x++;
                    if (low_y <= fine_y[j] && fine_y[j] <= high_y) match_y++;
                }

                if (match_x >= max_match_x)
                {
                    max_match_x = match_x;
                    max_avg_x = avg_x;
                }
                if (match_y >= max_match_y)
                {
                    max_match_y = match_y;
                    max_avg_y = avg_y;
                }
            }

            double d = fmax(max_avg_x * RANSAC_NL_RATE, RANSAC_MIN_D);
            double low_x = max_avg_x - d;
            double high_x = max_avg_x + d;
            d = fmax(max_avg_y * RANSAC_NL_RATE, RANSAC_MIN_D);
            double low_y = max_avg_y - d;
            double high_y = max_avg_y + d;

            std::vector<float> fit_x;
            std::vector<float> fit_y;
            for (int j = 0; j < fine_x.size(); j++)
            {
                if (low_x <= fine_x[j] && fine_x[j] <= high_x) fit_x.push_back(fine_x[j]);
                if (low_y <= fine_y[j] && fine_y[j] <= high_y) fit_y.push_back(fine_y[j]);
            }

            result[0] = (jfloat) (std::accumulate(fit_x.begin(), fit_x.end(), 0.) / fit_x.size());
            result[1] = (jfloat) (std::accumulate(fit_y.begin(), fit_y.end(), 0.) / fit_y.size());

            double avg2 = std::accumulate(fit_x.begin(), fit_x.end(), 0., sq_sum) / fit_x.size();
            result[2] = (jfloat) sqrt(avg2 - result[0] * result[0]);
            avg2 = std::accumulate(fit_y.begin(), fit_y.end(), 0., sq_sum) / fit_y.size();;
            result[3] = (jfloat) sqrt(avg2 - result[1] * result[1]);
        }
        else {
            result[0] = 0;
            result[1] = 0;
            result[2] = FLT_MAX;
            result[3] = FLT_MAX;
        }
    }

    cnt++;
    (*env).ReleaseFloatArrayElements(resultAddr, result, 0);
}

extern "C" JNIEXPORT void JNICALL
Java_com_example_android_1img_1processing_MainActivity_release(JNIEnv*, jobject) {
    grey_m1.release();
    grey_m2.release();
}