#include <opencv2/opencv.hpp>
#include <cstdlib>
#include <algorithm>

using namespace std;


int calculateMedian(cv::Mat img)
{
    std::vector<uchar> array;
    if (img.isContinuous()) 
    {
        array.assign(img.datastart, img.dataend);
    } 
    else 
    {
        for (int i = 0; i < img.rows; ++i) 
        {
            array.insert(array.end(), img.ptr<uchar>(i), img.ptr<uchar>(i)+img.cols);
        }
    }
    std::nth_element(array.begin(), array.begin() + array.size()/2, array.end());
    
    //std::cout << "Median: " << int(array[array.size()/2]) << std::endl;
    return int(array[array.size()/2]);
}

// OK
void rotateImage(cv::Mat img, cv::Mat& rotated, double angle)
{
    cv::Mat matRot = cv::getRotationMatrix2D( cv::Point(img.rows/2, img.cols/2), -angle, 1 );

    cv::warpAffine( img, rotated, matRot, img.size() );
}

// OK
void shiftImage(cv::Mat img, cv::Mat& shifted, int x, int y)
{
    shifted = shifted.zeros(img.rows,img.cols,CV_8U);
    
    cv::Rect source = cv::Rect(max(0,-x),max(0,-y), img.cols-abs(x),img.rows-abs(y));

    cv::Rect target = cv::Rect(max(0,x),max(0,y),img.cols-abs(x),img.rows-abs(y));
    img(source).copyTo(shifted(target));
    /*
    for(int i = 0; i < (img.rows); i++)
        for(int j = 0; j < (img.cols); j++)
        {
            if( i+y < img.rows && j+x < img.cols && i+y >= 0 && j+x >= 0)
                shifted.at<uchar>((i+y),(j+x)) = img.at<uchar>(i,j);
        }
    */  
}

// OK, src & ref are bitmaps
int xorImages(cv::Mat src, cv::Mat ref, cv::Mat& diff)
{
    diff = diff.zeros(src.rows,src.cols,CV_8U);
    int counter = 0;
  
    cv::absdiff(src,ref,diff);
    counter = cv::countNonZero(diff);
    return counter;
}

// OK
void toBitmap(cv::Mat gray, cv::Mat& bitmap, int median)
{
    bitmap = bitmap.zeros(gray.rows,gray.cols,CV_8U);

    threshold( gray, bitmap, median, 255, CV_THRESH_BINARY );

    /*for(int i = 0; i < gray.rows; i++)
        for(int j = 0; j < gray.cols; j++)
        {
            if ( gray.at<uchar>(i,j) > median )
            {
                bitmap.at<uchar>(i,j) = 255;
            }
            else
            {
                bitmap.at<uchar>(i,j) = 0;
            }
        }
     */
}

//OK
void generatePyramids(cv::Mat src, cv::Mat ref, std::vector<cv::Mat>& srcPyramid, std::vector<cv::Mat>& refPyramid, int level)
{
    cv::Mat srcBitmapItr;
    cv::Mat refBitmapItr;
    // May need to claculate median for every level !!
    int mediansrc,medianref;
    for (int i = 0; i < level; i++)
    {
        mediansrc = calculateMedian(src);
        medianref = calculateMedian(ref);
        toBitmap(src,srcBitmapItr,mediansrc);
        toBitmap(ref,refBitmapItr,medianref);
        srcPyramid.push_back(srcBitmapItr);
        refPyramid.push_back(refBitmapItr);
        
        cv::resize(src,src,cv::Size((src.cols/2),(src.rows/2)));
        cv::resize(ref,ref,cv::Size((ref.cols/2),(ref.rows/2)));
    }
}

int walkOnImage(cv::Mat src, cv::Mat ref, int& x, int& y)
{
    int error;
    int minerror = 1000000000;
    int deltaX, deltaY;
    cv::Mat shifted;
    cv::Mat diff;
    for( int rowstep = -(src.rows)+1; rowstep <=(src.rows-1); rowstep++)
        for( int colstep = -(src.cols)+1; colstep <=(src.cols-1); colstep++)
        {
            shiftImage(src, shifted, colstep, rowstep);
            error = xorImages(shifted, ref, diff);
            //std::cout << "ERROR: " << error << "-" << deltaX << "," << deltaY << std::endl;
            if (error < minerror)
            {
                minerror = error;
                deltaX = colstep;
                deltaY = rowstep;
            }
        }
    
    x = x + deltaX;
    y = y + deltaY;
    return minerror;
}

int checkLocalPixels(cv::Mat src, cv::Mat ref, int& x, int& y)
{
    int error;
    int minerror = 1000000000;
    int deltaX, deltaY;
    cv::Mat shifted;
    cv::Mat diff;
    for( int rowstep = -1; rowstep <=1; rowstep++)
        for( int colstep = -1; colstep <=1; colstep++)
        {
            shiftImage(src, shifted, x+colstep, y+rowstep);
            error = xorImages(shifted, ref, diff);
            //std::cout << rowstep << "," << colstep << "    " << error << std::endl;
            if (error < minerror)
            {
                minerror = error;
                deltaX = colstep;
                deltaY = rowstep;
            }
        }

    x = x + deltaX;
    y = y + deltaY;
    return minerror;
}


void mtb(cv::Mat src, cv::Mat ref, int& x, int& y, double& angle)
{
    std::vector<cv::Mat> srcPyramid;
    std::vector<cv::Mat> refPyramid;
    
    int minerror = 1000000000;
    int error;
    int tx,ty;
    cv::Mat rotated;
    
    // For every rotation angle 
    for(double rotangle = -0.5; rotangle <= 0.5; rotangle=rotangle+0.05)
    {
        std::cout << "Rotation: " << rotangle << " Degree" << std::endl;
        srcPyramid.clear();
        refPyramid.clear();
        tx = 0;
        ty = 0;
        
        if ( abs(rotangle-0) < 0.0000001)
            rotangle = 0;
        
        rotateImage(src, rotated, rotangle);
        generatePyramids(rotated, ref, srcPyramid, refPyramid, 6);
        
        // Find x,y offsets
        error = walkOnImage(srcPyramid[srcPyramid.size()-1], refPyramid[refPyramid.size()-1], tx, ty);
        
        for (int i = srcPyramid.size()-2; i >= 0; i--)
        {
            tx = 2*tx;
            ty = 2*ty;
            error = checkLocalPixels(srcPyramid[i], refPyramid[i], tx, ty);
            //std::cout << tx << "," << ty << "   " << error << std::endl;
        }
        
        if ( error < minerror)
        {
            //std::cout << tx << "," << ty << "   " << error << std::endl;
            minerror = error;
            angle = rotangle;
            x = tx;
            y = ty;
        }   
    }
}

// OK
void AlignToRef(cv::Mat ref, std::vector<cv::Mat> images)
{
    int x = 0;
    int y = 0;
    double angle = 0;
    for (int i = 0; i < images.size(); i++)
    {
        mtb(images[i], ref, x, y, angle);
        std::cout << "Image " << i << " x: " << x << " y: " << y << " angle: " << angle << std::endl;
    }
}


int main(int argc, char** argv) {
    cv::Mat src, ref;
    cv::Mat srcgray, refgray;
    src = cv::imread(argv[1]);
    ref = cv::imread(argv[2]);
    cv::cvtColor(src,srcgray,CV_RGB2GRAY);
    cv::cvtColor(ref,refgray,CV_RGB2GRAY);
    int x = 0;
    int y = 0;
    double angle = 0;
    mtb(srcgray, refgray, x, y, angle);
    
    std::cout << " x: " << x << " y: " << y << " angle: " << angle << std::endl;

    cv::Mat rotated;
    cv::Mat aligned;
    rotateImage(srcgray,rotated,angle);
    shiftImage(rotated,aligned,x,y);
    cv::imwrite("aligned.jpg", aligned);
    cv::imwrite("original.jpg", srcgray);
    cv::imwrite("referance.jpg",refgray);
    cv::waitKey(0);



    return 0;
}

