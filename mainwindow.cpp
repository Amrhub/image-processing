#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <QPushButton>
#include <QToolButton>
#include <QFileDialog>
#include <QMessageBox>
#include <QCheckBox>
#include <QDoubleValidator>
#include <QIntValidator>
#include <opencv2/opencv.hpp>
#include <iostream>
#include <string>
// #include "clickable_label.h"

using namespace cv;
using namespace std;

bool didEditFinish = false, shouldRotate;
int prevX, prevY, d0 = 50;
float angle, scale = 1;
Mat image, imageGrayed, ROI, dstTranslatedImage, dstRotatedImage, dstZoomedImage, dstAreaOfInterestImage, dstDeSkewedImage, dstSmoothedImage, dstFrequencyDomainImage;
vector<Point> vertices;
vector<Mat> images;
vector<Point2f> srcPoints, dstPoints;
int currentImageIndex = 0;
QString fileName;

struct NumberFrequency
{
    int frequency;
    double probability;
};

struct TrackbarWindowData
{
    cv::Mat image;
    cv::Mat dstImage;
    string windowName;
    MainWindow *mainWindow;
};

struct ZoomData
{
    int rectangleSize;
    // optional kernel make it optional to use a kernel
    cv::Mat kernel;
};

enum KeyCodes
{
    SPACE = 32,
    ENTER = 13,
    ESC = 27,
};

// map that holds the category and all QPushButton that are subItems of that category
QMap<Categories, std::vector<QToolButton *>> categorySubItems;

QMap<Categories, QPushButton *> categoryBtns;

void resetEdit()
{
    destroyAllWindows();
    didEditFinish = false;
    shouldRotate = false;
    prevX = 0;
    prevY = 0;
    angle = 0;
    scale = 1;
    dstPoints.clear();
    srcPoints.clear();
    vertices.clear();
}

// Translate
void translateWindowMouseHandler(int event, int x, int y, int flags, void *userdata)
{
    if (event == EVENT_RBUTTONDOWN)
    {
        dstTranslatedImage.copyTo(image);
        didEditFinish = true;
        return;
    }
    if (event == EVENT_LBUTTONDOWN)
    {
        prevX = x;
        prevY = y;
        return;
    }

    if (event == EVENT_LBUTTONUP)
    {
        prevX = 0;
        prevY = 0;
        return;
    }

    if (event == EVENT_MOUSEMOVE && prevX && prevY)
    {
        int txValue = x - prevX;
        int tyValue = y - prevY;
        prevX = x;
        prevY = y;
        Mat translationMatrix = (Mat_<float>(2, 3) << 1, 0, txValue, 0, 1, tyValue);
        warpAffine(dstTranslatedImage, dstTranslatedImage, translationMatrix, image.size());
        return;
    }
}

// Rotate
void rotationWindowMouseHandler(int event, int x, int y, int flags, void *userdata)
{
    if (event == EVENT_RBUTTONDOWN)
    {
        dstRotatedImage.copyTo(image);
        didEditFinish = true;
        return;
    }
    if (event == EVENT_LBUTTONDOWN)
    {
        prevX = x;
        prevY = y;
        shouldRotate = true;
        return;
    }

    if (event == EVENT_LBUTTONUP)
    {
        shouldRotate = false;
        return;
    }

    if (event == EVENT_MOUSEMOVE && shouldRotate)
    {
        int sensitivity = image.cols;
        int xDiff = x - prevX;
        angle = (xDiff * 1.0 / sensitivity * 1.0) * 360;
        Mat rotationMatrix = getRotationMatrix2D(Point2f(prevX, prevY), angle, scale);
        warpAffine(image, dstRotatedImage, rotationMatrix, image.size());
        return;
    }

    if (event == EVENT_MOUSEWHEEL)
    {
        float step = 0.01;
        scale += (y < 0 ? -1.0 * step : step);
        if (scale < 0.1)
            scale = 0.1;
        Mat rotationMatrix = getRotationMatrix2D(Point2f(prevX, prevY), angle, scale);
        warpAffine(image, dstRotatedImage, rotationMatrix, image.size());
        return;
    }
}

// Zoom
void zoomWindowMouseHandler(int event, int x, int y, int flags, void *zoomData)
{
    ZoomData *data = (ZoomData *)zoomData;
    int &rectangleSize = data->rectangleSize;

    if (event == EVENT_MOUSEMOVE)
    {
        //  erase the previous rectangle
        prevX = x;
        prevY = y;
        image.copyTo(dstZoomedImage);
        rectangle(dstZoomedImage, Point(prevX - rectangleSize, prevY - rectangleSize), Point(prevX + rectangleSize, prevY + rectangleSize), Scalar(0, 0, 0), 2);
    }

    if (event == EVENT_MOUSEWHEEL)
    {
        if (y == 0)
            return;
        rectangleSize += (y > 0 ? 10 : -10);
        if (rectangleSize < 10)
        {
            rectangleSize = 10;
        }

        image.copyTo(dstZoomedImage);
        rectangle(dstZoomedImage, Point(prevX - rectangleSize, prevY - rectangleSize), Point(prevX + rectangleSize, prevY + rectangleSize), Scalar(0, 0, 0), 2);
    }

    if (event == EVENT_LBUTTONDOWN)
    {
        int xStart = prevX - rectangleSize;
        int yStart = prevY - rectangleSize;
        int xEnd = prevX + rectangleSize;
        int yEnd = prevY + rectangleSize;
        rectangleSize -= 1;

        if (xStart < 0 || yStart < 0 || xEnd > image.cols || yEnd > image.rows)
        {
            QMessageBox::warning(nullptr, "Error", "Please select a valid area to zoom");
            return;
        }

        Mat croppedImage = image(Rect(prevX - rectangleSize, prevY - rectangleSize, rectangleSize * 2, rectangleSize * 2));
        cv::resize(croppedImage, image, Size(), 2, 2);
        image.copyTo(dstZoomedImage);
    }

    if (event == EVENT_RBUTTONDOWN)
    {
        image.copyTo(dstZoomedImage);
        didEditFinish = true;
    }
}

// Gray Level Slicing
void areaOfInterestMouseHandler(int event, int x, int y, int flags, void *areaOfInterestData)
{
    ZoomData *data = (ZoomData *)areaOfInterestData;
    int &rectangleSize = data->rectangleSize;

    if (event == EVENT_MOUSEMOVE)
    {
        //  erase the previous rectangle
        prevX = x;
        prevY = y;
        imageGrayed.copyTo(dstAreaOfInterestImage);
        rectangle(dstAreaOfInterestImage, Point(prevX - rectangleSize, prevY - rectangleSize), Point(prevX + rectangleSize, prevY + rectangleSize), Scalar(0, 0, 0), 2);
    }

    if (event == EVENT_MOUSEWHEEL)
    {
        if (y == 0)
            return;
        rectangleSize += (y > 0 ? 10 : -10);
        if (rectangleSize < 10)
        {
            rectangleSize = 10;
        }

        imageGrayed.copyTo(dstAreaOfInterestImage);
        rectangle(dstAreaOfInterestImage, Point(prevX - rectangleSize, prevY - rectangleSize), Point(prevX + rectangleSize, prevY + rectangleSize), Scalar(0, 0, 0), 2);
    }

    if (event == EVENT_LBUTTONDOWN)
    {
        int xStart = prevX - rectangleSize;
        int yStart = prevY - rectangleSize;
        int xEnd = prevX + rectangleSize;
        int yEnd = prevY + rectangleSize;
        int rangeFrom = 255;
        int rangeTo = 0;
        std::map<int, NumberFrequency> rangeValues = {};

        int totalSelectedPixels = pow(rectangleSize * 2, 2);

        // get the range from the rectangle selected
        for (int i = yStart; i < yEnd; i++)
        {
            for (int j = xStart; j < xEnd; j++)
            {
                if (rangeValues.find(imageGrayed.at<uchar>(i, j)) == rangeValues.end())
                {
                    rangeValues.insert({imageGrayed.at<uchar>(i, j), {1, 1.0 / (totalSelectedPixels * 1.0)}});
                    // rangeValues[imageGrayed.at<uchar>(i, j)] = {frequency : 1, probability : 1 / totalSelectedPixels};
                }
                else
                {
                    rangeValues.at(imageGrayed.at<uchar>(i, j)).frequency += 1;
                    rangeValues.at(imageGrayed.at<uchar>(i, j)).probability = rangeValues.at(imageGrayed.at<uchar>(i, j)).frequency / (totalSelectedPixels * 1.0);
                }
            }
        }

        for (auto const &[key, value] : rangeValues)
        {
            if (value.probability > 0.005)
            {
                if (key < rangeFrom)
                {
                    rangeFrom = key;
                }

                if (key > rangeTo)
                {
                    rangeTo = key;
                }
            }
        }

        cout << "xStart: " << xStart << " yStart: " << yStart << " xEnd: " << xEnd << " yEnd: " << yEnd << endl;
        cout << "Range from: " << rangeFrom << " Range to: " << rangeTo << endl;

        // get the value to be set if r(i,j) not in range [A,B] if this value is not provided r(i,j) = r(i,j) it will not replace the value out of range.
        for (int i = 0; i < imageGrayed.rows; i++)
        {
            for (int j = 0; j < imageGrayed.cols; j++)
            {
                int pointValue = imageGrayed.at<uchar>(i, j);
                if (pointValue > rangeFrom && pointValue < rangeTo)
                {
                    imageGrayed.at<uchar>(i, j) = 255;
                }
                else
                {
                    imageGrayed.at<uchar>(i, j) = 0;
                }
            }
        }
        imageGrayed.copyTo(dstAreaOfInterestImage);
    }

    if (event == EVENT_RBUTTONDOWN)
    {
        imageGrayed.copyTo(dstAreaOfInterestImage);
        didEditFinish = true;
    }
}

// DeSkew
void deSkewImageMouseHandler(int event, int x, int y, int, void *userdata)
{
    if (event == EVENT_LBUTTONDOWN)
    {
        if (srcPoints.size() < 3)
        {
            srcPoints.push_back(Point2f(x, y));
            circle(dstDeSkewedImage, Point(x, y), 5, Scalar(0, 0, 255), 2);
            cout << "Selected source point: (" << x << ", " << y << ")" << endl;
        }
        else if (dstPoints.size() < 3)
        {
            dstPoints.push_back(Point2f(x, y));
            circle(dstDeSkewedImage, Point(x, y), 5, Scalar(0, 255, 0), 2);
            cout << "Selected destination point: (" << x << ", " << y << ")" << endl;
        }

        if (srcPoints.size() == 3 && dstPoints.size() == 3)
        {
            Mat skewingMatrix = getAffineTransform(srcPoints, dstPoints);
            warpAffine(image, dstDeSkewedImage, skewingMatrix, image.size());
        }
    }

    if (event == EVENT_RBUTTONDOWN)
    {
        // image.copyTo(dstDeSkewedImage);
        didEditFinish = true;
        destroyWindow("Select Points");
    }
}

void smoothingFiltersMouseHandler(int event, int x, int y, int, void *smoothImageData)
{
    ZoomData *data = (ZoomData *)smoothImageData;
    int &rectangleSize = data->rectangleSize;
    Mat &kernel = data->kernel;

    if (event == EVENT_MOUSEMOVE)
    {
        //  erase the previous rectangle
        prevX = x;
        prevY = y;
        image.copyTo(dstSmoothedImage);
        rectangle(dstSmoothedImage, Point(prevX - rectangleSize, prevY - rectangleSize), Point(prevX + rectangleSize, prevY + rectangleSize), Scalar(0, 0, 0), 2);
    }

    if (event == EVENT_MOUSEWHEEL)
    {
        if (y == 0)
            return;
        rectangleSize += (y > 0 ? 10 : -10);
        if (rectangleSize < 10)
        {
            rectangleSize = 10;
        }

        image.copyTo(dstSmoothedImage);
        rectangle(dstSmoothedImage, Point(prevX - rectangleSize, prevY - rectangleSize), Point(prevX + rectangleSize, prevY + rectangleSize), Scalar(0, 0, 0), 2);
    }

    if (event == EVENT_LBUTTONDOWN)
    {
        if (image.channels() != 1)
            cvtColor(image, imageGrayed, COLOR_BGR2GRAY);
        else
            image.copyTo(imageGrayed);
        Mat tempImage = imageGrayed.clone();
        imageGrayed.copyTo(dstSmoothedImage);
        filter2D(imageGrayed, tempImage, CV_8UC1, kernel);
        int initialI = prevY - rectangleSize;
        int initialJ = prevX - rectangleSize;
        int finalI = prevY + rectangleSize;
        int finalJ = prevX + rectangleSize;

        // check if the selected area is out of bounds
        if (initialI < 0)
        {
            initialI = 0;
        }

        if (initialJ < 0)
        {
            initialJ = 0;
        }
        if (finalI > image.rows)
        {
            finalI = image.rows;
        }
        if (finalJ > image.cols)
        {
            finalJ = image.cols;
        }

        // apply the filter only in the selected area
        for (int i = initialI; i < finalI; i++)
        {
            for (int j = initialJ; j < finalJ; j++)
            {
                dstSmoothedImage.at<uchar>(i, j) = tempImage.at<uchar>(i, j);
            }
        }

        dstSmoothedImage.copyTo(image);
    }

    if (event == EVENT_RBUTTONDOWN)
    {
        image.copyTo(dstSmoothedImage);
        didEditFinish = true;
    }
}

void brightnessAdjustmentMouseHandler(int event, int x, int y, int, void *data)
{
    if (event == EVENT_RBUTTONDOWN)
    {
        TrackbarWindowData *userData = (TrackbarWindowData *)data;
        userData->dstImage.copyTo(image);
        userData->mainWindow->onImageProcessingSubmit(true);
        destroyWindow(userData->windowName);
    }
}

void frequencyDomainMouseHandler(int event, int x, int y, int, void *data)
{
    if (event == EVENT_RBUTTONDOWN)
    {
        TrackbarWindowData *userData = (TrackbarWindowData *)data;

        didEditFinish = true;
        destroyWindow(userData->windowName);
        userData->dstImage.copyTo(image);
        userData->mainWindow->onImageProcessingSubmit(true);
    }
}

void segmentationThresholdingMouseHandler(int event, int x, int y, int, void *data)
{
    cout << "EVENT: " << event << endl;
    if (event == EVENT_RBUTTONDOWN)
    {
        didEditFinish = true;
        TrackbarWindowData *userData = (TrackbarWindowData *)data;
        cout << "Destroying window: " << userData->windowName << endl;
        destroyWindow(userData->windowName);
        cout << "Copying image to main image" << endl;
        userData->dstImage.copyTo(image);
        cout << "Calling onImageProcessingSubmit" << endl;
        userData->mainWindow->onImageProcessingSubmit(true);
        cout << "Finished" << endl;
    }
}

int imageDepth2Bits(int depth)
{
    switch (depth)
    {
    case CV_8U:
    case CV_8S:
        return 8;
    case CV_16U:
    case CV_16S:
        return 16;
    case CV_32S:
    case CV_32F:
        return 32;
    case CV_64F:
        return 64;
    default:
        return 0;
    }
}

tuple<int, int, int, int> imageDetails(Mat img)
{
    return make_tuple((int)img.total(), (int)img.rows, (int)img.cols, (int)img.depth());
}

tuple<int, int, int> imageMinMaxAvg(Mat img)
{
    int max = 0;
    int min = 255;
    int pixelsValues = 0;

    for (int i = 0; i < img.rows; i++)
    {
        for (int j = 0; j < img.cols; j++)
        {
            int pointValue = img.at<uchar>(i, j);
            pixelsValues += pointValue;

            if (pointValue > max)
            {
                max = pointValue;
            }

            if (pointValue < min)
            {
                min = pointValue;
            }
        }
    }

    return make_tuple(min, max, pixelsValues / img.total());
}

// Without changing the format
QImage matToQImage(Mat img)
{
    if (img.empty())
        return QImage();

    if (img.type() == CV_8UC1) // Grayscale
    {
        return QImage(img.data, img.cols, img.rows, img.step,
                      QImage::Format_Grayscale8)
            .copy();
    }
    else if (img.type() == CV_8UC3) // RGB
    {
        Mat rgbImg;
        cvtColor(img, rgbImg, COLOR_BGR2RGB);
        return QImage(rgbImg.data, rgbImg.cols, rgbImg.rows, rgbImg.step,
                      QImage::Format_RGB888)
            .copy();
    }
    else if (img.type() == CV_8UC4) // RGBA
    {
        return QImage(img.data, img.cols, img.rows, img.step,
                      QImage::Format_RGBA8888)
            .copy();
    }
    else if (img.type() == CV_32FC1)
    {
        Mat img8bit;
        img.convertTo(img8bit, CV_8U, 255.0);
        return QImage(img8bit.data, img8bit.cols, img8bit.rows, img8bit.step,
                      QImage::Format_Grayscale8)
            .copy();
    }

    return QImage();
}

void showImage(string windowName, Mat image)
{
    namedWindow(windowName);
    imshow(windowName, image);
    waitKey(0);
    destroyWindow(windowName);
}

int imgSize(Mat img)
{
    return img.total() * imageDepth2Bits(img.depth());
}

int MainWindow::showFlipPopup()
{
    QMessageBox msgBox;
    msgBox.setWindowTitle("Select Option");
    msgBox.setText("Flip the image:");
    msgBox.setStandardButtons(QMessageBox::Close);
    QPushButton *horizontalButton = msgBox.addButton("Horizontally", QMessageBox::NoRole);
    QPushButton *verticalButton = msgBox.addButton("Vertically", QMessageBox::NoRole);
    QPushButton *bothButton = msgBox.addButton("Both", QMessageBox::NoRole);
    msgBox.exec();

    if (msgBox.clickedButton() == verticalButton)
    {
        return 1;
    }
    else if (msgBox.clickedButton() == horizontalButton)
    {
        return 0;
    }
    else if (msgBox.clickedButton() == bothButton)
    {
        return -1;
    }

    return -2;
}

string getCategoryName(Categories category)
{
    switch (category)
    {
    case Clarity:
        return "Clarity";
    case Adjust:
        return "Adjust";
    case Effect:
        return "Effect";
    case Detection:
        return "Detection";
    case UnCategorized:
        return "UnCategorized";
    default:
        return "None";
    }
}

void MainWindow::changeToolCategory(Categories category)
{
    // Hide all QToolButtons or QPushButtons in the subItemsScrollArea except the one at the index of the current category
    for (int i = 0; i < categorySubItems.size(); i++)
    {
        if (categorySubItems[(Categories)i].empty() || category == (Categories)i)
        {
            continue;
        }

        for (int j = 0; j < categorySubItems[(Categories)i].size(); j++)
        {
            categorySubItems[(Categories)i][j]->hide();
        }
    }

    if (category == None)
    {
        return;
    }

    // reset active style
    for (int i = 0; i < categoryBtns.size(); i++)
    {
        categoryBtns[(Categories)i]->setStyleSheet("QPushButton { color: #ffffff; }");
    }

    // set active style
    categoryBtns[category]->setStyleSheet("QPushButton { color: #4FA270; }");

    for (int i = 0; i < categorySubItems[category].size(); i++)
    {
        cout << "Show sub items for " << getCategoryName(category) << "::" << categorySubItems[category][i]->text().toStdString() << endl;
        categorySubItems[category][i]->show();
    }
}

void MainWindow::setupBtnFunctionalities()
{
    connect(ui->uploadBtn, &QPushButton::clicked, this, &MainWindow::onUploadBtnClicked);
    connect(ui->saveBtn, &QPushButton::clicked, this, &MainWindow::onSaveBtnClicked);
    connect(ui->imagePropertiesBtn, &QPushButton::clicked, this, &MainWindow::onImagePropertiesBtnClicked);

    connect(ui->cvtToGrayBtn, &QPushButton::clicked, this, &MainWindow::onCvtGrayBtnClicked);
    connect(ui->currentImageContainer, SIGNAL(clicked()), this, SLOT(onImageContainerClicked()));
    connect(ui->translateBtn, &QPushButton::clicked, this, &MainWindow::onTranslateBtnClicked);
    connect(ui->rotateBtn, &QPushButton::clicked, this, &MainWindow::onRotateBtnClicked);
    connect(ui->flipBtn, &QPushButton::clicked, this, &MainWindow::onFlipBtnClicked);
    connect(ui->brightnessAdjustBtn, &QPushButton::clicked, this, &MainWindow::onBrightnessAdjustBtnClicked);
    connect(ui->histogramEqBtn, &QPushButton::clicked, this, &MainWindow::onHistogramEqBtnClicked);
    connect(ui->negativeBtn, &QPushButton::clicked, this, &MainWindow::onNegativeBtnClicked);
    connect(ui->logTransformBtn, &QPushButton::clicked, this, &MainWindow::onLogTransformationBtnClicked);
    connect(ui->bitSlicingBtn, &QPushButton::clicked, this, &MainWindow::onBitSlicingBtnClicked);
    connect(ui->zoomBtn, &QPushButton::clicked, this, &MainWindow::onZoomBtnClicked);
    connect(ui->areaOfInterestBtn, &QPushButton::clicked, this, &MainWindow::onAreaOfInterestBtnClicked);
    connect(ui->deSkewImageBtn, &QPushButton::clicked, this, &MainWindow::onDeSkewBtnClicked);
    connect(ui->smoothingBtn, &QPushButton::clicked, this, &MainWindow::onSmoothingBtnClicked);
    connect(ui->medianBtn, &QPushButton::clicked, this, &MainWindow::onMedianBtnClicked);
    connect(ui->sobelBtn, &QPushButton::clicked, this, &MainWindow::onSobelBtnClicked);
    connect(ui->frequencyDomainBtn, &QPushButton::clicked, this, &MainWindow::onFrequencyDomainBtnClicked);
    connect(ui->segmentationBtn, &QPushButton::clicked, this, &MainWindow::onSegmentationBtnClicked);
    connect(ui->laplacianOfGaussianBtn, &QPushButton::clicked, this, &MainWindow::onLaplacianOfGaussianBtnClicked);

    connect(ui->undoBtn, &QPushButton::clicked, this, &MainWindow::onUndoBtnClicked);
    connect(ui->resetBtn, &QPushButton::clicked, this, &MainWindow::onResetBtnClicked);
    connect(ui->showDiffBtn, &QPushButton::pressed, this, &MainWindow::onShowDiffBtnPressed);
    connect(ui->showDiffBtn, &QPushButton::released, this, &MainWindow::onShowDiffBtnReleased);
    connect(ui->redoBtn, &QPushButton::clicked, this, &MainWindow::onRedoBtnClicked);

    connect(ui->clarityBtn, &QPushButton::clicked, this, [this]()
            { changeToolCategory(Clarity); });
    connect(ui->adjustBtn, &QPushButton::clicked, this, [this]()
            { changeToolCategory(Adjust); });
    connect(ui->effectBtn, &QPushButton::clicked, this, [this]()
            { changeToolCategory(Effect); });
    connect(ui->detectionBtn, &QPushButton::clicked, this, [this]()
            { changeToolCategory(Detection); });
    connect(ui->uncategorizedBtn, &QPushButton::clicked, this, [this]()
            { changeToolCategory(UnCategorized); });

    categoryBtns[Clarity] = ui->clarityBtn;
    categoryBtns[Adjust] = ui->adjustBtn;
    categoryBtns[Effect] = ui->effectBtn;
    categoryBtns[Detection] = ui->detectionBtn;
    categoryBtns[UnCategorized] = ui->uncategorizedBtn;

    categorySubItems[Clarity] = std::vector<QToolButton *>{ui->medianBtn, ui->smoothingBtn, ui->frequencyDomainBtn};
    categorySubItems[Adjust] = std::vector<QToolButton *>{ui->translateBtn, ui->rotateBtn, ui->flipBtn, ui->zoomBtn, ui->deSkewImageBtn};
    categorySubItems[Effect] = std::vector<QToolButton *>{ui->histogramEqBtn, ui->negativeBtn, ui->logTransformBtn, ui->cvtToGrayBtn, ui->areaOfInterestBtn};
    categorySubItems[Detection] = std::vector<QToolButton *>{ui->sobelBtn, ui->segmentationBtn, ui->laplacianOfGaussianBtn};
    categorySubItems[UnCategorized] = std::vector<QToolButton *>{ui->brightnessAdjustBtn, ui->bitSlicingBtn};

    changeToolCategory(Categories::Clarity);
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    MainWindow::setupBtnFunctionalities();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::onImageProcessingSubmit(bool shouldUpdateImages = true)
{
    QImage qImage = matToQImage(image);
    cout << "image type() " << image.type() << endl;
    if (image.type() == CV_32FC1)
    {
        image.convertTo(image, CV_8UC1, 255.0);
    }

    // Update the image gray on every image processing
    if (image.channels() != 1)
    {
        cvtColor(image, imageGrayed, COLOR_RGB2GRAY);
    }
    else
    {
        image.copyTo(imageGrayed);
    }
    ui->currentImageContainer->setPixmap(QPixmap::fromImage(qImage));
    ui->currentImageContainer->setScaledContents(true);

    if (shouldUpdateImages)
    {
        if (currentImageIndex != images.size() - 1)
        {
            images.erase(images.begin() + currentImageIndex + 1, images.end());
        }

        images.push_back(image.clone());
        currentImageIndex = images.size() - 1;
    }
    cout << "currentImageIndex " << currentImageIndex << endl;
    if (currentImageIndex == 0)
    {
        ui->undoBtn->setEnabled(false);
        ui->showDiffBtn->setEnabled(false);
    }
    else
    {
        ui->undoBtn->setEnabled(true);
        ui->showDiffBtn->setEnabled(true);
    }

    if (currentImageIndex == images.size() - 1)
    {
        ui->redoBtn->setEnabled(false);
    }
    else
    {
        ui->redoBtn->setEnabled(true);
    }

    if (images.size() == 1)
    {
        ui->resetBtn->setEnabled(false);
    }
    else
    {
        ui->resetBtn->setEnabled(true);
    }
}

void MainWindow::enableBtnsOnUpload()
{
    ui->saveBtn->setEnabled(true);
    ui->imagePropertiesBtn->setEnabled(true);

    ui->cvtToGrayBtn->setEnabled(true);
    ui->translateBtn->setEnabled(true);
    ui->rotateBtn->setEnabled(true);
    ui->flipBtn->setEnabled(true);
    ui->brightnessAdjustBtn->setEnabled(true);
    ui->histogramEqBtn->setEnabled(true);
    ui->negativeBtn->setEnabled(true);
    ui->logTransformBtn->setEnabled(true);
    ui->bitSlicingBtn->setEnabled(true);
    ui->zoomBtn->setEnabled(true);
    ui->areaOfInterestBtn->setEnabled(true);
    ui->deSkewImageBtn->setEnabled(true);
    ui->smoothingBtn->setEnabled(true);
    ui->medianBtn->setEnabled(true);
    ui->sobelBtn->setEnabled(true);
    ui->frequencyDomainBtn->setEnabled(true);
    ui->segmentationBtn->setEnabled(true);
    ui->laplacianOfGaussianBtn->setEnabled(true);
}

void MainWindow::onUploadBtnClicked()
{
    fileName = QFileDialog::getOpenFileName(this, "Open Image File", "", "Images (*.png *.xpm *.jpg *.jpeg *.bmp)");

    if (!fileName.isEmpty())
    {
        image = imread(fileName.toStdString());
        if (!image.empty())
        {
            MainWindow::enableBtnsOnUpload();
            if (images.empty())
            {
                images.push_back(image.clone());
            }
            else
            {
                images.erase(images.begin(), images.end());
                images.push_back(image.clone());
            }
            onImageProcessingSubmit(false);
            resetEdit();
        }
        else
        {
            QMessageBox::warning(this, "Error", "Failed to load the image.");
        }
    }
}

void MainWindow::onSaveBtnClicked()
{
    QString fileName = QFileDialog::getSaveFileName(this, "Save Image File", "", "Images (*.png *.xpm *.jpg *.jpeg *.bmp)");

    if (!fileName.isEmpty())
    {
        bool saved = imwrite(fileName.toStdString(), image);

        if (saved)
        {
            QMessageBox::information(this, "Success", "Image saved successfully.");
        }
    }
}

void MainWindow::onImagePropertiesBtnClicked()
{
    auto [total, rows, cols, depth] = imageDetails(image);
    auto [min, max, avg] = imageMinMaxAvg(image);
    QMessageBox msgBox;
    msgBox.setWindowTitle("Image Properties");
    msgBox.setText(QString::fromStdString("Path: " + fileName.toStdString() + "\n" + "Dimensions (RowsXCols): " + to_string(rows) + "x" + to_string(cols) + "\n" + "Total pixels: " + to_string(total) + "\n" + "Depth code: " + to_string(depth) + "\n" + "Minimum pixel value: " + to_string(min) + "\n" + "Maximum pixel value: " + to_string(max) + "\n" + "Dynamic range: (" + to_string(min) + ", " + to_string(max) + ")" + "\n" + "Average pixel value: " + to_string(avg) + "\n" + "Total number of bits required to store the image: " + to_string(imgSize(image))));
    msgBox.setStyleSheet("QLabel{min-width:500px; font-size: 18px;} QPushButton{ width:250px; font-size: 16px; }");
    msgBox.exec();
}

void MainWindow::onCvtGrayBtnClicked()
{
    if (image.channels() == 1)
    {
        QMessageBox::warning(this, "Error", "Image is already in grayscale.");
        return;
    }

    cvtColor(image, image, COLOR_RGB2GRAY);
    onImageProcessingSubmit();
}

void MainWindow::onImageContainerClicked()
{
    if (image.empty())
    {
        QMessageBox::warning(this, "Error", "No image to show. Please upload Image");
        return;
    }

    showImage("Image", image);
}

void MainWindow::onTranslateBtnClicked()
{
    resetEdit();
    string windowName = "Adjust position";
    namedWindow(windowName, WINDOW_NORMAL);

    // Register a mouse callback
    setMouseCallback(windowName, translateWindowMouseHandler, nullptr);
    image.copyTo(dstTranslatedImage);

    // Loop until the user input the termination input
    while (!didEditFinish)
    {
        imshow(windowName, dstTranslatedImage);
        int keyCode = waitKey(5);

        if (keyCode == KeyCodes::ESC)
        {
            destroyWindow(windowName);
            return;
        }
    }
    destroyWindow(windowName);
    onImageProcessingSubmit();
}

void MainWindow::onRotateBtnClicked()
{
    resetEdit();
    string windowName = "Adjust Rotation";
    namedWindow(windowName, WINDOW_NORMAL);
    setMouseCallback(windowName, rotationWindowMouseHandler, nullptr);
    image.copyTo(dstRotatedImage);

    while (!didEditFinish)
    {
        imshow(windowName, dstRotatedImage);
        int keyCode = waitKey(5);

        if (keyCode == KeyCodes::ESC)
        {
            destroyWindow(windowName);
            return;
        }
    }
    onImageProcessingSubmit();

    destroyWindow(windowName);
}

void MainWindow::onFlipBtnClicked()
{
    int flipOption = showFlipPopup();
    if (flipOption == -2)
    {
        return;
    }

    flip(image, image, flipOption);
    onImageProcessingSubmit();
}

void MainWindow::onBrightnessAdjustBtnClicked()
{
    resetEdit();
    string windowName = "Adjust Brightness";
    namedWindow(windowName, WINDOW_AUTOSIZE);

    TrackbarWindowData userData;
    imageGrayed.copyTo(userData.dstImage);
    userData.image = imageGrayed.clone();
    userData.windowName = windowName;
    userData.mainWindow = this;

    createTrackbar("Brightness", windowName, NULL, 100, [](int value, void *userData)
                   {
                   // Map the trackbar value to the range -10 to 10
                   float gammaValue = (value * 1.0) / 50.0 ;
                   cout << "Gamma Value: " << gammaValue << endl;

                   // Access the image from userData
                   TrackbarWindowData *data = (TrackbarWindowData *)userData;
                   cv::Mat &image = data->image;
                    cv::Mat &dstImage = data->dstImage;
                    image.copyTo(dstImage);

                   int imageBits = imageDepth2Bits(dstImage.depth());
                   int maxPixelValue = pow(2, imageBits) - 1;

                   dstImage.convertTo(dstImage, CV_32F);

                   for (int i = 0; i < dstImage.rows; i++)
                   {
                       for (int j = 0; j < dstImage.cols; j++)
                       {
                           float pixelValue = dstImage.at<float>(i, j);
                           dstImage.at<float>(i, j) = pow(pixelValue, gammaValue);
                       }
                   }

                   normalize(dstImage, dstImage, 0, maxPixelValue, NORM_MINMAX);
                   convertScaleAbs(dstImage, dstImage);

                   imshow(data->windowName, dstImage); }, &userData);
    setTrackbarPos("Brightness", windowName, 50);
    setTrackbarMin("Brightness", windowName, 1);
    setMouseCallback(windowName, brightnessAdjustmentMouseHandler, &userData);
    imshow(windowName, imageGrayed);
waiting_key:
    int keyCode = waitKey(0);

    if (keyCode == KeyCodes::ESC)
    {
        destroyWindow(windowName);
        return;
    }
    else
    {
        goto waiting_key;
    }
}

void MainWindow::onHistogramEqBtnClicked()
{
    equalizeHist(imageGrayed, image);
    onImageProcessingSubmit();
}

void MainWindow::onNegativeBtnClicked()
{
    Mat dstImage = imageGrayed.clone();

    for (int i = 0; i < dstImage.rows; i++)
    {
        for (int j = 0; j < dstImage.cols; j++)
        {
            int pixelValue = dstImage.at<uchar>(i, j);
            int maxPixelValue = pow(2, imageDepth2Bits(dstImage.depth())) - 1;
            dstImage.at<uchar>(i, j) = maxPixelValue - pixelValue;
        }
    }

    dstImage.copyTo(image);
    onImageProcessingSubmit();
}

void MainWindow::onLogTransformationBtnClicked()
{
    Mat dstImage = imageGrayed.clone();
    int imageBits = imageDepth2Bits(dstImage.depth());
    int maxPixelValue = pow(2, imageBits) - 1;

    dstImage.convertTo(dstImage, CV_32F);

    for (int i = 0; i < dstImage.rows; i++)
    {
        for (int j = 0; j < dstImage.cols; j++)
        {
            int pixelValue = dstImage.at<float>(i, j);
            dstImage.at<float>(i, j) = log(pixelValue + 1);
        }
    }

    normalize(dstImage, dstImage, 0, maxPixelValue, NORM_MINMAX);
    convertScaleAbs(dstImage, dstImage);
    dstImage.copyTo(image);
    onImageProcessingSubmit();
}

void MainWindow::onBitSlicingBtnClicked()
{
    Mat dstImage = imageGrayed.clone();
    int maxPixelValue = pow(2, imageDepth2Bits(dstImage.depth()));
    int msbValue = maxPixelValue / 2;

    for (int i = 0; i < dstImage.rows; i++)
    {
        for (int j = 0; j < dstImage.cols; j++)
        {
            int pixelValue = dstImage.at<uchar>(i, j);
            dstImage.at<uchar>(i, j) = pixelValue & msbValue ? 255 : 0;
        }
    }

    dstImage.copyTo(image);
    onImageProcessingSubmit();
}

void MainWindow::onZoomBtnClicked()
{
    resetEdit();
    string windowName = "Zoom Image";
    namedWindow(windowName, WINDOW_NORMAL);
    image.copyTo(dstZoomedImage);
    imshow(windowName, dstZoomedImage);

    ZoomData zoomData;

    zoomData.rectangleSize = 100;

    setMouseCallback(windowName, zoomWindowMouseHandler, &zoomData);

    while (!didEditFinish)
    {
        imshow(windowName, dstZoomedImage);
        int keyCode = waitKey(5);

        if (keyCode == KeyCodes::ESC)
        {
            images[currentImageIndex].copyTo(image);
            destroyWindow(windowName);
            return;
        }
    }
    destroyWindow(windowName);
    dstZoomedImage.copyTo(image);
    onImageProcessingSubmit();
}

void MainWindow::onAreaOfInterestBtnClicked()
{
    resetEdit();
    string windowName = "Area of Interest";
    namedWindow(windowName, WINDOW_AUTOSIZE);
    imageGrayed.copyTo(dstAreaOfInterestImage);
    imshow(windowName, dstAreaOfInterestImage);

    ZoomData data;
    data.rectangleSize = 100;
    setMouseCallback(windowName, areaOfInterestMouseHandler, &data);

    while (!didEditFinish)
    {
        imshow(windowName, dstAreaOfInterestImage);
        int keyCode = waitKey(5);

        if (keyCode == KeyCodes::ESC)
        {
            images[currentImageIndex].copyTo(image);
            if (image.channels() != 1)
            {
                cvtColor(image, imageGrayed, COLOR_RGB2GRAY);
            }
            else
            {
                image.copyTo(imageGrayed);
            }
            destroyWindow(windowName);
            return;
        }
    }

    destroyWindow(windowName);
    dstAreaOfInterestImage.copyTo(image);
    onImageProcessingSubmit();
}

void MainWindow::onDeSkewBtnClicked()
{
    // Show the image
    resetEdit();
    string windowName = "Select Points";
    namedWindow(windowName, WINDOW_AUTOSIZE);
    image.copyTo(dstDeSkewedImage);
    imshow(windowName, dstDeSkewedImage);

    // Set the mouse callback function
    setMouseCallback(windowName, deSkewImageMouseHandler, nullptr);

    while (!didEditFinish)
    {
        imshow(windowName, dstDeSkewedImage);
        int keyCode = waitKey(5);

        if (keyCode == KeyCodes::ESC)
        {
            destroyWindow(windowName);
            return;
        }
    }

    destroyWindow(windowName);
    dstDeSkewedImage.copyTo(image);
    onImageProcessingSubmit();
}

void MainWindow::onSmoothingBtnClicked()
{

    ZoomData data;
    data.rectangleSize = 100;

    QMessageBox msgBox;
    msgBox.setWindowTitle("Select The Smoothing Filter");
    msgBox.setText("Select the smoothing filter you want to apply:");
    msgBox.setStandardButtons(QMessageBox::Close);

    QPushButton *traditionalFilter = msgBox.addButton("Level 1", QMessageBox::NoRole);
    QPushButton *pyramidalFilter = msgBox.addButton("Level 2", QMessageBox::NoRole);
    QPushButton *circularFilter = msgBox.addButton("Level 3", QMessageBox::NoRole);
    QPushButton *coneFilter = msgBox.addButton("Level 4", QMessageBox::NoRole);

    msgBox.exec();

    if (msgBox.clickedButton() == traditionalFilter)
    {
        data.kernel = traditionalKernel3x3.clone();
    }
    else if (msgBox.clickedButton() == pyramidalFilter)
    {
        data.kernel = pyramidalKernel5x5.clone();
    }
    else if (msgBox.clickedButton() == circularFilter)
    {
        data.kernel = circularKernel5x5.clone();
    }
    else if (msgBox.clickedButton() == coneFilter)
    {
        data.kernel = coneKernel5x5.clone();
    }
    else
    {
        return;
    }

    resetEdit();
    string windowName = "Smoothing Filters";
    namedWindow(windowName, WINDOW_AUTOSIZE);
    image.copyTo(dstSmoothedImage);
    imshow(windowName, dstSmoothedImage);

    setMouseCallback(windowName, smoothingFiltersMouseHandler, &data);

    while (!didEditFinish)
    {
        imshow(windowName, dstSmoothedImage);
        int keyCode = waitKey(5);

        if (keyCode == KeyCodes::ESC)
        {
            images[currentImageIndex].copyTo(image);
            if (image.channels() != 1)
            {
                cvtColor(image, imageGrayed, COLOR_RGB2GRAY);
            }
            else
            {
                image.copyTo(imageGrayed);
            }
            destroyWindow(windowName);
            return;
        }
    }

    destroyWindow(windowName);
    dstSmoothedImage.copyTo(image);
    onImageProcessingSubmit();
}

void MainWindow::onMedianBtnClicked()
{
    Mat dstImage;

    medianBlur(imageGrayed, dstImage, 3);
    dstImage.copyTo(image);
    onImageProcessingSubmit();
}

void MainWindow::onSobelBtnClicked()
{
    QMessageBox msgBox;
    msgBox.setWindowTitle("Select Filter Orientation");
    msgBox.setText("Select the orientation of the Edge detection filter:");
    msgBox.setStandardButtons(QMessageBox::Close);
    QPushButton *horizontalBtn = msgBox.addButton("Horizontal", QMessageBox::NoRole);
    QPushButton *verticalBtn = msgBox.addButton("Vertical", QMessageBox::NoRole);
    QPushButton *bothBtn = msgBox.addButton("Both", QMessageBox::NoRole);

    msgBox.exec();

    if (msgBox.clickedButton() == horizontalBtn)
    {
        Mat dstImage;
        Sobel(imageGrayed, dstImage, CV_16UC1, 0, 1, 5);
        convertScaleAbs(dstImage, dstImage);
        dstImage.copyTo(image);
        onImageProcessingSubmit();
    }

    if (msgBox.clickedButton() == verticalBtn)
    {
        Mat dstImage;
        Sobel(imageGrayed, dstImage, CV_16UC1, 1, 0, 5);
        convertScaleAbs(dstImage, dstImage);
        dstImage.copyTo(image);
        onImageProcessingSubmit();
    }

    if (msgBox.clickedButton() == bothBtn)
    {
        Mat dstImage;
        Mat dstImageH;
        Mat dstImageV;
        Sobel(imageGrayed, dstImageH, CV_16UC1, 0, 1, 5);
        convertScaleAbs(dstImageH, dstImageH);
        Sobel(imageGrayed, dstImageV, CV_16UC1, 1, 0, 5);
        convertScaleAbs(dstImageV, dstImageV);
        addWeighted(dstImageH, 1, dstImageV, 1, 0, dstImage);
        dstImage.copyTo(image);
        onImageProcessingSubmit();
    }
}

void MainWindow::onFrequencyDomainBtnClicked()
{
    int isLowPassFilter = 0;
    QMessageBox msgBox;
    msgBox.setWindowTitle("Frequency Domain Filters");
    msgBox.setText("Select the filter you want to apply:");
    msgBox.setStandardButtons(QMessageBox::Close);
    QPushButton *lowPassFilter = msgBox.addButton("Smoothen && Blurring", QMessageBox::NoRole);
    QPushButton *highPassFilter = msgBox.addButton("Sharpen && Enhancing", QMessageBox::NoRole);
    msgBox.exec();

    if (msgBox.clickedButton() == lowPassFilter)
    {
        isLowPassFilter = 1;
    }
    else if (msgBox.clickedButton() == highPassFilter)
    {
        isLowPassFilter = 0;
    }
    else
    {
        return;
    }

    d0 = 50;
    resetEdit();
    string windowName = "To be determined";
    namedWindow(windowName, WINDOW_AUTOSIZE);
    imageGrayed.copyTo(dstFrequencyDomainImage);
    imshow(windowName, dstFrequencyDomainImage);
    createTrackbar("d0", windowName, nullptr, 255, [](int value, void *userData)
                   { d0 = value; }, nullptr);
    setTrackbarPos("d0", windowName, 50);
    setTrackbarMax("d0", windowName, 255);
    setTrackbarMin("d0", windowName, 1);

    TrackbarWindowData userData;
    userData.image = imageGrayed.clone();
    userData.dstImage = dstFrequencyDomainImage;
    userData.windowName = windowName;
    userData.mainWindow = this;
    setMouseCallback(windowName, frequencyDomainMouseHandler, &userData);

    while (!didEditFinish)
    {
        Mat srcImage = imageGrayed.clone();
        int m = getOptimalDFTSize(srcImage.rows);
        int n = getOptimalDFTSize(srcImage.cols);
        Mat padded;
        copyMakeBorder(srcImage, padded, 0, m - srcImage.rows, 0, n - srcImage.cols, BorderTypes::BORDER_CONSTANT);
        int maxPixelValue = pow(2, imageDepth2Bits(srcImage.depth())) - 1;
        padded.convertTo(padded, CV_32FC1, 1.0 / (maxPixelValue * 1.0));
        Mat planes[2] = {padded, Mat::zeros(padded.size(), CV_32FC1)};
        Mat complexI;
        merge(planes, 2, complexI);
        dft(complexI, complexI);
        split(complexI, planes);

        // DFT Pre-processing
        int cx = complexI.cols / 2; // n / 2
        int cy = complexI.rows / 2; // m / 2
        // Divide into 4 quarter
        Mat p1(complexI, Rect(0, 0, cx, cy));
        Mat p2(complexI, Rect(cx, 0, cx, cy));
        Mat p3(complexI, Rect(0, cy, cx, cy));
        Mat p4(complexI, Rect(cx, cy, cx, cy));

        Mat temp;
        p1.copyTo(temp);
        p4.copyTo(p1);
        temp.copyTo(p4);

        p2.copyTo(temp);
        p3.copyTo(p2);
        temp.copyTo(p3);
        split(complexI, planes);

        // Create Filter Matrix
        Mat filter(complexI.size(), CV_32FC1);
        for (int i = 0; i < filter.rows; i++)
        {
            for (int j = 0; j < filter.cols; j++)
            {
                double z1 = i - filter.rows / 2;
                double z2 = j - filter.cols / 2;
                if (sqrt(pow(z1, 2) + pow(z2, 2)) < d0)
                {
                    filter.at<float>(i, j) = isLowPassFilter ? 1 : 0;
                }
                else
                    filter.at<float>(i, j) = isLowPassFilter ? 0 : 1;
            }
        }

        // Apply filter
        // Split was done previously
        multiply(planes[0], filter, planes[0]);
        multiply(planes[1], filter, planes[1]);
        merge(planes, 2, complexI);
        idft(complexI, complexI);

        // DFT post-processing & visualization
        split(complexI, planes);
        magnitude(planes[0], planes[1], dstFrequencyDomainImage);
        normalize(dstFrequencyDomainImage, dstFrequencyDomainImage, 0, 1, NORM_MINMAX);
        imshow(windowName, dstFrequencyDomainImage);
        dstFrequencyDomainImage.copyTo(userData.dstImage);

    waiting_key:
        int keyCode = waitKey(0);

        if (keyCode == KeyCodes::ESC)
        {
            destroyWindow(windowName);
            break;
        }

        if (keyCode != KeyCodes::ENTER)
        {
            goto waiting_key;
        }
    }
}

void MainWindow::onSegmentationBtnClicked()
{
    resetEdit();
    int t0 = 80;

    QMessageBox msgBox;
    msgBox.setWindowTitle("Segmentation Method");
    msgBox.setText("Choose Segmentation method:");
    msgBox.setStandardButtons(QMessageBox::Close);
    QPushButton *automaticBtn = msgBox.addButton("Automatic", QMessageBox::NoRole);
    QPushButton *manualBtn = msgBox.addButton("Manual", QMessageBox::NoRole);
    msgBox.exec();

    if (msgBox.clickedButton() == automaticBtn)
    {
        int pixelValues = 0;

        // calculate t0
        for (int i = 0; i < imageGrayed.rows; i++)
        {
            for (int j = 0; j < imageGrayed.cols; j++)
            {
                pixelValues += imageGrayed.at<uchar>(i, j);
            }
        }

        t0 = pixelValues / imageGrayed.total();
        imageGrayed.copyTo(image);

        for (int i = 0; i < imageGrayed.rows; i++)
        {
            for (int j = 0; j < imageGrayed.cols; j++)
            {
                image.at<uchar>(i, j) = imageGrayed.at<uchar>(i, j) > t0 ? 255 : 0;
            }
        }

        onImageProcessingSubmit();
    }

    if (msgBox.clickedButton() == manualBtn)
    {
        QMessageBox instructionMsgBox;
        instructionMsgBox.setWindowTitle("Instructions");
        instructionMsgBox.setTextFormat(Qt::TextFormat::RichText);
        instructionMsgBox.setText(R"(
        <p>Instructions:</p>
        <ul>
            <li>Use the trackbar to adjust the threshold value (t0).</li>
            <li>Press ⏎ to proceed to the next attempt.</li>
            <li>Right Click to submit.</li>
            <li>Press ␛ to exit.</li>
        </ul>

        <b>Note:</b> <span> You only have 10 attempts to the find suitable threshold value (t0).</span>
    )");
        // QCheckBox *checkBox = new QCheckBox("Do not show this message again");
        // instructionMsgBox.setCheckBox(checkBox);
        instructionMsgBox.exec();

        string windowName = "Segmentation Thresholding, Manual T0";
        int attempts = 1;

        TrackbarWindowData userData;
        userData.image = imageGrayed.clone();
        userData.dstImage = imageGrayed.clone();
        userData.windowName = windowName;
        userData.mainWindow = this;

        namedWindow(windowName);
        setMouseCallback(windowName, segmentationThresholdingMouseHandler, &userData);
        createTrackbar("t0", windowName, &t0, 255);
        imshow(windowName, imageGrayed);

        // segmentation Thresholding, manually calculated T0
        while (attempts <= 10 && !didEditFinish)
        {
            for (int i = 0; i < imageGrayed.rows; i++)
            {
                for (int j = 0; j < imageGrayed.cols; j++)
                {
                    userData.dstImage.at<uchar>(i, j) = imageGrayed.at<uchar>(i, j) > t0 ? 255 : 0;
                }
            }
            imshow(windowName, userData.dstImage);

        waiting_key:
            int keyCode = pollKey();

            if (keyCode == KeyCodes::ESC)
            {
                destroyWindow(windowName);
                break;
            }

            if (keyCode != KeyCodes::ENTER && !didEditFinish)
            {
                goto waiting_key;
            }

            attempts++;
        }

        if (attempts > 10)
        {
            QMessageBox msgBox;
            msgBox.setWindowTitle("Attempts limit exceeded");
            msgBox.setText("Attempts limit exceeded, Would you like to submit?");
            QPushButton *submitBtn = msgBox.addButton("Yes", QMessageBox::ActionRole);
            QPushButton *rejectBtn = msgBox.addButton("No", QMessageBox::RejectRole);
            msgBox.exec();

            if (msgBox.clickedButton() == submitBtn)
            {
                destroyWindow(windowName);
                userData.dstImage.copyTo(image);
                onImageProcessingSubmit();
            }
            else
            {
                destroyWindow(windowName);
            }
        }
    }
}

void MainWindow::onLaplacianOfGaussianBtnClicked()
{

    // Edge Detection
    filter2D(imageGrayed, image, CV_8UC1, laplacianOfGaussianKernel);
    onImageProcessingSubmit();
}

void MainWindow::onRedoBtnClicked()
{
    currentImageIndex++;
    images.at(currentImageIndex).copyTo(image);
    onImageProcessingSubmit(false);
}

void MainWindow::onShowDiffBtnPressed()
{
    ui->currentImageContainer->setPixmap(QPixmap::fromImage(matToQImage(images.at(0))));
}

void MainWindow::onShowDiffBtnReleased()
{
    ui->currentImageContainer->setPixmap(QPixmap::fromImage(matToQImage(images.at(currentImageIndex))));
}

void MainWindow::onUndoBtnClicked()
{
    currentImageIndex--;
    images.at(currentImageIndex).copyTo(image);
    onImageProcessingSubmit(false);
}

void MainWindow::onResetBtnClicked()
{
    resetEdit();
    currentImageIndex = 0;
    images.at(0).copyTo(image);
    images.erase(images.begin() + 1, images.end());
    onImageProcessingSubmit(false);
}
