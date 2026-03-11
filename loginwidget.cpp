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

LoginWidget::LoginWidget(QWidget* parent) : QWidget(parent) {
    setupUI();
    m_bgPixmap.load("assets/login_bg.jpg");
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

void LoginWidget::setupUI() {
    setAttribute(Qt::WA_StyledBackground, true);

    QVBoxLayout* outer = new QVBoxLayout(this);
    outer->setContentsMargins(24,24,24,24);
    outer->setSpacing(14);
    outer->setAlignment(Qt::AlignCenter);

    QLabel* heroTitle = new QLabel("Smart Home", this);
    heroTitle->setAlignment(Qt::AlignCenter);
    heroTitle->setStyleSheet("color:white;font-size:38px;font-weight:700;letter-spacing:2px;");
    outer->addWidget(heroTitle);

    QLabel* heroSub = new QLabel("智能家居监控平台", this);
    heroSub->setAlignment(Qt::AlignCenter);
    heroSub->setStyleSheet("color:rgba(255,255,255,210);font-size:15px;");
    outer->addWidget(heroSub);

    QGroupBox* box = new QGroupBox("", this);
    box->setFixedWidth(420);
    box->setStyleSheet(
        "QGroupBox{"
        "font-size:18px;font-weight:700;padding:16px;"
        "margin-top:1px;"
        "padding-top:5px;"
        "border:1px solid rgba(255,255,255,255);"
        "border-radius:14px;"
        "background:rgba(255,255,255,215);"
        "color:#16313f;}"
        );

    QFormLayout* form = new QFormLayout(box);
    form->setSpacing(14);
    form->setContentsMargins(24,30,24,22);

    QString editStyle =
        "QLineEdit{border:1px solid #c7d5dd;border-radius:8px;padding:6px 10px;background:white;}"
        "QLineEdit:focus{border:1px solid #2a6f97;}";

    m_userEdit = new QLineEdit(box);
    m_userEdit->setPlaceholderText("请输入用户名");
    m_userEdit->setText("admin");
    m_userEdit->setFixedHeight(38);
    m_userEdit->setStyleSheet(editStyle);

    m_passEdit = new QLineEdit(box);
    m_passEdit->setPlaceholderText("请输入密码");
    m_passEdit->setText("admin123");
    m_passEdit->setEchoMode(QLineEdit::Password);
    m_passEdit->setFixedHeight(38);
    m_passEdit->setStyleSheet(editStyle);

    form->addRow("用户名:", m_userEdit);
    form->addRow("密  码:", m_passEdit);

    m_statusLbl = new QLabel("", box);
    m_statusLbl->setStyleSheet("color:#c0392b; font-size:12px;");
    m_statusLbl->setAlignment(Qt::AlignCenter);
    form->addRow(m_statusLbl);

    QString btnStyle =
        "QPushButton{background:#1b4965;color:white;border:none;border-radius:8px;padding:8px;font-size:14px;font-weight:600;}"
        "QPushButton:hover{background:#245f84;}"
        "QPushButton:pressed{background:#15384d;}";

    QPushButton* loginBtn = new QPushButton("登  录", box);
    loginBtn->setStyleSheet(btnStyle);
    loginBtn->setFixedHeight(40);
    connect(loginBtn, &QPushButton::clicked, this, &LoginWidget::onLogin);
    connect(m_passEdit, &QLineEdit::returnPressed, this, &LoginWidget::onLogin);
    form->addRow(loginBtn);

    QHBoxLayout* linkRow = new QHBoxLayout();
    QPushButton* regBtn   = new QPushButton("注册账号", box);
    QPushButton* resetBtn = new QPushButton("忘记密码", box);
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
    form->addRow(linkRow);

    outer->addWidget(box);

    QLabel* hint = new QLabel("默认账号 admin / 密码 admin123", this);
    hint->setAlignment(Qt::AlignCenter);
    hint->setStyleSheet("color:rgba(255,255,255,190);font-size:11px;margin-top:8px;");
    outer->addWidget(hint);
}

void LoginWidget::onLogin() {
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
