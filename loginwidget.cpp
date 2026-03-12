#include "loginwidget.h"
#include "databasemanager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QPushButton>
#include <QGroupBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QMessageBox>
#include <QPainter>
#include <QPaintEvent>
#include <QFileInfo>
#include <QResizeEvent>
#include <QFont>

// 登录页：负责登录校验、注册和重置密码入口。

LoginWidget::LoginWidget(QWidget* parent) : QWidget(parent) {
    setupUI();
    m_bgPixmap.load("assets/login_bg.jpg");
    applyResponsiveScale();
}

void LoginWidget::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);

    if (!m_bgPixmap.isNull()) {
        p.drawPixmap(rect(), m_bgPixmap);
        p.fillRect(rect(), QColor(0, 0, 0, 90));
        return;
    }

    QLinearGradient g(0, 0, width(), height());
    g.setColorAt(0.0, QColor("#0f2027"));
    g.setColorAt(0.5, QColor("#203a43"));
    g.setColorAt(1.0, QColor("#2c5364"));
    p.fillRect(rect(), g);

    p.setBrush(QColor(255, 255, 255, 35));
    p.setPen(Qt::NoPen);
    p.drawEllipse(QPoint(width() * 0.20, height() * 0.25), 120, 120);
    p.drawEllipse(QPoint(width() * 0.80, height() * 0.75), 160, 160);
    p.drawEllipse(QPoint(width() * 0.75, height() * 0.20), 70, 70);
}

void LoginWidget::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);
    applyResponsiveScale();
}

void LoginWidget::applyResponsiveScale() {
    const double sx = width() / 1200.0;
    const double sy = height() / 750.0;
    const double scale = qBound(0.60, qMin(sx, sy), 1.70);

    if (m_loginBox) {
        const int boxW = qBound(240, static_cast<int>(420 * scale), 760);
        const int boxH = qBound(250, static_cast<int>(320 * scale), 560);
        m_loginBox->setFixedWidth(boxW);
        m_loginBox->setFixedHeight(boxH);
    }

    if (m_formLayout) {
        const int sp = qBound(8, static_cast<int>(14 * scale), 20);
        const int ml = qBound(12, static_cast<int>(24 * scale), 40);
        const int mt = qBound(14, static_cast<int>(30 * scale), 50);
        const int mr = ml;
        const int mb = qBound(10, static_cast<int>(22 * scale), 36);
        m_formLayout->setSpacing(sp);
        m_formLayout->setContentsMargins(ml, mt, mr, mb);
    }

    if (m_heroTitle) {
        QFont f = m_heroTitle->font();
        f.setPointSize(qBound(22, static_cast<int>(34 * scale), 48));
        f.setBold(true);
        m_heroTitle->setFont(f);
    }

    if (m_heroSub) {
        QFont f = m_heroSub->font();
        f.setPointSize(qBound(11, static_cast<int>(14 * scale), 20));
        m_heroSub->setFont(f);
    }

    if (m_hintLbl) {
        QFont f = m_hintLbl->font();
        f.setPointSize(qBound(9, static_cast<int>(11 * scale), 14));
        m_hintLbl->setFont(f);
    }

    if (m_userEdit) {
        m_userEdit->setFixedHeight(qBound(32, static_cast<int>(38 * scale), 48));
    }
    if (m_passEdit) {
        m_passEdit->setFixedHeight(qBound(32, static_cast<int>(38 * scale), 48));
    }
    if (m_loginBtn) {
        m_loginBtn->setFixedHeight(qBound(34, static_cast<int>(40 * scale), 52));
    }
}

void LoginWidget::setupUI() {
    setAttribute(Qt::WA_StyledBackground, true);

    QVBoxLayout* outer = new QVBoxLayout(this);
    outer->setContentsMargins(24,24,24,24);
    outer->setSpacing(14);
    outer->setAlignment(Qt::AlignCenter);

    m_heroTitle = new QLabel("Smart Home", this);
    m_heroTitle->setAlignment(Qt::AlignCenter);
    m_heroTitle->setStyleSheet("color:white;font-weight:700;letter-spacing:2px;");
    outer->addWidget(m_heroTitle);

    m_heroSub = new QLabel("智能家居监控平台", this);
    m_heroSub->setAlignment(Qt::AlignCenter);
    m_heroSub->setStyleSheet("color:rgba(255,255,255,210);");
    outer->addWidget(m_heroSub);

    m_loginBox = new QGroupBox("", this);
    m_loginBox->setFixedWidth(420);
    m_loginBox->setStyleSheet(
        "QGroupBox{"
        "font-size:18px;font-weight:700;padding:16px;"
        "margin-top:1px;"
        "padding-top:5px;"
        "border:1px solid rgba(255,255,255,255);"
        "border-radius:14px;"
        "background:rgba(255,255,255,215);"
        "color:#16313f;}"
        );

    m_formLayout = new QFormLayout(m_loginBox);
    m_formLayout->setSpacing(14);
    m_formLayout->setContentsMargins(24,30,24,22);

    QString editStyle =
        "QLineEdit{border:1px solid #c7d5dd;border-radius:8px;padding:6px 10px;background:white;}"
        "QLineEdit:focus{border:1px solid #2a6f97;}";

    m_userEdit = new QLineEdit(m_loginBox);
    m_userEdit->setPlaceholderText("请输入用户名");
    m_userEdit->setText("admin");
    m_userEdit->setFixedHeight(38);
    m_userEdit->setStyleSheet(editStyle);

    m_passEdit = new QLineEdit(m_loginBox);
    m_passEdit->setPlaceholderText("请输入密码");
    m_passEdit->setText("admin123");
    m_passEdit->setEchoMode(QLineEdit::Password);
    m_passEdit->setFixedHeight(38);
    m_passEdit->setStyleSheet(editStyle);

    m_formLayout->addRow("用户名:", m_userEdit);
    m_formLayout->addRow("密  码:", m_passEdit);

    m_statusLbl = new QLabel("", m_loginBox);
    m_statusLbl->setStyleSheet("color:#c0392b; font-size:12px;");
    m_statusLbl->setAlignment(Qt::AlignCenter);
    m_formLayout->addRow(m_statusLbl);

    QString btnStyle =
        "QPushButton{background:#1b4965;color:white;border:none;border-radius:8px;padding:8px;font-size:14px;font-weight:600;}"
        "QPushButton:hover{background:#245f84;}"
        "QPushButton:pressed{background:#15384d;}";

    m_loginBtn = new QPushButton("登  录", m_loginBox);
    m_loginBtn->setStyleSheet(btnStyle);
    m_loginBtn->setFixedHeight(40);
    connect(m_loginBtn, &QPushButton::clicked, this, &LoginWidget::onLogin);
    connect(m_passEdit, &QLineEdit::returnPressed, this, &LoginWidget::onLogin);
    m_formLayout->addRow(m_loginBtn);

    QHBoxLayout* linkRow = new QHBoxLayout();
    QPushButton* regBtn   = new QPushButton("注册账号", m_loginBox);
    QPushButton* resetBtn = new QPushButton("忘记密码", m_loginBox);
    regBtn->setFlat(true);
    resetBtn->setFlat(true);
    QString linkStyle = "QPushButton{color:#2980b9;font-size:12px;}"
                        "QPushButton:hover{color:#154360;}";
    regBtn->setStyleSheet(linkStyle);
    resetBtn->setStyleSheet(linkStyle);
    connect(regBtn,   &QPushButton::clicked, this, &LoginWidget::onRegister);
    connect(resetBtn, &QPushButton::clicked, this, &LoginWidget::onResetPassword);
    linkRow->addWidget(regBtn);
    linkRow->addStretch();
    linkRow->addWidget(resetBtn);
    m_formLayout->addRow(linkRow);

    outer->addWidget(m_loginBox);

    m_hintLbl = new QLabel("默认账号 admin / 密码 admin123", this);
    m_hintLbl->setAlignment(Qt::AlignCenter);
    m_hintLbl->setStyleSheet("color:rgba(255,255,255,190);font-size:11px;margin-top:8px;");
    outer->addWidget(m_hintLbl);
}

void LoginWidget::onLogin() {
    // 先做输入校验，再调用数据库层进行账号密码验证。
    QString user = m_userEdit->text().trimmed();
    QString pass = m_passEdit->text();
    if (user.isEmpty() || pass.isEmpty()) {
        m_statusLbl->setText("用户名和密码不能为空");
        return;
    }
    if (DatabaseManager::instance()->validateUser(user, pass)) {
        emit loginSuccess(user);
    } else {
        m_statusLbl->setText("用户名或密码错误，请重试");
        m_passEdit->clear();
    }
}

void LoginWidget::onRegister() {
    // 注册流程：校验两次密码一致后写入 users 表。
    QDialog dlg(this);
    dlg.setWindowTitle("注册新账号");
    dlg.setFixedWidth(300);
    QFormLayout* f = new QFormLayout(&dlg);
    QLineEdit* uEdit = new QLineEdit(&dlg);
    QLineEdit* pEdit = new QLineEdit(&dlg);
    QLineEdit* p2Edit= new QLineEdit(&dlg);
    pEdit->setEchoMode(QLineEdit::Password);
    p2Edit->setEchoMode(QLineEdit::Password);
    f->addRow("用户名:", uEdit);
    f->addRow("密  码:", pEdit);
    f->addRow("确认密码:", p2Edit);
    QDialogButtonBox* bb = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dlg);
    f->addRow(bb);
    connect(bb, &QDialogButtonBox::accepted, &dlg, [&]{
        if (pEdit->text() != p2Edit->text()) {
            QMessageBox::warning(&dlg, "错误", "两次密码不一致");
            return;
        }
        if (DatabaseManager::instance()->addUser(uEdit->text(), pEdit->text())) {
            QMessageBox::information(&dlg, "成功", "注册成功，请登录");
            dlg.accept();
        } else {
            QMessageBox::warning(&dlg, "错误", "注册失败，用户名可能已存在");
        }
    });
    connect(bb, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
    dlg.exec();
}

void LoginWidget::onResetPassword() {
    // 重置流程：校验两次新密码一致后更新数据库。
    QDialog dlg(this);
    dlg.setWindowTitle("重置密码");
    dlg.setFixedWidth(300);
    QFormLayout* f = new QFormLayout(&dlg);
    QLineEdit* uEdit  = new QLineEdit(&dlg);
    QLineEdit* p1Edit = new QLineEdit(&dlg);
    QLineEdit* p2Edit = new QLineEdit(&dlg);
    p1Edit->setEchoMode(QLineEdit::Password);
    p2Edit->setEchoMode(QLineEdit::Password);
    f->addRow("用户名:", uEdit);
    f->addRow("新密码:", p1Edit);
    f->addRow("确认密码:", p2Edit);
    QDialogButtonBox* bb = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dlg);
    f->addRow(bb);
    connect(bb, &QDialogButtonBox::accepted, &dlg, [&]{
        if (p1Edit->text() != p2Edit->text()) {
            QMessageBox::warning(&dlg, "错误", "两次密码不一致"); return;
        }
        if (DatabaseManager::instance()->resetPassword(uEdit->text(), p1Edit->text())) {
            QMessageBox::information(&dlg, "成功", "密码已重置");
            dlg.accept();
        } else {
            QMessageBox::warning(&dlg, "错误", "用户名不存在");
        }
    });
    connect(bb, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
    dlg.exec();
}
