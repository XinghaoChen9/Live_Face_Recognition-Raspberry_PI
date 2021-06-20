#include "parallel_video_capture.hpp"

ParallelVideoCapture::ParallelVideoCapture(int index) : cv::VideoCapture(index), running_(false){}

ParallelVideoCapture::ParallelVideoCapture(const cv::String & filename) : cv::VideoCapture(filename), running_(false){}

ParallelVideoCapture::ParallelVideoCapture(const cv::String &filename, int apiPreference) : cv::VideoCapture(filename,apiPreference), running_(false){}

ParallelVideoCapture::~ParallelVideoCapture()
{
    stopCapture();
}


void ParallelVideoCapture::startCapture()
{
    if(isOpened())
    {
        ParallelVideoCapture*  ptr = static_cast<ParallelVideoCapture*> (this);
        
        if(ptr == nullptr)
        {
            throw std::logic_error("[ERROR][startCapture()] error to convert this object into ParallelVideoCapture* ptr");
        }

        thread_ptr_ = std::make_unique<std::thread>(&captureFromSource,std::ref(*ptr));

    }
    else{
        throw std::ios_base::failure("[ERROR][startCapture()] could not to open the source video\n");
    }

}

bool ParallelVideoCapture::isRunning() const
{
    return running_;
}

uint8_t ParallelVideoCapture::getIntervalMs() const
{
    return interval_ms_;
}

void ParallelVideoCapture::stopCapture()
{
    if(running_)
    {
        running_ = false;
    
        // we join the thread to sync it 
        // this will make out stopCapture function wait for the thread exits from it loop
        thread_ptr_->join(); 

    }

}
bool ParallelVideoCapture::read()
{
    mutex_.lock();
        bool readed = cv::VideoCapture::read(frame_);
    mutex_.unlock();
    
    return readed;
}
bool ParallelVideoCapture::grab()
{
    bool readed = cv::VideoCapture::grab();
    
    return readed;
}
bool ParallelVideoCapture::retrieve(cv::OutputArray image, int flag)
{
    mutex_.lock();
        bool readed = cv::VideoCapture::retrieve(frame_,flag);
    mutex_.unlock();
    
    return readed;
}

void ParallelVideoCapture::release()
{
    cv::VideoCapture::release();
}
cv::Mat ParallelVideoCapture::getFrame()
{
    mutex_.lock();
        cv::Mat img = frame_; // this doesn't invoke any copy constructor
    mutex_.unlock();

    return img;

}
void captureFromSource(ParallelVideoCapture & cap)
{
    bool readed = false;
    auto start = std::chrono::system_clock::now();
    auto end  = std::chrono::system_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    while(cap.isRunning())
    {
        elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

        if(elapsed.count()<cap.getIntervalMs())
        {
            usleep(1);

            end  = std::chrono::system_clock::now();
            continue;
        }
        start = std::chrono::system_clock::now();

        readed = cap.read();

        if(!readed)
            break;
        
    }

    cap.release();

}
