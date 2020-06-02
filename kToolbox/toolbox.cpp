#include "toolbox.h"

#include <QPainter>
#include <QPaintEvent>
#include <QTextItem>

ToolBox::ToolBox(QWidget *parent) :
    QWidget(parent), m_activated(-1), m_padding(10), m_subToolActivated(-1), m_buttonSize(40, 40),
    m_longPressed(false), m_toolBoxHandlePressed(false), m_toolBoxOrientation(Vertical),
    m_subToolsOrientation(Right), m_rowMode(Single), m_toolBoxState(Locked)
{
    setWindowFlags(Qt::Widget | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_NoSystemBackground, true);
    setAttribute(Qt::WA_TranslucentBackground, true);
    m_timer.setInterval(10);
    m_timer.setSingleShot(true);

    connect(&m_timer, &QTimer::timeout, this, &ToolBox::longPressEvent);
    setMouseTracking(true);
    setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this, &QWidget::customContextMenuRequested, this, &ToolBox::showContextMenu);

    m_contextMenu = new QMenu(this);
    m_unlockAction = new QAction("Unlock ToolBox", this);
    m_contextMenu->addAction(m_unlockAction);
    connect(m_unlockAction, &QAction::triggered, this, &ToolBox::slotUnlockToolBox);

    m_rotateAction = new QAction("Rotate ToolBox", this);
    m_contextMenu->addAction(m_rotateAction);
    connect(m_rotateAction, &QAction::triggered, this, &ToolBox::slotRotateToolBox);

    m_rowModeAction = new QAction("Change Row Mode", this);
    m_contextMenu->addAction(m_rowModeAction);
    connect(m_rowModeAction, &QAction::triggered, this, &ToolBox::slotChangeRowMode);

    m_flipAction = new QAction("Flip Sub Tools", this);
    m_contextMenu->addAction(m_flipAction);
    connect(m_flipAction, &QAction::triggered, this, &ToolBox::slotFlipToolBox);
}

void ToolBox::paintEvent(QPaintEvent *event)
{
    QPainter gc(this);
    gc.setRenderHint(QPainter::Antialiasing);

    QPoint tempTopLeft = event->rect().topLeft();
    QColor highlightedBack(86, 128, 194);
    QColor back(49, 49, 49);

    if(m_toolBoxState == Unlocked){
        QRect toolBoxHandle = QRect(tempTopLeft, QSize(m_buttonSize.width(), 7));
        gc.fillRect(toolBoxHandle, back);
        tempTopLeft += QPoint(0, 9);
    }
    QPoint topLeft = tempTopLeft;
    m_toolRects.clear();

    for(int i=0; i<m_breaks.count(); i++){
        int count = m_breaks[i] + 1;
        if(i>0){
            count = count - m_breaks[i-1] - 1;
        }
        int h = m_buttonSize.height() * (count);
        QRect backRect(topLeft, QSize(m_buttonSize.width(), h));
        QPainterPath p;
        p.addRoundedRect(backRect, 3, 3);
        gc.fillPath(p, back);
        topLeft += QPoint(0, 2+h);
    }

    int breakCount = 0;
    topLeft = tempTopLeft;

    for(int i=0;i<m_tools.count();i++){
        QRect toolRect(topLeft, m_buttonSize);
        QPainterPath path;
        path.addRoundedRect(toolRect, 3, 3);

        if(i == m_activated){
            gc.fillPath(path, highlightedBack);
        }

        if(!m_tools[i].subTools().isEmpty()){
            QPainterPath p;
            p.moveTo(toolRect.bottomRight());
            p.lineTo(toolRect.bottomRight() + QPoint(0, -5));
            p.lineTo(toolRect.bottomRight() + QPoint(-5, 0));
            gc.fillPath(p, Qt::black);
        }

        QRect iconRect = toolRect.adjusted(m_padding, m_padding, -m_padding, -m_padding);
        m_tools[i].icon().paint(&gc, iconRect);
        m_toolRects.push_back(toolRect);
        topLeft += QPoint(0, m_buttonSize.height());

        if(breakCount < m_breaks.count() && m_breaks[breakCount] == i){
            topLeft += QPoint(0, 2);
            breakCount++;
        }

    }

    QPoint subTopLeft(0,0);
    if(m_longPressed){
        QVector<Tool> tools = m_tools[m_activated].subTools();
        m_secondaryTools.clear();

        if(!tools.isEmpty()){
            subTopLeft = QPoint(m_toolRects[m_activated].topRight());
            for(int i=0; i<tools.count(); i++){
                QRect toolRect(subTopLeft, m_buttonSize + QSize(130, 0));
                if(m_subToolActivated == i){
                    gc.fillRect(toolRect, highlightedBack);
                }else{
                    gc.fillRect(toolRect, back);
                }
                QRect iconRect = toolRect.adjusted(m_padding, m_padding, -m_padding-130, -m_padding);
                tools[i].icon().paint(&gc, iconRect);
                gc.setPen(Qt::white);
                gc.drawText(iconRect.bottomRight() + QPoint(10, -5), tools[i].name());
                subTopLeft += QPoint(0, m_buttonSize.height());
                m_secondaryTools.push_back(toolRect);
            }
        }
    }

    int w = m_buttonSize.width();
    int h = topLeft.y() + m_buttonSize.height() + 2;

    if(m_longPressed){
        w = w*2 + 130;
        if(subTopLeft.y() > topLeft.y()){
            h = subTopLeft.y() + m_buttonSize.height() + 2;
        }
    }

    resize(w, h);
}

void ToolBox::addTool(Tool t)
{
    m_tools.push_back(t);
}

void ToolBox::addBreak()
{
    m_breaks.push_back(m_tools.count()-1);
}

void ToolBox::mousePressEvent(QMouseEvent *event)
{
    QPoint pos = event->pos();
    m_lastMousePos = pos;

    if(event->button() != Qt::LeftButton)
        return;

    if(m_toolBoxState == Unlocked){
        m_toolBoxHandlePressed = true;
        return;
    }

    for(int i=0; i<m_tools.count(); i++){
        if(m_toolRects[i].contains(pos)){
            m_activated = i;
            m_timer.start();
            update();
            return;
        }
    }
}

void ToolBox::mouseMoveEvent(QMouseEvent *event)
{
    QPoint pos = event->pos();

    if(m_toolBoxState == Unlocked){
        setCursor(Qt::OpenHandCursor);
        if(m_toolBoxHandlePressed){
            move(mapToParent(event->pos() - m_lastMousePos));
            setCursor(Qt::ClosedHandCursor);
            return;
        }
    }else{
        setCursor(Qt::ArrowCursor);
    }

    if(m_longPressed) {
        for(int i=0; i<m_secondaryTools.count(); i++){
            if(m_secondaryTools[i].contains(pos)){
                m_subToolActivated = i;
                update();
                return;
            }
        }
        m_subToolActivated = -1;
    }
}

void ToolBox::mouseReleaseEvent(QMouseEvent *event)
{
    Q_UNUSED(event)

    m_longPressed = false;

    if(m_subToolActivated >= 0){
        m_tools[m_activated].swap(m_subToolActivated);
    }
    m_timer.stop();
    m_subToolActivated = -1;
    m_toolBoxHandlePressed = false;
    update();
    setCursor(Qt::ArrowCursor);
}

void ToolBox::longPressEvent()
{
    m_longPressed = true && !m_toolBoxHandlePressed;
    update();
}

void ToolBox::showContextMenu(QPoint pos)
{
    m_contextMenu->exec(mapToGlobal(pos));
}

void ToolBox::slotChangeRowMode()
{
    if(m_rowMode == Single){
        m_rowMode = Double;
        m_flipAction->setEnabled(false);
    }else{
        m_rowMode = Single;
        m_flipAction->setEnabled(true);
    }
    update();
}

void ToolBox::slotRotateToolBox()
{
    if(m_toolBoxOrientation == Horizontal){
        m_toolBoxOrientation = Vertical;
    }else{
        m_toolBoxOrientation = Horizontal;
    }
    update();
}

void ToolBox::slotUnlockToolBox()
{
    if(m_toolBoxState == Locked){
        m_toolBoxState = Unlocked;
    }else{
        m_toolBoxState = Locked;
    }
    m_subToolActivated = -1;
    update();
}

void ToolBox::slotFlipToolBox()
{
    if(m_subToolsOrientation == Left){
        m_subToolsOrientation = Right;
    }else{
        m_subToolsOrientation = Left;
    }
    update();
}
