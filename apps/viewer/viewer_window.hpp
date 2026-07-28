#pragma once
#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <optional>
#include <imglib/image.hpp>

class ViewerWindow : public QWidget
{
    Q_OBJECT   // Mandatory for signal slots
public:
    ViewerWindow(QWidget* parent = nullptr);

private slots:
    void onOriginal();
    void onGrayscale();
    void onBlur();
    void onSobel();

private:
    // State
    std::optional<imglib::Image> original_;
    std::optional<imglib::Image> current_;

    // Widgets
    QLabel* image_label_;
    QPushButton* original_button_;
    QPushButton* grayscale_button_;
    QPushButton* blur_button_;
    QPushButton* sobel_button_;

    // Helper
    void displayImage(const imglib::Image& img);
};