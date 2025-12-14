#include "mainwindow.h"
#include <QFile>
#include <QTextStream>
#include "gamewindow.h"
#include <QPainter>
#include <cmath>
#include <QDebug>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), backgroundOffset(0)
{
    setObjectName("menuWindow");

    // Включаем отслеживание мыши
    setMouseTracking(true);
    if(centralWidget()) centralWidget()->setMouseTracking(true);

    QFile file(":/styles/styles.qss");
    if (file.open(QFile::ReadOnly | QFile::Text)) {
        QTextStream stream(&file);
        QString style = stream.readAll();
        this->setStyleSheet(style);
        file.close();
    }

    setupUI();
    setWindowTitle("Морской Бой - 8-BIT EDITION");
    resize(600, 500);
}

// Сеттер для свойства анимации фона
void MainWindow::setBackgroundOffset(float offset) {
    backgroundOffset = offset;
    update(); // Перерисовываем окно при изменении значения
}

void MainWindow::setupUI()
{
    QWidget *central = new QWidget(this);
    central->setMouseTracking(true);
    setCentralWidget(central);

    // Важно: мы не устанавливаем Layout на centralWidget,
    // чтобы свободно двигать дочерние контейнеры.

    // Инициализация контейнеров
    menuContainer = new QWidget(central);
    menuContainer->setMouseTracking(true); // Включаем трекинг для параллакса

    settingsContainer = new QWidget(central);
    settingsContainer->setMouseTracking(true); // Включаем трекинг для параллакса

    setupMenuContainer();
    setupSettingsContainer();

    // Позиционирование (будет обновлено в resizeEvent)
    menuContainer->move(0, 0);
    settingsContainer->move(-width(), 0); // Изначально скрыт слева
}

void MainWindow::setupMenuContainer() {
    QVBoxLayout *layout = new QVBoxLayout(menuContainer);
    layout->setSpacing(15);
    layout->setContentsMargins(40, 40, 40, 40);

    // Заголовок
    QLabel *titleLabel = new QLabel("МОРСКОЙ БОЙ", menuContainer);
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setStyleSheet(
        "font-size: 36px; font-weight: bold; color: #2c2c2c; "
        "background-color: transparent; border: 4px dashed #555; padding: 15px;"
        );

    QPushButton *btnSingle = new QPushButton("ОДИНОЧНАЯ ИГРА", menuContainer);
    QPushButton *btnMulti = new QPushButton("МУЛЬТИПЛЕЕР", menuContainer);
    QPushButton *btnSettings = new QPushButton("НАСТРОЙКИ", menuContainer);
    QPushButton *btnExit = new QPushButton("ВЫХОД", menuContainer);

    layout->addStretch(1);
    layout->addWidget(titleLabel);
    layout->addSpacing(30);
    layout->addWidget(btnSingle, 0, Qt::AlignCenter);
    layout->addWidget(btnMulti, 0, Qt::AlignCenter);
    layout->addWidget(btnSettings, 0, Qt::AlignCenter);
    layout->addWidget(btnExit, 0, Qt::AlignCenter);
    layout->addStretch(1);

    connect(btnSingle, &QPushButton::clicked, this, &MainWindow::onSinglePlayerClicked);
    connect(btnMulti, &QPushButton::clicked, this, &MainWindow::onMultiplayerClicked);
    connect(btnSettings, &QPushButton::clicked, this, &MainWindow::onSettingsClicked);
    connect(btnExit, &QPushButton::clicked, this, &MainWindow::onExitClicked);
}

void MainWindow::setupSettingsContainer() {
    // ИСПРАВЛЕНИЕ: Убрали фон у контейнера настроек, чтобы был виден параллакс
    // settingsContainer->setStyleSheet("background-color: rgba(248, 240, 227, 200);");

    QVBoxLayout *mainLayout = new QVBoxLayout(settingsContainer);
    mainLayout->setContentsMargins(20, 20, 20, 20);

    // Верхняя панель с кнопкой НАЗАД
    QHBoxLayout *topLayout = new QHBoxLayout();
    QPushButton *btnBack = new QPushButton("НАЗАД", settingsContainer);
    btnBack->setFixedSize(100, 40);
    btnBack->setStyleSheet("background-color: #c0392b; color: white;"); // Красная кнопка
    connect(btnBack, &QPushButton::clicked, this, &MainWindow::onBackFromSettingsClicked);

    QLabel *settingsTitle = new QLabel("НАСТРОЙКИ", settingsContainer);
    settingsTitle->setStyleSheet("font-size: 24px; font-weight: bold; color: #333;");
    settingsTitle->setAlignment(Qt::AlignCenter);

    topLayout->addWidget(btnBack);
    topLayout->addStretch();
    topLayout->addWidget(settingsTitle);
    topLayout->addStretch();
    // Пустой виджет для баланса
    QWidget *dummy = new QWidget(); dummy->setFixedSize(100, 40); topLayout->addWidget(dummy);

    // Центральная часть: Аватар
    QVBoxLayout *centerLayout = new QVBoxLayout();
    centerLayout->setAlignment(Qt::AlignCenter);

    QLabel *avatarLabel = new QLabel("ВАШ АВАТАР:", settingsContainer);
    avatarLabel->setStyleSheet("font-weight: bold; font-size: 18px;");
    avatarLabel->setAlignment(Qt::AlignCenter);

    // Превью текущего аватара
    currentAvatarPreview = new QLabel(settingsContainer);
    currentAvatarPreview->setFixedSize(120, 120);
    currentAvatarPreview->setStyleSheet("border: 4px solid #333; background-color: rgba(255,255,255,150);");
    currentAvatarPreview->setAlignment(Qt::AlignCenter);
    currentAvatarPreview->setText("NONE");

    btnChangeAvatar = new QPushButton("СМЕНИТЬ АВАТАР", settingsContainer);
    btnChangeAvatar->setFixedWidth(200);
    connect(btnChangeAvatar, &QPushButton::clicked, this, &MainWindow::onChangeAvatarClicked);

    centerLayout->addWidget(avatarLabel);
    centerLayout->addSpacing(15);
    centerLayout->addWidget(currentAvatarPreview);
    centerLayout->addSpacing(25);
    centerLayout->addWidget(btnChangeAvatar);

    mainLayout->addLayout(topLayout);
    mainLayout->addStretch();
    mainLayout->addLayout(centerLayout);
    mainLayout->addStretch();

    // --- ОВЕРЛЕЙ ВЫБОРА АВАТАРА ---
    // Создаем его как ребенка settingsContainer, но НЕ добавляем в layout.
    // Мы будем управлять его размером вручную в resizeEvent.
    avatarSelectionWidget = new QWidget(settingsContainer);
    avatarSelectionWidget->setMouseTracking(true); // Включаем трекинг для параллакса внутри оверлея
    avatarSelectionWidget->hide();

    // Применяем фон только к этому оверлею
    avatarSelectionWidget->setStyleSheet("background-color: rgba(248, 240, 227, 230);");

    // Макет внутри оверлея
    QVBoxLayout *overlayLayout = new QVBoxLayout(avatarSelectionWidget);
    overlayLayout->setAlignment(Qt::AlignCenter);

    QLabel *overlayTitle = new QLabel("ВЫБЕРИТЕ ПЕРСОНАЖА", avatarSelectionWidget);
    overlayTitle->setStyleSheet("font-size: 20px; font-weight: bold; color: #333; background: transparent;");
    overlayTitle->setAlignment(Qt::AlignCenter);

    QWidget *gridContainer = new QWidget(avatarSelectionWidget);
    gridContainer->setStyleSheet("background: transparent;");
    QGridLayout *grid = new QGridLayout(gridContainer);
    grid->setSpacing(15);

    // --- ГЕНЕРАЦИЯ ТЕСТОВЫХ АВАТАРОК ---
    QStringList avatars;
    avatars << ":/avatars/CRking.png" << ":/avatars/2.png" << ":/avatars/3.png" << ":/avatars/4.png";

    for(int i=0; i<4; ++i) {
        QPushButton *btn = new QPushButton(avatarSelectionWidget);
        btn->setFixedSize(80, 80);
        btn->setCursor(Qt::PointingHandCursor);
        btn->setStyleSheet("QPushButton { border: 2px solid #555; background-color: #fff; } QPushButton:hover { border: 4px solid #333; }");

        QString path = (i < avatars.size()) ? avatars[i] : "";
        QPixmap pix(80, 80);

        if (QFile::exists(path)) {
            pix.load(path);
        } else {
            // Файла нет, генерируем цвет
            QColor col = QColor::fromHsv((i * 60) % 360, 150, 200);
            pix.fill(col);
            QPainter p(&pix);
            p.drawText(pix.rect(), Qt::AlignCenter, QString::number(i+1));
        }

        QIcon icon(pix);
        btn->setIcon(icon);
        btn->setIconSize(QSize(60, 60));

        btn->setProperty("avatarPath", path.isEmpty() ? QString("color:%1").arg(i) : path);

        connect(btn, &QPushButton::clicked, this, [=](){
            QString p = btn->property("avatarPath").toString();
            onAvatarSelected(p);
        });

        grid->addWidget(btn, 0, i);
    }

    QPushButton *btnCancel = new QPushButton("ОТМЕНА", avatarSelectionWidget);
    btnCancel->setFixedWidth(150);
    btnCancel->setStyleSheet("background-color: #95a5a6; color: white;");
    connect(btnCancel, &QPushButton::clicked, this, &MainWindow::onChangeAvatarClicked);

    overlayLayout->addStretch();
    overlayLayout->addWidget(overlayTitle);
    overlayLayout->addSpacing(20);
    overlayLayout->addWidget(gridContainer);
    overlayLayout->addSpacing(30);
    overlayLayout->addWidget(btnCancel);
    overlayLayout->addStretch();
}

void MainWindow::resizeEvent(QResizeEvent *event) {
    // При изменении размера окна, растягиваем контейнеры
    QSize s = event->size();

    // Обновляем размер оверлея, если он существует
    if (avatarSelectionWidget) {
        avatarSelectionWidget->resize(s);
    }

    // Если меню активно (находится в 0,0)
    if (menuContainer->pos().x() == 0) {
        menuContainer->resize(s);
        settingsContainer->resize(s);
        settingsContainer->move(-s.width(), 0); // Прячем слева
        // Сбрасываем оффсет фона если мы в меню
        if (getBackgroundOffset() != 0) setBackgroundOffset(0);
    }
    // Если настройки активны (находятся в 0,0)
    else if (settingsContainer->pos().x() == 0) {
        settingsContainer->resize(s);
        menuContainer->resize(s);
        menuContainer->move(s.width(), 0); // Прячем справа
    }
    // Если идет анимация
    else {
        menuContainer->resize(s);
        settingsContainer->resize(s);
    }

    QMainWindow::resizeEvent(event);
}

void MainWindow::mouseMoveEvent(QMouseEvent *event) {
    mousePos = event->pos();
    update(); // Перерисовываем фон при движении мыши
}

void MainWindow::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, false);

    p.fillRect(rect(), QColor(248, 240, 227));

    int w = width();
    int h = height();
    int pixelSize = 4;
    int shiftX = (mousePos.x() - w/2) * 0.05;
    int shiftY = (mousePos.y() - h/2) * 0.05;

    // Ширина цикла для фона (экран + буфер для плавного появления)
    int cycleWidth = w + 200;

    auto drawPixelShip = [&](int originalX, int y, int size, QColor color) {
        p.setBrush(color);
        p.setPen(Qt::NoPen);
        int s = size;
        int shipShiftX = shiftX * 0.5;

        // --- ЛОГИКА ПРОКРУТКИ КОРАБЛЕЙ ---
        // Добавляем backgroundOffset к позиции
        int currentX = originalX + (int)backgroundOffset;

        // Зацикливаем координату X
        // Используем сложный модуль, чтобы корректно обрабатывать отрицательные числа при сдвиге
        int wrappedX = ((currentX % cycleWidth) + cycleWidth) % cycleWidth;

        // Сдвигаем обратно на -100, чтобы компенсировать буфер и позволить выплывать из-за левого края
        wrappedX -= 100;

        int finalX = wrappedX + shipShiftX;

        p.drawRect(finalX, y, s*6, s);
        p.drawRect(finalX + s, y-s, s*4, s);
        p.drawRect(finalX + s*2, y-s*2, s, s);
    };

    drawPixelShip(w*0.1, h*0.2, 5, QColor(200, 190, 180));
    drawPixelShip(w*0.8, h*0.6, 6, QColor(180, 170, 160));
    drawPixelShip(w*0.2, h*0.8, 4, QColor(190, 180, 170));

    p.setPen(Qt::NoPen);
    p.setBrush(QColor(169, 169, 169));

    for (int y = 50; y < h; y += 40) {
        int rowShift = shiftX * (float(y)/h);
        for (int x = -50; x < w + 50; x += 20) {

            // --- ЛОГИКА ПРОКРУТКИ ВОЛН ---
            int bgShift = (int)backgroundOffset;
            int finalX = x + rowShift + bgShift;

            // Зацикливаем волны. Используем ширину окна + запас 100
            int waveCycle = w + 100;
            finalX = (finalX % waveCycle + waveCycle) % waveCycle;

            // Если ушли слишком далеко вправо из-за модуля, сдвигаем влево для заполнения дыры
            if (finalX > w + 50) finalX -= waveCycle;
            // Коррекция для начала (чтобы не было пустой полосы слева при старте)
            if (finalX > w) finalX -= waveCycle;

            p.drawRect(finalX, y + shiftY * 0.2, pixelSize, pixelSize);
            p.drawRect(finalX + pixelSize, y + pixelSize + shiftY * 0.2, pixelSize, pixelSize);
        }
    }
}

// --- ЛОГИКА АНИМАЦИИ ---

void MainWindow::onSettingsClicked() {
    // Экран (камера) едет влево -> Объекты едут вправо
    // Меню уезжает вправо (в width)
    // Настройки выезжают из левой части (из -width в 0)

    // Подготовка позиций перед анимацией
    settingsContainer->move(-width(), 0);
    settingsContainer->show();

    animMenu = new QPropertyAnimation(menuContainer, "pos", this);
    animMenu->setDuration(500);
    animMenu->setStartValue(QPoint(0, 0));
    animMenu->setEndValue(QPoint(width(), 0));
    animMenu->setEasingCurve(QEasingCurve::InOutQuad);

    animSettings = new QPropertyAnimation(settingsContainer, "pos", this);
    animSettings->setDuration(500);
    animSettings->setStartValue(QPoint(-width(), 0));
    animSettings->setEndValue(QPoint(0, 0));
    animSettings->setEasingCurve(QEasingCurve::InOutQuad);

    // --- АНИМАЦИЯ ФОНА (ВПРАВО) ---
    // Фон сдвигается на ширину экрана вправо, создавая эффект уплывания
    animBackground = new QPropertyAnimation(this, "backgroundOffset", this);
    animBackground->setDuration(500);
    animBackground->setStartValue(backgroundOffset); // Обычно 0
    animBackground->setEndValue((float)width());     // Сдвигаем на ширину экрана
    animBackground->setEasingCurve(QEasingCurve::InOutQuad);

    animMenu->start(QAbstractAnimation::DeleteWhenStopped);
    animSettings->start(QAbstractAnimation::DeleteWhenStopped);
    animBackground->start(QAbstractAnimation::DeleteWhenStopped);
}

void MainWindow::onBackFromSettingsClicked() {
    // Возврат:
    // Настройки уезжают обратно влево (-width)
    // Меню возвращается из правой части (из width в 0)

    // Если оверлей открыт, скрываем его
    if (avatarSelectionWidget->isVisible()) avatarSelectionWidget->hide();

    animMenu = new QPropertyAnimation(menuContainer, "pos", this);
    animMenu->setDuration(500);
    animMenu->setStartValue(QPoint(width(), 0));
    animMenu->setEndValue(QPoint(0, 0));
    animMenu->setEasingCurve(QEasingCurve::InOutQuad);

    animSettings = new QPropertyAnimation(settingsContainer, "pos", this);
    animSettings->setDuration(500);
    animSettings->setStartValue(QPoint(0, 0));
    animSettings->setEndValue(QPoint(-width(), 0));
    animSettings->setEasingCurve(QEasingCurve::InOutQuad);

    // --- АНИМАЦИЯ ФОНА (ОБРАТНО ВЛЕВО) ---
    // Возвращаем фон в исходное положение
    animBackground = new QPropertyAnimation(this, "backgroundOffset", this);
    animBackground->setDuration(500);
    animBackground->setStartValue(backgroundOffset); // Текущее (width)
    animBackground->setEndValue(0.0f);               // Обратно в 0
    animBackground->setEasingCurve(QEasingCurve::InOutQuad);

    animMenu->start(QAbstractAnimation::DeleteWhenStopped);
    animSettings->start(QAbstractAnimation::DeleteWhenStopped);
    animBackground->start(QAbstractAnimation::DeleteWhenStopped);
}

void MainWindow::onChangeAvatarClicked() {
    // Показываем/скрываем панель выбора
    if (avatarSelectionWidget->isVisible()) {
        avatarSelectionWidget->hide();
    } else {
        avatarSelectionWidget->show();
        avatarSelectionWidget->raise(); // Поднимаем наверх
    }
}

void MainWindow::onAvatarSelected(const QString &path) {
    selectedAvatarPath = path;

    // Обновляем превью
    if (path.startsWith("color:")) {
        // Это наша заглушка
        int idx = path.split(":")[1].toInt();
        QPixmap pix(100, 100);
        pix.fill(QColor::fromHsv((idx * 60) % 360, 150, 200));
        currentAvatarPreview->setPixmap(pix);
    } else {
        // Реальный файл
        QPixmap pix(path);
        if (!pix.isNull()) {
            currentAvatarPreview->setPixmap(pix.scaled(110, 110, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        }
    }

    // Скрываем выбор
    avatarSelectionWidget->hide();
}

void MainWindow::onSinglePlayerClicked()
{
    // Передаем выбранный аватар в игру
    // Если это "цветная заглушка", GameWindow попробует загрузить это как путь и не найдет файл,
    // поэтому покажет стандартного человечка.
    // Чтобы работало с заглушкой, нужно допиливать GameWindow, но для файлов ресурсов всё ок.

    GameWindow *game = new GameWindow(selectedAvatarPath);
    this->hide();
    connect(game, &GameWindow::backToMenu, this, [=]() {
        this->show();
    });
    game->setAttribute(Qt::WA_DeleteOnClose);
    game->show();
}

void MainWindow::onMultiplayerClicked() { QMessageBox::information(this, "Инфо", "Скоро..."); }
void MainWindow::onExitClicked() { QApplication::quit(); }
