#include <imglib/ppm.hpp>
#include <imglib/filters.hpp>
#include "viewer_window.hpp"

ViewerWindow::ViewerWindow(QWidget* parent) : QWidget(parent)
{
    image_label_ = new QLabel(this);
    original_button_ = new QPushButton("Original", this);
    grayscale_button_ = new QPushButton("Grayscale", this);
    blur_button_ = new QPushButton("Blur", this);
    sobel_button_ = new QPushButton("Sobel", this);
    load_button_ = new QPushButton("Load Image", this);
    save_button_ = new QPushButton("Save Image", this);

    // Layout, image is on top and buttons are below it
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->addWidget(image_label_);
    layout->addWidget(original_button_);
    layout->addWidget(grayscale_button_);
    layout->addWidget(blur_button_);
    layout->addWidget(sobel_button_);
    layout->addWidget(load_button_);
    layout->addWidget(save_button_);

    // Signal slot connections
    connect(original_button_, &QPushButton::clicked, this, &ViewerWindow::onOriginal);
    connect(grayscale_button_, &QPushButton::clicked, this, &ViewerWindow::onGrayscale);
    connect(blur_button_, &QPushButton::clicked, this, &ViewerWindow::onBlur);
    connect(sobel_button_, &QPushButton::clicked, this, &ViewerWindow::onSobel);
    connect(load_button_, &QPushButton::clicked, this, &ViewerWindow::onLoad);
    connect(save_button_, &QPushButton::clicked, this, &ViewerWindow::onSave);

    original_ = imglib::load_ppm("test.ppm");
    current_ = original_;
    if (current_) displayImage(*current_);
}

void ViewerWindow::displayImage(const imglib::Image& img)
{
    // Initialize Qt image
    QImage qimg(img.data(), img.width(), img.height(),
                img.width() * img.channels(),
                (img.channels() == 3) ? QImage::Format_RGB888 : QImage::Format_Grayscale8);
    
    // Set the pixel map
    image_label_->setPixmap(QPixmap::fromImage(qimg));
}

void ViewerWindow::onOriginal()
{
    // Is original_ empty?
    if (!original_)
    {
        return;
    }

    // Set current to original and display the image
    current_ = original_;
    displayImage(*current_);
}

void ViewerWindow::onGrayscale()
{
    // Is original_ empty?
    if (!original_)
    {
        return;
    }

    // Apply grayscale filter to image
    current_ = imglib::to_grayscale(*original_);
    displayImage(*current_);
}

void ViewerWindow::onBlur()
{
    // Is original_ empty?
    if (!original_)
    {
        return;
    }

    // Apply box blur filter to image (kernel size is fixed to 5)
    current_ = imglib::box_blur(*original_, 5);
    displayImage(*current_);
}

void ViewerWindow::onSobel()
{
    // Is original_ empty?
    if (!original_)
    {
        return;
    }

    // Apply sobel filter to image after grayscaling it
    current_ = imglib::to_grayscale(*original_);
    current_ = imglib::sobel(*current_);
    displayImage(*current_);
}

void ViewerWindow::onLoad()
{
    QString path = QFileDialog::getOpenFileName(this, "Open Image", "", "PPM Images (*.ppm *.pgm)");
    if (path == "")
    {
        return;
    }

    auto loaded = imglib::load_ppm(path.toStdString());
    if (!loaded)
    { 
        return;
    }

    original_ = loaded;
    current_ = original_;
    displayImage(*current_);
}

void ViewerWindow::onSave()
{
    if (!current_)
    {
        return;
    }

    QString path = QFileDialog::getSaveFileName(this, "Save Image", "", "PPM Images (*.ppm *.pgm)");
    if (path == "")
    {
        return;
    }

    bool saved = imglib::save_ppm(path.toStdString(), *current_);
    if (!saved)
    {
        QMessageBox::warning(this, "Save Failed", "Could not save the image.");
    }
}