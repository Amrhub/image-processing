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

    cv::Mat image;
    cv::Mat imageGrayed;

private:
    Ui::MainWindow *ui;
};
#endif // MAINWINDOW_H
