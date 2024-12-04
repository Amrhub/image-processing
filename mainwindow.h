#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <opencv2/opencv.hpp>

QT_BEGIN_NAMESPACE
namespace Ui
{
    class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void setupBtnFunctionalities();
    void enableBtnsOnUpload();
    void initializeDataOnUpload();
    void setValidation();

    void onUploadButtonClicked();
    void onShowImageButtonClicked();
    void onConvertImage2GrayClicked();
    void onTranslateImageBtnClicked();
    void onRotateImageBtnClicked();
    void onSkewImageBtnClicked();
    void onFlipImageBtnClicked();
    void onZoomImageBtnClicked();
    void onHistogramBtnClicked();
    void onImageNegativeBtnClicked();
    void onLogarithmicTransformationBtnClicked();
    void onPowerTransformationBtnClicked();
    void onBitPlaneSlicingBtnClicked();
    void onGrayLevelSlicingBtnClicked();
    void onSmoothingFiltersBtnClicked();

    cv::Mat image;
    cv::Mat imageGrayed;
    cv::Mat traditionalKernel3x3 = (cv::Mat_<float>(3, 3) << 1, 1, 1, 1, 1, 1, 1, 1, 1) / 9;
    cv::Mat pyramidalKernel5x5 = (cv::Mat_<float>(5, 5) << 1, 2, 3, 2, 1, 2, 4, 6, 4, 2, 3, 6, 9, 6, 3, 2, 4, 6, 4, 2, 1, 2, 3, 2, 1) / 81;
    cv::Mat circularKernel5x5 = (cv::Mat_<float>(5, 5) << 0, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 0) / 21;
    cv::Mat coneKernel5x5 = (cv::Mat_<float>(5, 5) << 0, 0, 1, 0, 0, 0, 2, 2, 2, 0, 1, 2, 5, 2, 1, 0, 2, 2, 2, 0, 0, 0, 1, 0, 0) / 25;

private:
    Ui::MainWindow *ui;
};
#endif // MAINWINDOW_H
