#include <QApplication>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTextEdit>
#include <QPushButton>
#include <QKeyEvent>
#include <QShortcut>
#include <QStyle>
#include <QLabel>
#include <QPixmap>
#include <QScreen>
#include <QTimer>
#include <QDateTime>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsPixmapItem>
#include <QWheelEvent>
#include <QThread>
#include <fstream>
#include <string>
#include <iostream>
#include <QBuffer>
#include <QByteArray>
#include <QFile>
#include <QMessageBox>
#include "gemini_engine.h"

// Helper function to read .env file from directory above
std::string get_api_key_from_env() {
    std::ifstream file(".env");
    if (!file.is_open()) {
    return "";
}

    std::string line;
    while (std::getline(file, line)) {
        if (line.substr(0, 14) == "GEMINI_API_KEY") {
            // Dòng là "GEMINI_API_KEY=AIza..."
            // Nếu có dấu =, lấy từ ký tự sau dấu =
            size_t eqPos = line.find('=');
            if (eqPos != std::string::npos) {
                return line.substr(eqPos + 1);
            }
            // Nếu không có =, lấy từ ký tự sau khoảng trắng
            size_t spacePos = line.find(' ');
            if (spacePos != std::string::npos) {
                return line.substr(spacePos + 1);
        }
    }
}
    return "";
}

// Helper to encode file to base64
std::string encode_file_to_base64(const std::string& filename) {
    QFile file(QString::fromStdString(filename));
    if (!file.open(QIODevice::ReadOnly)) return "";
    return file.readAll().toBase64().toStdString();
}

class PreviewView : public QGraphicsView {
public:
    PreviewView(QGraphicsScene* scene, QWidget* parent = nullptr) : QGraphicsView(scene, parent) {
        setDragMode(QGraphicsView::ScrollHandDrag);
        setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
        setRenderHint(QPainter::Antialiasing);
    }
protected:
    void wheelEvent(QWheelEvent* event) override {
        double scaleFactor = 1.15;
        if (event->angleDelta().y() < 0) scaleFactor = 1.0 / scaleFactor;
        scale(scaleFactor, scaleFactor);
    }
};

class PromptEdit : public QTextEdit {
    Q_OBJECT
public:
    PromptEdit(QWidget* parent = nullptr) : QTextEdit(parent) {
        setPlaceholderText("Enter your prompt here...");
    }
protected:
    void keyPressEvent(QKeyEvent* e) override {
        if (e->key() == Qt::Key_Enter || e->key() == Qt::Key_Return) {
            if (e->modifiers() & Qt::ShiftModifier) {
                QTextEdit::keyPressEvent(e); // Cho phép xuống dòng
            } else {
                emit submitRequested(toPlainText());
                clear();
            }
        } else {
            QTextEdit::keyPressEvent(e);
        }
    }

signals:
    void submitRequested(const QString& text);
};

class MainWindow : public QWidget {
    Q_OBJECT
public:
    MainWindow() : lastCtrlPressTime(0), gemini(get_api_key_from_env()) {
        setWindowTitle("Screen Copilot");

        std::string apiKey = get_api_key_from_env();
        if (apiKey.empty()) {
            QTimer::singleShot(0, []() {
                QMessageBox::warning(nullptr, "API Key Missing",
                                   "GEMINI_API_KEY not found in .env file.\n"
                                   "The application may not function correctly.");
            });
        }

        // Bắt đầu với trạng thái thu gọn
        resize(500, 40); // Mỏng hơn nữa
        setFixedSize(500, 40); // Mỏng hơn nữa

        auto* mainLayout = new QVBoxLayout(this);
        mainLayout->setContentsMargins(2, 2, 2, 2); // Mỏng hơn
        mainLayout->setSpacing(2); // Mỏng hơn

        // Layout ngang chính: [Preview] [Prompt] [Capture Button]
        auto* horizontalLayout = new QHBoxLayout();
        horizontalLayout->setContentsMargins(0, 0, 0, 0); // Mỏng hơn

        // Hiển thị preview ảnh
        imagePreview = new QLabel("No image", this);
        imagePreview->setFixedSize(50, 35); // Mỏng hơn nữa
        imagePreview->setAlignment(Qt::AlignCenter);
        imagePreview->setStyleSheet("border: 1px solid gray; font-size: 8px;");
        imagePreview->setToolTip("double ctrl để preview ảnh chụp màn hình.\nnhấn vào đây để gỡ hình khỏi prompt");
        // Cho phép nhận sự kiện click
        imagePreview->installEventFilter(this);
        horizontalLayout->addWidget(imagePreview);

        // Ô Prompt
        promptEdit = new PromptEdit(this);
        promptEdit->setFixedHeight(35); // Mỏng hơn nữa
        promptEdit->setStyleSheet("padding: 2px;");
        horizontalLayout->addWidget(promptEdit);

        // Nút Capture
        auto* captureBtn = new QPushButton(this);
        captureBtn->setIcon(style()->standardIcon(QStyle::SP_ComputerIcon));
        captureBtn->setFixedSize(35, 35); // Mỏng hơn nữa
        captureBtn->setToolTip("chụp màn hình\n(ctrl+s)");
        connect(captureBtn, &QPushButton::clicked, this, &MainWindow::handleCapture);
        horizontalLayout->addWidget(captureBtn);

        mainLayout->addLayout(horizontalLayout);

        // Ô hiển thị response (nằm dưới)
        responseEdit = new QTextEdit(this);
        responseEdit->setReadOnly(true);
        responseEdit->setPlaceholderText("AI response...");
        responseEdit->hide(); // Mặc định ẩn
        mainLayout->addWidget(responseEdit);

        connect(promptEdit, &PromptEdit::submitRequested, this, &MainWindow::handleSubmit);

        // Shortcut Ctrl+S
        auto* shortcut = new QShortcut(QKeySequence("Ctrl+S"), this);
        connect(shortcut, &QShortcut::activated, this, &MainWindow::handleCapture);

        // Shortcut Esc để toggle response và expand cửa sổ
        auto* escShortcut = new QShortcut(QKeySequence("Esc"), this);
        connect(escShortcut, &QShortcut::activated, [this]() {
            bool isVisible = responseEdit->isVisible();
            if (isVisible) {
                // Đang hiện, giờ ẩn đi và thu gọn
                responseEdit->hide();
                setFixedSize(500, 40);
            } else {
                // Đang ẩn, giờ hiện ra và mở rộng
                setMinimumSize(500, 400);
                setMaximumSize(16777215, 16777215);
        resize(500, 400);
                responseEdit->show();
            }
        });

        // Cài đặt filter để tự động focus khi bắt đầu nhập liệu
        qApp->installEventFilter(this);

        // Luôn nằm trên cùng (Stay on top)
        setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint);
    }

protected:
    bool eventFilter(QObject* obj, QEvent* event) override {
        // Xử lý click vào ảnh preview
        if (obj == imagePreview && event->type() == QEvent::MouseButtonPress) {
            lastCapturedPixmap = QPixmap();
            imagePreview->setPixmap(QPixmap());
            imagePreview->setText("No image");
            return true;
        }

        if (event->type() == QEvent::KeyPress) {
            QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);

            // Bắt double Ctrl
            if (keyEvent->key() == Qt::Key_Control) {
                // Chỉ xử lý khi nhấn xuống lần đầu (tránh auto-repeat của OS)
                if (!keyEvent->isAutoRepeat()) {
                    qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
                    if (lastCtrlPressTime > 0 && (currentTime - lastCtrlPressTime < 400)) {
                        showFullPreview();
                        // Reset thời gian để không bị double trigger liên tiếp
                        lastCtrlPressTime = 0;
                    } else {
                        lastCtrlPressTime = currentTime;
                        // Hẹn giờ để reset nếu không nhấn lần 2
                        QTimer::singleShot(400, this, [this, currentTime]() {
                            if (lastCtrlPressTime == currentTime) {
                 lastCtrlPressTime = 0;
            }
                        });
                    }
                }
                return true; // Chặn sự kiện Ctrl để không ảnh hưởng các shortcut khác nếu cần
            } else if (keyEvent->key() != Qt::Key_Shift && keyEvent->key() != Qt::Key_Alt && keyEvent->key() != Qt::Key_Meta) {
                 // Reset nếu nhấn phím khác Ctrl
                 lastCtrlPressTime = 0;
            }
            // Nếu nhấn phím in được và không phải phím Ctrl, đưa focus vào ô nhập
            if (!keyEvent->text().isEmpty() && keyEvent->text()[0].isPrint() &&
                !(keyEvent->modifiers() & Qt::ControlModifier)) {
                if (!promptEdit->hasFocus()) {
                    promptEdit->setFocus();
                }
            }
        }
        return QWidget::eventFilter(obj, event);
    }
private:
    void showFullPreview() {
        if (lastCapturedPixmap.isNull()) return;

        if (previewWindow) {
            previewWindow->close();
            previewWindow->deleteLater();
        }

        previewWindow = new QWidget(nullptr, Qt::Window);
        previewWindow->setWindowTitle("Full Preview - Zoom/Pan (Esc to close)");
        previewWindow->resize(800, 600);

        auto* layout = new QVBoxLayout(previewWindow);
        layout->setContentsMargins(0, 0, 0, 0);

        QGraphicsScene* scene = new QGraphicsScene();
        scene->addPixmap(lastCapturedPixmap);

        PreviewView* view = new PreviewView(scene);
        layout->addWidget(view);

        auto* escShortcut = new QShortcut(QKeySequence("Esc"), previewWindow);
        connect(escShortcut, &QShortcut::activated, previewWindow, &QWidget::close);

        previewWindow->show();
    }

private slots:
    void handleCapture() {
    QScreen *screen = QGuiApplication::primaryScreen();
        if (screen) {
            lastCapturedPixmap = screen->grabWindow(0);
            imagePreview->setPixmap(lastCapturedPixmap.scaled(imagePreview->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
        }
    }

    void handleSubmit(const QString& text) {
        responseEdit->show(); // Hiện khi có submit

        // Mở rộng cửa sổ khi bắt đầu gửi request
        setMinimumSize(500, 400);
        setMaximumSize(16777215, 16777215);
        resize(500, 400);

        responseEdit->append("<b>You:</b> " + text);
        responseEdit->append("<i>Sending...</i>");

        // Chuyển việc gọi API sang QThread để không block UI
        auto* workerThread = QThread::create([this, text]() {
            try {
            std::vector<ContentPart> parts;
            parts.push_back({ContentPart::Type::TEXT, text.toStdString(), ""});

            if (!lastCapturedPixmap.isNull()) {
                QString tempPath = "temp_screenshot.png";
                lastCapturedPixmap.save(tempPath);
                std::string base64_data = encode_file_to_base64(tempPath.toStdString());
                parts.push_back({ContentPart::Type::IMAGE, base64_data, "image/png"});
            }

            PromptConfig config;
            config.model_name="gemini-3.1-flash-lite";
            std::string response = gemini.generate(parts, config);

                QMetaObject::invokeMethod(this, [this, response]() {
                    // Xóa dòng "Sending..."
                    QTextCursor cursor = responseEdit->textCursor();
                    cursor.movePosition(QTextCursor::End);
                    cursor.select(QTextCursor::LineUnderCursor);
                    cursor.removeSelectedText();
                    cursor.deletePreviousChar(); // Xóa ký tự xuống dòng thừa

            responseEdit->append("<b>AI:</b> " + QString::fromStdString(response));
                });
        } catch (const std::exception& e) {
            std::cout<<e.what();
                QMetaObject::invokeMethod(this, [this, e_string=e.what()]() {
            responseEdit->append("<b>Error:</b> " + QString::fromUtf8(e_string));
                });
        }
        });

        connect(workerThread, &QThread::finished, workerThread, &QThread::deleteLater);
        workerThread->start();
    }

private:
    PromptEdit* promptEdit;
    QLabel* imagePreview;
    QTextEdit* responseEdit;
    QPixmap lastCapturedPixmap;
    qint64 lastCtrlPressTime;
    QWidget* previewWindow = nullptr;
    GeminiEngine gemini;
};

#include "screen-copilot.moc"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    MainWindow window;

    // Di chuyển cửa sổ xuống góc phải TRÊN màn hình
    QScreen *screen = QGuiApplication::primaryScreen();
    QRect screenGeometry = screen->availableGeometry();
    int x = screenGeometry.width() - window.width() - 20;
    int y = 20; // Khoảng cách từ đỉnh màn hình
    window.move(x, y);

    window.show();
    return app.exec();
}

