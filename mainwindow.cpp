#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <QPushButton>
#include <QFileDialog>
#include <QMessageBox>
#include <QDoubleValidator>
#include <QIntValidator>
#include <opencv2/opencv.hpp>
#include <iostream>

using namespace cv;
using namespace std;

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

void MainWindow::setupBtnFunctionalities()
{
    connect(ui->uploadBtn, &QPushButton::clicked, this, &MainWindow::onUploadButtonClicked);
    connect(ui->grayImageBtn, &QPushButton::clicked, this, &MainWindow::onConvertImage2GrayClicked);
    connect(ui->showImageBtn, &QPushButton::clicked, this, &MainWindow::onShowImageButtonClicked);
    connect(ui->translateImageBtn, &QPushButton::clicked, this, &MainWindow::onTranslateImageBtnClicked);
    connect(ui->rotateImageBtn, &QPushButton::clicked, this, &MainWindow::onRotateImageBtnClicked);
    connect(ui->skewImageBtn, &QPushButton::clicked, this, &MainWindow::onSkewImageBtnClicked);
    connect(ui->flipImageBtn, &QPushButton::clicked, this, &MainWindow::onFlipImageBtnClicked);
    connect(ui->zoomImageBtn, &QPushButton::clicked, this, &MainWindow::onZoomImageBtnClicked);
    connect(ui->histogramBtn, &QPushButton::clicked, this, &MainWindow::onHistogramBtnClicked);
    connect(ui->imageNegativeBtn, &QPushButton::clicked, this, &MainWindow::onImageNegativeBtnClicked);
    connect(ui->logarithmicBtn, &QPushButton::clicked, this, &MainWindow::onLogarithmicTransformationBtnClicked);
    connect(ui->powerTransformationBtn, &QPushButton::clicked, this, &MainWindow::onPowerTransformationBtnClicked);
    connect(ui->bitPlaneSlicingBtn, &QPushButton::clicked, this, &MainWindow::onBitPlaneSlicingBtnClicked);
    connect(ui->grayLevelSlicingBtn, &QPushButton::clicked, this, &MainWindow::onGrayLevelSlicingBtnClicked);
}

void MainWindow::enableBtnsOnUpload()
{
    ui->grayImageBtn->setEnabled(true);
    ui->showImageBtn->setEnabled(true);
    ui->translateImageBtn->setEnabled(true);
    ui->rotateImageBtn->setEnabled(true);
    ui->skewImageBtn->setEnabled(true);
    ui->flipImageBtn->setEnabled(true);
    ui->zoomImageBtn->setEnabled(true);
    ui->histogramBtn->setEnabled(true);
    ui->imageNegativeBtn->setEnabled(true);
    ui->logarithmicBtn->setEnabled(true);
    ui->powerTransformationBtn->setEnabled(true);
    ui->bitPlaneSlicingBtn->setEnabled(true);
    ui->grayLevelSlicingBtn->setEnabled(true);
}

void MainWindow::setValidation()
{
    QValidator *doubleInputsValidator = new QDoubleValidator(-99999, 99999, 3);
    QValidator *inRangeValidator = new QIntValidator(0, pow(2, imageDepth2Bits(image.depth())) - 1);

    // Translate validations
    ui->txInput->setValidator(doubleInputsValidator);
    ui->tyInput->setValidator(doubleInputsValidator);

    // Rotate validations
    ui->rotateX->setValidator(doubleInputsValidator);
    ui->rotateY->setValidator(doubleInputsValidator);
    ui->scale->setValidator(doubleInputsValidator);
    ui->rotateAngle->setValidator(doubleInputsValidator);

    // Zoom validations
    ui->zoomXInput->setValidator(doubleInputsValidator);
    ui->zoomYInput->setValidator(doubleInputsValidator);
    ui->zoomTInput->setValidator(doubleInputsValidator);
    ui->zoomSInput->setValidator(doubleInputsValidator);

    // Gray Level Slicing
    ui->grayLevelFromInput->setValidator(inRangeValidator);
    ui->grayLevelToInput->setValidator(new QIntValidator(ui->grayLevelFromInput->text().toInt(), pow(2, imageDepth2Bits(image.depth())) - 1));
}

void MainWindow::initializeDataOnUpload()
{
    int rows = image.rows;
    int cols = image.cols;

    // set default rotate values
    ui->rotateX->setText(QString::fromStdString(to_string(cols / 2)));
    ui->rotateY->setText(QString::fromStdString(to_string(rows / 2)));
    ui->rotateAngle->setText(QString::fromStdString(to_string(45)));
    ui->scale->setText(QString::fromStdString(to_string(1)));

    // set default skewing values
    ui->skewXInput->setText(QString::fromStdString(to_string(0)));
    ui->skewYInput->setText(QString::fromStdString(to_string(0)));
    ui->skewXNewInput->setText(QString::fromStdString(to_string(0)));
    ui->skewYNewInput->setText(QString::fromStdString(to_string(0)));
    ui->skewXInput_2->setText(QString::fromStdString(to_string(cols - 1)));
    ui->skewYInput_2->setText(QString::fromStdString(to_string(0)));
    ui->skewXNewInput_2->setText(QString::fromStdString(to_string(cols - 1)));
    ui->skewYNewInput_2->setText(QString::fromStdString(to_string(0)));
    ui->skewYNewInput_3->setText(QString::fromStdString(to_string(rows - 1)));
    ui->skewYInput_3->setText(QString::fromStdString(to_string(rows - 1)));

    // set default zoom values
    ui->zoomXInput->setText(QString::fromStdString(to_string(0)));
    ui->zoomYInput->setText(QString::fromStdString(to_string(0)));
    ui->zoomXMultiplier->setText(QString::fromStdString(to_string(2)));
    ui->zoomYMultiplier->setText(QString::fromStdString(to_string(2)));
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    MainWindow::setupBtnFunctionalities();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::onUploadButtonClicked()
{
    QString fileName = QFileDialog::getOpenFileName(this, "Open Image File", "", "Images (*.png *.xpm *.jpg *.jpeg *.bmp)");

    if (!fileName.isEmpty())
    {
        image = imread(fileName.toStdString());
        if (!image.empty())
        {
            cvtColor(image, imageGrayed, COLOR_RGB2GRAY);

            auto [total, rows, cols, depth] = imageDetails(image);
            auto [min, max, avg] = imageMinMaxAvg(image);
            ui->imageDescription->setText(QString::fromStdString(
                "Path: " + fileName.toStdString() + "\n" +
                "Dimensions (RowsXCols): " + to_string(rows) + "x" + to_string(cols) + "\n" +
                "Total pixels: " + to_string(total) + "\n" +
                "Depth code: " + to_string(depth) + "\n" +
                "Minimum pixel value: " + to_string(min) + "\n" +
                "Maximum pixel value: " + to_string(max) + "\n" +
                "Dynamic range: (" + to_string(min) + ", " + to_string(max) + ")" + "\n" +
                "Average pixel value: " + to_string(avg) + "\n" +
                "Total number of bits required to store the image: " + to_string(imgSize(image))));

            MainWindow::enableBtnsOnUpload();
            MainWindow::setValidation();
            MainWindow::initializeDataOnUpload();
        }
        else
        {
            QMessageBox::warning(this, "Error", "Failed to load the image.");
        }
    }
}

void MainWindow::onShowImageButtonClicked()
{
    showImage("Original Image", image);
}

void MainWindow::onConvertImage2GrayClicked()
{
    Mat dstImage;
    cvtColor(image, dstImage, COLOR_RGB2GRAY);
    showImage("Grayed Image", dstImage);
}

void MainWindow::onTranslateImageBtnClicked()
{
    // get tx, ty values from inputs.
    float txValue = ui->txInput->text().toFloat();
    float tyValue = ui->tyInput->text().toFloat();
    Mat translationMatrix = (Mat_<float>(2, 3) << 1, 0, txValue, 0, 1, tyValue), dstTranslationImg;
    warpAffine(image, dstTranslationImg, translationMatrix, image.size());
    showImage("Translation", dstTranslationImg);
}

void MainWindow::onRotateImageBtnClicked()
{
    QString rotateXText = ui->rotateX->text();
    QString rotateYText = ui->rotateY->text();
    QString rotateAngleText = ui->rotateAngle->text();
    QString scaleText = ui->scale->text();
    float rotateX = rotateXText.toFloat();
    float rotateY = rotateYText.toFloat();
    float rotateAngle = ui->rotateAngle->text().toDouble();
    float scale = ui->scale->text().toDouble();

    Mat dstRotationImage;
    Mat rotationMatrix = getRotationMatrix2D(Point2f(rotateX, rotateY), rotateAngle, scale);
    warpAffine(image, dstRotationImage, rotationMatrix, image.size());
    showImage("Rotate", dstRotationImage);
}

void MainWindow::onSkewImageBtnClicked()
{
    float x1 = ui->skewXInput->text().toFloat();
    float y1 = ui->skewYInput->text().toFloat();
    float x2 = ui->skewXInput_2->text().toFloat();
    float y2 = ui->skewYInput_2->text().toFloat();
    float x3 = ui->skewXInput_3->text().toFloat();
    float y3 = ui->skewYInput_3->text().toFloat();

    float x1New = ui->skewXNewInput->text().toFloat();
    float y1New = ui->skewYNewInput->text().toFloat();
    float x2New = ui->skewXNewInput_2->text().toFloat();
    float y2New = ui->skewYNewInput_2->text().toFloat();
    float x3New = ui->skewXNewInput_3->text().toFloat();
    float y3New = ui->skewYNewInput_3->text().toFloat();

    Point2f srcPoints[3];
    srcPoints[0] = Point2f(x1, y1);
    srcPoints[1] = Point2f(x2, y2);
    srcPoints[2] = Point2f(x3, y3);

    Point2f dstPoints[3];
    dstPoints[0] = Point2f(x1New, y1New);
    dstPoints[1] = Point2f(x2New, y2New);
    dstPoints[2] = Point2f(x3New, y3New);

    Mat skewingMatrix = getAffineTransform(srcPoints, dstPoints);
    Mat dstSkewingImage;
    warpAffine(image, dstSkewingImage, skewingMatrix, image.size());
    showImage("SKew", dstSkewingImage);
}

void MainWindow::onFlipImageBtnClicked()
{
    Mat flippedImage;
    int flipCode = ui->flipCodeCombobox->currentText().toInt();
    flip(image, flippedImage, flipCode);
    string windowName;

    switch (flipCode)
    {
    case 0:
        windowName = "Flipping around X-axis";
        break;

    case 1:
        windowName = "Flipping around Y-axis";
        break;

    case -1:
        windowName = "Flipping around X-axis and Y-axis";
        break;
    }

    showImage(windowName, flippedImage);
}

void MainWindow::onZoomImageBtnClicked()
{
    double x = ui->zoomXInput->text().toDouble();
    double y = ui->zoomYInput->text().toDouble();
    double t = ui->zoomTInput->text().toDouble();
    double s = ui->zoomSInput->text().toDouble();
    float xMultiplier = ui->zoomXMultiplier->text().toFloat();
    float yMultiplier = ui->zoomYMultiplier->text().toFloat();
    int fillingTechnique = ui->fillingTechniqueCombobox->currentText().toInt();

    Mat dstImage;
    Mat croppedImage = image(Rect(x, y, t, s));
    cv::resize(croppedImage, dstImage, Size(), xMultiplier, yMultiplier, fillingTechnique);

    string windowName = "Zoom";
    windowName += fillingTechnique == 0 ? " (Nearest Neighbor)" : " (Bi-linear)";
    showImage(windowName, dstImage);
}

void MainWindow::onHistogramBtnClicked()
{
    // TODO add diagram and show equalized gray level.
    Mat dstImage;
    equalizeHist(imageGrayed, dstImage);
    showImage("Histogram equalized", dstImage);
}

void MainWindow::onImageNegativeBtnClicked()
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

    showImage("Image Negative Transformation", dstImage);
}

void MainWindow::onLogarithmicTransformationBtnClicked()
{
    Mat dstImage = imageGrayed.clone();
    dstImage.convertTo(dstImage, CV_32F);

    for (int i = 0; i < dstImage.rows; i++)
    {
        for (int j = 0; j < dstImage.cols; j++)
        {
            int pixelValue = dstImage.at<float>(i, j);
            dstImage.at<float>(i, j) = log(pixelValue + 1);
        }
    }

    normalize(dstImage, dstImage, 0, 255, NORM_MINMAX);
    convertScaleAbs(dstImage, dstImage);

    showImage("Logarithmic Transformation", dstImage);
}

void MainWindow::onPowerTransformationBtnClicked()
{
    Mat dstImage = imageGrayed.clone();
    float gammaValue = ui->gammaInput->text().toFloat();
    int totalImageBits = imageDepth2Bits(dstImage.depth());
    int maxPixelValue = pow(2, totalImageBits) - 1;

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
    showImage("Power Law Transformation", dstImage);
}

void MainWindow::onBitPlaneSlicingBtnClicked()
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

    showImage("Bit plane slicing", dstImage);
}

void MainWindow::onGrayLevelSlicingBtnClicked()
{
    Mat dstImage = imageGrayed.clone();
    int fromValue = ui->grayLevelFromInput->text().toInt();
    int toValue = ui->grayLevelFromInput->text().toInt();
    QString constantInput = ui->grayLevelOtherwiseInput->text();
    // TODO display pixelValues in image as value => frequency

    for (int i = 0; i < dstImage.rows; i++)
    {
        for (int j = 0; j < dstImage.cols; j++)
        {
            int pixelValue = dstImage.at<uchar>(i, j);
            if (pixelValue >= fromValue && pixelValue <= toValue)
                dstImage.at<uchar>(i, j) = 255;
            else
                dstImage.at<uchar>(i, j) = (!constantInput.isEmpty() ? constantInput.toInt() : pixelValue);
        }
    }

    showImage("Gray Level Slicing", dstImage);
}

// TBC