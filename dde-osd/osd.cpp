#include "osd.h"
#include <X11/extensions/shape.h>
#include <QtX11Extras/QX11Info>
#include <QDesktopWidget>
#include <QApplication>
#include <QPainter>
#include <QBrush>
#include <QPen>
#include <QPainterPath>
#include <QRect>


Osd::Osd(QWidget *parent)
    : QWidget(parent)
{

    initInterfaces();

    initGlobalVars();

    initBasicOperation();

    initConnects();
}

Osd::~Osd()
{

    // set the monitor mode when the app quits
    if (actionMode == SwitchMonitor) {
        if (m_CurrentIndexOfMonitorItem == 0) {
            // switch to duplicate mode
            m_DisplayInterface->SwitchMode(1, "");
        } else if (m_CurrentIndexOfMonitorItem == 1) {
            // switch to expanded mode
            m_DisplayInterface->SwitchMode(2, "");
        } else {
            // switch to one certain screen mode
            m_DisplayInterface->SwitchMode(3, m_ScreenList[m_CurrentIndexOfMonitorItem - 2]);
        }
    }

    // set the keyboard layout when the app quits
    if (actionMode == SwitchLayout && m_KeyboardList.length() > 0) {
        m_LayoutInterface->setCurrentLayout(m_KeyboardList[m_CurrentIndexOfKeyBoard]);
    }
}

void Osd::initInterfaces()
{
    m_VolumeInterface = new VolumeDbus("com.deepin.daemon.Audio",
                                       "/com/deepin/daemon/Audio/Sink0",
                                       QDBusConnection::sessionBus(), this);

    m_DisplayInterface = new DisplayDbus("com.deepin.daemon.Display",
                                         "/com/deepin/daemon/Display",
                                         QDBusConnection::sessionBus(), this);

    m_LayoutInterface = new LayoutDbus("com.deepin.daemon.InputDevices",
                                       "/com/deepin/daemon/InputDevice/Keyboard",
                                       QDBusConnection::sessionBus(), this);
}

void Osd::initGlobalVars()
{
    // image label
    m_ImageLabel = new QLabel(this);
    // m_Timer is used to record time , to quit the app properly
    m_Timer = new QTimer(this);

    // initial m_ListWidget
    m_ListWidget = new QListWidget(this);

    // initial m_MonitersWrapper
    m_MonitersWrapper = new QWidget(this);

    // m_CanAudioMuteRun is used to record the mute state of sound
    m_CanAudioMuteRun = m_VolumeInterface->mute();
    // to record whether AudioMute has run before this time
    m_AudioMuteNotRunFromAudioMute = true;

    // init SwitchMonitors's displaymode
    if (m_DisplayInterface->displayMode() == 0) {
        displaymode = Custom;
    } else if (m_DisplayInterface->displayMode() == 1) {
        displaymode = Duplicate;
    } else if (m_DisplayInterface->displayMode() == 2) {
        displaymode = Expanded;
    } else if (m_DisplayInterface->displayMode() == 3) {
        displaymode = OneScreen;
    }
}

void Osd::initBasicOperation()
{
    this->resize(BASE_SIZE, BASE_SIZE);
    setWindowFlags(Qt::X11BypassWindowManagerHint | Qt::WindowStaysOnTopHint);
    setAttribute(Qt::WA_TranslucentBackground, true);
    // set mouse penetration
    XShapeCombineRectangles(QX11Info::display(), winId(), ShapeInput, 0, 0, NULL, 0, ShapeSet, YXBanded);

    // set fixed size for image icon, and move it to app's center
    m_ImageLabel->setFixedSize(IMAGE_SIZE, IMAGE_SIZE);
    m_ImageLabel->move((this->width() - IMAGE_SIZE) / 2, (this->height() - IMAGE_SIZE) / 2);
}

void Osd::initConnects()
{
    // when reaches deadline, we need to quit the app immediately
    connect(m_Timer, &QTimer::timeout, qApp, &QCoreApplication::quit);
}

int Osd::latterAction()
{
    return actionMode;
}

void Osd::moveToCenter()
{
    // find out the screen that contains mouse.
    QDesktopWidget *desktop = QApplication::desktop();
    int primaryScreenKey = desktop->primaryScreen();
    for (int i = 0; i < desktop->screenCount(); i++) {
        QRect screen = desktop->screenGeometry(primaryScreenKey + i);
        if (screen.contains(QCursor::pos())) {
            m_MouseInScreen = screen;
            break;
        }
    }
    // move to corresponding screen
    this->move(m_MouseInScreen.x() + (m_MouseInScreen.width() - this->width()) / 2, m_MouseInScreen.y() + (m_MouseInScreen.height() - this->height()) / 2);
}

void Osd::setTimer()
{
    m_Timer->start(DEADLINE_TIME);
}

void Osd::tail_in_Work()
{
    moveToCenter();
    m_Timer->start(DEADLINE_TIME);
    this->repaint();
}

// this function is used to display normal osd'show, which is different from SwitchLayout and SwitchMonitors
void Osd::loadCorrespondingImage(QString whichImage)
{
    actionMode = Normal;
    m_ListWidget->setVisible(false);
    m_MonitersWrapper->setVisible(false);
    this->resize(BASE_SIZE, BASE_SIZE);
    if (whichImage == "NumLockOn") {
        m_Pixmap.load(":/images/numlock-enabled-symbolic.svg");
    } else if (whichImage == "NumLockOff") {
        m_Pixmap.load(":/images/numlock-disabled-symbolic.svg");
    } else if (whichImage == "CapsLockOn") {
        m_Pixmap.load(":/images/capslock-enabled-symbolic.svg");
    } else if (whichImage == "CapsLockOff") {
        m_Pixmap.load(":/images/capslock-disabled-symbolic.svg");
    } else if (whichImage == "TouchpadOn") {
        m_Pixmap.load(":/images/input-touchpad-symbolic.svg");
    } else if (whichImage == "TouchpadOff") {
        m_Pixmap.load(":/images/touchpad-disabled-symbolic.svg");
    } else if (whichImage == "TouchpadToggle") {
        m_Pixmap.load(":/images/touchpad-toggled-symbolic.svg");
    } else if (whichImage == "Brightness") {
        actionMode = NormalBrightness;
        m_Pixmap.load(":/images/display-brightness-symbolic.svg");
    } else if (whichImage == "AudioMute") {
        if (m_CanAudioMuteRun && m_AudioMuteNotRunFromAudioMute) {
            m_Pixmap.load(":/images/audio-volume-muted-symbolic-osd.svg");
            m_AudioMuteNotRunFromAudioMute = false;
        } else {
            loadCorrespondingImage("Audio");
        }
    } else if (whichImage == "Audio") {
        actionMode = NormalAudio;
        m_CanAudioMuteRun = true;
        m_AudioMuteNotRunFromAudioMute = true;
        double volume = m_VolumeInterface->volume();
        if (volume > 0.7 && volume <= 1.0) {
            m_Pixmap.load(":/images/audio-volume-high-symbolic-osd.svg");
        } else if (volume > 0.3 && volume <= 0.7) {
            m_Pixmap.load(":/images/audio-volume-medium-symbolic-osd.svg");
        } else if (volume > 0.0) {
            m_Pixmap.load(":/images/audio-volume-low-symbolic-osd.svg");
        } else if (volume == 0.0) {
            m_Pixmap.load(":/images/audio-volume-muted-symbolic-osd.svg");
        }
    }

    m_ImageLabel->setPixmap(m_Pixmap);
}

// --SwitchLayout
void Osd::loadSwitchLayout()
{
    actionMode = SwitchLayout;
    m_ImageLabel->setPixmap(QPixmap(""));
    m_MonitersWrapper->setVisible(false);
    m_ListWidget->setVisible(true);

    if (m_LayoutInterface->userLayoutList().length() > 1) {

        // give out the value of m_MaxTextWidth and m_KeyboradLayoutHeight, to help resize this app
        calculateKeyboardSize();
        this->resize(m_MaxTextWidth + LAYOUT_MARGIN * 4, m_KeyboradLayoutHeight + LAYOUT_MARGIN * 2);
        initKeyboard();
    } else {
        // if user's keyboard layout(s) just contain(s) 1 kind, quit the app immediately
        this->deleteLater();
    }
}

// the following 3 functions belong to SwitchLayout,which are calculateKeyboardSize(), initKeyboard() and reHiglightKeyboard()
void Osd::calculateKeyboardSize()
{
    m_MaxTextWidth = 0;

    // get the list of keyboard layout
    m_KeyboardList = m_LayoutInterface->userLayoutList();

    // set font-size to 14px, so that we can calculate the max width of all keyboardlayout texts
    QLabel *text = new QLabel;
    m_f = text->font();
    m_f.setPixelSize(14);
    text->setFont(m_f);

    int length = m_KeyboardList.length();
    // give the value of m_KeyboradLayoutHeight
    m_KeyboradLayoutHeight = (length > 5 ? KEYBOARD_ITEM_HEIGHT * 5 : KEYBOARD_ITEM_HEIGHT * length);

    // give the value of m_MaxTextWidth
    QFontMetrics metrics(text->font());
    for (int i = 0; i < length; i++) {
        text->setText(m_LayoutInterface->GetLayoutDesc(m_KeyboardList[i]));
        int textWidth = metrics.boundingRect(text->text()).width();
        m_MaxTextWidth = (textWidth > m_MaxTextWidth ? textWidth : m_MaxTextWidth);
    }
}

void Osd::initKeyboard()
{
    // variable "size" is the size of QListWidgetItem* item and QWidget* customItem
    QSize size(m_MaxTextWidth + LAYOUT_MARGIN * 2, KEYBOARD_ITEM_HEIGHT);

    // hLayout is used to wrap m_ListWidget
    QHBoxLayout *hLayout = new QHBoxLayout(this);
    // make sure m_ListWidget's margins be 10px
    hLayout->setContentsMargins(10, 10, 10, 10);

    // set m_ListWidget's parameters
    m_ListWidget->setStyleSheet("background:transparent");
    m_ListWidget->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_ListWidget->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_ListWidget->setFrameStyle(QFrame::NoFrame);
    m_ListWidget->resize(m_MaxTextWidth + LAYOUT_MARGIN * 2, m_KeyboradLayoutHeight);

    for (int i = 0, length = m_KeyboardList.length(); i < length; i++) {
        QListWidgetItem *item = new QListWidgetItem;
        // setFlags(Qt::NoItemFlags) can remove the default highlight
        item->setFlags(Qt::NoItemFlags);
        item->setSizeHint(size);

        // customItem and text within it
        QWidget *customItem = new QWidget;
        customItem->setFixedSize(size);
        QLabel *text = new QLabel(customItem);
        text->setFont(m_f);
        m_KeyboradTextList << text;
        // make sure left-margin and right-margin be 10px
        text->setContentsMargins(10, 0, 10, 0);
        text->setFixedSize(size);
        text->setText(m_LayoutInterface->GetLayoutDesc(m_KeyboardList[i]));

        // highlight the chosen customItem and text withint it, when app starts
        if (m_LayoutInterface->currentLayout() == m_KeyboardList[i]) {
            m_CurrentIndexOfKeyBoard = i;
            m_RecordInitialIndexOfKeyBoard = i;
            text->setStyleSheet(KEYBOARD_ITEM_HIGHLIGHT_STYLE);
        } else {
            text->setStyleSheet(KEYBOARD_ITEM_NORMAL_STYLE);
        }

        m_ListWidget->addItem(item);
        m_ListWidget->setItemWidget(item, customItem);
    }

    hLayout->addWidget(m_ListWidget);

    // make sure that the highlighted item can be displayed in view after app startss
    m_ListWidget->scrollToItem(m_ListWidget->item(m_CurrentIndexOfKeyBoard));

    // the following codes are about animation
    contentY = m_ListWidget->itemWidget(m_ListWidget->item(0))->y();
    m_animation = new QVariantAnimation(m_ListWidget);
    m_animation->setDuration(70);

    // when currentrow changes, check if new_contentY is different from contentY. If different,m_animation should start
    connect(m_ListWidget, &QListWidget::currentRowChanged,
    this, [this] {
        int new_contentY = m_ListWidget->itemWidget(m_ListWidget->item(0))->y();
        if (new_contentY != contentY  && m_KeyboardList.length() > 5)
        {
            m_animation->setStartValue(QVariant::fromValue(contentY));
            m_animation->setEndValue(QVariant::fromValue(new_contentY));

            contentY = new_contentY;

            m_animation->start();
        }
    });

    connect(m_animation, &QVariantAnimation::valueChanged, this, [this](const QVariant & value) {
        int contentY = m_ListWidget->itemWidget(m_ListWidget->item(0))->y();
        for (int i = 0; i < m_KeyboardList.length(); i++) {
            QWidget *w = m_ListWidget->itemWidget(m_ListWidget->item(i));
            w->move(0, w->y() - contentY + value.toInt());
        }
    });
}

void Osd::reHiglightKeyboard()
{
    if (m_CurrentIndexOfKeyBoard == 0) {
        // if m_CurrentIndexOfKeyBoard == 0,highlight the first one, and normalize the last one
        m_KeyboradTextList[0]->setStyleSheet(KEYBOARD_ITEM_HIGHLIGHT_STYLE);
        m_KeyboradTextList[m_KeyboradTextList.length() - 1]->setStyleSheet(KEYBOARD_ITEM_NORMAL_STYLE);
    } else {
        // highlight the current one, normalize the old one
        m_KeyboradTextList[m_CurrentIndexOfKeyBoard]->setStyleSheet(KEYBOARD_ITEM_HIGHLIGHT_STYLE);
        m_KeyboradTextList[m_CurrentIndexOfKeyBoard - 1]->setStyleSheet(KEYBOARD_ITEM_NORMAL_STYLE);
    }
    m_ListWidget->setCurrentRow(m_CurrentIndexOfKeyBoard);
}

void Osd::highlightNextLayout()
{
    if (m_CurrentIndexOfKeyBoard == m_KeyboardList.length() - 1) {
        m_CurrentIndexOfKeyBoard = 0;

        // make sure the listwidgetitem is positioned as initial after one-cycle selection
        if (m_KeyboardList.length() > 5) {
            for (int i = 0, length = m_KeyboardList.length(); i < length; i++) {
                QWidget *w = m_ListWidget->itemWidget(m_ListWidget->item(i));
                w->move(0, KEYBOARD_ITEM_HEIGHT * (i + 5 - length));
            }
        }
    } else {
        m_CurrentIndexOfKeyBoard++;
    }
    reHiglightKeyboard();
}

void Osd::loadSwitchMonitors()
{
    actionMode = SwitchMonitor;
    m_ImageLabel->setPixmap(QPixmap(""));
    m_ListWidget->setVisible(false);
    m_MonitersWrapper->setVisible(true);
    // get the list of all screens by using QString's method "split"
    QString screenNamesStr = (QString)m_DisplayInterface->QueryCurrentPlanName();
    m_ScreenList = screenNamesStr.split(",");

    if (m_ScreenList.length() > 1) {
        this->resize(BASE_SIZE * (m_ScreenList.length() + 2), BASE_SIZE);

        initMonitorItems();

        initCurrentScreenMode();
    } else {
        // if just 1 screen , quit the app immediately
        this->deleteLater();
    }
}

void Osd::initMonitorItems()
{
    m_MonitersWrapper->resize(this->size());
    m_HLayout = new QHBoxLayout(m_MonitersWrapper);

    // for duplicate mode
    QWidget *duplicateScreenItem = new QWidget(m_MonitersWrapper);
    QVBoxLayout *vLayoutOfDuplicateScreen = new QVBoxLayout(duplicateScreenItem);
    // image label for duplicate mode
    m_DuplicateScreenImage = new QLabel(duplicateScreenItem);
    m_DuplicateScreenImage->setFixedSize(IMAGE_SIZE, IMAGE_SIZE);
    m_DuplicateScreenImage->setPixmap(QPixmap(":/images/project_screen-duplicate-symbolic.svg"));
    // text label for duplicate mode
    m_DuplaicateScreenText = new QLabel(duplicateScreenItem);
    m_DuplaicateScreenText->setText(tr("Duplicate"));
    m_DuplaicateScreenText->setAlignment(Qt::AlignCenter);
    m_DuplaicateScreenText->setStyleSheet(MONITOR_TEXT_NORMAL_STYLE);
    // add above 2 widgets
    vLayoutOfDuplicateScreen->addWidget(m_DuplicateScreenImage, 0, Qt::AlignHCenter);
    vLayoutOfDuplicateScreen->addWidget(m_DuplaicateScreenText, 0, Qt::AlignHCenter);

    // for expanded mode
    QWidget *expandedScreenItem = new QWidget(m_MonitersWrapper);
    QVBoxLayout *vLayoutOfExpandedScreen = new QVBoxLayout(expandedScreenItem);
    // image label for expanded mode
    m_ExpandedScreenImage = new QLabel(expandedScreenItem);
    m_ExpandedScreenImage->setFixedSize(IMAGE_SIZE, IMAGE_SIZE);
    m_ExpandedScreenImage->setPixmap(QPixmap(":/images/project_screen-extend-symbolic.svg"));
    // text label for expanded mode
    m_ExpandedScreenText = new QLabel(expandedScreenItem);
    m_ExpandedScreenText->setText(tr("Extend"));
    m_ExpandedScreenText->setAlignment(Qt::AlignCenter);
    m_ExpandedScreenText->setStyleSheet(MONITOR_TEXT_NORMAL_STYLE);
    // add above 2 widgets
    vLayoutOfExpandedScreen->addWidget(m_ExpandedScreenImage, 0, Qt::AlignHCenter);
    vLayoutOfExpandedScreen->addWidget(m_ExpandedScreenText, 0, Qt::AlignHCenter);

    // add duplicate and expanded items
    m_HLayout->addWidget(duplicateScreenItem);
    m_HLayout->addWidget(expandedScreenItem);

    // for one-screen mode
    for (int i = 0, length = m_ScreenList.length(); i < length; i++) {
        // one-screen mode item
        QWidget *item = new QWidget(m_MonitersWrapper);
        QVBoxLayout *vLayout = new QVBoxLayout(item);
        // image label for one-screen mode
        QLabel *imageLabel = new QLabel(item);
        imageLabel->setFixedSize(IMAGE_SIZE, IMAGE_SIZE);
        imageLabel->setPixmap(QPixmap(":/images/project_screen-onlyone-symbolic.svg"));
        // text label for one-screen mode
        QLabel *textLabel = new QLabel(item);
        textLabel->setText(m_ScreenList[i]);
        textLabel->setAlignment(Qt::AlignCenter);
        textLabel->setStyleSheet(MONITOR_TEXT_NORMAL_STYLE);
        // store imagelabel and textlabel into lists, so that we can change their style later
        m_ImageLabelList << imageLabel;
        m_TextLabelList << textLabel;
        // add above 2 widgets
        vLayout->addWidget(imageLabel, 0, Qt::AlignHCenter);
        vLayout->addWidget(textLabel, 0, Qt::AlignHCenter);

        // add one-screen mode item
        m_HLayout->addWidget(item);
    }
}

void Osd::initCurrentScreenMode()
{
    // for each mode, we would do the same operations to change it's style when app starts
    switch (displaymode) {
    case Custom:
//        m_CurrentIndexOfMonitorItem = -1;
        break;
    case Duplicate:
        m_DuplicateScreenImage->setPixmap(QPixmap(":/images/project_screen-duplicate-symbolic-focus.svg"));
        m_DuplaicateScreenText->setStyleSheet(MONITOR_TEXT_HIGHLIGHT_STYLE);
        m_CurrentIndexOfMonitorItem = 0;
        break;
    case Expanded:
        m_ExpandedScreenImage->setPixmap(QPixmap(":/images/project_screen-extend-symbolic-focus.svg"));
        m_ExpandedScreenText->setStyleSheet(MONITOR_TEXT_HIGHLIGHT_STYLE);
        m_CurrentIndexOfMonitorItem = 1;
        break;
    case OneScreen:
        QString primaryScreenName = m_DisplayInterface->primary();
        for (int i = 0, length = m_ScreenList.length(); i < length; i++) {
            if (m_ScreenList[i] == primaryScreenName) {
                m_ImageLabelList[i]->setPixmap(QPixmap(":/images/project_screen-onlyone-symbolic-focus.svg"));
                m_TextLabelList[i]->setStyleSheet(MONITOR_TEXT_HIGHLIGHT_STYLE);
                m_CurrentIndexOfMonitorItem = i + 2;
            }
        }
        break;
    }
}

// when Meta-P is pressed , we need to refresh the highlight style
void Osd::reHighlightMonitor()
{
    if (m_CurrentIndexOfMonitorItem == 0) {
        m_DuplicateScreenImage->setPixmap(QPixmap(":/images/project_screen-duplicate-symbolic-focus.svg"));
        m_DuplaicateScreenText->setStyleSheet(MONITOR_TEXT_HIGHLIGHT_STYLE);
        m_ImageLabelList[m_ScreenList.length() - 1]->setPixmap(QPixmap(":/images/project_screen-onlyone-symbolic.svg"));
        m_TextLabelList[m_ScreenList.length() - 1]->setStyleSheet(MONITOR_TEXT_NORMAL_STYLE);
    } else if (m_CurrentIndexOfMonitorItem == 1) {
        m_ExpandedScreenImage->setPixmap(QPixmap(":/images/project_screen-extend-symbolic-focus.svg"));
        m_ExpandedScreenText->setStyleSheet(MONITOR_TEXT_HIGHLIGHT_STYLE);
        m_DuplicateScreenImage->setPixmap(QPixmap(":/images/project_screen-duplicate-symbolic.svg"));
        m_DuplaicateScreenText->setStyleSheet(MONITOR_TEXT_NORMAL_STYLE);
    } else if (m_CurrentIndexOfMonitorItem == 2) {
        m_ImageLabelList[0]->setPixmap(QPixmap(":/images/project_screen-onlyone-symbolic-focus.svg"));
        m_TextLabelList[0]->setStyleSheet(MONITOR_TEXT_HIGHLIGHT_STYLE);
        m_ExpandedScreenImage->setPixmap(QPixmap(":/images/project_screen-extend-symbolic.svg"));
        m_ExpandedScreenText->setStyleSheet(MONITOR_TEXT_NORMAL_STYLE);
    } else {
        m_ImageLabelList[m_CurrentIndexOfMonitorItem - 2]->setPixmap(QPixmap(":/images/project_screen-onlyone-symbolic-focus.svg"));
        m_TextLabelList[m_CurrentIndexOfMonitorItem - 2]->setStyleSheet(MONITOR_TEXT_HIGHLIGHT_STYLE);
        m_ImageLabelList[m_CurrentIndexOfMonitorItem - 3]->setPixmap(QPixmap(":/images/project_screen-onlyone-symbolic.svg"));
        m_TextLabelList[m_CurrentIndexOfMonitorItem - 3]->setStyleSheet(MONITOR_TEXT_NORMAL_STYLE);
    }
}

void Osd::highlightNextMonitor()
{
    if (m_CurrentIndexOfMonitorItem < (m_ScreenList.length() + 1)) {
        ++m_CurrentIndexOfMonitorItem;
    } else {
        m_CurrentIndexOfMonitorItem = 0;
    }
    reHighlightMonitor();
}

void Osd::paintEvent(QPaintEvent *)
{
    // paint app's background
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    QRect solidRect(1, 1, this->width() - 2, this->height() - 2);
    QBrush brush;
    brush.setStyle(Qt::SolidPattern);
    brush.setColor(QColor(0, 0, 0, 127));
    painter.setBrush(brush);
    painter.drawRoundedRect(solidRect, 10, 10);

    QPen pen;
    pen.setStyle(Qt::SolidLine);
    pen.setColor(QColor(255, 255, 255, 51));
    pen.setWidth(1);
    QPainterPath path;
    QRect hollowRect(0, 0, this->width(), this->height());
    path.addRoundedRect(hollowRect, 13, 13);
    painter.setPen(pen);
    painter.drawPath(path);

    if (actionMode == NormalAudio || actionMode == NormalBrightness) {
        // paint progressbar's background
        QRect progressBarBackRect(30, 106, 80, 4);
        brush.setColor(QColor(255, 255, 255, 51));
        painter.setBrush(brush);
        painter.drawRoundedRect(progressBarBackRect, 2, 2);

        if (actionMode == NormalAudio) {
            // paint audio progressbar
            QRect progressBarRect(30, 106, 80 * (m_VolumeInterface->volume() >= 1.0 ? 1.0 : m_VolumeInterface->volume()), 4);
            brush.setColor(QColor(255, 255, 255, 255));
            painter.setBrush(brush);
            painter.drawRoundedRect(progressBarRect, 2, 2);
        } else if (actionMode == NormalBrightness) {
            // paint brightness progressbar
            QRect progressBarRect(30, 106, 80 * m_DisplayInterface->brightness()[m_DisplayInterface->primary()], 4);
            brush.setColor(QColor(255, 255, 255, 255));
            painter.setBrush(brush);
            painter.drawRoundedRect(progressBarRect, 2, 2);
        }
    }
}

