/*
第四步：尝试漫水算法，学习参数设置
*/
#include<iostream>
#include<opencv2/opencv.hpp>

using namespace cv;
using namespace std;

const int ANGLE_TO_UP = 1;
const int WIDTH_GREATER_THAN_HEIGHT = 0;


#define DELAT_MAX 30//定义限幅滤波误差最大值
typedef	int filter_type;//定义限幅滤波数据类型

//为了防止有其他装甲板的干扰，加入限幅滤波
filter_type filter(filter_type effective_value, filter_type new_value, filter_type delat_max);



// 为辅助筛选装甲板，提高算法运行速度，做一次筛选预处理
RotatedRect& adjustRec(cv::RotatedRect& rec, const int mode);



/**
 * 函数名：  lightDetector
 * 参数：   Mat img，要展示的图像
 *         string 窗口名
 * 返回值： 无
 * 功能：   在指定窗口展示照片
 */

void img_show(Mat img, string windowName)
{
    // [1] 新建一个窗口
    namedWindow(windowName, WINDOW_NORMAL);
    // [2] 调整窗口大小
    resizeWindow(windowName, Size(img.cols / 2, img.rows / 2));
    // [3] 展示照片
    imshow(windowName, img);

    return;
}





/**
 * 函数名：  pretreat
 * 参数：   Mat src，要处理的图像
 * 返回值： Mat dst，处理完的图像
 * 功能：   对图片进行预处理
 * ***************************************************************************
 * 思路：
 * 1. 先将图像转为灰度图，对灰度图进行二值化找出亮度较高的区域1
 * 2. 再通过对原图通道相减并二值化得到包含目标颜色的区域2
 * 3. 将区域1和区域2进行相与操作并通过形态学操作得到灯柱候选区域
 */

Mat pretreat(Mat src)
{
    // [1-1] 得到灰度图
    Mat grey;
    cvtColor(src, grey, COLOR_BGR2GRAY);
    img_show(grey,"2-灰度图");
    Mat bigrey;
    // [1-2] 对灰度图进行二值化处理
    threshold(grey, bigrey, 110, 255, THRESH_BINARY);
    img_show(bigrey,"3-二值图");

    // [2-1] 将原图通道分离
    Mat channel[3];
    split(src, channel);
    Mat src_blue; // 存储B通道
    // [2-2] 通道相减得到蓝色通道（装甲板灯条为蓝色），得到单通道图
    subtract(channel[0], channel[2], src_blue);
    img_show(src_blue,"4-提取蓝色");
    // [2-3] 对得到的蓝色通道进行二值化处理
    threshold(src_blue, src_blue, 110, 255, THRESH_BINARY);
    // [2-4] 对得到的蓝色通道进行膨胀处理
    Mat element1 = getStructuringElement(MORPH_RECT, Size(10, 10)); //创建一个用于膨胀的图形
    dilate(src_blue, src_blue, element1);
    img_show(src_blue,"5-blue-二值化图&膨胀处理");

    // [3-1] 用原图蓝色通道二值之后的图片与（&）上原图通道相减二值膨胀之后的图片，提取出较为可信的候选区域
    Mat dst;
    dst = src_blue & bigrey;
    // [3-2] 对dst进行膨胀处理
    Mat element2 = getStructuringElement(MORPH_RECT, Size(2, 2));
    dilate(dst, dst, element2);
    img_show(dst,"6-预处理完成");
    return dst;
}

/**
 * 函数名：  lightDetector
 * 参数：   Mat src，要画标记的图像
 *         Mat treat_img，待处理的图像
 * 返回值： 无
 * 功能：   进行灯柱寻找，并在装甲板中心标记
 * ***************************************************************************
 * 思路：
 * 1. 寻找轮廓
 * 2. 寻找灯柱
 * 3. 灯柱配对
 * 4. 画标记并输出坐标
 */

void lightDetector(Mat src,Mat treat_img)
{
    // [1] 寻找轮廓
    vector<RotatedRect> vc;//存储筛选后的轮廓（即灯柱候选区域）
    vector<RotatedRect> vRec;//存放匹配到的装甲板
    vector<vector<Point>> contours;// 存储发现的轮廓（点阵）
    findContours(treat_img.clone(),contours, RETR_EXTERNAL,CHAIN_APPROX_SIMPLE);//仅检测外围轮廓，仅保留拐点？？？？？？？？？？？

    // [2] 寻找灯柱
    // [2-1] 遍历所有轮廓
    for (int i = 0;i < (int)contours.size();i++)
    {
        // [2-2] 根据面积筛选灯条
        // [2-2-1] 得到灯柱面积
        float Contour_Area = contourArea(contours[i]);
        // [2-2-2] 去除较小轮廓
        if (Contour_Area < 15 || contours[i].size() <= 10)//15、10
            continue;

        // [2-3] 根据长宽比筛选灯柱
        // [2-3-1] 用椭圆拟合区域得到外接矩形(才有长宽比)
        RotatedRect Light_Rec = fitEllipse(contours[i]);
        Light_Rec = adjustRec(Light_Rec, ANGLE_TO_UP);
        // [2-3-2] 此时，还可以使用角度筛选（灯柱大致垂直）
        if (Light_Rec.angle > 10 )
            continue;
        // [2-3-3] 长宽比和轮廓面积比限制
        if (Light_Rec.size.width / Light_Rec.size.height > 1.5 || Contour_Area / Light_Rec.size.area() < 0.5)//1.5、0.5
            continue;

        // [2-4] 扩大灯柱的面积
        Light_Rec. size.height *= 1.1;
        Light_Rec.size.width *= 1.1;

        // [3] 存储灯柱候选区域
        vc.push_back(Light_Rec);
    }


    // [3] 匹配灯柱
    // [3-1] 遍历灯柱候选区域，两两比对
    for (size_t i = 0; i < vc.size(); i++)
    {
        for (size_t j = i + 1; (j < vc.size()); j++)
        {
            // [3-2] 角度比对
            float Contour_angle = abs(vc[i].angle - vc[j].angle); //角度差
            if (Contour_angle >= 7)
                continue;

            // [3-3] 根据长度差比率、宽度差比率配对
            float Contour_Len1 = abs(vc[i].size.height - vc[j].size.height) / max(vc[i].size.height, vc[j].size.height);
            float Contour_Len2 = abs(vc[i].size.width - vc[j].size.width) / max(vc[i].size.width, vc[j].size.width);
            if (Contour_Len1 > 0.25 || Contour_Len2 > 0.25)
                continue;

            // [4] 关于装甲板
            // [4-1] 定义一个装甲板(旋转)矩阵
            RotatedRect ARMOR; //装甲板矩阵
            ARMOR.center.x = (vc[i].center.x + vc[j].center.x) / 2.; //x坐标
            ARMOR.center.y = (vc[i].center.y + vc[j].center.y) / 2.; //y坐标
            ARMOR.angle = (vc[i].angle + vc[j].angle) / 2.; //角度

            // [4-2] 最后一次筛选
            float nh, nw, yDiff, xDiff;
            nh = (vc[i].size.height + vc[j].size.height) / 2; //高度
            // 宽度
            nw = sqrt((vc[i].center.x - vc[j].center.x) * (vc[i].center.x - vc[j].center.x) + (vc[i].center.y - vc[j].center.y) * (vc[i].center.y - vc[j].center.y));
            float ratio = nw / nh; //匹配到的装甲板的长宽比
            xDiff = abs(vc[i].center.x - vc[j].center.x) / nh; //x差比率
            yDiff = abs(vc[i].center.y - vc[j].center.y) / nh; //y差比率
            if (ratio < 1.0 || ratio > 5.0 || xDiff < 0.5 || yDiff > 2.0)
                continue;

            // [5] 绘制装甲板轮廓，中心点
            // [5-1] 装甲板的长、宽
            ARMOR.size.height = nh;
            ARMOR.size.width = nw;
            // [5-2] 存放匹配到的装甲板
            vRec.push_back(ARMOR);
            // [5-3] 为绘制做准备
            Point2f point1;
            Point2f point2;
            point1.x=vc[i].center.x;point1.y=vc[i].center.y+20;
            point2.x=vc[j].center.x;point2.y=vc[j].center.y-20;
            int xmidnum = (point1.x+point2.x)/2;
            int ymidnum = (point1.y+point2.y)/2;
            //此时轮廓已筛选完毕，为了方便输出，我们将得到的数据就此输出处理
            ARMOR.center.x = filter(ARMOR.center.x,xmidnum, DELAT_MAX);
            ARMOR.center.y = filter(ARMOR.center.y,ymidnum, DELAT_MAX);
            // [5-4] 绘制
//            Scalar color(rand() & 255, rand() & 255, rand() & 255);//随机颜色拟合线
            rectangle(src, point1,point2, Scalar(0, 255, 0), 2);//将装甲板框起来
            circle(src,ARMOR.center,10,Scalar(0, 255, 0));//在装甲板中心画一个圆

            // [6] 输出坐标
            string center_x=to_string(ARMOR.center.x);
            string center_y=to_string(ARMOR.center.y);
            string text="("+center_x+","+center_y+")";//坐标
            Point point;//文字位置
            point.x=src.cols/2;
            point.y=src.rows/2;
            Scalar color(0,0,255);//字体颜色
            putText(src,text,point, FONT_HERSHEY_COMPLEX,1,color,1);
        }

    }

}



// 曝光度调节函数
Mat exposureCtr(Mat src);

//得到蓝色通道
Mat getBlue(Mat src);


int main()
{
    // 1.读入图像，判断读入是否成功
    Mat src = imread("D:\\Picture\\armour.jpg");
//    Mat src_test= imread("D:\\Picture\\armour.jpg");
    if (src.empty())
    {
        cout << "can't read this image!" << endl;
        return 0;
    }


    //2.降低曝光
    Mat expo_low_dst=exposureCtr(src);
    img_show(expo_low_dst,"1-降低曝光");

    //3.提取蓝色通道(为灰度图)
    Mat grayImg=getBlue(expo_low_dst);
    img_show(grayImg,"2-提取蓝色通道");


    //4.漫水算法
    Rect ccomp;
    ccomp.width=80;
    ccomp.height=80;
    floodFill(grayImg, Point(50,300), Scalar(255, 255,255), &ccomp, Scalar(13, 13, 13),Scalar(13, 13, 13));
    img_show(grayImg,"3-漫水算法");
//    lightDetector(src_test,grayImg);

    //5.漫水算法后的二值图
    threshold(grayImg,grayImg,110,255,THRESH_BINARY);
    Mat element1 = getStructuringElement(MORPH_RECT, Size(8, 8)); //创建一个用于膨胀的图形
    //6.对漫水算法后的图进行膨胀
    dilate(grayImg, grayImg, element1);
    //7.反转颜色（黑->白，白->黑）
    for(int i=0;i<grayImg.rows;i++)
        for(int j=0;j<grayImg.cols;j++)
            if(grayImg.at<uchar>(i,j)==0)
                grayImg.at<uchar>(i,j)=255;
            else
                grayImg.at<uchar>(i,j)=0;
    img_show(grayImg,"4-漫水算法-反转");

    //8.边缘检测
    Mat canny;
    Canny(grayImg,canny,110,200);
    img_show(canny,"5-边缘检测");
    //9.对边缘检测后的图进行膨胀处理，使轮廓明显
    Mat element2 = getStructuringElement(MORPH_RECT, Size(200, 200)); //创建一个用于膨胀的图形
    dilate(canny, canny, element1);
    img_show(canny,"6-边缘检测反转");
    lightDetector(src,canny);

    //显示原图
    img_show(src,"原图");
//    img_show(src_test,"原图test---4-漫水算法-反转");



    waitKey(0);
    system("pause");
    return EXIT_SUCCESS;
}


/*
* 功能：为辅助筛选装甲板，提高算法运行速度，做一次筛选预处理
* 说明：由于灯条是竖着的，借此纠正不是竖着的轮廓，方便算法查找
*/
RotatedRect& adjustRec(cv::RotatedRect& rec, const int mode)
   {
       using std::swap;

       float& width = rec.size.width;
       float& height = rec.size.height;
       float& angle = rec.angle;

       if (mode == WIDTH_GREATER_THAN_HEIGHT)
       {
           if (width < height)
           {
               swap(width, height);
               angle += 90.0;
           }
       }

       while (angle >= 90.0) angle -= 180.0;
       while (angle < -90.0) angle += 180.0;

       if (mode == ANGLE_TO_UP)
       {
           if (angle >= 45.0)
           {
               swap(width, height);
               angle -= 90.0;
           }
           else if (angle < -45.0)
           {
               swap(width, height);
               angle += 90.0;
           }
       }
   return rec;
}


/*
* 功能：为了防止有其他装甲板的干扰，加入限幅滤波
*/
filter_type filter(filter_type effective_value, filter_type new_value, filter_type delat_max)
{
    if ( ( new_value - effective_value > delat_max ) || ( effective_value - new_value > delat_max ))
    {
        new_value=effective_value;
        return effective_value;
    }
    else
    {
        new_value=effective_value;
        return new_value;
    }
}



/*
* 功能：曝光度调节函数
*/
Mat exposureCtr(Mat src)
{
    Mat   dst;
    double gama = 1.0;

    // 提示并输入 γ 的值
    cout << "* Enter the gama value [-1,1]: ";
    cin >> gama;
    // 构建查找表
    Mat lookUpTable(1, 256, CV_8U);
    uchar* p = lookUpTable.ptr();
    for (int i = 0; i < 256; ++i)
        p[i] = saturate_cast<uchar>(pow(i / 255.0, gama) * 255.0);

   // 使用查找表进行对比度亮度调整
    LUT(src, lookUpTable, dst);

    return dst;
}


/*
* 功能：得到蓝色通道
*/
Mat getBlue(Mat src)
{
    vector <Mat> channels;
    split(src,channels);//分离色彩通道
    //预处理删除己方装甲板颜色
    Mat dst=channels.at(0)-channels.at(2)-channels.at(1);//Get blue-red image;

    return dst;
}
