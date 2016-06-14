/***************************************************************************
 *   Copyright (C) 2006-2008, 2014, 2016 by Hanna Knutsson                 *
 *   hanna_k@fmgirl.com                                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <QButtonGroup>
#include <QCheckBox>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QDesktopWidget>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QLayout>
#include <QMenu>
#include <QObject>
#include <QPainter>
#include <QPixmap>
#include <QPushButton>
#include <QRadioButton>
#include <QTextStream>
#include <QTimer>
#include <QVBoxLayout>
#include <QAction>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QUrl>
#include <QFileDialog>
#include <QTabWidget>
#include <QSaveFile>
#include <QSpinBox>
#include <QMimeType>
#include <QMimeData>
#include <QMimeDatabase>
#include <QTemporaryFile>
#include <QCommandLineParser>
#include <QDateEdit>
#include <QCalendarWidget>

#include <KConfigGroup>
#include <KSharedConfig>
#include <kactioncollection.h>
#include <kautosavefile.h>
#include <kconfig.h>
#include <kdeversion.h>
#include <khtml_part.h>
#include <khtmlview.h>
#include <kfileitem.h>
#include <kmessagebox.h>
#include <kpagewidget.h>
#include <krecentfilesaction.h>
#include <kselectaction.h>
#include <kstandardaction.h>
#include <kstdaccel.h>
#include <kstdguiitem.h>
#include <kseparator.h>
#include <ktextedit.h>
#include <klocalizedstring.h>
#include <kwindowconfig.h>
#include <kio/filecopyjob.h>
#include <kjobwidgets.h>

#include "budget.h"
#include "categoriescomparisonchart.h"
#include "categoriescomparisonreport.h"
#include "editscheduledtransactiondialog.h"
#include "editsplitdialog.h"
#include "eqonomize.h"
#include "eqonomizemonthselector.h"
#include "eqonomizevalueedit.h"
#include "importcsvdialog.h"
#include "ledgerdialog.h"
#include "overtimechart.h"
#include "overtimereport.h"
#include "qifimportexport.h"
#include "recurrence.h"
#include "recurrenceeditwidget.h"
#include "security.h"
#include "transactionlistwidget.h"
#include "transactioneditwidget.h"

#include <cmath>

QTreeWidgetItem *selectedItem(QTreeWidget *w) {
	QList<QTreeWidgetItem*> list = w->selectedItems();
	if(list.isEmpty()) return NULL;
	return list.first();
}

double averageMonth(const QDate &date1, const QDate &date2) {
	double average_month = (double) date1.daysInYear() / (double) 12;
	int years = 1;
	QDate ydate;
	ydate.setDate(date1.year(), 1, 1);
	ydate = ydate.addYears(1);
	while(ydate.year() <= date2.year()) {
		average_month += (double) ydate.daysInYear() / (double) 12;
		years++;
		ydate = ydate.addYears(1);
	}
	return average_month / years;
}
double averageYear(const QDate &date1, const QDate &date2) {
	double average_year = date1.daysInYear();
	int years = 1;
	QDate ydate;
	ydate.setDate(date1.year(), 1, 1);
	ydate = ydate.addYears(1);
	while(ydate.year() <= date2.year()) {
		average_year += ydate.daysInYear();
		years++;
		ydate = ydate.addYears(1);
	}
	return average_year / years;
}
double yearsBetweenDates(const QDate &date1, const QDate &date2) {
	double years = 0.0;
	if(date1.year() == date2.year()) {
		int days = date1.daysTo(date2) + 1;
		years = (double) days / (double) date2.daysInYear();
	} else {
		QDate yeardate;
		yeardate.setDate(date1.year(), 1, 1);
		years += (1.0 - (date1.dayOfYear() - 1.0) / (double) date1.daysInYear());
		yeardate = yeardate.addYears(1);
		while(yeardate.year() != date2.year()) {
			years += 1.0;
			yeardate = yeardate.addYears(1);
		}
		years += (double) date2.dayOfYear() / (double) date2.daysInYear();
	}
	return years;
}
double monthsBetweenDates(const QDate &date1, const QDate &date2) {
	double months = 0.0;
	if(date1.year() == date2.year()) {
		if(date1.month() == date2.month()) {
			int days = date1.daysTo(date2) + 1;
			months = (double) days / (double) date2.daysInMonth();
		} else {
			QDate monthdate;
			monthdate.setDate(date1.year(), date1.month(), 1);
			months += (1.0 - ((date1.day() - 1.0) / (double) date1.daysInMonth()));
			months += (date2.month() - date1.month() - 1);
			monthdate.setDate(date2.year(), date2.month(), 1);
			months += (double) date2.day() / (double) date2.daysInMonth();
		}
	} else {
		QDate monthdate;
		monthdate.setDate(date1.year(), date1.month(), 1);
		months += (1.0 - ((date1.day() - 1.0) / (double) date1.daysInMonth()));
		months += (12 - date1.month());
		QDate yeardate;
		yeardate.setDate(date1.year(), 1, 1);
		yeardate = yeardate.addYears(1);
		while(yeardate.year() != date2.year()) {
			months += 12;
			yeardate = yeardate.addYears(1);
		}
		monthdate.setDate(date2.year(), date2.month(), 1);
		months += (double) date2.day() / (double) date2.daysInMonth();
		months += date2.month() - 1;
	}
	return months;
}

class SecurityListViewItem : public QTreeWidgetItem {
	protected:
		Security *o_security;
	public:
		SecurityListViewItem(Security *sec, QString s1, QString s2 = QString::null, QString s3 = QString::null, QString s4 = QString::null, QString s5 = QString::null, QString s6 = QString::null, QString s7 = QString::null, QString s8 = QString::null) : QTreeWidgetItem(UserType), o_security(sec) {
			setText(0, s1);
			setText(1, s2);
			setText(2, s3);
			setText(3, s4);
			setText(4, s5);
			setText(5, s6);
			setText(6, s7);
			setText(7, s8);
			setTextAlignment(1, Qt::AlignRight);
			setTextAlignment(2, Qt::AlignRight);
			setTextAlignment(3, Qt::AlignRight);
			setTextAlignment(4, Qt::AlignRight);
			setTextAlignment(5, Qt::AlignRight);
			setTextAlignment(6, Qt::AlignRight);
			setTextAlignment(7, Qt::AlignCenter);
			setTextAlignment(8, Qt::AlignCenter);
		}
		bool operator<(const QTreeWidgetItem &i_pre) const {
			int col = 0;
			if(treeWidget()) col = treeWidget()->sortColumn();
			SecurityListViewItem *i = (SecurityListViewItem*) &i_pre;
			if(col >= 2 && col <= 6) {
				double d1 = text(col).toDouble();
				double d2 = i->text(col).toDouble();
				return d1 < d2;
			}
			return QTreeWidgetItem::operator<(i_pre);
		}
		Security* security() const {return o_security;}
		double cost, value, rate, profit;
};

class ScheduleListViewItem : public QTreeWidgetItem {
	protected:
		ScheduledTransaction *o_strans;
		QDate d_date;
	public:
		ScheduleListViewItem(ScheduledTransaction *strans, const QDate &trans_date);
		bool operator<(const QTreeWidgetItem &i_pre) const;
		ScheduledTransaction *scheduledTransaction() const;
		void setScheduledTransaction(ScheduledTransaction *strans);
		const QDate &date() const;
		void setDate(const QDate &newdate);
};

ScheduleListViewItem::ScheduleListViewItem(ScheduledTransaction *strans, const QDate &trans_date) : QTreeWidgetItem(UserType) {
	setTextAlignment(3, Qt::AlignRight);
	setTextAlignment(4, Qt::AlignCenter);
	setTextAlignment(5, Qt::AlignCenter);
	setScheduledTransaction(strans);
	setDate(trans_date);
}
bool ScheduleListViewItem::operator<(const QTreeWidgetItem &i_pre) const {
	int col = 0;
	if(treeWidget()) col = treeWidget()->sortColumn();
	ScheduleListViewItem *i = (ScheduleListViewItem*) &i_pre;
	if(col == 0) {
		return d_date < i->date();
	} else if(col == 3) {
		return o_strans->transaction()->value() < i->scheduledTransaction()->transaction()->value();
	}
	return QTreeWidgetItem::operator<(i_pre);
}
ScheduledTransaction *ScheduleListViewItem::scheduledTransaction() const {
	return o_strans;
}
const QDate &ScheduleListViewItem::date() const {
	return d_date;
}
void ScheduleListViewItem::setDate(const QDate &newdate) {
	d_date = newdate;
	setText(0, QLocale().toString(d_date, QLocale::ShortFormat));
}
void ScheduleListViewItem::setScheduledTransaction(ScheduledTransaction *strans) {
	o_strans = strans;
	Transaction *trans = strans->transaction();
	setText(2, trans->description());
	setText(3, QLocale().toCurrencyString(trans->value()));
	setText(4, trans->fromAccount()->name());
	setText(5, trans->toAccount()->name());
	setText(6, trans->comment());
	switch(trans->type()) {
		case TRANSACTION_TYPE_TRANSFER: {setText(1, i18n("Transfer")); break;}
		case TRANSACTION_TYPE_INCOME: {
			if(((Income*) trans)->security()) setText(1, i18n("Dividend"));
			else setText(1, i18n("Income"));
			break;
		}
		case TRANSACTION_TYPE_EXPENSE: {setText(1, i18n("Expense")); break;}
		case TRANSACTION_TYPE_SECURITY_BUY: {setText(1, i18n("Security Buy")); break;}
		case TRANSACTION_TYPE_SECURITY_SELL: {setText(1, i18n("Security Sell")); break;}
	}
}

class ConfirmScheduleListViewItem : public QTreeWidgetItem {
	protected:
		Transaction *o_trans;
	public:
		ConfirmScheduleListViewItem(Transaction *trans);
		bool operator<(const QTreeWidgetItem &i_pre) const;
		Transaction *transaction() const;
		void setTransaction(Transaction *trans);
};

ConfirmScheduleListViewItem::ConfirmScheduleListViewItem(Transaction *trans) : QTreeWidgetItem(UserType) {
	setTransaction(trans);
	setTextAlignment(3, Qt::AlignRight);
}
bool ConfirmScheduleListViewItem::operator<(const QTreeWidgetItem &i_pre) const {
	int col = 0;
	if(treeWidget()) col = treeWidget()->sortColumn();
	ConfirmScheduleListViewItem *i = (ConfirmScheduleListViewItem*) &i_pre;
	if(col == 0) {
		return o_trans->date() < i->transaction()->date();
	} else if(col == 3) {
		return o_trans->value() < i->transaction()->value();
	}
	return QTreeWidgetItem::operator<(i_pre);
}
Transaction *ConfirmScheduleListViewItem::transaction() const {
	return o_trans;
}
void ConfirmScheduleListViewItem::setTransaction(Transaction *trans) {
	o_trans = trans;
	setText(0, QLocale().toString(trans->date(), QLocale::ShortFormat));
	switch(trans->type()) {
		case TRANSACTION_TYPE_TRANSFER: {setText(1, i18n("Transfer")); break;}
		case TRANSACTION_TYPE_INCOME: {
			if(((Income*) trans)->security()) setText(1, i18n("Dividend"));
			else setText(1, i18n("Income"));
			break;
		}
		case TRANSACTION_TYPE_EXPENSE: {setText(1, i18n("Expense")); break;}
		case TRANSACTION_TYPE_SECURITY_BUY: {setText(1, i18n("Security Buy")); break;}
		case TRANSACTION_TYPE_SECURITY_SELL: {setText(1, i18n("Security Sell")); break;}
	}
	setText(2, trans->description());
	setText(3, QLocale().toCurrencyString(trans->value()));
}


class SecurityTransactionListViewItem : public QTreeWidgetItem {
	public:
		SecurityTransactionListViewItem(QString, QString=QString::null, QString=QString::null, QString=QString::null);
		bool operator<(const QTreeWidgetItem &i_pre) const;
		SecurityTransaction *trans;
		Income *div;
		ReinvestedDividend *rediv;
		ScheduledTransaction *strans, *sdiv;
		SecurityTrade *ts;
		QDate date;
		double shares, value;
};

SecurityTransactionListViewItem::SecurityTransactionListViewItem(QString s1, QString s2, QString s3, QString s4) : QTreeWidgetItem(UserType), trans(NULL), div(NULL), rediv(NULL), strans(NULL), sdiv(NULL), ts(NULL), shares(-1.0), value(-1.0) {
	setText(0, s1);
	setText(1, s2);
	setText(2, s3);
	setText(3, s4);
	setTextAlignment(0, Qt::AlignLeft);
	setTextAlignment(1, Qt::AlignLeft);
	setTextAlignment(2, Qt::AlignRight);
	setTextAlignment(3, Qt::AlignRight);
}
bool SecurityTransactionListViewItem::operator<(const QTreeWidgetItem &i_pre) const {
	int col = 0;
	if(treeWidget()) col = treeWidget()->sortColumn();
	SecurityTransactionListViewItem *i = (SecurityTransactionListViewItem*) &i_pre;
	if(col == 0) {
		return date < i->date;
	} else if(col == 2) {
		return value < i->value;
	} else if(col == 2) {
		return shares < i->shares;
	}
	return QTreeWidgetItem::operator<(i_pre);
}
class QuotationListViewItem : public QTreeWidgetItem {
	public:
		QuotationListViewItem(const QDate &date_, double value_);
		bool operator<(const QTreeWidgetItem &i_pre) const;
		QDate date;
		double value;
};

QuotationListViewItem::QuotationListViewItem(const QDate &date_, double value_) : QTreeWidgetItem(UserType), date(date_), value(value_) {
	setText(0, QLocale().toString(date, QLocale::ShortFormat));
	setText(1, QLocale().toCurrencyString(value));
	setTextAlignment(0, Qt::AlignRight);
	setTextAlignment(1, Qt::AlignRight);
}
bool QuotationListViewItem::operator<(const QTreeWidgetItem &i_pre) const {
	QuotationListViewItem *i = (QuotationListViewItem*) &i_pre;
	if(!treeWidget() || treeWidget()->sortColumn() == 0) {
		return date < i->date;
	} else {
		return value < i->value;
	}
	return QTreeWidgetItem::operator<(i_pre);
}

RefundDialog::RefundDialog(Transaction *trans, QWidget *parent) : QDialog(parent, 0), transaction(trans) {

	if(trans->type() == TRANSACTION_TYPE_INCOME) setWindowTitle(i18n("Repayment"));
	else setWindowTitle(i18n("Refund"));
	setModal(true);

	QVBoxLayout *box1 = new QVBoxLayout(this);
	
	QGridLayout *layout = new QGridLayout();
	box1->addLayout(layout);

	layout->addWidget(new QLabel(i18n("Date:"), this), 0, 0);
	dateEdit = new QDateEdit(this);
	dateEdit->setCalendarPopup(true);
	layout->addWidget(dateEdit, 0, 1);
	dateEdit->setFocus();

	if(trans->type() == TRANSACTION_TYPE_INCOME) layout->addWidget(new QLabel(i18n("Cost:"), this), 1, 0);
	else layout->addWidget(new QLabel(i18n("Income:"), this), 1, 0);
	valueEdit = new EqonomizeValueEdit(trans->value(), false, true, this);
	layout->addWidget(valueEdit, 1, 1);

	layout->addWidget(new QLabel(i18n("Account:"), this), 2, 0);
	accountCombo = new QComboBox(this);
	accountCombo->setEditable(false);
	int i = 0;
	AssetsAccount *account = transaction->budget()->assetsAccounts.first();
	while(account) {
		if(account != transaction->budget()->balancingAccount && account->accountType() != ASSETS_TYPE_SECURITIES) {
			accountCombo->addItem(account->name());
			if((transaction->type() == TRANSACTION_TYPE_EXPENSE && account == ((Expense*) transaction)->from()) || (transaction->type() == TRANSACTION_TYPE_INCOME && account == ((Income*) transaction)->to())) accountCombo->setCurrentIndex(i);
			i++;
		}
		account = transaction->budget()->assetsAccounts.next();
	}
	layout->addWidget(accountCombo, 2, 1);

	layout->addWidget(new QLabel(i18n("Quantity:"), this), 3, 0);
	quantityEdit = new EqonomizeValueEdit(trans->quantity(), 2, false, false, this);
	layout->addWidget(quantityEdit, 3, 1);

	layout->addWidget(new QLabel(i18n("Comments:"), this), 4, 0);
	commentsEdit = new QLineEdit(this);
	if(trans->type() == TRANSACTION_TYPE_INCOME) commentsEdit->setText(i18n("Repayment"));
	else commentsEdit->setText(i18n("Refund"));
	layout->addWidget(commentsEdit, 4, 1);
	
	QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
	buttonBox->button(QDialogButtonBox::Ok)->setShortcut(Qt::CTRL | Qt::Key_Return);
	buttonBox->button(QDialogButtonBox::Ok)->setDefault(true);
	connect(buttonBox->button(QDialogButtonBox::Cancel), SIGNAL(clicked()), this, SLOT(reject()));
	connect(buttonBox->button(QDialogButtonBox::Ok), SIGNAL(clicked()), this, SLOT(accept()));
	box1->addWidget(buttonBox);

}
Transaction *RefundDialog::createRefund() {
	if(!validValues()) return NULL;
	Transaction *trans = NULL;
	int i = 0;
	int cur_i = accountCombo->currentIndex();
	AssetsAccount *account = transaction->budget()->assetsAccounts.first();
	while(account) {
		if(account != transaction->budget()->balancingAccount && account->accountType() != ASSETS_TYPE_SECURITIES) {
			if(i == cur_i) break;
			i++;
		}
		account = transaction->budget()->assetsAccounts.next();
	}
	trans = transaction->copy();
	if(transaction->type() == TRANSACTION_TYPE_EXPENSE) {
		((Expense*) trans)->setFrom(account);
	} else {
		((Income*) trans)->setTo(account);
	}
	trans->setQuantity(-quantityEdit->value());
	trans->setValue(-valueEdit->value());
	trans->setDate(dateEdit->date());
	trans->setComment(commentsEdit->text());
	return trans;
}
void RefundDialog::accept() {
	if(validValues()) {
		QDialog::accept();
	}
}
bool RefundDialog::validValues() {
	if(!dateEdit->date().isValid()) {
		KMessageBox::error(this, i18n("Invalid date."));
		return false;
	}
	return true;
}


EditReinvestedDividendDialog::EditReinvestedDividendDialog(Budget *budg, Security *sec, bool select_security, QWidget *parent)  : QDialog(parent, 0), budget(budg) {

	setWindowTitle(i18n("Reinvested Dividend"));
	setModal(true);

	QVBoxLayout *box1 = new QVBoxLayout(this);
	QGridLayout *layout = new QGridLayout();
	box1->addLayout(layout);

	securityCombo = NULL;

	layout->addWidget(new QLabel(i18n("Security:"), this), 0, 0);
	if(select_security) {
		securityCombo = new QComboBox(this);
		securityCombo->setEditable(false);
		Security *c_sec = budget->securities.first();
		int i2 = 0;
		while(c_sec) {
			securityCombo->addItem(c_sec->name());
			if(c_sec == sec) securityCombo->setCurrentIndex(i2);
			c_sec = budget->securities.next();
			i2++;
		}
		layout->addWidget(securityCombo, 0, 1);
	} else {
		layout->addWidget(new QLabel(sec->name(), this), 0, 1);
	}

	layout->addWidget(new QLabel(i18n("Shares added:"), this), 1, 0);
	sharesEdit = new EqonomizeValueEdit(0.0, selectedSecurity()->decimals(), false, false, this);
	layout->addWidget(sharesEdit, 1, 1);
	sharesEdit->setFocus();

	layout->addWidget(new QLabel(i18n("Date:"), this), 2, 0);
	dateEdit = new QDateEdit(this);
	dateEdit->setCalendarPopup(true);
	layout->addWidget(dateEdit, 2, 1);
	
	QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
	buttonBox->button(QDialogButtonBox::Ok)->setShortcut(Qt::CTRL | Qt::Key_Return);
	buttonBox->button(QDialogButtonBox::Ok)->setDefault(true);
	connect(buttonBox->button(QDialogButtonBox::Cancel), SIGNAL(clicked()), this, SLOT(reject()));
	connect(buttonBox->button(QDialogButtonBox::Ok), SIGNAL(clicked()), this, SLOT(accept()));
	box1->addWidget(buttonBox);

	if(securityCombo) connect(securityCombo, SIGNAL(activated(int)), this, SLOT(securityChanged()));

}
Security *EditReinvestedDividendDialog::selectedSecurity() {
	if(securityCombo) {
		int i = securityCombo->currentIndex();
		if(i >= 0) {
			Security *c_sec = budget->securities.first();
			while(i > 0 && c_sec) {
				c_sec = budget->securities.next();
				i--;
			}
			return c_sec;
		}
	}
	return NULL;
}
void EditReinvestedDividendDialog::securityChanged() {
	if(selectedSecurity()) {
		if(sharesEdit) sharesEdit->setPrecision(selectedSecurity()->decimals());
	}
}
void EditReinvestedDividendDialog::setDividend(ReinvestedDividend *rediv) {
	dateEdit->setDate(rediv->date);
	sharesEdit->setValue(rediv->shares);
}
bool EditReinvestedDividendDialog::modifyDividend(ReinvestedDividend *rediv) {
	if(!validValues()) return false;
	rediv->date = dateEdit->date();
	rediv->shares = sharesEdit->value();
	return true;
}
ReinvestedDividend *EditReinvestedDividendDialog::createDividend() {
	if(!validValues()) return NULL;
	return new ReinvestedDividend(dateEdit->date(), sharesEdit->value());
}
void EditReinvestedDividendDialog::accept() {
	if(validValues()) {
		QDialog::accept();
	}
}
bool EditReinvestedDividendDialog::validValues() {
	if(!dateEdit->date().isValid()) {
		KMessageBox::error(this, i18n("Invalid date."));
		return false;
	}
	return true;
}

EditSecurityTradeDialog::EditSecurityTradeDialog(Budget *budg, Security *sec, QWidget *parent)  : QDialog(parent, 0), budget(budg) {

	setWindowTitle(i18n("Security Trade"));
	setModal(true);

	QVBoxLayout *box1 = new QVBoxLayout(this);
	QGridLayout *layout = new QGridLayout();
	box1->addLayout(layout);

	layout->addWidget(new QLabel(i18n("From security:"), this), 0, 0);
	fromSecurityCombo = new QComboBox(this);
	fromSecurityCombo->setEditable(false);
	Security *c_sec = budget->securities.first();
	if(!sec) sec = c_sec;
	int i = 0;
	while(c_sec) {
		fromSecurityCombo->addItem(c_sec->name());
		if(c_sec == sec) {
			fromSecurityCombo->setCurrentIndex(i);
		}
		c_sec = budget->securities.next();
		i++;
	}
	layout->addWidget(fromSecurityCombo, 0, 1);

	layout->addWidget(new QLabel(i18n("Shares moved:"), this), 1, 0);
	QHBoxLayout *sharesLayout = new QHBoxLayout();
	fromSharesEdit = new EqonomizeValueEdit(0.0, sec ? sec->shares() : 10000.0, 1.0, 0.0, sec ? sec->decimals() : 4, false, this);
	fromSharesEdit->setSizePolicy(QSizePolicy::Expanding, fromSharesEdit->sizePolicy().verticalPolicy());
	sharesLayout->addWidget(fromSharesEdit);
	QPushButton *maxSharesButton = new QPushButton(i18n("All"), this);
	maxSharesButton->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
	sharesLayout->addWidget(maxSharesButton);
	layout->addLayout(sharesLayout, 1, 1);
	fromSharesEdit->setFocus();

	layout->addWidget(new QLabel(i18n("To security:"), this), 2, 0);
	toSecurityCombo = new QComboBox(this);
	toSecurityCombo->setEditable(false);
	c_sec = budget->securities.first();
	bool sel = false;
	i = 0;
	while(c_sec) {
		toSecurityCombo->addItem(c_sec->name());
		if(sec && !sel && c_sec != sec && c_sec->account() == sec->account()) {
			sel = true;
			toSecurityCombo->setCurrentIndex(i);
		}
		c_sec = budget->securities.next();
		i++;
	}
	layout->addWidget(toSecurityCombo, 2, 1);

	layout->addWidget(new QLabel(i18n("Shares received:"), this), 3, 0);
	toSharesEdit = new EqonomizeValueEdit(0.0, sec ? sec->decimals() : 4, false, false, this);
	layout->addWidget(toSharesEdit, 3, 1);

	layout->addWidget(new QLabel(i18n("Value:"), this), 4, 0);
	valueEdit = new EqonomizeValueEdit(false, this);
	layout->addWidget(valueEdit, 4, 1);

	layout->addWidget(new QLabel(i18n("Date:"), this), 5, 0);
	dateEdit = new QDateEdit(this);
	dateEdit->setCalendarPopup(true);
	layout->addWidget(dateEdit, 5, 1);
	
	QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
	buttonBox->button(QDialogButtonBox::Ok)->setShortcut(Qt::CTRL | Qt::Key_Return);
	buttonBox->button(QDialogButtonBox::Ok)->setDefault(true);
	connect(buttonBox->button(QDialogButtonBox::Cancel), SIGNAL(clicked()), this, SLOT(reject()));
	connect(buttonBox->button(QDialogButtonBox::Ok), SIGNAL(clicked()), this, SLOT(accept()));
	box1->addWidget(buttonBox);

	toSecurityChanged();
	fromSecurityChanged();

	connect(maxSharesButton, SIGNAL(clicked()), this, SLOT(maxShares()));
	connect(fromSecurityCombo, SIGNAL(activated(int)), this, SLOT(fromSecurityChanged()));
	connect(toSecurityCombo, SIGNAL(activated(int)), this, SLOT(toSecurityChanged()));

}
void EditSecurityTradeDialog::maxShares() {
	fromSharesEdit->setValue(fromSharesEdit->maxValue());
}
Security *EditSecurityTradeDialog::selectedFromSecurity() {
	int index = fromSecurityCombo->currentIndex();
	Security *sec = budget->securities.first();
	int i = 0;
	while(sec) {
		if(i == index) {
			return sec;
		}
		i++;
		sec = budget->securities.next();
	}
	return NULL;
}
Security *EditSecurityTradeDialog::selectedToSecurity() {
	int index = toSecurityCombo->currentIndex();
	Security *sec = budget->securities.first();
	int i = 0;
	while(sec) {
		if(i == index) {
			return sec;
		}
		i++;
		sec = budget->securities.next();
	}
	return NULL;
}
void EditSecurityTradeDialog::fromSecurityChanged() {
	Security *sec = selectedFromSecurity();
	if(sec) {
		fromSharesEdit->setPrecision(sec->decimals());
		fromSharesEdit->setMaxValue(sec->shares());
	}
}
void EditSecurityTradeDialog::toSecurityChanged() {
	Security *sec = selectedToSecurity();
	if(sec) toSharesEdit->setPrecision(sec->decimals());
}
void EditSecurityTradeDialog::setSecurityTrade(SecurityTrade *ts) {
	dateEdit->setDate(ts->date);
	Security *sec = budget->securities.first();
	int i = 0;
	while(sec) {
		if(sec == ts->from_security) {
			fromSecurityCombo->setCurrentIndex(i);
			break;
		}
		i++;
		sec = budget->securities.next();
	}
	fromSharesEdit->setMaxValue(ts->from_security->shares() + ts->from_shares);
	fromSharesEdit->setValue(ts->from_shares);
	toSharesEdit->setValue(ts->to_shares);
	valueEdit->setValue(ts->value);
	sec = budget->securities.first();
	i = 0;
	while(sec) {
		if(sec == ts->to_security) {
			toSecurityCombo->setCurrentIndex(i);
			break;
		}
		i++;
		sec = budget->securities.next();
	}
}
SecurityTrade *EditSecurityTradeDialog::createSecurityTrade() {
	if(!validValues()) return NULL;
	return new SecurityTrade(dateEdit->date(), valueEdit->value(), fromSharesEdit->value(), selectedFromSecurity(), toSharesEdit->value(), selectedToSecurity());
}
bool EditSecurityTradeDialog::checkSecurities() {
	if(toSecurityCombo->count() < 2) {
		KMessageBox::error(this, i18n("No other security available for trade in the account."));
		return false;
	}
	return true;
}
void EditSecurityTradeDialog::accept() {
	if(validValues()) {
		QDialog::accept();
	}
}
bool EditSecurityTradeDialog::validValues() {
	if(selectedFromSecurity() == selectedToSecurity()) {
		KMessageBox::error(this, i18n("Selected to and from securities are the same."));
		return false;
	}
	if(!dateEdit->date().isValid()) {
		KMessageBox::error(this, i18n("Invalid date."));
		return false;
	}
	if(fromSharesEdit->value() == 0.0 || toSharesEdit->value() == 0.0) {
		KMessageBox::error(this, i18n("Zero shares not allowed."));
		return false;
	}
	if(valueEdit->value() == 0.0) {
		KMessageBox::error(this, i18n("Zero value not allowed."));
		return false;
	}
	return true;
}

EditQuotationsDialog::EditQuotationsDialog(QWidget *parent) : QDialog(parent, 0) {

	setWindowTitle(i18n("Quotations"));
	setModal(true);

	QVBoxLayout *quotationsVLayout = new QVBoxLayout(this);

	titleLabel = new QLabel(i18n("Quotations"), this);
	quotationsVLayout->addWidget(titleLabel);
	QHBoxLayout *quotationsLayout = new QHBoxLayout();
	quotationsVLayout->addLayout(quotationsLayout);

	quotationsView = new QTreeWidget(this);
	quotationsView->setSortingEnabled(true);
	quotationsView->sortByColumn(0, Qt::DescendingOrder);
	quotationsView->setAllColumnsShowFocus(true);
	quotationsView->setColumnCount(2);
	QStringList headers;
	headers << i18n("Date");
	headers << i18n("Price per Share");
	quotationsView->setHeaderLabels(headers);
	quotationsView->setRootIsDecorated(false);
	quotationsView->header()->setSectionsClickable(false);
	QSizePolicy sp = quotationsView->sizePolicy();
	sp.setHorizontalPolicy(QSizePolicy::MinimumExpanding);
	quotationsView->setSizePolicy(sp);
	quotationsLayout->addWidget(quotationsView);

	QVBoxLayout *buttonsLayout = new QVBoxLayout();
	quotationsLayout->addLayout(buttonsLayout);
	quotationEdit = new EqonomizeValueEdit(0.01, INT_MAX / pow(10, MONETARY_DECIMAL_PLACES) - 1.0, 1.0, 0.01, MONETARY_DECIMAL_PLACES, true, this);
	buttonsLayout->addWidget(quotationEdit);
	dateEdit = new QDateEdit(this);
	dateEdit->setCalendarPopup(true);
	dateEdit->setDate(QDate::currentDate());
	buttonsLayout->addWidget(dateEdit);
	addButton = new QPushButton(this);
	KGuiItem::assign(addButton, KStandardGuiItem::add());
	buttonsLayout->addWidget(addButton);
	changeButton = new QPushButton(this);
	KGuiItem::assign(changeButton, KStandardGuiItem::apply());
	changeButton->setEnabled(false);
	buttonsLayout->addWidget(changeButton);
	deleteButton = new QPushButton(this);
	KGuiItem::assign(deleteButton, KStandardGuiItem::del());
	deleteButton->setEnabled(false);
	buttonsLayout->addWidget(deleteButton);
	buttonsLayout->addStretch(1);
	
	QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
	buttonBox->button(QDialogButtonBox::Ok)->setShortcut(Qt::CTRL | Qt::Key_Return);
	buttonBox->button(QDialogButtonBox::Ok)->setDefault(true);
	connect(buttonBox->button(QDialogButtonBox::Cancel), SIGNAL(clicked()), this, SLOT(reject()));
	connect(buttonBox->button(QDialogButtonBox::Ok), SIGNAL(clicked()), this, SLOT(accept()));
	quotationsVLayout->addWidget(buttonBox);

	connect(quotationsView, SIGNAL(itemSelectionChanged()), this, SLOT(onSelectionChanged()));
	connect(addButton, SIGNAL(clicked()), this, SLOT(addQuotation()));
	connect(changeButton, SIGNAL(clicked()), this, SLOT(changeQuotation()));
	connect(deleteButton, SIGNAL(clicked()), this, SLOT(deleteQuotation()));

}
void EditQuotationsDialog::setSecurity(Security *security) {
	quotationsView->clear();
	QList<QTreeWidgetItem *> items;
	QMap<QDate, double>::const_iterator it_end = security->quotations.end();
	for(QMap<QDate, double>::const_iterator it = security->quotations.begin(); it != it_end; ++it) {
		items.append(new QuotationListViewItem(it.key(), it.value()));
	}
	quotationsView->addTopLevelItems(items);
	quotationsView->setSortingEnabled(true);
	titleLabel->setText(i18n("Quotations for %1", security->name()));
}
void EditQuotationsDialog::modifyQuotations(Security *security) {
	security->quotations.clear();
	QTreeWidgetItemIterator it(quotationsView);
	QuotationListViewItem *i = (QuotationListViewItem*) *it;
	while(i) {
		security->quotations[i->date] = i->value;
		security->quotations_auto[i->date] = false;
		++it;
		i = (QuotationListViewItem*) *it;
	}
}
void EditQuotationsDialog::onSelectionChanged() {
	QuotationListViewItem *i = (QuotationListViewItem*) selectedItem(quotationsView);
	if(i) {
		changeButton->setEnabled(true);
		deleteButton->setEnabled(true);
		dateEdit->setDate(i->date);
		quotationEdit->setValue(i->value);
	} else {
		changeButton->setEnabled(false);
		deleteButton->setEnabled(false);
	}
}
void EditQuotationsDialog::addQuotation() {
	QDate date = dateEdit->date();
	if(!date.isValid()) {
		KMessageBox::error(this, i18n("Invalid date."));
		return;
	}
	QTreeWidgetItemIterator it(quotationsView);
	QuotationListViewItem *i = (QuotationListViewItem*) *it;
	while(i) {
		if(i->date == date) {
			i->value = quotationEdit->value();
			i->setText(1, QLocale().toCurrencyString(i->value));
			return;
		}
		++it;
		i = (QuotationListViewItem*) *it;
	}
	quotationsView->insertTopLevelItem(0, new QuotationListViewItem(date, quotationEdit->value()));
	quotationsView->setSortingEnabled(true);
}
void EditQuotationsDialog::changeQuotation() {
	QuotationListViewItem *i = (QuotationListViewItem*) selectedItem(quotationsView);
	if(i == NULL) return;
	QDate date = dateEdit->date();
	if(!date.isValid()) {
		KMessageBox::error(this, i18n("Invalid date."));
		return;
	}
	QTreeWidgetItemIterator it(quotationsView);
	QuotationListViewItem *i2 = (QuotationListViewItem*) *it;
	while(i2) {
		if(i2->date == date) {
			i2->value = quotationEdit->value();
			i2->setText(1, QLocale().toCurrencyString(i2->value));
			return;
		}
		++it;
		i2 = (QuotationListViewItem*) *it;
	}
	i->date = date;
	i->value = quotationEdit->value();
	i->setText(0, QLocale().toString(date, QLocale::ShortFormat));
	i->setText(1, QLocale().toCurrencyString(i->value));
}
void EditQuotationsDialog::deleteQuotation() {
	QuotationListViewItem *i = (QuotationListViewItem*) selectedItem(quotationsView);
	if(i == NULL) return;
	delete i;
}


OverTimeReportDialog::OverTimeReportDialog(Budget *budg, QWidget *parent) : QDialog(parent, 0) {
	setWindowTitle(i18n("Report"));
	setModal(true);
	QVBoxLayout *box1 = new QVBoxLayout(this);
	report = new OverTimeReport(budg, this);
	report->updateDisplay();
	box1->addWidget(report);
	QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Close);
	buttonBox->button(QDialogButtonBox::Close)->setShortcut(Qt::CTRL | Qt::Key_Return);
	buttonBox->button(QDialogButtonBox::Close)->setDefault(true);
	connect(buttonBox->button(QDialogButtonBox::Close), SIGNAL(clicked()), this, SLOT(reject()));
	box1->addWidget(buttonBox);
}
void OverTimeReportDialog::reject() {
	report->saveConfig();
	QDialog::reject();
}
CategoriesComparisonReportDialog::CategoriesComparisonReportDialog(bool extra_parameters, Budget *budg, QWidget *parent) : QDialog(parent, 0) {
	setWindowTitle(i18n("Report"));
	setModal(true);
	QVBoxLayout *box1 = new QVBoxLayout(this);
	report = new CategoriesComparisonReport(budg, this, extra_parameters);
	report->updateDisplay();
	box1->addWidget(report);
	QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Close);
	buttonBox->button(QDialogButtonBox::Close)->setShortcut(Qt::CTRL | Qt::Key_Return);
	buttonBox->button(QDialogButtonBox::Close)->setDefault(true);
	connect(buttonBox->button(QDialogButtonBox::Close), SIGNAL(clicked()), this, SLOT(reject()));
	box1->addWidget(buttonBox);
}
void CategoriesComparisonReportDialog::reject() {
	report->saveConfig();
	QDialog::reject();
}
OverTimeChartDialog::OverTimeChartDialog(bool extra_parameters, Budget *budg, QWidget *parent) : QDialog(parent, 0) {
	setWindowTitle(i18n("Chart"));
	setModal(true);
	QVBoxLayout *box1 = new QVBoxLayout(this);
	chart = new OverTimeChart(budg, this, extra_parameters);
	box1->addWidget(chart);
	QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Close);
	buttonBox->button(QDialogButtonBox::Close)->setShortcut(Qt::CTRL | Qt::Key_Return);
	buttonBox->button(QDialogButtonBox::Close)->setDefault(true);
	connect(buttonBox->button(QDialogButtonBox::Close), SIGNAL(clicked()), this, SLOT(reject()));
	box1->addWidget(buttonBox);
}
void OverTimeChartDialog::reject() {
	chart->saveConfig();
	QDialog::reject();
}
CategoriesComparisonChartDialog::CategoriesComparisonChartDialog(Budget *budg, QWidget *parent) : QDialog(parent, 0) {
	setWindowTitle(i18n("Chart"));
	setModal(true);
	QVBoxLayout *box1 = new QVBoxLayout(this);
	chart = new CategoriesComparisonChart(budg, this);
	box1->addWidget(chart);
	QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Close);
	buttonBox->button(QDialogButtonBox::Close)->setShortcut(Qt::CTRL | Qt::Key_Return);
	buttonBox->button(QDialogButtonBox::Close)->setDefault(true);
	connect(buttonBox->button(QDialogButtonBox::Close), SIGNAL(clicked()), this, SLOT(reject()));
	box1->addWidget(buttonBox);
}
void CategoriesComparisonChartDialog::reject() {
	chart->saveConfig();
	QDialog::reject();
}

ConfirmScheduleDialog::ConfirmScheduleDialog(bool extra_parameters, Budget *budg, QWidget *parent, QString title) : QDialog(parent, 0), budget(budg), b_extra(extra_parameters) {

	setWindowTitle(title);
	setModal(true);

	QVBoxLayout *box1 = new QVBoxLayout(this);
	QLabel *label = new QLabel(i18n("The following transactions was scheduled to occur today or before today.\nConfirm that they have indeed occurred (or will occur today)."), this);
	box1->addWidget(label);
	QHBoxLayout *box2 = new QHBoxLayout();
	box1->addLayout(box2);
	transactionsView = new QTreeWidget(this);
	transactionsView->sortByColumn(0, Qt::AscendingOrder);
	transactionsView->setAllColumnsShowFocus(true);
	transactionsView->setColumnCount(4);
	QStringList headers;
	headers << i18n("Date");
	headers << i18n("Type");
	headers << i18n("Description");
	headers << i18n("Amount");
	transactionsView->setHeaderLabels(headers);
	transactionsView->setRootIsDecorated(false);
	QSizePolicy sp = transactionsView->sizePolicy();
	sp.setHorizontalPolicy(QSizePolicy::MinimumExpanding);
	transactionsView->setSizePolicy(sp);
	box2->addWidget(transactionsView);
	QDialogButtonBox *buttons = new QDialogButtonBox(Qt::Vertical, this);
	editButton = buttons->addButton(i18n("Edit..."), QDialogButtonBox::ActionRole);
	editButton->setEnabled(false);
	postponeButton = buttons->addButton(i18n("Postpone..."), QDialogButtonBox::ActionRole);
	postponeButton->setEnabled(false);
	removeButton = buttons->addButton(QString(), QDialogButtonBox::ActionRole);
	KGuiItem::assign(removeButton, KStandardGuiItem::del());
	removeButton->setEnabled(false);
	box2->addWidget(buttons);
	
	QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok);
	buttonBox->button(QDialogButtonBox::Ok)->setShortcut(Qt::CTRL | Qt::Key_Return);
	buttonBox->button(QDialogButtonBox::Ok)->setDefault(true);
	connect(buttonBox->button(QDialogButtonBox::Ok), SIGNAL(clicked()), this, SLOT(accept()));
	box1->addWidget(buttonBox);
	
	connect(transactionsView, SIGNAL(itemSelectionChanged()), this, SLOT(transactionSelectionChanged()));
	connect(removeButton, SIGNAL(clicked()), this, SLOT(remove()));
	connect(editButton, SIGNAL(clicked()), this, SLOT(edit()));
	connect(postponeButton, SIGNAL(clicked()), this, SLOT(postpone()));
	updateTransactions();

}
void ConfirmScheduleDialog::transactionSelectionChanged() {
	QTreeWidgetItem *i = selectedItem(transactionsView);
	if(i == NULL) {
		editButton->setEnabled(false);
		removeButton->setEnabled(false);
		postponeButton->setEnabled(false);
	} else {
		editButton->setEnabled(true);
		removeButton->setEnabled(true);
		postponeButton->setEnabled(true);
	}
}
void ConfirmScheduleDialog::remove() {
	QTreeWidgetItem *i = selectedItem(transactionsView);
	if(i == NULL) return;
	Transaction *trans = ((ConfirmScheduleListViewItem*) i)->transaction();
	delete trans;
	delete i;
	if(transactionsView->topLevelItemCount() == 0) reject();
}
void ConfirmScheduleDialog::postpone() {
	QTreeWidgetItem *i = selectedItem(transactionsView);
	if(i == NULL) return;
	QDialog *dialog = new QDialog(this, 0);
	dialog->setWindowTitle(i18n("Date"));
	QVBoxLayout *box1 = new QVBoxLayout(dialog);
	QCalendarWidget *datePicker = new QCalendarWidget(dialog);
	datePicker->setSelectedDate(QDate::currentDate().addDays(1));
	box1->addWidget(datePicker);
	QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
	buttonBox->button(QDialogButtonBox::Ok)->setShortcut(Qt::CTRL | Qt::Key_Return);
	buttonBox->button(QDialogButtonBox::Ok)->setDefault(true);
	connect(buttonBox->button(QDialogButtonBox::Cancel), SIGNAL(clicked()), dialog, SLOT(reject()));
	connect(buttonBox->button(QDialogButtonBox::Ok), SIGNAL(clicked()), dialog, SLOT(accept()));
	box1->addWidget(buttonBox);
	if(dialog->exec() == QDialog::Accepted) {
		if(datePicker->selectedDate() > QDate::currentDate()) {
			Transaction *trans = ((ConfirmScheduleListViewItem*) i)->transaction();
			trans->setDate(datePicker->selectedDate());
			budget->addScheduledTransaction(new ScheduledTransaction(budget, trans, NULL));
			delete i;
		} else {
			KMessageBox::error(this, i18n("Can only postpone to future dates."));
		}
	}
	dialog->deleteLater();
	if(transactionsView->topLevelItemCount() == 0) reject();
}
void ConfirmScheduleDialog::edit() {
	QTreeWidgetItem *i = selectedItem(transactionsView);
	if(i == NULL) return;
	Transaction *trans = ((ConfirmScheduleListViewItem*) i)->transaction();
	Security *security = NULL;
	if(trans->type() == TRANSACTION_TYPE_SECURITY_BUY || trans->type() == TRANSACTION_TYPE_SECURITY_SELL) {
		security = ((SecurityTransaction*) trans)->security();
	} else if(trans->type() == TRANSACTION_TYPE_INCOME && ((Income*) trans)->security()) {
		security = ((Income*) trans)->security();
	}
	TransactionEditDialog *dialog = new TransactionEditDialog(b_extra, trans->type(), false, false, security, SECURITY_ALL_VALUES, security != NULL, budget, this);
	dialog->editWidget->updateAccounts();
	dialog->editWidget->setTransaction(trans);
	//if(trans->type() == TRANSACTION_TYPE_SECURITY_SELL) dialog->editWidget->setMaxSharesDate(QDate::currentDate());
	if(dialog->editWidget->checkAccounts() && dialog->exec() == QDialog::Accepted) {
		if(dialog->editWidget->modifyTransaction(trans)) {
			if(trans->date() > QDate::currentDate()) {
				budget->addScheduledTransaction(new ScheduledTransaction(budget, trans, NULL));
				delete i;
			} else {
				((ConfirmScheduleListViewItem*) i)->setTransaction(trans);
			}
		}
	}
	dialog->deleteLater();
	if(transactionsView->topLevelItemCount() == 0) reject();
}
void ConfirmScheduleDialog::updateTransactions() {
	ScheduledTransaction *strans = budget->scheduledTransactions.first();
	QList<QTreeWidgetItem *> items;
	while(strans) {
		Transaction *trans = NULL;
		if(strans->firstOccurrence() < QDate::currentDate() || (QTime::currentTime().hour() >= 18 && strans->firstOccurrence() == QDate::currentDate())) {
			bool b = strans->isOneTimeTransaction();
			trans = strans->realize(strans->firstOccurrence());
			if(b) budget->removeScheduledTransaction(strans);
			strans = budget->scheduledTransactions.first();
		} else {
			strans = budget->scheduledTransactions.next();
		}
		if(trans) {
			QTreeWidgetItem *i = new ConfirmScheduleListViewItem(trans);
			items.append(i);
		}
	}
	transactionsView->addTopLevelItems(items);
	transactionsView->setSortingEnabled(true);
	QTreeWidgetItemIterator it(transactionsView);
	QTreeWidgetItem *i = *it;
	if(i) i->setSelected(true);
}
Transaction *ConfirmScheduleDialog::firstTransaction() {
	current_index = 0;
	current_item = (ConfirmScheduleListViewItem*) transactionsView->topLevelItem(current_index);
	if(current_item) return current_item->transaction();
	return NULL;
}
Transaction *ConfirmScheduleDialog::nextTransaction() {
	current_index++;
	current_item = (ConfirmScheduleListViewItem*) transactionsView->topLevelItem(current_index);
	if(current_item) return current_item->transaction();
	return NULL;
}

SecurityTransactionsDialog::SecurityTransactionsDialog(Security *sec, Eqonomize *parent, QString title) : QDialog(parent, 0), security(sec), mainWin(parent) {

	setWindowTitle(title);
	setModal(true);

	QVBoxLayout *box1 = new QVBoxLayout(this);
	box1->addWidget(new QLabel(i18n("Transactions for %1", security->name()), this));
	QHBoxLayout *box2 = new QHBoxLayout();
	box1->addLayout(box2);
	transactionsView = new EqonomizeTreeWidget(this);
	transactionsView->sortByColumn(0, Qt::AscendingOrder);
	transactionsView->setAllColumnsShowFocus(true);
	transactionsView->setColumnCount(4);
	QStringList headers;
	headers << i18n("Date");
	headers << i18n("Type");
	headers << i18n("Value");
	headers << i18n("Shares");
	transactionsView->setHeaderLabels(headers);
	transactionsView->setRootIsDecorated(false);
	QSizePolicy sp = transactionsView->sizePolicy();
	sp.setHorizontalPolicy(QSizePolicy::MinimumExpanding);
	transactionsView->setSizePolicy(sp);
	box2->addWidget(transactionsView);
	QDialogButtonBox *buttons = new QDialogButtonBox(Qt::Vertical, this);
	editButton = buttons->addButton(i18n("Edit..."), QDialogButtonBox::ActionRole);
	editButton->setEnabled(false);
	removeButton = buttons->addButton(QString(), QDialogButtonBox::ActionRole);
	KGuiItem::assign(removeButton, KStandardGuiItem::del());
	removeButton->setEnabled(false);
	box2->addWidget(buttons);
	
	QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Close);
	buttonBox->button(QDialogButtonBox::Close)->setShortcut(Qt::CTRL | Qt::Key_Return);
	buttonBox->button(QDialogButtonBox::Close)->setDefault(true);
	connect(buttonBox->button(QDialogButtonBox::Close), SIGNAL(clicked()), this, SLOT(reject()));
	box1->addWidget(buttonBox);
	
	connect(transactionsView, SIGNAL(itemSelectionChanged()), this, SLOT(transactionSelectionChanged()));
	connect(transactionsView, SIGNAL(doubleClicked(QTreeWidgetItem*, int)), this, SLOT(edit(QTreeWidgetItem*)));
	connect(removeButton, SIGNAL(clicked()), this, SLOT(remove()));
	connect(editButton, SIGNAL(clicked()), this, SLOT(edit()));
	updateTransactions();
}
void SecurityTransactionsDialog::transactionSelectionChanged() {
	QTreeWidgetItem *i = selectedItem(transactionsView);
	if(i == NULL) {
		editButton->setEnabled(false);
		removeButton->setEnabled(false);
	} else {
		editButton->setEnabled(true);
		removeButton->setEnabled(true);
	}
}
void SecurityTransactionsDialog::remove() {
	SecurityTransactionListViewItem *i = (SecurityTransactionListViewItem*) selectedItem(transactionsView);
	if(i == NULL) return;
	if(i->trans) {
		SecurityTransaction *trans = i->trans;
		security->budget()->removeTransaction(trans, true);
		mainWin->transactionRemoved(trans);
		delete trans;
	} else if(i->div) {
		Income *div = i->div;
		security->budget()->removeTransaction(div, true);
		mainWin->transactionRemoved(div);
		delete div;
	} else if(i->rediv) {
		ReinvestedDividend *rediv = i->rediv;
		security->reinvestedDividends.removeRef(rediv);
		mainWin->updateSecurity(security);
		mainWin->updateSecurityAccount(security->account());
		mainWin->setModified(true);
	} else if(i->strans) {
		ScheduledTransaction *strans = i->strans;
		security->budget()->removeScheduledTransaction(strans, true);
		mainWin->scheduledTransactionRemoved(strans);
		delete strans;
	} else if(i->sdiv) {
		ScheduledTransaction *sdiv = i->sdiv;
		security->budget()->removeScheduledTransaction(sdiv, true);
		mainWin->scheduledTransactionRemoved(sdiv);
		delete sdiv;
	} else if(i->ts) {
		SecurityTrade *ts = i->ts;
		security->budget()->removeSecurityTrade(ts, true);
		mainWin->updateSecurity(ts->from_security);
		mainWin->updateSecurity(ts->to_security);
		mainWin->updateSecurityAccount(ts->from_security->account());
		if(ts->to_security->account() != ts->from_security->account()) {
			mainWin->updateSecurityAccount(ts->to_security->account());
		}
		mainWin->setModified(true);
		delete ts;
	}
	delete i;
}
void SecurityTransactionsDialog::edit() {
	edit(selectedItem(transactionsView));
}
void SecurityTransactionsDialog::edit(QTreeWidgetItem *i_pre) {
	if(i_pre == NULL) return;
	SecurityTransactionListViewItem *i = (SecurityTransactionListViewItem*) i_pre;
	bool b = false;
	if(i->trans) b = mainWin->editTransaction(i->trans, this);
	else if(i->div) b = mainWin->editTransaction(i->div, this);
	else if(i->rediv) b = mainWin->editReinvestedDividend(i->rediv, security, this);
	else if(i->strans) b = mainWin->editScheduledTransaction(i->strans, this);
	else if(i->sdiv) b = mainWin->editScheduledTransaction(i->sdiv, this);
	else if(i->ts) b = mainWin->editSecurityTrade(i->ts, this);
	if(b) updateTransactions();
}
void SecurityTransactionsDialog::updateTransactions() {
	transactionsView->clear();
	QList<QTreeWidgetItem *> items;
	SecurityTransaction *trans = security->transactions.first();
	while(trans) {
		SecurityTransactionListViewItem *i = new SecurityTransactionListViewItem(QLocale().toString(trans->date(), QLocale::ShortFormat), QString::null, QLocale().toCurrencyString(trans->value()), QLocale().toString(trans->shares(), 'f', security->decimals()));
		i->trans = trans;
		i->date = trans->date();
		i->value = trans->value();
		i->shares = trans->shares();
		if(trans->type() == TRANSACTION_TYPE_SECURITY_BUY) i->setText(1, i18n("Shares Bought"));
		else i->setText(1, i18n("Shares Sold"));
		items.append(i);
		trans = security->transactions.next();
	}
	Income *div = security->dividends.first();
	while(div) {
		SecurityTransactionListViewItem *i = new SecurityTransactionListViewItem(QLocale().toString(div->date(), QLocale::ShortFormat), i18n("Dividend"), QLocale().toCurrencyString(div->value()), "-");
		i->div = div;
		i->date = div->date();
		i->value = div->value();
		items.append(i);
		div = security->dividends.next();
	}
	ReinvestedDividend *rediv = security->reinvestedDividends.first();
	while(rediv) {
		SecurityTransactionListViewItem *i = new SecurityTransactionListViewItem(QLocale().toString(rediv->date, QLocale::ShortFormat), i18n("Reinvested Dividend"), "-", QLocale().toString(rediv->shares, 'f', security->decimals()));
		i->rediv = rediv;
		i->date = rediv->date;
		i->shares = rediv->shares;
		items.append(i);
		rediv = security->reinvestedDividends.next();
	}
	SecurityTrade *ts = security->tradedShares.first();
	while(ts) {
		double shares;
		if(ts->from_security == security) shares = ts->from_shares;
		else shares = ts->to_shares;
		SecurityTransactionListViewItem *i = new SecurityTransactionListViewItem(QLocale().toString(ts->date, QLocale::ShortFormat), ts->from_security == security ? i18n("Shares Sold (Traded)") :  i18n("Shares Bought (Traded)"), QLocale().toCurrencyString(ts->value), QLocale().toString(shares, 'f', security->decimals()));
		i->ts = ts;
		i->date = ts->date;
		i->shares = shares;
		i->value = ts->value;
		items.append(i);
		ts = security->tradedShares.next();
	}
	ScheduledTransaction *strans = security->scheduledTransactions.first();
	while(strans) {
		SecurityTransactionListViewItem *i = new SecurityTransactionListViewItem(QLocale().toString(strans->transaction()->date(), QLocale::ShortFormat), QString::null, QLocale().toCurrencyString(strans->transaction()->value()), QLocale().toString(((SecurityTransaction*) strans->transaction())->shares(), 'f', security->decimals()));
		i->strans = strans;
		i->date = strans->transaction()->date();
		i->value = strans->transaction()->value();
		i->shares = ((SecurityTransaction*) strans->transaction())->shares();
		if(strans->recurrence()) {
			if(strans->transaction()->type() == TRANSACTION_TYPE_SECURITY_BUY) i->setText(1, i18n("Shares Bought (Recurring)"));
			else i->setText(1, i18n("Shares Sold (Recurring)"));
		} else {
			if(strans->transaction()->type() == TRANSACTION_TYPE_SECURITY_BUY) i->setText(1, i18n("Shares Bought (Scheduled)"));
			else i->setText(1, i18n("Shares Sold (Scheduled)"));
		}
		items.append(i);
		strans = security->scheduledTransactions.next();
	}
	strans = security->scheduledDividends.first();
	while(strans) {
		SecurityTransactionListViewItem *i = new SecurityTransactionListViewItem(QLocale().toString(strans->transaction()->date(), QLocale::ShortFormat), QString::null, QLocale().toCurrencyString(strans->transaction()->value()), "-");
		i->sdiv = strans;
		i->date = strans->transaction()->date();
		i->value = strans->transaction()->value();
		if(strans->recurrence()) {
			i->setText(1, i18n("Recurring Dividend"));
		} else {
			i->setText(1, i18n("Scheduled Dividend"));
		}
		items.append(i);
		strans = security->scheduledDividends.next();
	}
	transactionsView->addTopLevelItems(items);
	transactionsView->setSortingEnabled(true);
}

EditSecurityDialog::EditSecurityDialog(Budget *budg, QWidget *parent, QString title) : QDialog(parent, 0), budget(budg) {

	setWindowTitle(title);
	setModal(true);
	
	QVBoxLayout *box1 = new QVBoxLayout(this);

	QGridLayout *grid = new QGridLayout();
	box1->addLayout(grid);
	grid->addWidget(new QLabel(i18n("Type:"), this), 0, 0);
	typeCombo = new QComboBox(this);
	typeCombo->setEditable(false);
	typeCombo->addItem(i18n("Mutual Fund"));
	typeCombo->addItem(i18n("Bond"));
	typeCombo->addItem(i18n("Stock"));
	typeCombo->addItem(i18n("Other"));
	grid->addWidget(typeCombo, 0, 1);
	typeCombo->setFocus();
	grid->addWidget(new QLabel(i18n("Name:"), this), 1, 0);
	nameEdit = new QLineEdit(this);
	grid->addWidget(nameEdit, 1, 1);
	grid->addWidget(new QLabel(i18n("Account:"), this), 2, 0);
	accountCombo = new QComboBox(this);
	accountCombo->setEditable(false);
	AssetsAccount *account = budget->assetsAccounts.first();
	while(account) {
		if(account->accountType() == ASSETS_TYPE_SECURITIES) {
			accountCombo->addItem(account->name());
			accounts.push_back(account);
		}
		account = budget->assetsAccounts.next();
	}
	grid->addWidget(accountCombo, 2, 1);
	grid->addWidget(new QLabel(i18n("Decimals in Shares:"), this), 3, 0);
	decimalsEdit = new QSpinBox(this);
	decimalsEdit->setRange(0, 8);
	decimalsEdit->setSingleStep(1);
	decimalsEdit->setValue(4);
	grid->addWidget(decimalsEdit, 3, 1);
	grid->addWidget(new QLabel(i18n("Initial Shares:"), this), 4, 0);
	sharesEdit = new EqonomizeValueEdit(0.0, 4, false, false, this);
	grid->addWidget(sharesEdit, 4, 1);
	quotationLabel = new QLabel(i18n("Initial quotation:"), this);
	grid->addWidget(quotationLabel, 5, 0);
	quotationEdit = new EqonomizeValueEdit(false, this);
	grid->addWidget(quotationEdit, 5, 1);
	quotationDateLabel = new QLabel(i18n("Date:"), this);
	grid->addWidget(quotationDateLabel, 6, 0);
	quotationDateEdit = new QDateEdit(this);
	quotationDateEdit->setCalendarPopup(true);
	grid->addWidget(quotationDateEdit, 6, 1);
	grid->addWidget(new QLabel(i18n("Description:"), this), 7, 0);
	descriptionEdit = new KTextEdit(this);
	grid->addWidget(descriptionEdit, 8, 0, 1, 2);
	nameEdit->setFocus();
	
	QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
	buttonBox->button(QDialogButtonBox::Ok)->setShortcut(Qt::CTRL | Qt::Key_Return);
	buttonBox->button(QDialogButtonBox::Ok)->setDefault(true);
	connect(buttonBox->button(QDialogButtonBox::Cancel), SIGNAL(clicked()), this, SLOT(reject()));
	connect(buttonBox->button(QDialogButtonBox::Ok), SIGNAL(clicked()), this, SLOT(accept()));
	box1->addWidget(buttonBox);

	connect(decimalsEdit, SIGNAL(valueChanged(int)), this, SLOT(decimalsChanged(int)));

}
bool EditSecurityDialog::checkAccount() {
	if(accountCombo->count() == 0) {
		KMessageBox::error(this, i18n("No suitable account or income category available."));
		return false;
	}
	return true;
}
void EditSecurityDialog::decimalsChanged(int i) {
	sharesEdit->setRange(0.0, INT_MAX / pow(10.0, i) - 1, 1.0, i);
}
Security *EditSecurityDialog::newSecurity() {
	if(!checkAccount()) return NULL;
	SecurityType type;
	switch(typeCombo->currentIndex()) {
		case 1: {type = SECURITY_TYPE_BOND; break;}
		case 2: {type = SECURITY_TYPE_STOCK; break;}
		case 3: {type = SECURITY_TYPE_OTHER; break;}
		default: {type = SECURITY_TYPE_MUTUAL_FUND; break;}
	}
	Security *security = new Security(budget, accounts[accountCombo->currentIndex()], type, sharesEdit->value(), decimalsEdit->value(), nameEdit->text(), descriptionEdit->toPlainText());
	if(quotationEdit->value() > 0.0) {
		security->setQuotation(quotationDateEdit->date(), quotationEdit->value());
	}
	return security;
}
bool EditSecurityDialog::modifySecurity(Security *security) {
	if(!checkAccount()) return false;
	security->setName(nameEdit->text());
	security->setInitialShares(sharesEdit->value());
	security->setDescription(descriptionEdit->toPlainText());
	security->setAccount(accounts[accountCombo->currentIndex()]);
	security->setDecimals(decimalsEdit->value());
	switch(typeCombo->currentIndex()) {
		case 1: {security->setType(SECURITY_TYPE_BOND); break;}
		case 2: {security->setType(SECURITY_TYPE_STOCK); break;}
		case 3: {security->setType(SECURITY_TYPE_OTHER); break;}
		default: {security->setType(SECURITY_TYPE_MUTUAL_FUND); break;}
	}
	return true;
}
void EditSecurityDialog::setSecurity(Security *security) {
	nameEdit->setText(security->name());
	quotationEdit->hide();
	quotationDateEdit->hide();
	quotationLabel->hide();
	quotationDateLabel->hide();
	descriptionEdit->setPlainText(security->description());
	decimalsEdit->setValue(security->decimals());
	decimalsChanged(security->decimals());
	sharesEdit->setValue(security->initialShares());
	for(QVector<AssetsAccount*>::size_type i = 0; i < accounts.size(); i++) {
		if(security->account() == accounts[i]) {
			accountCombo->setCurrentIndex(i);
			break;
		}
	}
	switch(security->type()) {
		case SECURITY_TYPE_BOND: {typeCombo->setCurrentIndex(1); break;}
		case SECURITY_TYPE_STOCK: {typeCombo->setCurrentIndex(2); break;}
		case SECURITY_TYPE_OTHER: {typeCombo->setCurrentIndex(3); break;}
		default: {typeCombo->setCurrentIndex(0); break;}
	}
}

EditAssetsAccountDialog::EditAssetsAccountDialog(Budget *budg, QWidget *parent, QString title) : QDialog(parent, 0), budget(budg) {

	setWindowTitle(title);
	setModal(true);

	QVBoxLayout *box1 = new QVBoxLayout(this);
	
	QGridLayout *grid = new QGridLayout();
	box1->addLayout(grid);
	grid->addWidget(new QLabel(i18n("Type:"), this), 0, 0);
	typeCombo = new QComboBox(this);
	typeCombo->setEditable(false);
	typeCombo->addItem(i18n("Cash"));
	typeCombo->addItem(i18n("Current Account"));
	typeCombo->addItem(i18n("Savings Account"));
	typeCombo->addItem(i18n("Credit Card"));
	typeCombo->addItem(i18n("Liabilities"));
	typeCombo->addItem(i18n("Securities"));
	grid->addWidget(typeCombo, 0, 1);
	grid->addWidget(new QLabel(i18n("Name:"), this), 1, 0);
	nameEdit = new QLineEdit(this);
	grid->addWidget(nameEdit, 1, 1);
	grid->addWidget(new QLabel(i18n("Initial balance:"), this), 2, 0);
	valueEdit = new EqonomizeValueEdit(true, this);
	grid->addWidget(valueEdit, 2, 1);
	budgetButton = new QCheckBox(i18n("Default account for budgeted transactions"), this);
	budgetButton->setChecked(false);
	grid->addWidget(budgetButton, 3, 0, 1, 2);
	grid->addWidget(new QLabel(i18n("Description:"), this), 4, 0);
	descriptionEdit = new KTextEdit(this);
	grid->addWidget(descriptionEdit, 5, 0, 1, 2);
	nameEdit->setFocus();
	current_account = NULL;
	
	QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
	buttonBox->button(QDialogButtonBox::Ok)->setShortcut(Qt::CTRL | Qt::Key_Return);
	buttonBox->button(QDialogButtonBox::Ok)->setDefault(true);
	connect(buttonBox->button(QDialogButtonBox::Cancel), SIGNAL(clicked()), this, SLOT(reject()));
	connect(buttonBox->button(QDialogButtonBox::Ok), SIGNAL(clicked()), this, SLOT(accept()));
	box1->addWidget(buttonBox);
	
	connect(typeCombo, SIGNAL(activated(int)), this, SLOT(typeActivated(int)));

}
void EditAssetsAccountDialog::typeActivated(int index) {
	valueEdit->setEnabled(index != 5);
	budgetButton->setChecked(index == 1);
	budgetButton->setEnabled(index != 5);
}
AssetsAccount *EditAssetsAccountDialog::newAccount() {
	AssetsType type;
	switch(typeCombo->currentIndex()) {
		case 1: {type = ASSETS_TYPE_CURRENT; break;}
		case 2: {type = ASSETS_TYPE_SAVINGS; break;}
		case 3: {type = ASSETS_TYPE_CREDIT_CARD; break;}
		case 4: {type = ASSETS_TYPE_LIABILITIES; break;}
		case 5: {type = ASSETS_TYPE_SECURITIES;  break;}
		default: {type = ASSETS_TYPE_CASH; break;}
	}
	AssetsAccount *account = new AssetsAccount(budget, type, nameEdit->text(), valueEdit->value(), descriptionEdit->toPlainText());
	account->setAsBudgetAccount(budgetButton->isChecked());
	return account;
}
void EditAssetsAccountDialog::modifyAccount(AssetsAccount *account) {
	account->setName(nameEdit->text());
	account->setInitialBalance(valueEdit->value());
	account->setDescription(descriptionEdit->toPlainText());
	account->setAsBudgetAccount(budgetButton->isChecked());
	switch(typeCombo->currentIndex()) {
		case 1: {account->setAccountType(ASSETS_TYPE_CURRENT); break;}
		case 2: {account->setAccountType(ASSETS_TYPE_SAVINGS); break;}
		case 3: {account->setAccountType(ASSETS_TYPE_CREDIT_CARD); break;}
		case 4: {account->setAccountType(ASSETS_TYPE_LIABILITIES); break;}
		case 5: {account->setAccountType(ASSETS_TYPE_SECURITIES);  break;}
		default: {account->setAccountType(ASSETS_TYPE_CASH); break;}
	}
}
void EditAssetsAccountDialog::setAccount(AssetsAccount *account) {
	current_account = account;
	nameEdit->setText(account->name());
	valueEdit->setValue(account->initialBalance());
	descriptionEdit->setPlainText(account->description());
	budgetButton->setChecked(account->isBudgetAccount());
	switch(account->accountType()) {
		case ASSETS_TYPE_CURRENT: {typeCombo->setCurrentIndex(1); break;}
		case ASSETS_TYPE_SAVINGS: {typeCombo->setCurrentIndex(2); break;}
		case ASSETS_TYPE_CREDIT_CARD: {typeCombo->setCurrentIndex(3); break;}
		case ASSETS_TYPE_LIABILITIES: {typeCombo->setCurrentIndex(4); break;}
		case ASSETS_TYPE_SECURITIES: {typeCombo->setCurrentIndex(5);  break;}
		default: {typeCombo->setCurrentIndex(0); break;}
	}
	typeActivated(typeCombo->currentIndex());
}
void EditAssetsAccountDialog::accept() {
	QString sname = nameEdit->text().trimmed();
	if(sname.isEmpty()) {
		nameEdit->setFocus();
		KMessageBox::error(this, i18n("Empty name."));
		return;
	}
	AssetsAccount *aaccount = budget->findAssetsAccount(sname);
	if(aaccount && aaccount != current_account) {
		nameEdit->setFocus();
		KMessageBox::error(this, i18n("The entered name is used by another account."));
		return;
	}
	QDialog::accept();
}


EditIncomesAccountDialog::EditIncomesAccountDialog(Budget *budg, QWidget *parent, QString title) : QDialog(parent, 0), budget(budg) {

	setWindowTitle(title);
	setModal(true);

	QVBoxLayout *box1 = new QVBoxLayout(this);
	
	QGridLayout *grid = new QGridLayout();
	box1->addLayout(grid);
	grid->addWidget(new QLabel(i18n("Name:"), this), 0, 0);
	nameEdit = new QLineEdit(this);
	grid->addWidget(nameEdit, 0, 1);
	budgetButton = new QCheckBox(i18n("Monthly budget:"), this);
	budgetButton->setChecked(false);
	grid->addWidget(budgetButton, 1, 0);
	budgetEdit = new EqonomizeValueEdit(false, this);
	budgetEdit->setEnabled(false);
	grid->addWidget(budgetEdit, 1, 1);
	grid->addWidget(new QLabel(i18n("Description:"), this), 2, 0);
	descriptionEdit = new KTextEdit(this);
	grid->addWidget(descriptionEdit, 3, 0, 1, 2);
	nameEdit->setFocus();
	current_account = NULL;
	
	QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
	buttonBox->button(QDialogButtonBox::Ok)->setShortcut(Qt::CTRL | Qt::Key_Return);
	buttonBox->button(QDialogButtonBox::Ok)->setDefault(true);
	connect(buttonBox->button(QDialogButtonBox::Cancel), SIGNAL(clicked()), this, SLOT(reject()));
	connect(buttonBox->button(QDialogButtonBox::Ok), SIGNAL(clicked()), this, SLOT(accept()));
	box1->addWidget(buttonBox);
	
	connect(budgetButton, SIGNAL(toggled(bool)), this, SLOT(budgetEnabled(bool)));
}
IncomesAccount *EditIncomesAccountDialog::newAccount() {
	IncomesAccount *account = new IncomesAccount(budget, nameEdit->text(), descriptionEdit->toPlainText());
	if(budgetButton->isChecked()) {
		account->setMonthlyBudget(QDate::currentDate().year(), QDate::currentDate().month(), budgetEdit->value());
	}
	return account;
}
void EditIncomesAccountDialog::modifyAccount(IncomesAccount *account) {
	account->setName(nameEdit->text());
	account->setDescription(descriptionEdit->toPlainText());
}
void EditIncomesAccountDialog::setAccount(IncomesAccount *account) {
	current_account = account;
	nameEdit->setText(account->name());
	budgetEdit->hide();
	budgetButton->hide();
	descriptionEdit->setPlainText(account->description());
}
void EditIncomesAccountDialog::budgetEnabled(bool b) {
	budgetEdit->setEnabled(b);
}
void EditIncomesAccountDialog::accept() {
	QString sname = nameEdit->text().trimmed();
	if(sname.isEmpty()) {
		nameEdit->setFocus();
		KMessageBox::error(this, i18n("Empty name."));
		return;
	}
	IncomesAccount *iaccount = budget->findIncomesAccount(sname);
	if(iaccount && iaccount != current_account) {
		nameEdit->setFocus();
		KMessageBox::error(this, i18n("The entered name is used by another income category."));
		return;
	}
	QDialog::accept();
}

EditExpensesAccountDialog::EditExpensesAccountDialog(Budget *budg, QWidget *parent, QString title) : QDialog(parent, 0), budget(budg) {

	setWindowTitle(title);
	setModal(true);

	QVBoxLayout *box1 = new QVBoxLayout(this);
	
	QGridLayout *grid = new QGridLayout();
	box1->addLayout(grid);
	grid->addWidget(new QLabel(i18n("Name:"), this), 0, 0);
	nameEdit = new QLineEdit(this);
	grid->addWidget(nameEdit, 0, 1);
	budgetButton = new QCheckBox(i18n("Monthly budget:"), this);
	budgetButton->setChecked(false);
	grid->addWidget(budgetButton, 1, 0);
	budgetEdit = new EqonomizeValueEdit(false, this);
	budgetEdit->setEnabled(false);
	grid->addWidget(budgetEdit, 1, 1);
	grid->addWidget(new QLabel(i18n("Description:"), this), 2, 0);
	descriptionEdit = new KTextEdit(this);
	grid->addWidget(descriptionEdit, 3, 0, 1, 2);
	nameEdit->setFocus();
	current_account = NULL;
	
	QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
	buttonBox->button(QDialogButtonBox::Ok)->setShortcut(Qt::CTRL | Qt::Key_Return);
	buttonBox->button(QDialogButtonBox::Ok)->setDefault(true);
	connect(buttonBox->button(QDialogButtonBox::Cancel), SIGNAL(clicked()), this, SLOT(reject()));
	connect(buttonBox->button(QDialogButtonBox::Ok), SIGNAL(clicked()), this, SLOT(accept()));
	box1->addWidget(buttonBox);
	
	connect(budgetButton, SIGNAL(toggled(bool)), this, SLOT(budgetEnabled(bool)));
}
ExpensesAccount *EditExpensesAccountDialog::newAccount() {
	ExpensesAccount *account = new ExpensesAccount(budget, nameEdit->text(), descriptionEdit->toPlainText());
	if(budgetButton->isChecked()) {
		account->setMonthlyBudget(QDate::currentDate().year(), QDate::currentDate().month(), budgetEdit->value());
	}
	return account;
}
void EditExpensesAccountDialog::modifyAccount(ExpensesAccount *account) {
	account->setName(nameEdit->text());
	account->setDescription(descriptionEdit->toPlainText());
}
void EditExpensesAccountDialog::setAccount(ExpensesAccount *account) {
	current_account = account;
	nameEdit->setText(account->name());
	budgetEdit->hide();
	budgetButton->hide();
	descriptionEdit->setPlainText(account->description());
}
void EditExpensesAccountDialog::budgetEnabled(bool b) {
	budgetEdit->setEnabled(b);
}
void EditExpensesAccountDialog::accept() {
	QString sname = nameEdit->text().trimmed();
	if(sname.isEmpty()) {
		nameEdit->setFocus();
		KMessageBox::error(this, i18n("Empty name."));
		return;
	}
	ExpensesAccount *eaccount = budget->findExpensesAccount(sname);
	if(eaccount && eaccount != current_account) {
		nameEdit->setFocus();
		KMessageBox::error(this, i18n("The entered name is used by another expense category."));
		return;
	}
	QDialog::accept();
}

#define CHANGE_COLUMN				2
#define BUDGET_COLUMN				1
#define VALUE_COLUMN				3

class TotalListViewItem : public QTreeWidgetItem {
	public:
		TotalListViewItem(QTreeWidget *parent, QString label1, QString label2 = QString::null, QString label3 = QString::null, QString label4 = QString::null) : QTreeWidgetItem(parent, UserType) {
			setText(0, label1);
			setText(1, label2);
			setText(2, label3);
			setText(3, label4);
			setTextAlignment(BUDGET_COLUMN, Qt::AlignRight);
			setTextAlignment(CHANGE_COLUMN, Qt::AlignRight);
			setTextAlignment(VALUE_COLUMN, Qt::AlignRight);
			setBackground(0, parent->palette().alternateBase());
			setBackground(1, parent->palette().alternateBase());
			setBackground(2, parent->palette().alternateBase());
			setBackground(3, parent->palette().alternateBase());
		}
		TotalListViewItem(QTreeWidget *parent, QTreeWidgetItem *after, QString label1, QString label2 = QString::null, QString label3 = QString::null, QString label4 = QString::null) : QTreeWidgetItem(parent, after, UserType) {
			setText(0, label1);
			setText(1, label2);
			setText(2, label3);
			setText(3, label4);
			setTextAlignment(BUDGET_COLUMN, Qt::AlignRight);
			setTextAlignment(CHANGE_COLUMN, Qt::AlignRight);
			setTextAlignment(VALUE_COLUMN, Qt::AlignRight);
			setBackground(0, parent->palette().alternateBase());
			setBackground(1, parent->palette().alternateBase());
			setBackground(2, parent->palette().alternateBase());
			setBackground(3, parent->palette().alternateBase());
		}
};

class AccountsListView : public EqonomizeTreeWidget {
	public:
		AccountsListView(QWidget *parent) : EqonomizeTreeWidget(parent) {
			setContentsMargins(25, 25, 25, 25);
		}
};

Eqonomize::Eqonomize() : KXmlGuiWindow(0) {

	cr_tmp_file = NULL;

	expenses_budget = 0.0;
	expenses_budget_diff = 0.0;
	incomes_budget = 0.0;
	incomes_budget_diff = 0.0;
	incomes_accounts_value = 0.0;
	incomes_accounts_change = 0.0;
	expenses_accounts_value = 0.0;
	expenses_accounts_change = 0.0;
	assets_accounts_value = 0.0;
	assets_accounts_change = 0.0;
	total_value = 0.0;
	total_cost = 0.0;
	total_profit = 0.0;
	total_rate = 0.0;

	partial_budget = false;

	modified = false;
	modified_auto_save = false;

	budget = new Budget();

	KConfigGroup config = KSharedConfig::openConfig()->group("General Options");
	b_extra = config.readEntry("useExtraProperties", false);
	int initial_period_pre = config.readEntry("initialPeriod", int(INITIAL_PERIOD_CURRENT_MONTH));
	if(initial_period_pre < INITIAL_PERIOD_CURRENT_MONTH || initial_period_pre > INITIAL_PERIOD_LAST) initial_period_pre = INITIAL_PERIOD_CURRENT_MONTH;
	InitialPeriod initial_period = (InitialPeriod) initial_period_pre;


	prev_cur_date = QDate::currentDate();
	QDate curdate = prev_cur_date;
	bool from_enabled = true;

	switch(initial_period) {
		case INITIAL_PERIOD_CURRENT_MONTH: {
			from_date.setDate(curdate.year(), curdate.month(), 1);			
			to_date = curdate;			
			break;
		}
		case INITIAL_PERIOD_CURRENT_YEAR: {
			from_date.setDate(curdate.year(), 1, 1);
			to_date = curdate;
			break;
		}
		case INITIAL_PERIOD_CURRENT_WHOLE_MONTH: {
			from_date.setDate(curdate.year(), curdate.month(), 1);
			to_date = from_date.addDays(curdate.daysInMonth() - 1);
			break;
		}
		case INITIAL_PERIOD_CURRENT_WHOLE_YEAR: {
			from_date.setDate(curdate.year(), 1, 1);
			to_date = from_date.addDays(curdate.daysInYear() - 1);
			break;
		}
		case INITIAL_PERIOD_LAST: {			
			from_date = QDate::fromString(config.readEntry("initialFromDate"), Qt::ISODate);
			if(!from_date.isValid()) from_date.setDate(curdate.year(), curdate.month(), 1);
			from_enabled = config.readEntry("initialFromDateEnabled", false);
			to_date = QDate::fromString(config.readEntry("initialToDate"), Qt::ISODate);
			if(!to_date.isValid()) to_date = from_date.addDays(from_date.daysInMonth() - 1);
			break;
		}
	}

	frommonth_begin.setDate(from_date.year(), from_date.month(), 1);
	prevmonth_begin = to_date.addMonths(-1);
	prevmonth_begin = prevmonth_begin.addDays(-(prevmonth_begin.day() - 1));

	setAcceptDrops(true);

	QWidget *w_top = new QWidget(this);
	setCentralWidget(w_top);

	QVBoxLayout *topLayout = new QVBoxLayout(w_top);
	
	tabs = new KPageWidget(w_top);
	topLayout->addWidget(tabs);
	
	accounts_page = new QWidget(this);
	accounts_page_item = new KPageWidgetItem(accounts_page, i18n("Accounts"));
	accounts_page_item->setHeader(i18n("Accounts & Categories"));
	accounts_page_item->setIcon(QIcon::fromTheme("go-home"));
	tabs->addPage(accounts_page_item);
	expenses_page = new QWidget(this);
	expenses_page_item = new KPageWidgetItem(expenses_page, i18n("Expenses"));
	expenses_page_item->setHeader(i18n("Expenses"));
	expenses_page_item->setIcon(QIcon::fromTheme("go-previous"));
	tabs->addPage(expenses_page_item);
	incomes_page = new QWidget(this);
	incomes_page_item = new KPageWidgetItem(incomes_page, i18n("Incomes"));
	incomes_page_item->setHeader(i18n("Incomes"));
	incomes_page_item->setIcon(QIcon::fromTheme("go-next"));
	tabs->addPage(incomes_page_item);
	transfers_page = new QWidget(this);
	transfers_page_item = new KPageWidgetItem(transfers_page, i18n("Transfers"));
	transfers_page_item->setHeader(i18n("Transfers"));
	transfers_page_item->setIcon(QIcon::fromTheme("view-refresh"));
	tabs->addPage(transfers_page_item);
	securities_page = new QWidget(this);
	securities_page_item = new KPageWidgetItem(securities_page, i18n("Securities"));
	securities_page_item->setHeader(i18n("Securities"));
	securities_page_item->setIcon(QIcon::fromTheme("view-list-text"));
	tabs->addPage(securities_page_item);
	schedule_page = new QWidget(this);
	schedule_page_item = new KPageWidgetItem(schedule_page, i18n("Schedule"));
	schedule_page_item->setHeader(i18n("Scheduled Transactions"));
	schedule_page_item->setIcon(QIcon::fromTheme("view-pim-calendar"));
	tabs->addPage(schedule_page_item);

	connect(tabs, SIGNAL(currentPageChanged(KPageWidgetItem*, KPageWidgetItem*)), this, SLOT(onPageChange(KPageWidgetItem*, KPageWidgetItem*)));

	QVBoxLayout *accountsLayout = new QVBoxLayout(accounts_page);
	accountsLayout->setContentsMargins(0, 0, 0, 0);

	QVBoxLayout *accountsLayoutView = new QVBoxLayout();
	accountsLayout->addLayout(accountsLayoutView);
	accountsView = new AccountsListView(accounts_page);
	accountsView->setSortingEnabled(false);
	accountsView->setAllColumnsShowFocus(true);
	accountsView->setColumnCount(4);
	QStringList accountsViewHeaders;
	accountsViewHeaders << i18n("Account / Category");
	accountsViewHeaders << i18n("Remaining Budget (%1)", QLocale().currencySymbol());
	accountsViewHeaders << i18n("Change (%1)", QLocale().currencySymbol());
	accountsViewHeaders << i18n("Total (%1)", QLocale().currencySymbol());
	accountsView->setHeaderLabels(accountsViewHeaders);
	accountsView->setRootIsDecorated(false);
	accountsView->header()->setSectionsClickable(false);
	accountsView->header()->setStretchLastSection(false);
	QSizePolicy sp = accountsView->sizePolicy();
	sp.setVerticalPolicy(QSizePolicy::MinimumExpanding);
	accountsView->setSizePolicy(sp);
	QFontMetrics fm(accountsView->font());
	accountsView->resizeColumnToContents(0);
	accountsView->resizeColumnToContents(1);
	accountsView->resizeColumnToContents(2);
	accountsView->resizeColumnToContents(3);
	int w = fm.width(i18nc("%2 remains of %1 budget", "%2 of %1", QLocale().toString(10000.0, 'f', MONETARY_DECIMAL_PLACES), QLocale().toString(100000.0, 'f', MONETARY_DECIMAL_PLACES)));
	if(accountsView->columnWidth(BUDGET_COLUMN) < w) accountsView->setColumnWidth(BUDGET_COLUMN, w);
	w = fm.width(QLocale().toString(999999999.99, 'f', MONETARY_DECIMAL_PLACES));
	if(accountsView->columnWidth(CHANGE_COLUMN) < w) accountsView->setColumnWidth(CHANGE_COLUMN, w);
	w = fm.width(QLocale().toString(999999999.99, 'f', MONETARY_DECIMAL_PLACES) + " ");
	if(accountsView->columnWidth(VALUE_COLUMN) < w) accountsView->setColumnWidth(VALUE_COLUMN, w);
	w = fm.width(i18n("Account / Category"));
	if(accountsView->columnWidth(0) < w) accountsView->setColumnWidth(0, w);
	accountsView->header()->setSectionResizeMode(0, QHeaderView::Stretch);
	assetsItem = new TotalListViewItem(accountsView, i18n("Accounts"), QString::null, QLocale().toString(0.0, 'f', MONETARY_DECIMAL_PLACES), QLocale().toString(0.0, 'f', MONETARY_DECIMAL_PLACES) + " ");
	incomesItem = new TotalListViewItem(accountsView, assetsItem, i18n("Incomes"), "-", QLocale().toString(0.0, 'f', MONETARY_DECIMAL_PLACES), QLocale().toString(0.0, 'f', MONETARY_DECIMAL_PLACES) + " ");
	expensesItem = new TotalListViewItem(accountsView, incomesItem, i18n("Expenses"), "-", QLocale().toString(0.0, 'f', MONETARY_DECIMAL_PLACES), QLocale().toString(0.0, 'f', MONETARY_DECIMAL_PLACES) + " ");
	accountsLayoutView->addWidget(accountsView);
	QHBoxLayout *accountsLayoutFooter = new QHBoxLayout();
	accountsLayoutView->addLayout(accountsLayoutFooter);
	accountsLayoutFooter->addStretch(1);
	footer1 = new QLabel(QString("* ") + i18n("Includes budgeted transactions"), accounts_page);
	accountsLayoutFooter->addWidget(footer1);
	footer1->hide();

	accountsTabs = new QTabWidget(accounts_page);
	accountsTabs->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);

	QWidget *periodWidget = new QWidget(accounts_page);
	accountsTabs->addTab(periodWidget, i18n("Period"));
	QVBoxLayout *accountsPeriodLayout = new QVBoxLayout(periodWidget);
	QHBoxLayout *accountsPeriodLayout2 = new QHBoxLayout();
	accountsPeriodLayout->addLayout(accountsPeriodLayout2);
	accountsPeriodFromButton = new QCheckBox(i18n("From"), periodWidget);
	accountsPeriodFromButton->setChecked(from_enabled);
	accountsPeriodLayout2->addWidget(accountsPeriodFromButton);
	accountsPeriodFromEdit = new QDateEdit(periodWidget);
	accountsPeriodFromEdit->setCalendarPopup(true);
	accountsPeriodFromEdit->setDate(from_date);
	accountsPeriodFromEdit->setEnabled(true);
	sp = accountsPeriodFromEdit->sizePolicy();
	sp.setHorizontalPolicy(QSizePolicy::Expanding);
	accountsPeriodFromEdit->setSizePolicy(sp);
	accountsPeriodLayout2->addWidget(accountsPeriodFromEdit);
	accountsPeriodLayout2->addWidget(new QLabel(i18n("To"), periodWidget));
	accountsPeriodToEdit = new QDateEdit(periodWidget);
	accountsPeriodToEdit->setCalendarPopup(true);
	accountsPeriodToEdit->setDate(to_date);
	accountsPeriodToEdit->setEnabled(true);
	accountsPeriodToEdit->setSizePolicy(sp);
	accountsPeriodLayout2->addWidget(accountsPeriodToEdit);
	QHBoxLayout *accountsPeriodLayout3 = new QHBoxLayout();
	accountsPeriodLayout->addLayout(accountsPeriodLayout3);
	QPushButton *prevYearButton = new QPushButton(QIcon::fromTheme("arrow-left-double"), "", periodWidget);	
	accountsPeriodLayout2->addWidget(prevYearButton);
	QPushButton *prevMonthButton = new QPushButton(QIcon::fromTheme("arrow-left"), "", periodWidget);
	accountsPeriodLayout2->addWidget(prevMonthButton);
	QPushButton *nextMonthButton = new QPushButton(QIcon::fromTheme("arrow-right"), "", periodWidget);
	accountsPeriodLayout2->addWidget(nextMonthButton);
	QPushButton *nextYearButton = new QPushButton(QIcon::fromTheme("arrow-right-double"), "", periodWidget);
	accountsPeriodLayout2->addWidget(nextYearButton);
	QPushButton *accountsPeriodButton = new QPushButton(i18n("Select Period"), periodWidget);
	QMenu *accountsPeriodMenu = new QMenu(this);
	ActionAP_1 = accountsPeriodMenu->addAction(i18n("Current Month"));
	ActionAP_2 = accountsPeriodMenu->addAction(i18n("Current Year"));
	ActionAP_3 = accountsPeriodMenu->addAction(i18n("Current Whole Month"));
	ActionAP_4 = accountsPeriodMenu->addAction(i18n("Current Whole Year"));
	ActionAP_5 = accountsPeriodMenu->addAction(i18n("Whole Past Month"));
	ActionAP_6 = accountsPeriodMenu->addAction(i18n("Whole Past Year"));
	ActionAP_7 = accountsPeriodMenu->addAction(i18n("Previous Month"));
	ActionAP_8 = accountsPeriodMenu->addAction(i18n("Previous Year"));
	accountsPeriodButton->setMenu(accountsPeriodMenu);
	accountsPeriodLayout3->addWidget(accountsPeriodButton);
	partialBudgetButton = new QCheckBox(i18n("Show partial budget"), periodWidget);
	accountsPeriodLayout3->addWidget(partialBudgetButton);
	accountsPeriodLayout3->addStretch(1);

	QWidget *budgetWidget = new QWidget(accounts_page);
	accountsTabs->addTab(budgetWidget, i18n("Edit Budget"));
	QVBoxLayout *budgetLayout = new QVBoxLayout(budgetWidget);
	QHBoxLayout *budgetLayout2 = new QHBoxLayout();
	budgetLayout->addLayout(budgetLayout2);
	budgetButton = new QCheckBox(i18n("Budget:"), budgetWidget);
	budgetButton->setChecked(false);
	budgetLayout2->addWidget(budgetButton);
	budgetEdit = new EqonomizeValueEdit(false, budgetWidget);
	budgetEdit->setEnabled(false);
	sp = budgetEdit->sizePolicy();
	sp.setHorizontalPolicy(QSizePolicy::Expanding);
	budgetEdit->setSizePolicy(sp);
	budgetEdit->setFocus();
	budgetLayout2->addWidget(budgetEdit);
	budgetLayout2->addWidget(new QLabel(i18n("Month:"), budgetWidget));
	budgetMonthEdit = new EqonomizeMonthSelector(budgetWidget);
	sp = budgetMonthEdit->sizePolicy();
	sp.setHorizontalPolicy(QSizePolicy::Expanding);
	budgetMonthEdit->setSizePolicy(sp);
	budgetLayout2->addWidget(budgetMonthEdit);
	QHBoxLayout *budgetLayout3 = new QHBoxLayout();
	budgetLayout->addLayout(budgetLayout3);
	budgetLayout3->addWidget(new QLabel(i18n("Result previous month:"), budgetWidget));
	prevMonthBudgetLabel = new QLabel("-", budgetWidget);
	budgetLayout3->addWidget(prevMonthBudgetLabel);
	budgetLayout3->addStretch(1);

	accountsLayout->addWidget(accountsTabs);

	accountPopupMenu = NULL;

	connect(budgetMonthEdit, SIGNAL(dateChanged(const QDate&)), this, SLOT(budgetMonthChanged(const QDate&)));
	connect(budgetEdit, SIGNAL(valueChanged(double)), this, SLOT(budgetChanged(double)));
	connect(budgetEdit, SIGNAL(returnPressed()), this, SLOT(budgetEditReturnPressed()));
	connect(budgetButton, SIGNAL(toggled(bool)), this, SLOT(budgetToggled(bool)));
	connect(budgetButton, SIGNAL(toggled(bool)), budgetEdit, SLOT(setEnabled(bool)));
	connect(partialBudgetButton, SIGNAL(toggled(bool)), this, SLOT(setPartialBudget(bool)));
	connect(accountsPeriodMenu, SIGNAL(triggered(QAction*)), this, SLOT(periodSelected(QAction*)));
	connect(prevMonthButton, SIGNAL(clicked()), this, SLOT(prevMonth()));
	connect(nextMonthButton, SIGNAL(clicked()), this, SLOT(nextMonth()));
	connect(prevYearButton, SIGNAL(clicked()), this, SLOT(prevYear()));
	connect(nextYearButton, SIGNAL(clicked()), this, SLOT(nextYear()));
	connect(accountsPeriodFromButton, SIGNAL(toggled(bool)), accountsPeriodFromEdit, SLOT(setEnabled(bool)));
	connect(accountsPeriodFromButton, SIGNAL(toggled(bool)), this, SLOT(filterAccounts()));
	connect(accountsPeriodFromEdit, SIGNAL(dateChanged(const QDate&)), this, SLOT(accountsPeriodFromChanged(const QDate&)));
	connect(accountsPeriodToEdit, SIGNAL(dateChanged(const QDate&)), this, SLOT(accountsPeriodToChanged(const QDate&)));
	connect(accountsView, SIGNAL(itemSelectionChanged()), this, SLOT(accountsSelectionChanged()));
	connect(accountsView, SIGNAL(doubleClicked(QTreeWidgetItem*, int)), this, SLOT(accountExecuted(QTreeWidgetItem*, int)));
	connect(accountsView, SIGNAL(returnPressed(QTreeWidgetItem*)), this, SLOT(accountExecuted(QTreeWidgetItem*)));
	accountsView->setContextMenuPolicy(Qt::CustomContextMenu);
	connect(accountsView, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(popupAccountsMenu(const QPoint&)));

	expensesLayout = new QVBoxLayout(expenses_page);
	expensesLayout->setContentsMargins(0, 0, 0, 0);
	expensesWidget = new TransactionListWidget(b_extra, TRANSACTION_TYPE_EXPENSE, budget, this, expenses_page);
	expensesLayout->addWidget(expensesWidget);

	incomesLayout = new QVBoxLayout(incomes_page);
	incomesLayout->setContentsMargins(0, 0, 0, 0);
	incomesWidget = new TransactionListWidget(b_extra, TRANSACTION_TYPE_INCOME, budget, this, incomes_page);
	incomesLayout->addWidget(incomesWidget);

	transfersLayout = new QVBoxLayout(transfers_page);
	transfersLayout->setContentsMargins(0, 0, 0, 0);
	transfersWidget = new TransactionListWidget(b_extra, TRANSACTION_TYPE_TRANSFER, budget, this, transfers_page);
	transfersLayout->addWidget(transfersWidget);

	QWidget *securities_page_bak = securities_page;
	QVBoxLayout *securitiesLayout_pre = new QVBoxLayout(securities_page);
	securitiesLayout_pre->setContentsMargins(0, 0, 0, 0);
	securities_page = new QWidget(securities_page);
	securitiesLayout_pre->addWidget(securities_page);
	QVBoxLayout *securitiesLayout = new QVBoxLayout(securities_page);

	QDialogButtonBox *securitiesButtons = new QDialogButtonBox(securities_page);
	newSecurityButton = securitiesButtons->addButton(i18n("New Security..."), QDialogButtonBox::ActionRole);
	newSecurityButton->setEnabled(true);
	newSecurityTransactionButton = securitiesButtons->addButton(i18n("New Transaction"), QDialogButtonBox::ActionRole);
	QMenu *newSecurityTransactionMenu = new QMenu(this);
	newSecurityTransactionButton->setMenu(newSecurityTransactionMenu);
	setQuotationButton = securitiesButtons->addButton(i18n("Set Quotation..."), QDialogButtonBox::ActionRole);
	setQuotationButton->setEnabled(false);
	securitiesLayout->addWidget(securitiesButtons);

	QVBoxLayout *securitiesViewLayout = new QVBoxLayout();
	securitiesLayout->addLayout(securitiesViewLayout);
	securitiesView = new EqonomizeTreeWidget(securities_page);
	securitiesView->setSortingEnabled(true);
	securitiesView->sortByColumn(0, Qt::AscendingOrder);
	securitiesView->setAllColumnsShowFocus(true);
	securitiesView->setColumnCount(9);
	QStringList securitiesViewHeaders;
	securitiesViewHeaders << i18n("Name");
	securitiesViewHeaders << i18n("Value");
	securitiesViewHeaders << i18n("Shares");
	securitiesViewHeaders << i18n("Quotation");
	securitiesViewHeaders << i18n("Cost");
	securitiesViewHeaders << i18n("Profit");
	securitiesViewHeaders << i18n("Yearly Rate");
	securitiesViewHeaders << i18n("Type");
	securitiesViewHeaders << i18n("Account");
	securitiesView->setHeaderLabels(securitiesViewHeaders);
	securitiesView->setRootIsDecorated(false);
	sp = securitiesView->sizePolicy();
	sp.setVerticalPolicy(QSizePolicy::MinimumExpanding);
	securitiesView->setSizePolicy(sp);
	securitiesViewLayout->addWidget(securitiesView);

	securitiesStatLabel = new QLabel(securities_page);
	securitiesViewLayout->addWidget(securitiesStatLabel);

	QGroupBox *periodGroup = new QGroupBox(i18n("Statistics Period"), securities_page);
	QVBoxLayout *securitiesPeriodLayout = new QVBoxLayout(periodGroup);
	QHBoxLayout *securitiesPeriodLayout2 = new QHBoxLayout();
	securitiesPeriodLayout->addLayout(securitiesPeriodLayout2);
	securitiesPeriodFromButton = new QCheckBox(i18n("From"), periodGroup);
	securitiesPeriodFromButton->setChecked(false);
	securitiesPeriodLayout2->addWidget(securitiesPeriodFromButton);
	securitiesPeriodFromEdit = new QDateEdit(periodGroup);
	securitiesPeriodFromEdit->setCalendarPopup(true);
	securities_from_date.setDate(curdate.year(), 1, 1);
	securitiesPeriodFromEdit->setDate(securities_from_date);
	securitiesPeriodFromEdit->setEnabled(false);
	sp = securitiesPeriodFromEdit->sizePolicy();
	sp.setHorizontalPolicy(QSizePolicy::Expanding);
	securitiesPeriodFromEdit->setSizePolicy(sp);
	securitiesPeriodLayout2->addWidget(securitiesPeriodFromEdit);
	securitiesPeriodLayout2->addWidget(new QLabel(i18n("To"), periodGroup));
	securitiesPeriodToEdit = new QDateEdit(periodGroup);
	securitiesPeriodToEdit->setCalendarPopup(true);
	securities_to_date = curdate;
	securitiesPeriodToEdit->setDate(securities_to_date);
	securitiesPeriodToEdit->setEnabled(true);
	securitiesPeriodToEdit->setSizePolicy(sp);
	securitiesPeriodLayout2->addWidget(securitiesPeriodToEdit);
	securitiesLayout->addWidget(periodGroup);
	QHBoxLayout *securitiesPeriodLayout3 = new QHBoxLayout();
	securitiesPeriodLayout2->addLayout(securitiesPeriodLayout3);
	QPushButton *securitiesPrevYearButton = new QPushButton(QIcon::fromTheme("arrow-left-double"), "", periodGroup);
	securitiesPeriodLayout3->addWidget(securitiesPrevYearButton);
	QPushButton *securitiesPrevMonthButton = new QPushButton(QIcon::fromTheme("arrow-left"), "", periodGroup);
	securitiesPeriodLayout3->addWidget(securitiesPrevMonthButton);
	QPushButton *securitiesNextMonthButton = new QPushButton(QIcon::fromTheme("arrow-right"), "", periodGroup);
	securitiesPeriodLayout3->addWidget(securitiesNextMonthButton);
	QPushButton *securitiesNextYearButton = new QPushButton(QIcon::fromTheme("arrow-right-double"), "", periodGroup);
	securitiesPeriodLayout3->addWidget(securitiesNextYearButton);

	securities_page = securities_page_bak;

	securitiesPopupMenu = NULL;

	updateSecuritiesStatistics();

	connect(securitiesPrevMonthButton, SIGNAL(clicked()), this, SLOT(securitiesPrevMonth()));
	connect(securitiesNextMonthButton, SIGNAL(clicked()), this, SLOT(securitiesNextMonth()));
	connect(securitiesPrevYearButton, SIGNAL(clicked()), this, SLOT(securitiesPrevYear()));
	connect(securitiesNextYearButton, SIGNAL(clicked()), this, SLOT(securitiesNextYear()));
	connect(securitiesPeriodFromButton, SIGNAL(toggled(bool)), securitiesPeriodFromEdit, SLOT(setEnabled(bool)));
	connect(securitiesPeriodFromButton, SIGNAL(toggled(bool)), this, SLOT(updateSecurities()));
	connect(securitiesPeriodFromEdit, SIGNAL(dateChanged(const QDate&)), this, SLOT(securitiesPeriodFromChanged(const QDate&)));
	connect(securitiesPeriodToEdit, SIGNAL(dateChanged(const QDate&)), this, SLOT(securitiesPeriodToChanged(const QDate&)));
	connect(newSecurityButton, SIGNAL(clicked()), this, SLOT(newSecurity()));
	connect(setQuotationButton, SIGNAL(clicked()), this, SLOT(setQuotation()));
	connect(securitiesView, SIGNAL(itemSelectionChanged()), this, SLOT(securitiesSelectionChanged()));
	connect(securitiesView, SIGNAL(doubleClicked(QTreeWidgetItem*, int)), this, SLOT(securitiesExecuted(QTreeWidgetItem*, int)));
	connect(securitiesView, SIGNAL(returnPressed(QTreeWidgetItem*)), this, SLOT(securitiesExecuted(QTreeWidgetItem*)));
	securitiesView->setContextMenuPolicy(Qt::CustomContextMenu);
	connect(securitiesView, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(popupSecuritiesMenu(const QPoint&)));

	QVBoxLayout *scheduleLayout = new QVBoxLayout(schedule_page);
	scheduleLayout->setContentsMargins(0, 0, 0, 0);

	QDialogButtonBox *scheduleButtons = new QDialogButtonBox(schedule_page);
	newScheduleButton = scheduleButtons->addButton(i18n("New Schedule"), QDialogButtonBox::ActionRole);
	QMenu *newScheduleMenu = new QMenu(this);
	newScheduleButton->setMenu(newScheduleMenu);
	editScheduleButton = scheduleButtons->addButton(i18n("Edit"), QDialogButtonBox::ActionRole);
	editScheduleMenu = new QMenu(this);
	editScheduleButton->setMenu(editScheduleMenu);
	editScheduleButton->setEnabled(false);
	removeScheduleButton = scheduleButtons->addButton(KStandardGuiItem::remove().text(), QDialogButtonBox::ActionRole);
	removeScheduleMenu = new QMenu(this);
	removeScheduleButton->setMenu(removeScheduleMenu);
	removeScheduleButton->setEnabled(false);
	scheduleLayout->addWidget(scheduleButtons);

	scheduleView = new EqonomizeTreeWidget(schedule_page);
	scheduleView->setAllColumnsShowFocus(true);
	scheduleView->setColumnCount(7);
	QStringList scheduleViewHeaders;
	scheduleViewHeaders << i18n("Next Occurrence");
	scheduleViewHeaders << i18n("Type");
	scheduleViewHeaders << i18n("Description");
	scheduleViewHeaders << i18n("Amount");
	scheduleViewHeaders << i18n("From");
	scheduleViewHeaders << i18n("To");
	scheduleViewHeaders << i18n("Comments");
	scheduleView->setHeaderLabels(scheduleViewHeaders);
	scheduleView->setRootIsDecorated(false);
	sp = scheduleView->sizePolicy();
	sp.setVerticalPolicy(QSizePolicy::MinimumExpanding);
	scheduleView->setSizePolicy(sp);
	scheduleLayout->addWidget(scheduleView);
	scheduleView->sortByColumn(0, Qt::AscendingOrder);
	scheduleView->setSortingEnabled(true);

	schedulePopupMenu = NULL;

	connect(scheduleView, SIGNAL(itemSelectionChanged()), this, SLOT(scheduleSelectionChanged()));
	connect(scheduleView, SIGNAL(doubleClicked(QTreeWidgetItem*, int)), this, SLOT(scheduleExecuted(QTreeWidgetItem*)));
	connect(scheduleView, SIGNAL(returnPressed(QTreeWidgetItem*)), this, SLOT(scheduleExecuted(QTreeWidgetItem*)));
	scheduleView->setContextMenuPolicy(Qt::CustomContextMenu);
	connect(scheduleView, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(popupScheduleMenu(const QPoint&)));
	connect(scheduleView, SIGNAL(itemSelectionChanged()), this, SLOT(updateTransactionActions()));

	setupActions();

	ActionSelectInitialPeriod->setCurrentItem(initial_period_pre);

	newScheduleMenu->addAction(ActionNewExpense);
	newScheduleMenu->addAction(ActionNewIncome);
	newScheduleMenu->addAction(ActionNewTransfer);
	editScheduleMenu->addAction(ActionEditSchedule);
	editScheduleMenu->addAction(ActionEditOccurrence);
	removeScheduleMenu->addAction(ActionDeleteSchedule);
	removeScheduleMenu->addAction(ActionDeleteOccurrence);

	newSecurityTransactionMenu->addAction(ActionBuyShares);
	newSecurityTransactionMenu->addAction(ActionSellShares);
	newSecurityTransactionMenu->addAction(ActionNewSecurityTrade);
	newSecurityTransactionMenu->addAction(ActionNewDividend);
	newSecurityTransactionMenu->addAction(ActionNewReinvestedDividend);

	setupGUI(ToolBar | Keys | Save | Create);

	setAutoSaveSettings();
	readOptions();

	if(first_run) {
		QDesktopWidget desktop;
		resize(QSize(750, 650).boundedTo(desktop.availableGeometry(this).size()));
	}
	

	QTimer *scheduleTimer = new QTimer();
	connect(scheduleTimer, SIGNAL(timeout()), this, SLOT(checkSchedule()));
	scheduleTimer->start(1000 * 60 * 30);

	auto_save_timeout = true;

	QTimer *autoSaveTimer = new QTimer();
	connect(autoSaveTimer, SIGNAL(timeout()), this, SLOT(onAutoSaveTimeout()));
	autoSaveTimer->start(1000 * 60 * 1);

	QTimer *dateTimer = new QTimer();
	connect(dateTimer, SIGNAL(timeout()), this, SLOT(checkDate()));
	dateTimer->start(1000 * 60);

}
Eqonomize::~Eqonomize() {}

void Eqonomize::useExtraProperties(bool b) {

	b_extra = b;

	delete expensesWidget;
	expensesWidget = new TransactionListWidget(b_extra, TRANSACTION_TYPE_EXPENSE, budget, this, expenses_page);
	expensesLayout->addWidget(expensesWidget);
	expensesWidget->updateAccounts();
	expensesWidget->transactionsReset();
	if(tabs->currentPage()->widget() == expenses_page) {
		expensesWidget->onDisplay();
		updateTransactionActions();
	}
	expensesWidget->show();

	delete incomesWidget;
	incomesWidget = new TransactionListWidget(b_extra, TRANSACTION_TYPE_INCOME, budget, this, incomes_page);
	incomesLayout->addWidget(incomesWidget);
	incomesWidget->updateAccounts();
	incomesWidget->transactionsReset();
	if(tabs->currentPage()->widget() == incomes_page) {
		incomesWidget->onDisplay();
		updateTransactionActions();
	}
	incomesWidget->show();

}

void Eqonomize::checkDate() {
	if(QDate::currentDate() != prev_cur_date) {
		QDate prev_cur_date_bak = prev_cur_date;
		prev_cur_date = QDate::currentDate();
		if(to_date == prev_cur_date_bak) {
			accountsPeriodToEdit->setDate(prev_cur_date);
			accountsPeriodToChanged(prev_cur_date);
		}
		if(securities_to_date == prev_cur_date_bak) {
			securitiesPeriodToEdit->setDate(prev_cur_date);
			securitiesPeriodToChanged(prev_cur_date);
		}
		expensesWidget->currentDateChanged(prev_cur_date_bak, prev_cur_date);
		incomesWidget->currentDateChanged(prev_cur_date_bak, prev_cur_date);
		transfersWidget->currentDateChanged(prev_cur_date_bak, prev_cur_date);
	}
}

void Eqonomize::showExpenses() {
	tabs->setCurrentPage(expenses_page_item);
	onPageChange(expenses_page);
}
void Eqonomize::showIncomes() {
	tabs->setCurrentPage(incomes_page_item);
	onPageChange(incomes_page);
}
void Eqonomize::showTransfers() {
	tabs->setCurrentPage(transfers_page_item);
	onPageChange(transfers_page);
}

void Eqonomize::setPartialBudget(bool b) {
	partial_budget = b;
	filterAccounts();
}
void Eqonomize::budgetEditReturnPressed() {
	QTreeWidgetItem *i = selectedItem(accountsView);
	if(!i) return;
	int index = accountsView->indexOfTopLevelItem(i);
	index++;
	i = accountsView->topLevelItem(index);
	if(!i && account_items.contains(i) && account_items[i]->type() == ACCOUNT_TYPE_INCOMES) i = expensesItem->child(0);
	if(i) {
		i->setSelected(true);
		if(budgetEdit->isEnabled()) budgetEdit->lineEdit()->selectAll();
		else budgetButton->setFocus();
	}
}
void Eqonomize::budgetMonthChanged(const QDate &date) {
	accountsPeriodFromEdit->blockSignals(true);
	accountsPeriodFromButton->blockSignals(true);
	accountsPeriodToEdit->blockSignals(true);
	from_date = date;
	to_date = from_date.addDays(from_date.daysInMonth() - 1);
	accountsPeriodFromButton->setChecked(true);
	accountsPeriodFromEdit->setDate(from_date);
	accountsPeriodToEdit->setDate(to_date);
	accountsPeriodFromEdit->blockSignals(false);
	accountsPeriodFromButton->blockSignals(false);
	accountsPeriodToEdit->blockSignals(false);
	filterAccounts();
}
void Eqonomize::budgetChanged(double value) {
	QTreeWidgetItem *i = selectedItem(accountsView);
	if(!account_items.contains(i)) return;
	Account *account = account_items[i];
	if(account->type() == ACCOUNT_TYPE_ASSETS) return;
	CategoryAccount *ca = (CategoryAccount*) account;
	QDate month = budgetMonthEdit->date();
	ca->mbudgets[month] = value;
	ca = budget->incomesAccounts.first();
	while(ca) {
		if(!ca->mbudgets.contains(month)) {
			value = ca->monthlyBudget(month);
			ca->mbudgets[month] = value;
		}
		ca = budget->incomesAccounts.next();
	}
	ca = budget->expensesAccounts.first();
	while(ca) {
		if(!ca->mbudgets.contains(month)) {
			value = ca->monthlyBudget(month);
			ca->mbudgets[month] = value;
		}
		ca = budget->expensesAccounts.next();
	}
	setModified(true);
	filterAccounts();
}
void Eqonomize::budgetToggled(bool b) {
	QTreeWidgetItem *i = selectedItem(accountsView);
	if(!account_items.contains(i)) return;
	Account *account = account_items[i];
	if(account->type() == ACCOUNT_TYPE_ASSETS) return;
	CategoryAccount *ca = (CategoryAccount*) account;
	QDate month = budgetMonthEdit->date();
	if(b) ca->mbudgets[month] = budgetEdit->value();
	else ca->mbudgets[month] = -1;
	ca = budget->incomesAccounts.first();
	while(ca) {
		if(!ca->mbudgets.contains(month)) {
			double value = ca->monthlyBudget(month);
			ca->mbudgets[month] = value;
		}
		ca = budget->incomesAccounts.next();
	}
	ca = budget->expensesAccounts.first();
	while(ca) {
		if(!ca->mbudgets.contains(month)) {
			double value = ca->monthlyBudget(month);
			ca->mbudgets[month] = value;
		}
		ca = budget->expensesAccounts.next();
	}
	setModified(true);
	filterAccounts();
}
void Eqonomize::periodSelected(QAction *a) {
	accountsPeriodFromEdit->blockSignals(true);
	accountsPeriodFromButton->blockSignals(true);
	accountsPeriodToEdit->blockSignals(true);	
	QDate curdate = QDate::currentDate();
	if(a == ActionAP_1) {
		from_date.setDate(curdate.year(), curdate.month(), 1);
		to_date = curdate;
	} else if(a == ActionAP_2) {
		from_date.setDate(curdate.year(), 1, 1);
		to_date = curdate;
	} else if(a == ActionAP_3) {
		from_date.setDate(curdate.year(), curdate.month(), 1);
		to_date = from_date.addDays(curdate.daysInMonth() - 1);
	} else if(a == ActionAP_4) {
		from_date.setDate(curdate.year(), 1, 1);
		to_date = from_date.addDays(curdate.daysInYear() - 1);
	} else if(a == ActionAP_5) {
		to_date = curdate;
		from_date = to_date.addMonths(-1).addDays(1);
	} else if(a == ActionAP_6) {
		to_date = curdate;
		from_date = to_date.addYears(-1).addDays(1);
	} else if(a == ActionAP_7) {
		curdate = curdate.addMonths(-1);
		from_date.setDate(curdate.year(), curdate.month(), 1);
		to_date = from_date.addDays(curdate.daysInMonth() - 1);
	} else if(a == ActionAP_8) {
		curdate = curdate.addYears(-1);
		from_date.setDate(curdate.year(), 1, 1);
		to_date = from_date.addDays(curdate.daysInYear() - 1);
	}
	accountsPeriodFromButton->setChecked(true);
	accountsPeriodFromEdit->setEnabled(true);
	accountsPeriodFromEdit->setDate(from_date);
	accountsPeriodToEdit->setDate(to_date);
	accountsPeriodFromEdit->blockSignals(false);
	accountsPeriodFromButton->blockSignals(false);
	accountsPeriodToEdit->blockSignals(false);
	filterAccounts();
}

void Eqonomize::newSecurity() {
	EditSecurityDialog *dialog = new EditSecurityDialog(budget, this, i18n("New Security"));
	if(dialog->checkAccount() && dialog->exec() == QDialog::Accepted) {
		Security *security = dialog->newSecurity();
		if(security) {
			budget->addSecurity(security);
			appendSecurity(security);
			updateSecurityAccount(security->account());
			setModified(true);
		}
	}
	dialog->deleteLater();
}
void Eqonomize::editSecurity(QTreeWidgetItem *i) {
	if(!i) return;
	Security *security = ((SecurityListViewItem*) i)->security();
	EditSecurityDialog *dialog = new EditSecurityDialog(budget, this, i18n("Edit Security"));
	dialog->setSecurity(security);
	if(dialog->checkAccount() && dialog->exec() == QDialog::Accepted) {
		if(dialog->modifySecurity(security)) {
			updateSecurity(i);
			updateSecurityAccount(security->account());
			setModified(true);
			incomesWidget->filterTransactions();
			transfersWidget->filterTransactions();
		}
	}
	dialog->deleteLater();
}
void Eqonomize::editSecurity() {
	QTreeWidgetItem *i = selectedItem(securitiesView);
	if(i == NULL) return;
	editSecurity(i);
}
void Eqonomize::updateSecuritiesStatistics() {
	securitiesStatLabel->setText(QString("<div align=\"right\"><b>%1</b> %5 &nbsp; <b>%2</b> %6 &nbsp; <b>%3</b> %7 &nbsp; <b>%4</b> %8\%</div>").arg(i18n("Total value:")).arg(i18n("Cost:")).arg(i18n("Profit:")).arg(i18n("Rate:")).arg(QLocale().toCurrencyString(total_value), QLocale().toCurrencyString(total_cost)).arg(QLocale().toCurrencyString(total_profit)).arg(QLocale().toString(total_rate * 100)));
}
void Eqonomize::deleteSecurity() {
	SecurityListViewItem *i = (SecurityListViewItem*) selectedItem(securitiesView);
	if(i == NULL) return;
	Security *security = i->security();
	bool has_trans = budget->securityHasTransactions(security);
	if(!has_trans || KMessageBox::warningContinueCancel(this, i18n("Are you sure you want to delete the security \"%1\" and all associated transactions?", security->name())) == KMessageBox::Continue) {
		total_value -= i->value;
		total_rate *= total_cost;
		total_cost -= i->cost;
		total_rate -= i->cost * i->rate;
		if(total_cost != 0.0) total_rate /= total_cost;
		total_profit -= i->profit;
		updateSecuritiesStatistics();
		delete i;
		budget->removeSecurity(security);
		if(has_trans) {
			filterAccounts();
			updateScheduledTransactions();
			expensesWidget->filterTransactions();
			incomesWidget->filterTransactions();
			transfersWidget->filterTransactions();
			emit transactionsModified();
		} else {
			updateSecurityAccount(security->account());
		}
		setModified(true);
	}
}
void Eqonomize::buySecurities() {
	SecurityListViewItem *i = (SecurityListViewItem*) selectedItem(securitiesView);
	newScheduledTransaction(TRANSACTION_TYPE_SECURITY_BUY, i == NULL ? NULL : i->security(), true);
}
void Eqonomize::sellSecurities() {
	SecurityListViewItem *i = (SecurityListViewItem*) selectedItem(securitiesView);
	newScheduledTransaction(TRANSACTION_TYPE_SECURITY_SELL, i == NULL ? NULL : i->security(), true);
}
void Eqonomize::newDividend() {
	SecurityListViewItem *i = (SecurityListViewItem*) selectedItem(securitiesView);
	newScheduledTransaction(TRANSACTION_TYPE_INCOME, i == NULL ? NULL : i->security(), true);
}
void Eqonomize::editReinvestedDividend(ReinvestedDividend *rediv, Security *security) {
	editReinvestedDividend(rediv, security, this);
}
bool Eqonomize::editReinvestedDividend(ReinvestedDividend *rediv, Security *security, QWidget *parent) {
	EditReinvestedDividendDialog *dialog = new EditReinvestedDividendDialog(budget, security, true, parent);
	dialog->setDividend(rediv);
	if(dialog->exec() == QDialog::Accepted) {
		if(dialog->modifyDividend(rediv)) {
			security->reinvestedDividends.setAutoDelete(false);
			security->reinvestedDividends.removeRef(rediv);
			security->reinvestedDividends.setAutoDelete(true);
			updateSecurity(security);
			updateSecurityAccount(security->account());
			security = dialog->selectedSecurity();
			security->reinvestedDividends.inSort(rediv);
			updateSecurity(security);
			updateSecurityAccount(security->account());
			setModified(true);
			dialog->deleteLater();
			return true;
		}
	}
	dialog->deleteLater();
	return false;
}
void Eqonomize::newReinvestedDividend() {
	if(budget->securities.count() == 0) {
		KMessageBox::error(this, i18n("No security available."));
		return;
	}
	SecurityListViewItem *i = (SecurityListViewItem*) selectedItem(securitiesView);
	Security *security = NULL;
	if(i) security = i->security();
	EditReinvestedDividendDialog *dialog = new EditReinvestedDividendDialog(budget, security, true, this);
	if(dialog->exec() == QDialog::Accepted) {
		ReinvestedDividend *rediv = dialog->createDividend();
		if(rediv) {
			security = dialog->selectedSecurity();
			security->reinvestedDividends.inSort(rediv);
			updateSecurity(security);
			updateSecurityAccount(security->account());
			setModified(true);
		}
	}
	dialog->deleteLater();
}
void Eqonomize::editSecurityTrade(SecurityTrade *ts) {
	editSecurityTrade(ts, this);
}
bool Eqonomize::editSecurityTrade(SecurityTrade *ts, QWidget *parent) {
	EditSecurityTradeDialog *dialog = new EditSecurityTradeDialog(budget, ts->from_security, parent);
	dialog->setSecurityTrade(ts);
	if(dialog->exec() == QDialog::Accepted) {
		SecurityTrade *ts_new = dialog->createSecurityTrade();
		if(ts_new) {
			budget->removeSecurityTrade(ts, true);
			budget->addSecurityTrade(ts_new);
			updateSecurity(ts_new->from_security);
			updateSecurity(ts_new->to_security);
			if(ts->to_security != ts_new->to_security) updateSecurity(ts->to_security);
			if(ts->from_security != ts_new->from_security) updateSecurity(ts->from_security);
			updateSecurityAccount(ts_new->from_security->account());
			if(ts_new->to_security->account() != ts_new->from_security->account()) {
				updateSecurityAccount(ts_new->to_security->account());
			}
			if(ts->to_security->account() != ts_new->to_security->account() && ts->to_security->account() != ts_new->from_security->account()) {
				updateSecurityAccount(ts->to_security->account());
			}
			setModified(true);
			delete ts;
			dialog->deleteLater();
			return true;
		}
	}
	dialog->deleteLater();
	return false;
}
void Eqonomize::newSecurityTrade() {
	SecurityListViewItem *i = (SecurityListViewItem*) selectedItem(securitiesView);
	EditSecurityTradeDialog *dialog = new EditSecurityTradeDialog(budget, i == NULL ? NULL : i->security(), this);
	if(dialog->checkSecurities() && dialog->exec() == QDialog::Accepted) {
		SecurityTrade *ts = dialog->createSecurityTrade();
		if(ts) {
			budget->addSecurityTrade(ts);
			updateSecurity(ts->from_security);
			updateSecurity(ts->to_security);
			updateSecurityAccount(ts->from_security->account());
			if(ts->to_security->account() != ts->from_security->account()) {
				updateSecurityAccount(ts->to_security->account());
			}
			setModified(true);
		}
	}
	dialog->deleteLater();
}
void Eqonomize::setQuotation() {
	SecurityListViewItem *i = (SecurityListViewItem*) selectedItem(securitiesView);
	if(i == NULL) return;
	QDialog *dialog = new QDialog(this, 0);
	dialog->setWindowTitle(i18n("Set Quotation (%1)", i->security()->name()));
	dialog->setModal(true);
	QVBoxLayout *box1 = new QVBoxLayout(dialog);
	QGridLayout *grid = new QGridLayout();
	box1->addLayout(grid);
	grid->addWidget(new QLabel(i18n("Price per share:"), dialog), 0, 0);
	EqonomizeValueEdit *quotationEdit = new EqonomizeValueEdit(0.01, INT_MAX / pow(10, MONETARY_DECIMAL_PLACES) - 1.0, pow(10, -MONETARY_DECIMAL_PLACES), i->security()->getQuotation(QDate::currentDate()), MONETARY_DECIMAL_PLACES, true, dialog);
	quotationEdit->setFocus();
	grid->addWidget(quotationEdit, 0, 1);
	grid->addWidget(new QLabel(i18n("Date:"), dialog), 1, 0);
	QDateEdit *dateEdit = new QDateEdit(dialog);
	dateEdit->setCalendarPopup(true);
	grid->addWidget(dateEdit, 1, 1);
	QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
	buttonBox->button(QDialogButtonBox::Ok)->setShortcut(Qt::CTRL | Qt::Key_Return);
	buttonBox->button(QDialogButtonBox::Ok)->setDefault(true);
	connect(buttonBox->button(QDialogButtonBox::Cancel), SIGNAL(clicked()), dialog, SLOT(reject()));
	connect(buttonBox->button(QDialogButtonBox::Ok), SIGNAL(clicked()), dialog, SLOT(accept()));
	box1->addWidget(buttonBox);
	while(dialog->exec() == QDialog::Accepted) {
		QDate date = dateEdit->date();
		if(!date.isValid()) {
			KMessageBox::error(this, i18n("Invalid date."));
		} else if(date > QDate::currentDate()) {
			KMessageBox::error(this, i18n("Future dates are not allowed."));
		} else {
			i->security()->setQuotation(date, quotationEdit->value());
			updateSecurity(i);
			updateSecurityAccount(i->security()->account(), true);
			setModified(true);
			break;
		}
	}
	dialog->deleteLater();
}
void Eqonomize::editQuotations() {
	SecurityListViewItem *i = (SecurityListViewItem*) selectedItem(securitiesView);
	if(!i) return;
	Security *security = i->security();
	EditQuotationsDialog *dialog = new EditQuotationsDialog(this);
	dialog->setSecurity(security);
	if(dialog->exec() == QDialog::Accepted) {
		dialog->modifyQuotations(security);
		updateSecurity(i);
		updateSecurityAccount(security->account());
		setModified(true);
	}
	dialog->deleteLater();
}
void Eqonomize::editSecurityTransactions() {
	SecurityListViewItem *i = (SecurityListViewItem*) selectedItem(securitiesView);
	if(!i) return;
	SecurityTransactionsDialog *dialog = new SecurityTransactionsDialog(i->security(), this, i18n("Security Transactions"));
	dialog->exec();
	dialog->deleteLater();
}
void Eqonomize::securitiesSelectionChanged() {
	SecurityListViewItem *i = (SecurityListViewItem*) selectedItem(securitiesView);
	setQuotationButton->setEnabled(i != NULL);
	ActionEditSecurity->setEnabled(i != NULL);
	ActionDeleteSecurity->setEnabled(i != NULL);
	ActionSetQuotation->setEnabled(i != NULL);
	ActionEditQuotations->setEnabled(i != NULL);
	ActionEditSecurityTransactions->setEnabled(i != NULL);
}
void Eqonomize::securitiesExecuted(QTreeWidgetItem *i) {
	if(i == NULL) return;
	editSecurity(i);
}
void Eqonomize::securitiesExecuted(QTreeWidgetItem *i, int c) {
	if(i == NULL) return;
	switch(c) {
		case 4: {}
		case 5: {}
		case 6: {}
		case 1: {}
		case 2: {
			editSecurityTransactions();
			break;
		}
		case 3: {
			setQuotation();
			break;
		}
		case 8: {
			if(((SecurityListViewItem*) i)->security()) {
				editAccount(((SecurityListViewItem*) i)->security()->account());
			}
			break;
		}
		default: {
			editSecurity(i);
			break;
		}
	}
}

void Eqonomize::popupSecuritiesMenu(const QPoint &p) {
	if(!securitiesPopupMenu) {
		securitiesPopupMenu = new QMenu(this);
		securitiesPopupMenu->addAction(ActionNewSecurity);
		securitiesPopupMenu->addAction(ActionEditSecurity);
		securitiesPopupMenu->addAction(ActionDeleteSecurity);
		securitiesPopupMenu->addSeparator();
		securitiesPopupMenu->addAction(ActionBuyShares);
		securitiesPopupMenu->addAction(ActionSellShares);
		securitiesPopupMenu->addAction(ActionNewSecurityTrade);
		securitiesPopupMenu->addAction(ActionNewDividend);
		securitiesPopupMenu->addAction(ActionNewReinvestedDividend);
		securitiesPopupMenu->addAction(ActionEditSecurityTransactions);
		securitiesPopupMenu->addSeparator();
		securitiesPopupMenu->addAction(ActionSetQuotation);
		securitiesPopupMenu->addAction(ActionEditQuotations);
	}
	securitiesPopupMenu->popup(securitiesView->viewport()->mapToGlobal(p));
}
void Eqonomize::appendSecurity(Security *security) {
	double value = 0.0, cost = 0.0, rate = 0.0, profit = 0.0, quotation = 0.0, shares = 0.0;
	value = security->value(securities_to_date, true);
	cost = security->cost(securities_to_date);
	if(securities_to_date > QDate::currentDate()) quotation = security->expectedQuotation(securities_to_date);
	else quotation = security->getQuotation(securities_to_date);
	shares = security->shares(securities_to_date, true);
	if(securitiesPeriodFromButton->isChecked()) {
		rate = security->yearlyRate(securities_from_date, securities_to_date);
		profit = security->profit(securities_from_date, securities_to_date, true);
	} else {
		rate = security->yearlyRate(securities_to_date);
		profit = security->profit(securities_to_date, true);
	}
	SecurityListViewItem *i = new SecurityListViewItem(security, security->name(), QLocale().toCurrencyString(value), QLocale().toString(shares, 'f', security->decimals()), QLocale().toCurrencyString(quotation), QLocale().toCurrencyString(cost), QLocale().toCurrencyString(profit), QLocale().toString(rate * 100) + "%");
	i->setText(8, security->account()->name());
	i->value = value;
	i->cost = cost;
	i->rate = rate;
	i->profit = profit;
	switch(security->type()) {
		case SECURITY_TYPE_BOND: {i->setText(7, i18n("Bond")); break;}
		case SECURITY_TYPE_STOCK: {i->setText(7, i18n("Stock")); break;}
		case SECURITY_TYPE_MUTUAL_FUND: {i->setText(7, i18n("Mutual Fund")); break;}
		case SECURITY_TYPE_OTHER: {i->setText(7, i18n("Other")); break;}
	}
	securitiesView->insertTopLevelItem(securitiesView->topLevelItemCount(), i);
	total_rate *= total_value;
	total_value += value;
	total_cost += cost;
	total_rate += value * rate;
	if(total_cost != 0.0) total_rate /= total_value;
	total_profit += profit;
	updateSecuritiesStatistics();
	securitiesView->setSortingEnabled(true);
}
void Eqonomize::updateSecurity(Security *security) {
	QTreeWidgetItemIterator it(securitiesView);
	QTreeWidgetItem *i = *it;
	while(i != NULL) {
		if(((SecurityListViewItem*) i)->security() == security) {
			updateSecurity(i);
			break;
		}
		++it;
		i = *it;
	}
}
void Eqonomize::updateSecurity(QTreeWidgetItem *i) {
	Security *security = ((SecurityListViewItem*) i)->security();
	total_rate *= total_value;
	total_value -= ((SecurityListViewItem*) i)->value;
	total_cost -= ((SecurityListViewItem*) i)->cost;
	total_rate -= ((SecurityListViewItem*) i)->value * ((SecurityListViewItem*) i)->rate;
	if(total_cost != 0.0) total_rate /= total_value;
	total_profit -= ((SecurityListViewItem*) i)->profit;
	double value = 0.0, cost = 0.0, rate = 0.0, profit = 0.0, quotation = 0.0, shares = 0.0;
	value = security->value(securities_to_date, true);
	cost = security->cost(securities_to_date);
	if(securities_to_date > QDate::currentDate()) quotation = security->expectedQuotation(securities_to_date);
	else quotation = security->getQuotation(securities_to_date);
	shares = security->shares(securities_to_date, true);
	if(securitiesPeriodFromButton->isChecked()) {
		rate = security->yearlyRate(securities_from_date, securities_to_date);
		profit = security->profit(securities_from_date, securities_to_date, true);
	} else {
		rate = security->yearlyRate(securities_to_date);
		profit = security->profit(securities_to_date, true);
	}
	((SecurityListViewItem*) i)->value = value;
	((SecurityListViewItem*) i)->cost = cost;
	((SecurityListViewItem*) i)->rate = rate;
	((SecurityListViewItem*) i)->profit = profit;
	total_rate *= total_value;
	total_value += value;
	total_cost += cost;
	total_rate += value * rate;
	if(total_cost != 0.0) total_rate /= total_value;
	total_profit += profit;
	i->setText(0, security->name());
	switch(security->type()) {
		case SECURITY_TYPE_BOND: {i->setText(7, i18n("Bond")); break;}
		case SECURITY_TYPE_STOCK: {i->setText(7, i18n("Stock")); break;}
		case SECURITY_TYPE_MUTUAL_FUND: {i->setText(7, i18n("Mutual Fund")); break;}
		case SECURITY_TYPE_OTHER: {i->setText(7, i18n("Other")); break;}
	}
	i->setText(1, QLocale().toCurrencyString(value));
	i->setText(2, QLocale().toString(shares, 'f', security->decimals()));
	i->setText(3, QLocale().toCurrencyString(quotation));
	i->setText(4, QLocale().toCurrencyString(cost));
	i->setText(5, QLocale().toCurrencyString(profit));
	i->setText(6, QLocale().toString(rate * 100) + "%");
	i->setText(8, security->account()->name());
	updateSecuritiesStatistics();
}
void Eqonomize::updateSecurities() {
	securitiesView->clear();
	total_value = 0.0;
	total_cost = 0.0;
	total_profit = 0.0;
	total_rate = 0.0;
	Security *security = budget->securities.first();
	while(security) {
		appendSecurity(security);
		security = budget->securities.next();
	}
}
void  Eqonomize::newSplitTransaction() {
	newSplitTransaction(this);
}
bool Eqonomize::newSplitTransaction(QWidget *parent, AssetsAccount *account) {
	EditSplitDialog *dialog = new EditSplitDialog(budget, parent, account, b_extra);
	if(dialog->checkAccounts() && dialog->exec() == QDialog::Accepted) {
		SplitTransaction *split = dialog->createSplitTransaction();
		if(split) {
			budget->addSplitTransaction(split);
			splitTransactionAdded(split);
			dialog->deleteLater();
			return true;
		}
	}
	dialog->deleteLater();
	return false;
}
bool Eqonomize::editSplitTransaction(SplitTransaction *split) {
	return editSplitTransaction(split, this);
}
bool Eqonomize::editSplitTransaction(SplitTransaction *split, QWidget *parent)  {
	EditSplitDialog *dialog = new EditSplitDialog(budget, parent, NULL, b_extra);
	dialog->setSplitTransaction(split);
	if(dialog->exec() == QDialog::Accepted) {
		SplitTransaction *new_split = dialog->createSplitTransaction();
		if(new_split) {
			removeSplitTransaction(split);
			budget->addSplitTransaction(new_split);
			splitTransactionAdded(new_split);
			dialog->deleteLater();
			return true;
		}
	}
	dialog->deleteLater();
	return false;
}
bool Eqonomize::splitUpTransaction(SplitTransaction *split) {
	expensesWidget->onSplitRemoved(split);
	incomesWidget->onSplitRemoved(split);
	transfersWidget->onSplitRemoved(split);
	split->clear(true);
	budget->removeSplitTransaction(split, true);
	splitTransactionRemoved(split);
	delete split;
	setModified(true);
	return true;
}
bool Eqonomize::removeSplitTransaction(SplitTransaction *split) {
	budget->removeSplitTransaction(split, true);
	splitTransactionRemoved(split);
	delete split;
	return true;
}
bool Eqonomize::newScheduledTransaction(int transaction_type, Security *security, bool select_security) {
	return newScheduledTransaction(transaction_type, security, select_security, this);
}
bool Eqonomize::newScheduledTransaction(int transaction_type, Security *security, bool select_security, QWidget *parent, Account *account) {
	ScheduledTransaction *strans = EditScheduledTransactionDialog::newScheduledTransaction(transaction_type, budget, parent, security, select_security, account, b_extra);
	if(strans) {
		if(!strans->recurrence() && strans->transaction()->date() <= QDate::currentDate()) {
			Transaction *trans = strans->transaction()->copy();
			delete strans;
			budget->addTransaction(trans);
			transactionAdded(trans);
		} else {
			budget->addScheduledTransaction(strans);
			scheduledTransactionAdded(strans);
			checkSchedule();
		}
		return true;
	}
	return false;
}
void Eqonomize::newScheduledExpense() {
	newScheduledTransaction(TRANSACTION_TYPE_EXPENSE);
}
void Eqonomize::newScheduledIncome() {
	newScheduledTransaction(TRANSACTION_TYPE_INCOME);
}
void Eqonomize::newScheduledTransfer() {
	newScheduledTransaction(TRANSACTION_TYPE_TRANSFER);
}
bool Eqonomize::editScheduledTransaction(ScheduledTransaction *strans) {
	return editScheduledTransaction(strans, this);
}
bool Eqonomize::editScheduledTransaction(ScheduledTransaction *strans, QWidget *parent) {
	ScheduledTransaction *old_strans = strans->copy();
	if(EditScheduledTransactionDialog::editScheduledTransaction(strans, parent, true, b_extra)) {
		if(!strans->recurrence() && strans->transaction()->date() <= QDate::currentDate()) {
			Transaction *trans = strans->transaction()->copy();
			budget->removeScheduledTransaction(strans, true);
			scheduledTransactionRemoved(strans, old_strans);
			delete strans;
			budget->addTransaction(trans);
			transactionAdded(trans);
		} else {
			scheduledTransactionModified(strans, old_strans);
			checkSchedule();
		}
		delete old_strans;
		return true;
	}
	delete old_strans;
	return false;
}
bool Eqonomize::editOccurrence(ScheduledTransaction *strans, const QDate &date) {
	return editOccurrence(strans, date, this);
}
bool Eqonomize::editOccurrence(ScheduledTransaction *strans, const QDate &date, QWidget *parent) {
	Security *security = NULL;
	if(strans->transaction()->type() == TRANSACTION_TYPE_SECURITY_BUY || strans->transaction()->type() == TRANSACTION_TYPE_SECURITY_SELL) {
		security = ((SecurityTransaction*) strans->transaction())->security();
	} else if(strans->transaction()->type() == TRANSACTION_TYPE_INCOME && ((Income*) strans->transaction())->security()) {
		security = ((Income*) strans->transaction())->security();
	}
	TransactionEditDialog *dialog = new TransactionEditDialog(b_extra, strans->transaction()->type(), false, false, security, SECURITY_ALL_VALUES, security != NULL, budget, parent);
	dialog->editWidget->updateAccounts();
	dialog->editWidget->setScheduledTransaction(strans, date);
	if(dialog->editWidget->checkAccounts() && dialog->exec() == QDialog::Accepted) {
		Transaction *trans = dialog->editWidget->createTransaction();
		if(trans) {
			if(trans->date() > QDate::currentDate()) {
				ScheduledTransaction *strans_new = new ScheduledTransaction(budget, trans, NULL);
				budget->addScheduledTransaction(strans_new);
				scheduledTransactionAdded(strans_new);
			} else {
				budget->addTransaction(trans);
				transactionAdded(trans);
			}
			ScheduledTransaction *old_strans = strans->copy();
			strans->addException(date);
			scheduledTransactionModified(strans, old_strans);
			delete old_strans;
			dialog->deleteLater();
			return true;
		}
	}
	dialog->deleteLater();
	return false;
}
void Eqonomize::editScheduledTransaction() {
	ScheduleListViewItem *i = (ScheduleListViewItem*) selectedItem(scheduleView);
	if(i == NULL) return;
	editScheduledTransaction(i->scheduledTransaction());
}
void Eqonomize::editOccurrence() {
	ScheduleListViewItem *i = (ScheduleListViewItem*) selectedItem(scheduleView);
	if(i == NULL) return;
	editOccurrence(i->scheduledTransaction(), i->date());
}
bool Eqonomize::removeScheduledTransaction(ScheduledTransaction *strans) {
	budget->removeScheduledTransaction(strans, true);
	scheduledTransactionRemoved(strans);
	delete strans;
	return true;
}
void Eqonomize::removeScheduledTransaction() {
	ScheduleListViewItem *i = (ScheduleListViewItem*) selectedItem(scheduleView);
	if(i == NULL) return;
	removeScheduledTransaction(i->scheduledTransaction());
}
bool Eqonomize::removeOccurrence(ScheduledTransaction *strans, const QDate &date) {
	if(strans->isOneTimeTransaction()) {
		removeScheduledTransaction(strans);
	} else {
		ScheduledTransaction *oldstrans = strans->copy();
		strans->addException(date);
		scheduledTransactionModified(strans, oldstrans);
		delete oldstrans;
	}
	return true;
}
void Eqonomize::removeOccurrence() {
	ScheduleListViewItem *i = (ScheduleListViewItem*) selectedItem(scheduleView);
	if(i == NULL) return;
	removeOccurrence(i->scheduledTransaction(), i->date());
}
void Eqonomize::scheduleSelectionChanged() {
	ScheduleListViewItem *i = (ScheduleListViewItem*) selectedItem(scheduleView);
	if(i == NULL) {
		editScheduleButton->setEnabled(false);
		removeScheduleButton->setEnabled(false);
	} else {
		editScheduleButton->setEnabled(true);
		removeScheduleButton->setEnabled(true);
		ActionEditSchedule->setEnabled(true);
		ActionDeleteSchedule->setEnabled(true);
		ActionEditOccurrence->setEnabled(!i->scheduledTransaction()->isOneTimeTransaction());
		ActionDeleteOccurrence->setEnabled(!i->scheduledTransaction()->isOneTimeTransaction());
	}
}
void Eqonomize::scheduleExecuted(QTreeWidgetItem *i) {
	if(i == NULL) return;
	editScheduledTransaction(((ScheduleListViewItem*) i)->scheduledTransaction());
}
void Eqonomize::popupScheduleMenu(const QPoint &p) {
	if(!schedulePopupMenu) {
		schedulePopupMenu = new QMenu(this);
		schedulePopupMenu->addAction(ActionEditScheduledTransaction);
		schedulePopupMenu->addAction(ActionEditTransaction);
		schedulePopupMenu->addSeparator();
		schedulePopupMenu->addAction(ActionDeleteScheduledTransaction);
		schedulePopupMenu->addAction(ActionDeleteTransaction);
	}
	schedulePopupMenu->popup(scheduleView->viewport()->mapToGlobal(p));
}

void Eqonomize::editSelectedScheduledTransaction() {
	TransactionListWidget *w = NULL;
	if(tabs->currentPage()->widget() == accounts_page) return;
	else if(tabs->currentPage()->widget() == expenses_page) w = expensesWidget;
	else if(tabs->currentPage()->widget() == incomes_page) w = incomesWidget;
	else if(tabs->currentPage()->widget() == transfers_page) w = transfersWidget;
	else if(tabs->currentPage()->widget() == securities_page) return;
	else if(tabs->currentPage()->widget() == schedule_page) {
		editScheduledTransaction();
		return;
	}
	if(!w) return;
	w->editScheduledTransaction();
}
void Eqonomize::editSelectedSplitTransaction() {
	TransactionListWidget *w = NULL;
	if(tabs->currentPage()->widget() == expenses_page) w = expensesWidget;
	else if(tabs->currentPage()->widget() == incomes_page) w = incomesWidget;
	else if(tabs->currentPage()->widget() == transfers_page) w = transfersWidget;
	else 	return;
	if(!w) return;
	w->editSplitTransaction();
}
void Eqonomize::joinSelectedTransactions() {
	TransactionListWidget *w = NULL;
	if(tabs->currentPage()->widget() == expenses_page) w = expensesWidget;
	else if(tabs->currentPage()->widget() == incomes_page) w = incomesWidget;
	else if(tabs->currentPage()->widget() == transfers_page) w = transfersWidget;
	else 	return;
	if(!w) return;
	w->joinTransactions();
}
void Eqonomize::splitUpSelectedTransaction() {
	TransactionListWidget *w = NULL;
	if(tabs->currentPage()->widget() == expenses_page) w = expensesWidget;
	else if(tabs->currentPage()->widget() == incomes_page) w = incomesWidget;
	else if(tabs->currentPage()->widget() == transfers_page) w = transfersWidget;
	else 	return;
	if(!w) return;
	w->splitUpTransaction();
}
bool Eqonomize::editTransaction(Transaction *trans) {
	return editTransaction(trans, this);
}
bool Eqonomize::editTransaction(Transaction *trans, QWidget *parent) {
	Transaction *oldtrans = trans->copy();
	Recurrence *rec = NULL;
	if(trans->parentSplit()) {
		SplitTransaction *split = trans->parentSplit();
		Security *security = NULL;
		if(trans->type() == TRANSACTION_TYPE_SECURITY_BUY || trans->type() == TRANSACTION_TYPE_SECURITY_SELL) {
			security = ((SecurityTransaction*) trans)->security();
		} else if(trans->type() == TRANSACTION_TYPE_INCOME && ((Income*) trans)->security()) {
			security = ((Income*) trans)->security();
		}
		TransactionEditDialog *dialog = new TransactionEditDialog(b_extra, trans->type(), true, trans->fromAccount() == split->account(), security, SECURITY_ALL_VALUES, security != NULL, budget, parent);
		dialog->editWidget->updateAccounts(split->account());
		dialog->editWidget->setTransaction(trans);
		if(dialog->exec() == QDialog::Accepted) {
			if(dialog->editWidget->modifyTransaction(trans)) {
				transactionModified(trans, oldtrans);
				delete oldtrans;
				return true;
			}
		}
		dialog->deleteLater();
	} else if(EditScheduledTransactionDialog::editTransaction(trans, rec, parent, true, b_extra)) {
		if(!rec && trans->date() <= QDate::currentDate()) {
			transactionModified(trans, oldtrans);
		} else {
			budget->removeTransaction(trans, true);
			transactionRemoved(trans);
			ScheduledTransaction *strans = new ScheduledTransaction(budget, trans, rec);
			budget->addScheduledTransaction(strans);
			scheduledTransactionAdded(strans);
		}
		delete oldtrans;
		return true;
	}
	delete oldtrans;
	return false;
}
void Eqonomize::newRefund() {
	if(tabs->currentPage()->widget() == expenses_page) expensesWidget->newRefundRepayment();
}
void Eqonomize::newRepayment() {
	if(tabs->currentPage()->widget() == incomes_page) incomesWidget->newRefundRepayment();
}
void Eqonomize::newRefundRepayment() {
	if(tabs->currentPage()->widget() == expenses_page) expensesWidget->newRefundRepayment();
	else if(tabs->currentPage()->widget() == incomes_page) incomesWidget->newRefundRepayment();
}
bool Eqonomize::newRefundRepayment(Transaction *trans) {
	if(trans->type() != TRANSACTION_TYPE_EXPENSE && trans->type() != TRANSACTION_TYPE_INCOME) return false;
	RefundDialog *dialog = new RefundDialog(trans, this);
	if(dialog->exec() == QDialog::Accepted) {
		Transaction *new_trans = dialog->createRefund();
		if(new_trans) {
			budget->addTransaction(new_trans);
			transactionAdded(new_trans);
			dialog->deleteLater();
			return true;
		}
	}
	dialog->deleteLater();
	return false;
}
void Eqonomize::editSelectedTransaction() {
	TransactionListWidget *w = NULL;
	if(tabs->currentPage()->widget() == accounts_page) return;
	else if(tabs->currentPage()->widget() == expenses_page) w = expensesWidget;
	else if(tabs->currentPage()->widget() == incomes_page) w = incomesWidget;
	else if(tabs->currentPage()->widget() == transfers_page) w = transfersWidget;
	else if(tabs->currentPage()->widget() == securities_page) return;
	else if(tabs->currentPage()->widget() == schedule_page) {
		editOccurrence();
		return;
	}
	if(!w) return;
	w->editTransaction();
}
void Eqonomize::deleteSelectedScheduledTransaction() {
	TransactionListWidget *w = NULL;
	if(tabs->currentPage()->widget() == accounts_page) return;
	else if(tabs->currentPage()->widget() == expenses_page) w = expensesWidget;
	else if(tabs->currentPage()->widget() == incomes_page) w = incomesWidget;
	else if(tabs->currentPage()->widget() == transfers_page) w = transfersWidget;
	else if(tabs->currentPage()->widget() == securities_page) return;
	else if(tabs->currentPage()->widget() == schedule_page) {
		removeScheduledTransaction();
		return;
	}
	if(!w) return;
	w->removeScheduledTransaction();
}
void Eqonomize::deleteSelectedSplitTransaction() {
	TransactionListWidget *w = NULL;
	if(tabs->currentPage()->widget() == expenses_page) w = expensesWidget;
	else if(tabs->currentPage()->widget() == incomes_page) w = incomesWidget;
	else if(tabs->currentPage()->widget() == transfers_page) w = transfersWidget;
	else 	return;
	if(!w) return;
	w->removeSplitTransaction();
}
void Eqonomize::deleteSelectedTransaction() {
	TransactionListWidget *w = NULL;
	if(tabs->currentPage()->widget() == accounts_page) return;
	else if(tabs->currentPage()->widget() == expenses_page) w = expensesWidget;
	else if(tabs->currentPage()->widget() == incomes_page) w = incomesWidget;
	else if(tabs->currentPage()->widget() == transfers_page) w = transfersWidget;
	else if(tabs->currentPage()->widget() == securities_page) return;
	else if(tabs->currentPage()->widget() == schedule_page) {
		removeOccurrence();
		return;
	}
	if(!w) return;
	w->removeTransaction();
}

void Eqonomize::onPageChange(KPageWidgetItem *page, KPageWidgetItem*) {
	onPageChange(page->widget());
}

void Eqonomize::onPageChange(QWidget *w) {
	if(w == expenses_page) {
		expensesWidget->onDisplay();
	} else if(w == incomes_page) {
		incomesWidget->onDisplay();
	} else if(w == transfers_page) {
		transfersWidget->onDisplay();
	}
	if(w == accounts_page) {
		accountsSelectionChanged();
	} else {
		ActionDeleteAccount->setEnabled(false);
		ActionEditAccount->setEnabled(false);
		ActionBalanceAccount->setEnabled(false);
		ActionShowAccountTransactions->setEnabled(false);
	}
	if(w == securities_page) {
		securitiesSelectionChanged();
	} else {
		ActionEditSecurity->setEnabled(false);
		ActionDeleteSecurity->setEnabled(false);
		ActionSetQuotation->setEnabled(false);
		ActionEditQuotations->setEnabled(false);
		ActionEditSecurityTransactions->setEnabled(false);
	}
	updateTransactionActions();
}
void Eqonomize::updateTransactionActions() {
	TransactionListWidget *w = NULL;
	bool b_transaction = false, b_scheduledtransaction = false;
	if(tabs->currentPage()->widget() == accounts_page) {b_transaction = false; b_scheduledtransaction = false;}
	else if(tabs->currentPage()->widget() == expenses_page) w = expensesWidget;
	else if(tabs->currentPage()->widget() == incomes_page) w = incomesWidget;
	else if(tabs->currentPage()->widget() == transfers_page) w = transfersWidget;
	else if(tabs->currentPage()->widget() == securities_page) {b_transaction = false; b_scheduledtransaction = false;}
	else if(tabs->currentPage()->widget() == schedule_page) {
		ScheduleListViewItem *i = (ScheduleListViewItem*) selectedItem(scheduleView);
		b_transaction = i && !i->scheduledTransaction()->isOneTimeTransaction();
		b_scheduledtransaction = (i != NULL);
	}
	if(w) {
		w->updateTransactionActions();
	} else {
		ActionJoinTransactions->setEnabled(false);
		ActionSplitUpTransaction->setEnabled(false);
		ActionEditSplitTransaction->setEnabled(false);
		ActionDeleteSplitTransaction->setEnabled(false);
		ActionEditTransaction->setEnabled(b_transaction);
		ActionDeleteTransaction->setEnabled(b_transaction);
		ActionEditScheduledTransaction->setEnabled(b_scheduledtransaction);
		ActionDeleteScheduledTransaction->setEnabled(b_scheduledtransaction);
		ActionNewRefund->setEnabled(false);
		ActionNewRepayment->setEnabled(false);
		ActionNewRefundRepayment->setEnabled(false);
	}
}

void Eqonomize::popupAccountsMenu(const QPoint &p) {
	QTreeWidgetItem *i = accountsView->itemAt(p);
	if(i == NULL) return;
	if(i == assetsItem || (account_items.contains(i) && (account_items[i]->type() == ACCOUNT_TYPE_ASSETS))) {
		ActionAddAccount->setText("Add Account");
	} else {
		ActionAddAccount->setText("Add Category");
	}
	if(!accountPopupMenu) {
		accountPopupMenu = new QMenu(this);
		accountPopupMenu->addAction(ActionAddAccount);
		accountPopupMenu->addAction(ActionEditAccount);
		accountPopupMenu->addAction(ActionBalanceAccount);
		accountPopupMenu->addAction(ActionDeleteAccount);
		accountPopupMenu->addSeparator();
		accountPopupMenu->addAction(ActionShowAccountTransactions);
	}
	accountPopupMenu->popup(accountsView->viewport()->mapToGlobal(p));

}

void Eqonomize::showAccountTransactions(bool b) {
	QTreeWidgetItem *i = selectedItem(accountsView);
	if(i == NULL || i == assetsItem) return;
	if(i == incomesItem) {
		if(b) incomesWidget->setFilter(QDate(), to_date, -1.0, -1.0, NULL, NULL);
		else incomesWidget->setFilter(accountsPeriodFromButton->isChecked() ? from_date : QDate(), to_date, -1.0, -1.0, NULL, NULL);
		incomesWidget->showFilter();
		tabs->setCurrentPage(incomes_page_item);
		onPageChange(incomes_page);
	} else if(i == expensesItem) {
		if(b) expensesWidget->setFilter(QDate(), to_date, -1.0, -1.0, NULL, NULL);
		else expensesWidget->setFilter(accountsPeriodFromButton->isChecked() ? from_date : QDate(), to_date, -1.0, -1.0, NULL, NULL);
		expensesWidget->showFilter();
		tabs->setCurrentPage(expenses_page_item);
		onPageChange(expenses_page);
	} else {
		Account *account = account_items[i];
		AccountType type = account->type();
		if(type == ACCOUNT_TYPE_INCOMES) {
			if(b) incomesWidget->setFilter(QDate(), to_date, -1.0, -1.0, account, NULL);
			else incomesWidget->setFilter(accountsPeriodFromButton->isChecked() ? from_date : QDate(), to_date, -1.0, -1.0, account, NULL);
			incomesWidget->showFilter();
			tabs->setCurrentPage(incomes_page_item);
			onPageChange(incomes_page);
		} else if(type == ACCOUNT_TYPE_EXPENSES) {
			if(b) expensesWidget->setFilter(QDate(), to_date, -1.0, -1.0, NULL, account);
			else expensesWidget->setFilter(accountsPeriodFromButton->isChecked() ? from_date : QDate(), to_date, -1.0, -1.0, NULL, account);
			expensesWidget->showFilter();
			tabs->setCurrentPage(expenses_page_item);
			onPageChange(expenses_page);
		} else if(((AssetsAccount*) account)->accountType() == ASSETS_TYPE_SECURITIES) {
			tabs->setCurrentPage(securities_page_item);
			onPageChange(securities_page);
		} else {
			LedgerDialog *dialog = new LedgerDialog((AssetsAccount*) account, this, i18n("Ledger"), b_extra);
			QSize dialog_size = KSharedConfig::openConfig()->group("Ledger").readEntry("size", QSize());
			if(!dialog_size.isValid()) {
				QDesktopWidget desktop;
				dialog_size = QSize(900, 600).boundedTo(desktop.availableGeometry(this).size());
			}
			dialog->resize(dialog_size);
			dialog->show();
			connect(this, SIGNAL(timeToSaveConfig()), dialog, SLOT(saveConfig()));
		}
	}
}
void Eqonomize::accountsPeriodToChanged(const QDate &date) {
	bool error = false;
	if(!date.isValid()) {
		KMessageBox::error(this, i18n("Invalid date."));
		error = true;
	}
	if(!error && accountsPeriodFromEdit->date() > date) {
		if(accountsPeriodFromButton->isChecked()) {
			KMessageBox::error(this, i18n("To date is before from date."));
		}
		from_date = date;
		accountsPeriodFromEdit->blockSignals(true);
		accountsPeriodFromEdit->setDate(from_date);
		accountsPeriodFromEdit->blockSignals(false);
	}
	if(error) {
		accountsPeriodToEdit->setFocus();
		accountsPeriodToEdit->blockSignals(true);
		accountsPeriodToEdit->setDate(to_date);
		accountsPeriodToEdit->blockSignals(false);
		accountsPeriodToEdit->selectAll();
		return;
	}
	to_date = date;
	filterAccounts();
}
void Eqonomize::accountsPeriodFromChanged(const QDate &date) {
	bool error = false;
	if(!date.isValid()) {
		KMessageBox::error(this, i18n("Invalid date."));
		error = true;
	}
	if(!error && date > accountsPeriodToEdit->date()) {
		KMessageBox::error(this, i18n("From date is after to date."));
		to_date = date;
		accountsPeriodToEdit->blockSignals(true);
		accountsPeriodToEdit->setDate(to_date);
		accountsPeriodToEdit->blockSignals(false);
	}
	if(error) {
		accountsPeriodFromEdit->setFocus();
		accountsPeriodFromEdit->blockSignals(true);
		accountsPeriodFromEdit->setDate(from_date);
		accountsPeriodFromEdit->blockSignals(false);
		accountsPeriodFromEdit->selectAll();
		return;
	}
	from_date = date;
	if(accountsPeriodFromButton->isChecked()) filterAccounts();
}
void Eqonomize::prevMonth() {
	accountsPeriodFromEdit->blockSignals(true);
	accountsPeriodToEdit->blockSignals(true);
	from_date = from_date.addMonths(-1);
	accountsPeriodFromEdit->setDate(from_date);
	if((to_date == QDate::currentDate() && from_date.day() == 1) || to_date.day() == to_date.daysInMonth()) {
		to_date = to_date.addMonths(-1);
		to_date.setDate(to_date.year(), to_date.month(), to_date.daysInMonth());
	} else {
		to_date = to_date.addMonths(-1);
	}
	accountsPeriodToEdit->setDate(to_date);
	accountsPeriodFromEdit->blockSignals(false);
	accountsPeriodToEdit->blockSignals(false);
	filterAccounts();
}
void Eqonomize::nextMonth() {
	accountsPeriodFromEdit->blockSignals(true);
	accountsPeriodToEdit->blockSignals(true);
	from_date = from_date.addMonths(1);
	accountsPeriodFromEdit->setDate(from_date);
	to_date = accountsPeriodToEdit->date();
	if((to_date == QDate::currentDate() && from_date.day() == 1) || to_date.day() == to_date.daysInMonth()) {
		to_date = to_date.addMonths(1);
		to_date.setDate(to_date.year(), to_date.month(), to_date.daysInMonth());
	} else {
		to_date = to_date.addMonths(1);
	}
	accountsPeriodToEdit->setDate(to_date);
	accountsPeriodFromEdit->blockSignals(false);
	accountsPeriodToEdit->blockSignals(false);
	filterAccounts();
}
void Eqonomize::currentMonth() {
	accountsPeriodFromEdit->blockSignals(true);
	accountsPeriodToEdit->blockSignals(true);
	QDate curdate = QDate::currentDate();
	from_date.setDate(curdate.year(), curdate.month(), 1);
	accountsPeriodFromEdit->setDate(from_date);
	to_date = curdate;
	accountsPeriodToEdit->setDate(to_date);
	accountsPeriodFromEdit->blockSignals(false);
	accountsPeriodToEdit->blockSignals(false);
	filterAccounts();
}
void Eqonomize::prevYear() {
	accountsPeriodFromEdit->blockSignals(true);
	accountsPeriodToEdit->blockSignals(true);
	from_date = from_date.addYears(-1);
	accountsPeriodFromEdit->setDate(from_date);
	if((to_date == QDate::currentDate() && from_date.day() == 1) || to_date.day() == to_date.daysInMonth()) {
		to_date = to_date.addYears(-1);
		to_date.setDate(to_date.year(), to_date.month(), to_date.daysInMonth());
	} else {
		to_date = to_date.addYears(-1);
	}
	accountsPeriodToEdit->setDate(to_date);
	accountsPeriodFromEdit->blockSignals(false);
	accountsPeriodToEdit->blockSignals(false);
	filterAccounts();
}
void Eqonomize::nextYear() {
	accountsPeriodFromEdit->blockSignals(true);
	accountsPeriodToEdit->blockSignals(true);
	from_date = from_date.addYears(1);
	accountsPeriodFromEdit->setDate(from_date);
	if((to_date == QDate::currentDate() && from_date.day() == 1) || to_date.day() == to_date.daysInMonth()) {
		to_date = to_date.addYears(1);
		to_date.setDate(to_date.year(), to_date.month(), to_date.daysInMonth());
	} else {
		to_date = to_date.addYears(1);
	}
	accountsPeriodToEdit->setDate(to_date);
	accountsPeriodFromEdit->blockSignals(false);
	accountsPeriodToEdit->blockSignals(false);
	filterAccounts();
}
void Eqonomize::currentYear() {
	accountsPeriodFromEdit->blockSignals(true);
	accountsPeriodToEdit->blockSignals(true);
	QDate curdate = QDate::currentDate();
	from_date.setDate(curdate.year(), 1, 1);
	accountsPeriodFromEdit->setDate(from_date);
	to_date = curdate;
	accountsPeriodToEdit->setDate(to_date);
	accountsPeriodFromEdit->blockSignals(false);
	accountsPeriodToEdit->blockSignals(false);
	filterAccounts();
}

void Eqonomize::securitiesPeriodToChanged(const QDate &date) {
	bool error = false;
	if(!date.isValid()) {
		KMessageBox::error(this, i18n("Invalid date."));
		error = true;
	}
	if(!error && securitiesPeriodFromEdit->date() > date) {
		if(securitiesPeriodFromButton->isChecked()) {
			KMessageBox::error(this, i18n("To date is before from date."));
		}
		securities_from_date = date;
		securitiesPeriodFromEdit->blockSignals(true);
		securitiesPeriodFromEdit->setDate(securities_from_date);
		securitiesPeriodFromEdit->blockSignals(false);
	}
	if(error) {
		securitiesPeriodToEdit->setFocus();
		securitiesPeriodToEdit->blockSignals(true);
		securitiesPeriodToEdit->setDate(securities_to_date);
		securitiesPeriodToEdit->blockSignals(false);
		securitiesPeriodToEdit->selectAll();
		return;
	}
	securities_to_date = date;
	updateSecurities();
}
void Eqonomize::securitiesPeriodFromChanged(const QDate &date) {
	bool error = false;
	if(!date.isValid()) {
		KMessageBox::error(this, i18n("Invalid date."));
		error = true;
	}
	if(!error && date > securitiesPeriodToEdit->date()) {
		KMessageBox::error(this, i18n("From date is after to date."));
		securities_to_date = date;
		securitiesPeriodToEdit->blockSignals(true);
		securitiesPeriodToEdit->setDate(securities_to_date);
		securitiesPeriodToEdit->blockSignals(false);
	}
	if(error) {
		securitiesPeriodFromEdit->setFocus();
		securitiesPeriodFromEdit->blockSignals(true);
		securitiesPeriodFromEdit->setDate(securities_from_date);
		securitiesPeriodFromEdit->blockSignals(false);
		securitiesPeriodFromEdit->selectAll();
		return;
	}
	securities_from_date = date;
	if(securitiesPeriodFromButton->isChecked()) updateSecurities();
}
void Eqonomize::securitiesPrevMonth() {
	securitiesPeriodFromEdit->blockSignals(true);
	securitiesPeriodToEdit->blockSignals(true);
	securities_from_date = securities_from_date.addMonths(-1);
	securitiesPeriodFromEdit->setDate(securities_from_date);
	if((securities_to_date == QDate::currentDate() && securities_from_date.day() == 1) || securities_to_date.day() == securities_to_date.daysInMonth()) {
		securities_to_date = securities_to_date.addMonths(-1);
		securities_to_date.setDate(securities_to_date.year(), securities_to_date.month(), securities_to_date.daysInMonth());
	} else {
		securities_to_date = securities_to_date.addMonths(-1);
	}
	securitiesPeriodToEdit->setDate(securities_to_date);
	securitiesPeriodFromEdit->blockSignals(false);
	securitiesPeriodToEdit->blockSignals(false);
	updateSecurities();
}
void Eqonomize::securitiesNextMonth() {	
	securitiesPeriodFromEdit->blockSignals(true);
	securitiesPeriodToEdit->blockSignals(true);
	securities_from_date = securities_from_date.addMonths(1);
	securitiesPeriodFromEdit->setDate(securities_from_date);
	securities_to_date = securitiesPeriodToEdit->date();
	if((securities_to_date == QDate::currentDate() && securities_from_date.day() == 1) || securities_to_date.day() == securities_to_date.daysInMonth()) {
		securities_to_date = securities_to_date.addMonths(1);
		securities_to_date.setDate(securities_to_date.year(), securities_to_date.month(), securities_to_date.daysInMonth());
	} else {
		securities_to_date = securities_to_date.addMonths(1);
	}
	securitiesPeriodToEdit->setDate(securities_to_date);
	securitiesPeriodFromEdit->blockSignals(false);
	securitiesPeriodToEdit->blockSignals(false);
	updateSecurities();
}
void Eqonomize::securitiesCurrentMonth() {	
	securitiesPeriodFromEdit->blockSignals(true);
	securitiesPeriodToEdit->blockSignals(true);
	QDate curdate = QDate::currentDate();
	securities_from_date.setDate(curdate.year(), curdate.month(), 1);
	securitiesPeriodFromEdit->setDate(securities_from_date);
	securities_to_date = curdate;
	securitiesPeriodToEdit->setDate(securities_to_date);
	securitiesPeriodFromEdit->blockSignals(false);
	securitiesPeriodToEdit->blockSignals(false);
	updateSecurities();
}
void Eqonomize::securitiesPrevYear() {
	securitiesPeriodFromEdit->blockSignals(true);
	securitiesPeriodToEdit->blockSignals(true);
	securities_from_date = securities_from_date.addYears(-1);
	securitiesPeriodFromEdit->setDate(securities_from_date);
	if((securities_to_date == QDate::currentDate() && securities_from_date.day() == 1) || securities_to_date.day() == securities_to_date.daysInMonth()) {
		securities_to_date = securities_to_date.addYears(-1);
		securities_to_date.setDate(securities_to_date.year(), securities_to_date.month(), securities_to_date.daysInMonth());
	} else {
		securities_to_date = securities_to_date.addYears(-1);
	}
	securitiesPeriodToEdit->setDate(securities_to_date);
	securitiesPeriodFromEdit->blockSignals(false);
	securitiesPeriodToEdit->blockSignals(false);
	updateSecurities();
}
void Eqonomize::securitiesNextYear() {	
	securitiesPeriodFromEdit->blockSignals(true);
	securitiesPeriodToEdit->blockSignals(true);
	securities_from_date = securities_from_date.addYears(1);
	securitiesPeriodFromEdit->setDate(securities_from_date);
	if((securities_to_date == QDate::currentDate() && securities_from_date.day() == 1) || securities_to_date.day() == securities_to_date.daysInMonth()) {
		securities_to_date = securities_to_date.addYears(1);
		securities_to_date.setDate(securities_to_date.year(), securities_to_date.month(), securities_to_date.daysInMonth());
	} else {
		securities_to_date = securities_to_date.addYears(1);
	}
	securitiesPeriodToEdit->setDate(securities_to_date);
	securitiesPeriodFromEdit->blockSignals(false);
	securitiesPeriodToEdit->blockSignals(false);
	updateSecurities();
}
void Eqonomize::securitiesCurrentYear() {
	securitiesPeriodFromEdit->blockSignals(true);
	securitiesPeriodToEdit->blockSignals(true);
	QDate curdate = QDate::currentDate();
	securities_from_date.setDate(curdate.year(), 1, 1);
	securitiesPeriodFromEdit->setDate(securities_from_date);
	securities_to_date = curdate;
	securitiesPeriodToEdit->setDate(securities_to_date);
	securitiesPeriodFromEdit->blockSignals(false);
	securitiesPeriodToEdit->blockSignals(false);
	updateSecurities();
}

void Eqonomize::setModified(bool has_been_modified) {
	modified_auto_save = has_been_modified;
	if(has_been_modified) autoSave();
	if(modified == has_been_modified) return;
	modified = has_been_modified;
	ActionSave->setEnabled(modified);
	if(!current_url.isValid()) setCaption(i18n("Untitled"), has_been_modified);
	else setCaption(current_url.fileName(), has_been_modified);
	if(modified) emit budgetUpdated();
	else auto_save_timeout = true;
}
void Eqonomize::createDefaultBudget() {
	if(!askSave()) return;
	budget->clear();
	current_url = "";
	ActionFileReload->setEnabled(false);
	KConfigGroup config = KSharedConfig::openConfig()->group("General Options");
	config.writeEntry("lastURL", current_url.url());
	config.sync();
	budget->addAccount(new AssetsAccount(budget, ASSETS_TYPE_CASH, i18n("Cash"), 100.0));
	budget->addAccount(new AssetsAccount(budget, ASSETS_TYPE_CURRENT, i18n("Check Account"), 1000.0));
	budget->addAccount(new AssetsAccount(budget, ASSETS_TYPE_SAVINGS, i18n("Savings Account"), 10000.0));
	budget->addAccount(new IncomesAccount(budget, i18n("Salary")));
	budget->addAccount(new IncomesAccount(budget, i18n("Other")));
	budget->addAccount(new ExpensesAccount(budget, i18n("Bills")));
	budget->addAccount(new ExpensesAccount(budget, i18n("Clothing")));
	budget->addAccount(new ExpensesAccount(budget, i18n("Groceries")));
	budget->addAccount(new ExpensesAccount(budget, i18n("Leisure")));
	budget->addAccount(new ExpensesAccount(budget, i18n("Other")));
	reloadBudget();
	setModified(false);
	emit accountsModified();
	emit transactionsModified();
	emit budgetUpdated();
}
void Eqonomize::reloadBudget() {
	incomes_accounts_value = 0.0;
	incomes_accounts_change = 0.0;
	expenses_accounts_value = 0.0;
	expenses_accounts_change = 0.0;
	assets_accounts_value = 0.0;
	assets_accounts_change = 0.0;
	expenses_budget = 0.0;
	expenses_budget_diff = 0.0;
	incomes_budget = 0.0;
	incomes_budget_diff = 0.0;
	account_value.clear();
	account_change.clear();
	for(QMap<QTreeWidgetItem*, Account*>::Iterator it = account_items.begin(); it != account_items.end(); ++it) {
		delete it.key();
	}
	account_items.clear();
	item_accounts.clear();
	AssetsAccount *aaccount = budget->assetsAccounts.first();
	while(aaccount) {
		if(aaccount != budget->balancingAccount) {
			appendAssetsAccount(aaccount);
		}
		aaccount = budget->assetsAccounts.next();
	}
	IncomesAccount *iaccount = budget->incomesAccounts.first();
	while(iaccount) {
		appendIncomesAccount(iaccount);
		iaccount = budget->incomesAccounts.next();
	}
	ExpensesAccount *eaccount = budget->expensesAccounts.first();
	while(eaccount) {
		appendExpensesAccount(eaccount);
		eaccount = budget->expensesAccounts.next();
	}
	account_value[budget->balancingAccount] = 0.0;
	account_change[budget->balancingAccount] = 0.0;
	expensesWidget->updateAccounts();
	incomesWidget->updateAccounts();
	transfersWidget->updateAccounts();
	assetsItem->setExpanded(true);
	incomesItem->setExpanded(true);
	expensesItem->setExpanded(true);
	expensesWidget->transactionsReset();
	incomesWidget->transactionsReset();
	transfersWidget->transactionsReset();
	filterAccounts();
	updateScheduledTransactions();
	updateSecurities();
}
void Eqonomize::openURL(const QUrl& url) {

	if(url != current_url && crashRecovery(QUrl(url))) return;

	QString tmpfile;
	if(!url.isLocalFile()) {
		QTemporaryFile tf;
		tf.setAutoRemove(false);
		if(!tf.open()) {
			KMessageBox::error(this, i18n("Couldn't fetch %1: %2", url.toString(), QString("Unable to create temporary file %1.").arg(tf.fileName())));
			return;
		}
		KIO::FileCopyJob *job = KIO::file_copy(url, QUrl::fromLocalFile(tf.fileName()), KIO::Overwrite);
		KJobWidgets::setWindow(job, this);
		if(!job->exec()) {
			if(job->error()) {
				KMessageBox::error(this, i18n("Couldn't fetch %1: %2", url.toString(), job->errorString()));
			}
			return;
		}
		tmpfile = tf.fileName();
		tf.close();
	} else {
		tmpfile = url.toLocalFile();
	}
	QString errors;
	QString error = budget->loadFile(tmpfile, errors);
	if(!url.isLocalFile()) {
		QFile::remove(tmpfile);
	}
	if(!error.isNull()) {
		KMessageBox::error(this, i18n("Error loading %1: %2.", url.toString(), error), i18n("Couldn't open file"));
		return;
	}
	if(!errors.isEmpty()) {
		KMessageBox::error(this, errors);
	}
	setCaption(url.fileName(), false);
	current_url = url;
	ActionFileReload->setEnabled(true);
	KConfigGroup config = KSharedConfig::openConfig()->group("General Options");
	config.writeEntry("lastURL", current_url.url());
	if(cr_tmp_file) {
		cr_tmp_file->releaseLock();
		delete cr_tmp_file;
		cr_tmp_file = NULL;
	}
	config.sync();
	ActionOpenRecent->addUrl(url);

	reloadBudget();

	emit accountsModified();
	emit transactionsModified();
	emit budgetUpdated();

	setModified(false);

	checkSchedule(true);

}

bool Eqonomize::saveURL(const QUrl& url) {

	if(url.isLocalFile()) {
		bool exists = QFile::exists(url.toLocalFile());
		if(url != current_url && exists) {
			if(KMessageBox::warningYesNo(this, i18n("The selected file already exists. Would you like to overwrite the old copy?")) != KMessageBox::Yes) return false;
		}
		mode_t  perms = 0x0600;
		if(exists) {
			QFile::copy(url.toLocalFile(), url.toLocalFile() + "~");
		}

		QString error = budget->saveFile(url.toLocalFile(), perms);
		if(!error.isNull()) {
			KMessageBox::error(this, i18n("Error saving %1: %2.", url.toString(), error), i18n("Couldn't save file"));
			return false;
		} else {
			setCaption(url.fileName(), false);
			current_url = url;
			ActionFileReload->setEnabled(true);
			KConfigGroup config = KSharedConfig::openConfig()->group("General Options");
			config.writeEntry("lastURL", current_url.url());
			if(cr_tmp_file) {
				cr_tmp_file->releaseLock();
				delete cr_tmp_file;
				cr_tmp_file = NULL;
			}
			config.sync();
			ActionOpenRecent->addUrl(url);
			setModified(false);
		}
		return true;
	}

	QTemporaryFile tf;
	tf.open();
	tf.setAutoRemove(true);
	QString error = budget->saveFile(tf.fileName());
	if(!error.isNull()) {
		KMessageBox::error(this, i18n("Error saving %1: %2.").arg(url.toString(), error), i18n("Couldn't save file"));
		return false;
	}
	KIO::FileCopyJob *job = KIO::file_copy(QUrl::fromLocalFile(tf.fileName()), url, KIO::Overwrite);
	KJobWidgets::setWindow(job, this);
	if(job->exec()) {
		setCaption(url.fileName(), false);
		current_url = url;
		ActionFileReload->setEnabled(true);
		KConfigGroup config = KSharedConfig::openConfig()->group("General Options");
		config.writeEntry("lastURL", current_url.url());
		if(cr_tmp_file) {
			cr_tmp_file->releaseLock();
			delete cr_tmp_file;
			cr_tmp_file = NULL;
		}
		config.sync();
		ActionOpenRecent->addUrl(url);
		setModified(false);
		return true;
	} else {
		if(job->error()) {
			KMessageBox::error(this, i18n("Failed to upload file to %1: %2", url.toString(), job->errorString()), i18n("Couldn't save file"));
		}
	}
	return false;
}

void Eqonomize::importCSV() {
	ImportCSVDialog *dialog = new ImportCSVDialog(budget, this);
	if(dialog->exec() == QDialog::Accepted) {
		reloadBudget();
		emit accountsModified();
		emit transactionsModified();
		setModified(true);
	}
	dialog->deleteLater();
}

void Eqonomize::importQIF() {
	if(importQIFFile(budget, this, b_extra)) {
		reloadBudget();
		emit accountsModified();
		emit transactionsModified();
		setModified(true);
	}
}
void Eqonomize::exportQIF() {
	exportQIFFile(budget, this, b_extra);
}



void Eqonomize::showOverTimeReport() {
	OverTimeReportDialog *dialog = new OverTimeReportDialog(budget, 0);
	QSize dialog_size = KSharedConfig::openConfig()->group("Over Time Report").readEntry("size", QSize());
	if(!dialog_size.isValid()) {
		QDesktopWidget desktop;
		dialog_size = QSize(750, 650).boundedTo(desktop.availableGeometry(this).size());
	}
	dialog->resize(dialog_size);
	dialog->show();
	connect(this, SIGNAL(accountsModified()), dialog->report, SLOT(updateAccounts()));
	connect(this, SIGNAL(transactionsModified()), dialog->report, SLOT(updateTransactions()));
	connect(this, SIGNAL(timeToSaveConfig()), dialog->report, SLOT(saveConfig()));
}
void Eqonomize::showCategoriesComparisonReport() {
	CategoriesComparisonReportDialog *dialog = new CategoriesComparisonReportDialog(b_extra, budget, 0);
	QSize dialog_size = KSharedConfig::openConfig()->group("Categories Comparison Report").readEntry("size", QSize());
	if(!dialog_size.isValid()) {
		QDesktopWidget desktop;
		dialog_size = QSize(750, 670).boundedTo(desktop.availableGeometry(this).size());
	}
	dialog->resize(dialog_size);
	dialog->show();
	connect(this, SIGNAL(accountsModified()), dialog->report, SLOT(updateAccounts()));
	connect(this, SIGNAL(transactionsModified()), dialog->report, SLOT(updateTransactions()));
	connect(this, SIGNAL(timeToSaveConfig()), dialog->report, SLOT(saveConfig()));
}
void Eqonomize::showOverTimeChart() {
	OverTimeChartDialog *dialog = new OverTimeChartDialog(b_extra, budget, 0);	
	QSize dialog_size = KSharedConfig::openConfig()->group("Over Time Chart").readEntry("size", QSize());
	if(!dialog_size.isValid()) {
		QDesktopWidget desktop;
		dialog_size = QSize(850, b_extra ? 750 : 730).boundedTo(desktop.availableGeometry(this).size());
	}
	dialog->resize(dialog_size);
	dialog->show();
	dialog->chart->updateDisplay();
	connect(this, SIGNAL(accountsModified()), dialog->chart, SLOT(updateAccounts()));
	connect(this, SIGNAL(transactionsModified()), dialog->chart, SLOT(updateTransactions()));
	connect(this, SIGNAL(timeToSaveConfig()), dialog->chart, SLOT(saveConfig()));
}

void Eqonomize::showCategoriesComparisonChart() {
	CategoriesComparisonChartDialog *dialog = new CategoriesComparisonChartDialog(budget, 0);	
	QSize dialog_size = KSharedConfig::openConfig()->group("Categories Comparison Chart").readEntry("size", QSize());
	if(!dialog_size.isValid()) {
		QDesktopWidget desktop;
		dialog_size = QSize(750, 700).boundedTo(desktop.availableGeometry(this).size());
	}
	dialog->resize(dialog_size);
	dialog->show();
	dialog->chart->updateDisplay();
	connect(this, SIGNAL(accountsModified()), dialog->chart, SLOT(updateAccounts()));
	connect(this, SIGNAL(transactionsModified()), dialog->chart, SLOT(updateTransactions()));
	connect(this, SIGNAL(timeToSaveConfig()), dialog->chart, SLOT(saveConfig()));
}

QString htmlize_string(QString str) {
	str.replace('<', "&lt;");
	str.replace('>', "&gt;");
	str.replace('&', "&amp;");
	str.replace('\"', "&quot;");
	return str;
}

bool Eqonomize::exportScheduleList(QTextStream &outf, int fileformat) {

	switch(fileformat) {
		case 'h': {
			outf.setCodec("UTF-8");
			outf << "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 Transitional//EN\" \"http://www.w3.org/TR/html4/loose.dtd\">" << '\n';
			outf << "<html>" << '\n';
			outf << "\t<head>" << '\n';
			outf << "\t\t<title>"; outf << htmlize_string(i18n("Transaction Schedule")); outf << "</title>" << '\n';
			outf << "\t\t<meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\">" << '\n';
			outf << "\t\t<meta name=\"GENERATOR\" content=\"Eqonomize!\">" << '\n';
			outf << "\t</head>" << '\n';
			outf << "\t<body>" << '\n';
			outf << "\t\t<table border=\"1\" cellspacing=\"0\" cellpadding=\"5\">" << '\n';
			outf << "\t\t\t<caption>"; outf << htmlize_string(i18n("Transaction Schedule")); outf << "</caption>" << '\n';
			outf << "\t\t\t<thead>" << '\n';
			outf << "\t\t\t\t<tr>" << '\n';
			QTreeWidgetItem *header = scheduleView->headerItem();
			outf << "\t\t\t\t\t<th>" << htmlize_string(header->text(0)) << "</th>";
			outf << "<th>" << htmlize_string(header->text(1)) << "</th>";
			outf << "<th>" << htmlize_string(header->text(2)) << "</th>";
			outf << "<th>" << htmlize_string(header->text(3)) << "</th>";
			outf << "<th>" << htmlize_string(header->text(4)) << "</th>";
			outf << "<th>" << htmlize_string(header->text(5)) << "</th>";
			outf << "<th>" << htmlize_string(header->text(6)) << "</th>" << "\n";
			outf << "\t\t\t\t</tr>" << '\n';
			outf << "\t\t\t</thead>" << '\n';
			outf << "\t\t\t<tbody>" << '\n';
			QTreeWidgetItemIterator it(scheduleView);
			ScheduleListViewItem *i = (ScheduleListViewItem*) *it;
			while(i) {
				outf << "\t\t\t\t<tr>" << '\n';
				outf << "\t\t\t\t\t<td nowrap align=\"right\">" << htmlize_string(QLocale().toString(i->date(), QLocale::ShortFormat)) << "</td>";
				outf << "<td>" << htmlize_string(i->text(1)) << "</td>";
				outf << "<td>" << htmlize_string(i->text(2)) << "</td>";
				outf << "<td nowrap align=\"right\">" << htmlize_string(i->text(3)) << "</td>";
				outf << "<td align=\"center\">" << htmlize_string(i->text(4)) << "</td>";
				outf << "<td align=\"center\">" << htmlize_string(i->text(5)) << "</td>";
				outf << "<td>" << htmlize_string(i->text(6)) << "</td>" << "\n";
				outf << "\t\t\t\t</tr>" << '\n';
				++it;
				i = (ScheduleListViewItem*) *it;
			}
			outf << "\t\t\t</tbody>" << '\n';
			outf << "\t\t</table>" << '\n';
			outf << "\t</body>" << '\n';
			outf << "</html>" << '\n';
			break;
		}
		case 'c': {
			//outf.setEncoding(Q3TextStream::Locale);
			QTreeWidgetItem *header = scheduleView->headerItem();
			outf << "\"" << header->text(0) << "\",\"" << header->text(1) << "\",\"" << header->text(2) << "\",\"" << header->text(3) << "\",\"" << header->text(4) << "\",\""<< header->text(5) << "\",\"" << header->text(6) << "\"\n";
			QTreeWidgetItemIterator it(scheduleView);
			ScheduleListViewItem *i = (ScheduleListViewItem*) *it;
			while(i) {
				outf << "\"" << QLocale().toString(i->date(), QLocale::ShortFormat) << "\",\"" << i->text(1) << "\",\"" << i->text(2) << "\",\"" << i->text(3) << "\",\"" << i->text(4) << "\",\"" << i->text(5) << "\",\"" << i->text(6) << "\"\n";
				++it;
				i = (ScheduleListViewItem*) *it;
			}
			break;
		}
	}

	return true;

}

bool Eqonomize::exportSecuritiesList(QTextStream &outf, int fileformat) {

	switch(fileformat) {
		case 'h': {
			outf.setCodec("UTF-8");
			outf << "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 Transitional//EN\" \"http://www.w3.org/TR/html4/loose.dtd\">" << '\n';
			outf << "<html>" << '\n';
			outf << "\t<head>" << '\n';
			outf << "\t\t<title>"; outf << htmlize_string(i18n("Securities")); outf << "</title>" << '\n';
			outf << "\t\t<meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\">" << '\n';
			outf << "\t\t<meta name=\"GENERATOR\" content=\"Eqonomize!\">" << '\n';
			outf << "\t</head>" << '\n';
			outf << "\t<body>" << '\n';
			outf << "\t\t<table border=\"1\" cellspacing=\"0\" cellpadding=\"5\">" << '\n';
			outf << "\t\t\t<caption>"; outf << htmlize_string(i18n("Securities")); outf << "</caption>" << '\n';
			outf << "\t\t\t<thead>" << '\n';
			outf << "\t\t\t\t<tr>" << '\n';
			QTreeWidgetItem *header = securitiesView->headerItem();
			outf << "\t\t\t\t\t<th>" << htmlize_string(header->text(0)) << "</th>";
			outf << "<th>" << htmlize_string(header->text(1)) << "</th>";
			outf << "<th>" << htmlize_string(header->text(2)) << "</th>";
			outf << "<th>" << htmlize_string(header->text(3)) << "</th>";
			outf << "<th>" << htmlize_string(header->text(4)) << "</th>";
			outf << "<th>" << htmlize_string(header->text(5)) << "</th>";
			outf << "<th>" << htmlize_string(header->text(6)) << "</th>";
			outf << "<th>" << htmlize_string(header->text(7)) << "</th>";
			outf << "<th>" << htmlize_string(header->text(8)) << "</th>" << "\n";
			outf << "\t\t\t\t</tr>" << '\n';
			outf << "\t\t\t</thead>" << '\n';
			outf << "\t\t\t<tbody>" << '\n';
			QTreeWidgetItemIterator it(securitiesView);
			SecurityListViewItem *i = (SecurityListViewItem*) *it;
			while(i) {
				outf << "\t\t\t\t<tr>" << '\n';
				outf << "\t\t\t\t\t<td>" << htmlize_string(i->text(0)) << "</td>";
				outf << "<td nowrap align=\"right\">" << htmlize_string(i->text(1)) << "</td>";
				outf << "<td nowrap align=\"right\">" << htmlize_string(i->text(2)) << "</td>";
				outf << "<td nowrap align=\"right\">" << htmlize_string(i->text(3)) << "</td>";
				outf << "<td nowrap align=\"right\">" << htmlize_string(i->text(4)) << "</td>";
				outf << "<td nowrap align=\"right\">" << htmlize_string(i->text(5)) << "</td>";
				outf << "<td nowrap align=\"right\">" << htmlize_string(i->text(6)) << "</td>";
				outf << "<td align=\"center\">" << htmlize_string(i->text(7)) << "</td>";
				outf << "<td align=\"center\">" << htmlize_string(i->text(8)) << "</td>" << "\n";
				outf << "\t\t\t\t</tr>" << '\n';
				++it;
				i = (SecurityListViewItem*) *it;
			}
			outf << "\t\t\t\t<tr>" << '\n';
			if(securitiesView->topLevelItemCount() > 1) {
				outf << "\t\t\t\t\t<td style=\"border-top: thin solid\"><b>" << htmlize_string(i18n("Total")) << "</b></td>";
				outf << "<td nowrap align=\"right\" style=\"border-top: thin solid\"><b>" << htmlize_string(QLocale().toCurrencyString(total_value)) << "</b></td>";
				outf << "<td align=\"right\" style=\"border-top: thin solid\"><b>-</b></td>";
				outf << "<td align=\"right\" style=\"border-top: thin solid\"><b>-</b></td>";
				outf << "<td nowrap align=\"right\" style=\"border-top: thin solid\"><b>" << htmlize_string(QLocale().toCurrencyString(total_cost)) << "</b></td>";
				outf << "<td nowrap align=\"right\" style=\"border-top: thin solid\"><b>" << htmlize_string(QLocale().toCurrencyString(total_profit)) << "</b></td>";
				outf << "<td nowrap align=\"right\" style=\"border-top: thin solid\"><b>" << htmlize_string(QLocale().toString(total_rate * 100) + "%") << "</b></td>";
				outf << "<td align=\"center\" style=\"border-top: thin solid\"><b>-</b></td>";
				outf << "<td align=\"center\" style=\"border-top: thin solid\"><b>-</b></td>" << "\n";
				outf << "\t\t\t\t</tr>" << '\n';
			}
			outf << "\t\t\t</tbody>" << '\n';
			outf << "\t\t</table>" << '\n';
			outf << "\t</body>" << '\n';
			outf << "</html>" << '\n';
			break;
		}
		case 'c': {
			//outf.setEncoding(Q3TextStream::Locale);
			QTreeWidgetItem *header = scheduleView->headerItem();
			outf << "\"" << header->text(0) << "\",\"" << header->text(1) << "\",\"" << header->text(2) << "\",\"" << header->text(3) << "\",\"" << header->text(4) << "\",\"" << header->text(5) << "\",\"" << header->text(6) << "\",\"" << header->text(7) << "\",\"" << header->text(8) << "\"\n";
			QTreeWidgetItemIterator it(securitiesView);
			SecurityListViewItem *i = (SecurityListViewItem*) *it;
			while(i) {
				outf << "\"" << i->text(0) << "\",\"" << i->text(1) << "\",\"" << i->text(2) << "\",\"" << i->text(3) << "\",\"" << i->text(4) << "\",\"" << i->text(5) << "\",\"" << i->text(6) << "\",\"" << i->text(7) << "\",\"" << i->text(8) << '\n';
				++it;
				i = (SecurityListViewItem*) *it;
			}
			break;
		}
	}

	return true;

}

bool Eqonomize::exportAccountsList(QTextStream &outf, int fileformat) {

	switch(fileformat) {
		case 'h': {
			outf.setCodec("UTF-8");
			outf << "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 Transitional//EN\" \"http://www.w3.org/TR/html4/loose.dtd\">" << '\n';
			outf << "<html>" << '\n';
			outf << "\t<head>" << '\n';
			outf << "\t\t<title>"; outf << i18nc("html format", "Accounts &amp; Categories"); outf << "</title>" << '\n';
			outf << "\t\t<meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\">" << '\n';
			outf << "\t\t<meta name=\"GENERATOR\" content=\"Eqonomize!\">" << '\n';
			outf << "\t</head>" << '\n';
			outf << "\t<body>" << '\n';
			if(accountsPeriodFromButton->isChecked()) outf << "\t\t<h1>" << i18nc("html format", "Accounts &amp; Categories (%1&ndash;%2)", htmlize_string(QLocale().toString(from_date, QLocale::ShortFormat)), htmlize_string(QLocale().toString(to_date, QLocale::ShortFormat))) << "</h1>" << '\n';
			else outf << "\t\t<h1>" << i18nc("html format", "Accounts &amp; Categories (to %1)", htmlize_string(QLocale().toString(to_date, QLocale::ShortFormat))) << "</h1>" << '\n';
			outf << "\t\t<table cellspacing=\"0\" cellpadding=\"5\">" << '\n';
			outf << "\t\t\t<caption>"; outf << htmlize_string(i18n("Accounts")); outf << "</caption>" << '\n';
			outf << "\t\t\t<thead>" << '\n';
			outf << "\t\t\t\t<tr>" << '\n';
			outf << "\t\t\t\t\t<th style=\"border-bottom: thin solid\">" << htmlize_string(i18n("Name")) << "</th>";
			outf << "<th style=\"border-bottom: thin solid\">" << htmlize_string(i18n("Type")) << "</th>";
			outf << "<th style=\"border-bottom: thin solid\">" << htmlize_string(i18n("Change"));
			bool includes_budget = (to_date > QDate::currentDate() && (expenses_budget >= 0.0 || incomes_budget >= 0.0));
			if(includes_budget) outf << "*";
			outf << "</th>";
			outf << "<th style=\"border-bottom: thin solid\">" << htmlize_string(i18n("Balance"));
			if(includes_budget) outf << "*";
			outf << "</th>" << '\n';
			outf << "\t\t\t\t</tr>" << '\n';
			outf << "\t\t\t</thead>" << '\n';
			outf << "\t\t\t<tfoot>" << '\n';
			outf << "\t\t\t\t<tr>" << '\n';
			if(includes_budget) {
				outf << "\t\t\t\t\t<th align=\"right\" colspan=\"" << QString::number(4) << "\" style=\"font-weight: normal; border-top: thin solid\">" << "<small>";
				outf << "*" << htmlize_string(i18n("Includes budgeted transactions"));
				outf << "</small>" << "</th>" << '\n';
				outf << "\t\t\t\t</tr>" << '\n';
				outf << "\t\t\t</tfoot>" << '\n';
			}
			outf << "\t\t\t<tbody>" << '\n';
			Account *account = budget->assetsAccounts.first();
			while(account) {
				outf << "\t\t\t\t<tr>" << '\n';
				outf << "\t\t\t\t\t<td>" << htmlize_string(account->name());
				if(includes_budget && ((AssetsAccount*) account)->isBudgetAccount()) outf << "*";
				outf << "</td>";
				outf << "<td>";
				switch(((AssetsAccount*) account)->accountType()) {
					case ASSETS_TYPE_CURRENT: {outf << htmlize_string(i18n("Current Account")); break;}
					case ASSETS_TYPE_SAVINGS: {outf << htmlize_string(i18n("Savings Account")); break;}
					case ASSETS_TYPE_CREDIT_CARD: {outf << htmlize_string(i18n("Credit Card")); break;}
					case ASSETS_TYPE_LIABILITIES: {outf << htmlize_string(i18n("Liabilities")); break;}
					case ASSETS_TYPE_SECURITIES: {outf << htmlize_string(i18n("Securities"));  break;}
					default: {outf << htmlize_string(i18n("Cash")); break;}
				}
				outf << "</td>";
				outf << "<td nowrap align=\"right\">" << htmlize_string(QLocale().toCurrencyString(account_change[account])) << "</td>";
				outf << "<td nowrap align=\"right\">" << htmlize_string(QLocale().toCurrencyString(account_value[account])) << "</td>" << "\n";
				outf << "\t\t\t\t</tr>" << '\n';
				account = budget->assetsAccounts.next();
			}
			outf << "\t\t\t\t<tr>" << '\n';
			outf << "\t\t\t\t\t<td style=\"border-top: thin solid\"><b>" << htmlize_string(i18n("Total"));
			if(includes_budget) outf << "*";
			outf << "</b></td>";
			outf << "<td style=\"border-top: thin solid\">";
			outf << "</td>";
			outf << "<td nowrap align=\"right\" style=\"border-top: thin solid\"><b>" << htmlize_string(QLocale().toCurrencyString(assets_accounts_change)) << "</b></td>";
			outf << "<td nowrap align=\"right\" style=\"border-top: thin solid\"><b>" << htmlize_string(QLocale().toCurrencyString(assets_accounts_value)) << "</b></td>" << "\n";
			outf << "\t\t\t\t</tr>" << '\n';
			outf << "\t\t\t</tbody>" << '\n';
			outf << "\t\t</table>" << '\n';
			outf << "\t\t<br>" << '\n';
			outf << "\t\t<br>" << '\n';
			outf << "\t\t<table cellspacing=\"0\" cellpadding=\"5\">" << '\n';
			outf << "\t\t\t<caption>"; outf << htmlize_string(i18n("Incomes")); outf << "</caption>" << '\n';
			outf << "\t\t\t<thead>" << '\n';
			outf << "\t\t\t\t<tr>" << '\n';
			outf << "\t\t\t\t\t<th style=\"border-bottom: thin solid\">" << htmlize_string(i18n("Category")) << "</th>";
			outf << "<th style=\"border-bottom: thin solid\">" << htmlize_string(i18n("Budget")) << "</th>";
			outf << "<th style=\"border-bottom: thin solid\">" << htmlize_string(i18n("Remaining Budget")) << "</th>";
			outf << "<th style=\"border-bottom: thin solid\">" << htmlize_string(i18n("Change")) << "</th>" << '\n';
			outf << "<th style=\"border-bottom: thin solid\">" << htmlize_string(i18n("Total Incomes")) << "</th>" << '\n';
			outf << "\t\t\t\t</tr>" << '\n';
			outf << "\t\t\t</thead>" << '\n';
			outf << "\t\t\t<tbody>" << '\n';
			account = budget->incomesAccounts.first();
			while(account) {
				outf << "\t\t\t\t<tr>" << '\n';
				outf << "\t\t\t\t\t<td>" << htmlize_string(account->name()) << "</td>";
				if(account_budget[account] < 0.0) {
					outf << "<td align=\"right\">-</td>";
					outf << "<td align=\"right\">-</td>";
				} else {
					outf << "<td nowrap align=\"right\">";
					outf << htmlize_string(QLocale().toCurrencyString(account_budget[account]));
					outf << "</td>";
					outf << "<td nowrap align=\"right\">";
					outf << htmlize_string(QLocale().toCurrencyString(account_budget_diff[account]));
					outf << "</td>";
				}
				outf << "<td nowrap align=\"right\">" << htmlize_string(QLocale().toCurrencyString(account_change[account])) << "</td>";
				outf << "<td nowrap align=\"right\">" << htmlize_string(QLocale().toCurrencyString(account_value[account])) << "</td>" << "\n";
				outf << "\t\t\t\t</tr>" << '\n';
				account = budget->incomesAccounts.next();
			}
			outf << "\t\t\t\t<tr>" << '\n';
			outf << "\t\t\t\t\t<td style=\"border-top: thin solid\"><b>" << htmlize_string(i18n("Total")) << "</b></td>";
			if(incomes_budget >= 0.0) {
				outf << "<td nowrap align=\"right\" style=\"border-top: thin solid\"><b>" << htmlize_string(QLocale().toCurrencyString(incomes_budget)) << "</b></td>";
				outf << "<td nowrap align=\"right\" style=\"border-top: thin solid\"><b>" << htmlize_string(QLocale().toCurrencyString(incomes_budget_diff)) << "</b></td>";
			} else {
				outf << "<td align=\"right\" style=\"border-top: thin solid\"><b>" << "-" << "</b></td>";
				outf << "<td align=\"right\" style=\"border-top: thin solid\"><b>" << "-" << "</b></td>";
			}
			outf << "<td nowrap align=\"right\" style=\"border-top: thin solid\"><b>" << htmlize_string(QLocale().toCurrencyString(incomes_accounts_change)) << "</b></td>";
			outf << "<td nowrap align=\"right\" style=\"border-top: thin solid\"><b>" << htmlize_string(QLocale().toCurrencyString(incomes_accounts_value)) << "</b></td>" << "\n";
			outf << "\t\t\t\t</tr>" << '\n';
			outf << "\t\t\t</tbody>" << '\n';
			outf << "\t\t</table>" << '\n';
			outf << "\t\t<br>" << '\n';
			outf << "\t\t<br>" << '\n';
			outf << "\t\t<table cellspacing=\"0\" cellpadding=\"5\">" << '\n';
			outf << "\t\t\t<caption>"; outf << htmlize_string(i18n("Costs")); outf << "</caption>" << '\n';
			outf << "\t\t\t<thead>" << '\n';
			outf << "\t\t\t\t<tr>" << '\n';
			outf << "\t\t\t\t\t<th style=\"border-bottom: thin solid\">" << htmlize_string(i18n("Category")) << "</th>";
			outf << "<th style=\"border-bottom: thin solid\">" << htmlize_string(i18n("Budget")) << "</th>";
			outf << "<th style=\"border-bottom: thin solid\">" << htmlize_string(i18n("Remaining Budget")) << "</th>";
			outf << "<th style=\"border-bottom: thin solid\">" << htmlize_string(i18n("Change")) << "</th>" << '\n';
			outf << "<th style=\"border-bottom: thin solid\">" << htmlize_string(i18n("Total Expenses")) << "</th>" << '\n';
			outf << "\t\t\t\t</tr>" << '\n';
			outf << "\t\t\t</thead>" << '\n';
			outf << "\t\t\t<tbody>" << '\n';
			account = budget->expensesAccounts.first();
			while(account) {
				outf << "\t\t\t\t<tr>" << '\n';
				outf << "\t\t\t\t\t<td>" << htmlize_string(account->name()) << "</td>";
				if(account_budget[account] < 0.0) {
					outf << "<td align=\"right\">-</td>";
					outf << "<td align=\"right\">-</td>";
				} else {
					outf << "<td nowrap align=\"right\">";
					outf << htmlize_string(QLocale().toCurrencyString(account_budget[account]));
					outf << "</td>";
					outf << "<td nowrap align=\"right\">";
					outf << htmlize_string(QLocale().toCurrencyString(account_budget_diff[account]));
					outf << "</td>";
				}
				outf << "<td nowrap align=\"right\">" << htmlize_string(QLocale().toCurrencyString(account_change[account])) << "</td>";
				outf << "<td nowrap align=\"right\">" << htmlize_string(QLocale().toCurrencyString(account_value[account])) << "</td>" << "\n";
				outf << "\t\t\t\t</tr>" << '\n';
				account = budget->expensesAccounts.next();
			}
			outf << "\t\t\t\t<tr>" << '\n';
			outf << "\t\t\t\t\t<td style=\"border-top: thin solid\"><b>" << htmlize_string(i18n("Total")) << "</b></td>";
			if(expenses_budget >= 0.0) {
				outf << "<td nowrap align=\"right\" style=\"border-top: thin solid\"><b>" << htmlize_string(QLocale().toCurrencyString(expenses_budget)) << "</b></td>";
				outf << "<td nowrap align=\"right\" style=\"border-top: thin solid\"><b>" << htmlize_string(QLocale().toCurrencyString(expenses_budget_diff)) << "</b></td>";
			} else {
				outf << "<td align=\"right\" style=\"border-top: thin solid\"><b>" << "-" << "</b></td>";
				outf << "<td align=\"right\" style=\"border-top: thin solid\"><b>" << "-" << "</b></td>";
			}
			outf << "<td nowrap align=\"right\" style=\"border-top: thin solid\"><b>" << htmlize_string(QLocale().toCurrencyString(expenses_accounts_change)) << "</b></td>";
			outf << "<td nowrap align=\"right\" style=\"border-top: thin solid\"><b>" << htmlize_string(QLocale().toCurrencyString(expenses_accounts_value)) << "</b></td>" << "\n";
			outf << "\t\t\t\t</tr>" << '\n';
			outf << "\t\t\t</tbody>" << '\n';
			outf << "\t\t</table>" << '\n';
			outf << "\t</body>" << '\n';
			outf << "</html>" << '\n';
			break;
		}
		case 'c': {
			//outf.setEncoding(Q3TextStream::Locale);
			outf << "\"" << i18n("Account/Category") << "\",\"" << i18n("Change") << "\",\"" << i18n("Value") << "\"\n";
			Account *account = budget->assetsAccounts.first();
			while(account) {
				outf << "\"" << account->name() << "\",\"" << QLocale().toCurrencyString(account_change[account]) << "\",\"" << QLocale().toCurrencyString(account_value[account]) << "\"\n";
				account = budget->assetsAccounts.next();
			}
			account = budget->incomesAccounts.first();
			while(account) {
				outf << "\"" << account->name() << "\",\"" << QLocale().toCurrencyString(account_change[account]) << "\",\"" << QLocale().toCurrencyString(account_value[account]) << "\"\n";
				account = budget->incomesAccounts.next();
			}
			account = budget->expensesAccounts.first();
			while(account) {
				outf << "\"" << account->name() << "\",\"" << QLocale().toCurrencyString(account_change[account]) << "\",\"" << QLocale().toCurrencyString(account_value[account]) << "\"\n";
				account = budget->expensesAccounts.next();
			}
			break;
		}
	}

	return true;
}
void Eqonomize::printView() {
	if(tabs->currentPage()->widget() == expenses_page) {
		if(expensesWidget->isEmpty()) {
			KMessageBox::error(this, i18n("Empty expenses list."));
			return;
		}
	} else if(tabs->currentPage()->widget() == incomes_page) {
		if(incomesWidget->isEmpty()) {
			KMessageBox::error(this, i18n("Empty incomes list."));
			return;
		}
	} else if(tabs->currentPage()->widget() == transfers_page) {
		if(transfersWidget->isEmpty()) {
			KMessageBox::error(this, i18n("Empty transfers list."));
			return;
		}
	} else if(tabs->currentPage()->widget() == securities_page) {
		if(securitiesView->topLevelItemCount() == 0) {
			KMessageBox::error(this, i18n("Empty securities list."));
			return;
		}
	} else if(tabs->currentPage()->widget() == schedule_page) {
		if(scheduleView->topLevelItemCount() == 0) {
			KMessageBox::error(this, i18n("Empty schedule list."));
			return;
		}
	}

	QString str;
	QTextStream stream(&str, QIODevice::WriteOnly);
	saveView(stream, 'h');
	KHTMLPart *htmlpart = new KHTMLPart(this);
	htmlpart->begin();
	htmlpart->write(str);
	htmlpart->end();
	htmlpart->view()->print();
	delete htmlpart;
}
bool Eqonomize::saveView(QTextStream &file, int fileformat) {
	if(tabs->currentPage()->widget() == accounts_page) {return exportAccountsList(file, fileformat);}
	else if(tabs->currentPage()->widget() == expenses_page) {return expensesWidget->exportList(file, fileformat);}
	else if(tabs->currentPage()->widget() == incomes_page) {return incomesWidget->exportList(file, fileformat);}
	else if(tabs->currentPage()->widget() == transfers_page) {return transfersWidget->exportList(file, fileformat);}
	else if(tabs->currentPage()->widget() == securities_page) {return exportSecuritiesList(file, fileformat);}
	else if(tabs->currentPage()->widget() == schedule_page) {return exportScheduleList(file, fileformat);}
	return false;
}
void Eqonomize::saveView() {
	if(tabs->currentPage()->widget() == expenses_page) {
		if(expensesWidget->isEmpty()) {
			KMessageBox::error(this, i18n("Empty expenses list."));
			return;
		}
	} else if(tabs->currentPage()->widget() == incomes_page) {
		if(incomesWidget->isEmpty()) {
			KMessageBox::error(this, i18n("Empty incomes list."));
			return;
		}
	} else if(tabs->currentPage()->widget() == transfers_page) {
		if(transfersWidget->isEmpty()) {
			KMessageBox::error(this, i18n("Empty transfers list."));
			return;
		}
	} else if(tabs->currentPage()->widget() == securities_page) {
		if(securitiesView->topLevelItemCount() == 0) {
			KMessageBox::error(this, i18n("Empty securities list."));
			return;
		}
	} else if(tabs->currentPage()->widget() == schedule_page) {
		if(scheduleView->topLevelItemCount() == 0) {
			KMessageBox::error(this, i18n("Empty schedule list."));
			return;
		}
	}
	char filetype = 'h';
	QMimeDatabase db;
	QString html_filter = db.mimeTypeForName("text/html").filterString();
	QString csv_filter = db.mimeTypeForName("text/csv").filterString();
	QString filter = html_filter;
	QUrl url = QFileDialog::getSaveFileUrl(this, QString(), QUrl(), html_filter + ";;" + csv_filter, &filter);
	if(filter == csv_filter) filetype = 'c';
	if(url.isEmpty() && url.isValid()) return;
	if(url.isLocalFile()) {
		if(QFile::exists(url.toLocalFile())) {
			if(KMessageBox::warningYesNo(this, i18n("The selected file already exists. Would you like to overwrite the old copy?")) != KMessageBox::Yes) return;

		}
		QFileInfo info(url.toLocalFile());
		if(info.isDir()) {
			KMessageBox::error(this, i18n("You selected a directory!"));
			return;
		}
		QSaveFile ofile(url.toLocalFile());
		ofile.open(QIODevice::WriteOnly);
		if(!ofile.isOpen()) {
			ofile.cancelWriting();
			KMessageBox::error(this, i18n("Couldn't open file for writing."));
			return;
		}
		QTextStream stream(&ofile);
		saveView(stream, filetype);
		if(!ofile.commit()) {
			KMessageBox::error(this, i18n("Error while writing file; file was not saved."));
			return;
		}
		return;
	}
	
	QTemporaryFile tf;
	tf.open();
	tf.setAutoRemove(true);
	QTextStream stream(&tf);
	if(saveView(stream, filetype)) {
		KIO::FileCopyJob *job = KIO::file_copy(QUrl::fromLocalFile(tf.fileName()), url, KIO::Overwrite);
		KJobWidgets::setWindow(job, this);
		if(!job->exec()) {
			if(job->error()) {
				KMessageBox::error(this, i18n("Failed to upload file to %1: %2", url.toString(), job->errorString()));
			}
		}
	}

}

#define NEW_ACTION(action, text, icon, shortcut, receiver, slot, x, name) action = x->addAction(name); action->setText(text); action->setIcon(QIcon::fromTheme(icon)); x->setDefaultShortcut(action, shortcut); connect(action, SIGNAL(triggered()), receiver, slot);

#define NEW_ACTION_2(action, text, shortcut, receiver, slot, x, name) action = x->addAction(name); action->setText(text); x->setDefaultShortcut(action, shortcut); connect(action, SIGNAL(triggered()), receiver, slot);

#define NEW_TOGGLE_ACTION(action, text, shortcut, receiver, slot, x, name) action = new QAction(this); x->addAction(name, action); action->setText(text);  x->setDefaultShortcut(action, shortcut); action->setCheckable(true); connect(action, SIGNAL(triggered(bool)), receiver, slot);

void Eqonomize::setupActions() {

	actionCollection()->addAction(KStandardAction::New, this, SLOT(fileNew()));
	actionCollection()->addAction(KStandardAction::Open, this, SLOT(fileOpen()));
	ActionSave = actionCollection()->addAction(KStandardAction::Save, this, SLOT(fileSave()));
	ActionOpenRecent = KStandardAction::openRecent(this, SLOT(fileOpenRecent(const QUrl&)), this);
	actionCollection()->addAction(ActionOpenRecent->objectName(), ActionOpenRecent);
	actionCollection()->addAction(KStandardAction::SaveAs, this, SLOT(fileSaveAs()));
	actionCollection()->addAction(KStandardAction::Quit, this, SLOT(close()));
	ActionFileReload = actionCollection()->addAction(KStandardAction::Revert, this, SLOT(fileReload()));

	NEW_ACTION(ActionSaveView, i18n("Export View..."), "document-export", 0, this, SLOT(saveView()), actionCollection(), "save_view");
	ActionPrintView = actionCollection()->addAction(KStandardAction::Print, this, SLOT(printView()));
	ActionPrintView->setText(i18n("Print View..."));

	ActionSelectInitialPeriod = new KSelectAction(i18n("Initial Period"), this);
	actionCollection()->addAction("select_initial_period", ActionSelectInitialPeriod);
	QStringList period_list;
	period_list << i18n("Current Month");
	period_list << i18n("Current Year");
	period_list << i18n("Current Whole Month");
	period_list << i18n("Current Whole Year");
	period_list << i18n("Remember Last Dates");
	ActionSelectInitialPeriod->setItems(period_list);

	NEW_ACTION(ActionImportCSV, i18n("Import CSV File..."), "document-import", 0, this, SLOT(importCSV()), actionCollection(), "import_csv");
	NEW_ACTION(ActionImportQIF, i18n("Import QIF File..."), "document-import", 0, this, SLOT(importQIF()), actionCollection(), "import_qif");
	NEW_ACTION(ActionExportQIF, i18n("Export As QIF File..."), "document-export", 0, this, SLOT(exportQIF()), actionCollection(), "export_qif");

	NEW_ACTION(ActionAddAccount, i18n("Add Account..."), "document-new", 0, this, SLOT(addAccount()), actionCollection(), "add_account");
	NEW_ACTION(ActionNewAssetsAccount, i18n("New Account..."), "document-new", 0, this, SLOT(newAssetsAccount()), actionCollection(), "new_assets_account");
	NEW_ACTION(ActionNewIncomesAccount, i18n("New Income Category..."), "document-new", 0, this, SLOT(newIncomesAccount()), actionCollection(), "new_incomes_account");
	NEW_ACTION(ActionNewExpensesAccount, i18n("New Expense Category..."), "document-new", 0, this, SLOT(newExpensesAccount()), actionCollection(), "new_expenses_account");
	NEW_ACTION_2(ActionBalanceAccount, i18n("Balance..."), 0, this, SLOT(balanceAccount()), actionCollection(), "balance_account");
	NEW_ACTION(ActionEditAccount, i18n("Edit..."), "document-open", 0, this, SLOT(editAccount()), actionCollection(), "edit_account");
	NEW_ACTION(ActionDeleteAccount, KStandardGuiItem::remove().text(), "edit-delete", 0, this, SLOT(deleteAccount()), actionCollection(), "delete_account");

	NEW_ACTION_2(ActionShowAccountTransactions, i18n("Show Transactions"), 0, this, SLOT(showAccountTransactions()), actionCollection(), "show_account_transactions");

	NEW_ACTION(ActionNewExpense, i18n("New Expense..."), "document-new", Qt::CTRL+Qt::Key_E, this, SLOT(newScheduledExpense()), actionCollection(), "new_expense");
	NEW_ACTION(ActionNewIncome, i18n("New Income..."), "document-new", Qt::CTRL+Qt::Key_I, this, SLOT(newScheduledIncome()), actionCollection(), "new_income");
	NEW_ACTION(ActionNewTransfer, i18n("New Transfer..."), "document-new", Qt::CTRL+Qt::Key_T, this, SLOT(newScheduledTransfer()), actionCollection(), "new_transfer");
	NEW_ACTION(ActionNewSplitTransaction, i18n("New Split Transaction..."), "document-new", Qt::CTRL+Qt::Key_W, this, SLOT(newSplitTransaction()), actionCollection(), "new_split_transaction");
	NEW_ACTION(ActionEditTransaction, i18n("Edit Transaction(s) (Occurrence)..."), "document-open", 0, this, SLOT(editSelectedTransaction()), actionCollection(), "edit_transaction");
	NEW_ACTION(ActionEditOccurrence, i18n("Edit Occurrence..."), "document-open", 0, this, SLOT(editOccurrence()), actionCollection(), "edit_occurrence");
	NEW_ACTION(ActionEditScheduledTransaction, i18n("Edit Schedule (Recurrence)..."), "document-open", 0, this, SLOT(editSelectedScheduledTransaction()), actionCollection(), "edit_scheduled_transaction");
	NEW_ACTION(ActionEditSchedule, i18n("Edit Schedule..."), "document-open", 0, this, SLOT(editScheduledTransaction()), actionCollection(), "edit_schedule");
	NEW_ACTION(ActionDeleteTransaction, i18n("Remove Transaction(s) (Occurrence)"), "edit-delete", 0, this, SLOT(deleteSelectedTransaction()), actionCollection(), "delete_transaction");
	NEW_ACTION(ActionDeleteOccurrence, i18n("Remove Occurrence"), "edit-delete", 0, this, SLOT(removeOccurrence()), actionCollection(), "delete_occurrence");
	NEW_ACTION(ActionDeleteScheduledTransaction, i18n("Delete Schedule (Recurrence)"), "edit-delete", 0, this, SLOT(deleteSelectedScheduledTransaction()), actionCollection(), "delete_scheduled_transaction");
	NEW_ACTION(ActionDeleteSchedule, i18n("Delete Schedule"), "edit-delete", 0, this, SLOT(removeScheduledTransaction()), actionCollection(), "delete_schedule");
	NEW_ACTION(ActionEditSplitTransaction, i18n("Edit Split Transaction..."), "document-open", 0, this, SLOT(editSelectedSplitTransaction()), actionCollection(), "edit_split_transaction");
	NEW_ACTION(ActionDeleteSplitTransaction, i18n("Remove Split Transaction"), "edit-delete", 0, this, SLOT(deleteSelectedSplitTransaction()), actionCollection(), "delete_split_transaction");
	NEW_ACTION_2(ActionJoinTransactions, i18n("Join Transactions..."), 0, this, SLOT(joinSelectedTransactions()), actionCollection(), "joins_transactions");
	NEW_ACTION_2(ActionSplitUpTransaction, i18n("Split Up Transaction"), 0, this, SLOT(splitUpSelectedTransaction()), actionCollection(), "split_up_transaction");

	NEW_ACTION(ActionNewRefund, i18n("Refund..."), "go-next", 0, this, SLOT(newRefund()), actionCollection(), "new_refund");
	NEW_ACTION(ActionNewRepayment, i18n("Repayment..."), "go-previous", 0, this, SLOT(newRepayment()), actionCollection(), "new_repayment");
	NEW_ACTION(ActionNewRefundRepayment, i18n("New Refund/Repayment..."), "document-new", 0, this, SLOT(newRefundRepayment()), actionCollection(), "new_refund_repayment");

	NEW_ACTION(ActionNewSecurity, i18n("New Security..."), "document-new", 0, this, SLOT(newSecurity()), actionCollection(), "new_security");
	NEW_ACTION(ActionEditSecurity, i18n("Edit Security..."), "document-open", 0, this, SLOT(editSecurity()), actionCollection(), "edit_security");
	NEW_ACTION(ActionDeleteSecurity, i18n("Remove Security"), "edit-delete", 0, this, SLOT(deleteSecurity()), actionCollection(), "delete_security");
	NEW_ACTION(ActionSellShares, i18n("Shares Sold..."), "go-next", 0, this, SLOT(sellSecurities()), actionCollection(), "sell_shares");
	NEW_ACTION(ActionBuyShares, i18n("Shares Bought..."), "go-previous", 0, this, SLOT(buySecurities()), actionCollection(), "buy_shares");
	NEW_ACTION(ActionNewDividend, i18n("Dividend..."), "go-next", 0, this, SLOT(newDividend()), actionCollection(), "new_dividend");
	NEW_ACTION(ActionNewReinvestedDividend, i18n("Reinvested Dividend..."), "go-next", 0, this, SLOT(newReinvestedDividend()), actionCollection(), "new_reinvested_dividend");
	NEW_ACTION_2(ActionNewSecurityTrade, i18n("Shares Moved..."), 0, this, SLOT(newSecurityTrade()), actionCollection(), "new_security_trade");
	NEW_ACTION(ActionSetQuotation, i18n("Set Quotation..."), "view-calendar-day", 0, this, SLOT(setQuotation()), actionCollection(), "set_quotation");
	NEW_ACTION(ActionEditQuotations, i18n("Edit Quotations..."), "view-calendar-list", 0, this, SLOT(editQuotations()), actionCollection(), "edit_quotations");
	NEW_ACTION(ActionEditSecurityTransactions, i18n("Transactions..."), "view-list-details", 0, this, SLOT(editSecurityTransactions()), actionCollection(), "edit_security_transactions");

	NEW_ACTION(ActionOverTimeReport, i18n("Development Over Time Report..."), "view-list-text", 0, this, SLOT(showOverTimeReport()), actionCollection(), "over_time_report");
	NEW_ACTION(ActionCategoriesComparisonReport, i18n("Categories Comparison Report..."), "view-list-text", 0, this, SLOT(showCategoriesComparisonReport()), actionCollection(), "categories_comparison_report");
	NEW_ACTION(ActionCategoriesComparisonChart, i18n("Categories Comparison Chart..."), "view-statistics", 0, this, SLOT(showCategoriesComparisonChart()), actionCollection(), "categories_comparison_chart");
	NEW_ACTION(ActionOverTimeChart, i18n("Development Over Time Chart..."), "view-statistics", 0, this, SLOT(showOverTimeChart()), actionCollection(), "over_time_chart");

	NEW_TOGGLE_ACTION(ActionExtraProperties, i18n("Use Additional Transaction Properties"), 0, this, SLOT(useExtraProperties(bool)), actionCollection(), "extra_properties");
	ActionExtraProperties->setChecked(b_extra);

	ActionSave->setEnabled(false);
	ActionFileReload->setEnabled(false);
	ActionBalanceAccount->setEnabled(false);
	ActionEditAccount->setEnabled(false);
	ActionDeleteAccount->setEnabled(false);
	ActionShowAccountTransactions->setEnabled(false);
	ActionEditTransaction->setEnabled(false);
	ActionDeleteTransaction->setEnabled(false);
	ActionEditSplitTransaction->setEnabled(false);
	ActionDeleteSplitTransaction->setEnabled(false);
	ActionJoinTransactions->setEnabled(false);
	ActionSplitUpTransaction->setEnabled(false);
	ActionEditScheduledTransaction->setEnabled(false);
	ActionDeleteScheduledTransaction->setEnabled(false);
	ActionNewRefund->setEnabled(false);
	ActionNewRepayment->setEnabled(false);
	ActionNewRefundRepayment->setEnabled(false);
	ActionEditSecurity->setEnabled(false);
	ActionDeleteSecurity->setEnabled(false);
	ActionSellShares->setEnabled(true);
	ActionBuyShares->setEnabled(true);
	ActionNewDividend->setEnabled(true);
	ActionNewReinvestedDividend->setEnabled(true);
	ActionNewSecurityTrade->setEnabled(true);
	ActionSetQuotation->setEnabled(false);
	ActionEditQuotations->setEnabled(false);
	ActionEditSecurityTransactions->setEnabled(false);

}

bool Eqonomize::crashRecovery(QUrl url) {
	KConfigGroup config = KSharedConfig::openConfig()->group("General Options");
	QList<KAutoSaveFile*> staleFiles;
	if(url.isLocalFile()) {
		staleFiles = KAutoSaveFile::staleFiles(url);
	} else if(url.isEmpty()) {
		staleFiles = KAutoSaveFile::staleFiles(QUrl::fromLocalFile("UNSAVED EQZ"));
	}
	if(!staleFiles.isEmpty()) {
		if(KMessageBox::questionYesNo(this, i18n("Eqonomize! exited unexpectedly before the file was saved and data was lost.\nDo you want to load the last auto-saved version of the file?"), i18n("Crash Recovery")) == KMessageBox::Yes) {
			QString errors;
			QString error = budget->loadFile(staleFiles.first()->fileName(), errors);
			if(!error.isNull()) {
				KMessageBox::error(this, i18n("Error loading %1: %2.", cr_tmp_file->fileName(), error), i18n("Couldn't open file"));
				config.sync();
				foreach(KAutoSaveFile *stale, staleFiles) {
					stale->remove();
					delete stale;
				}
				return false;
			}
			if(!errors.isEmpty()) {
				KMessageBox::error(this, errors);
			}
			current_url = url;
			ActionFileReload->setEnabled(true);
			setCaption(current_url.fileName(), false);
			foreach(KAutoSaveFile *stale, staleFiles) {
				stale->remove();
				delete stale;
			}
			if(cr_tmp_file) {
				cr_tmp_file->releaseLock();
				delete cr_tmp_file;
				cr_tmp_file = NULL;
			}

			reloadBudget();

			emit accountsModified();
			emit transactionsModified();
			emit budgetUpdated();

			setModified(true);
			checkSchedule(true);

			return true;

		}
	}
	foreach(KAutoSaveFile *stale, staleFiles) {
		stale->remove();
		delete stale;
	}

	return false;

}
void Eqonomize::onAutoSaveTimeout() {
	auto_save_timeout = true;
	if(modified_auto_save) {
		autoSave();
	}
}
void Eqonomize::autoSave() {
	if(auto_save_timeout) {
		saveCrashRecovery();
		modified_auto_save = false;
		auto_save_timeout = false;
	}
}
void Eqonomize::saveCrashRecovery() {
	if(!cr_tmp_file) {
		if(current_url.isEmpty()) cr_tmp_file = new KAutoSaveFile(QUrl::fromLocalFile("UNSAVED EQZ"), this);
		else cr_tmp_file = new KAutoSaveFile(current_url, this);
	}
	if(budget->saveFile(cr_tmp_file->fileName()).isNull()) {
		KConfigGroup config = KSharedConfig::openConfig()->group("General Options");
		config.writeEntry("lastURL", current_url.url());
		config.sync();
	}
}

void Eqonomize::setCommandLineParser(QCommandLineParser *p) {
	parser = p;
}
void Eqonomize::onActivateRequested(const QStringList &arguments, const QString &workingDirectory) {
	if(!arguments.isEmpty()) {
		parser->process(arguments);
		if(parser->isSet("expenses")) {
			showExpenses();
		} else if(parser->isSet("incomes")) {
			showIncomes();
		} else if(parser->isSet("transfers")) {
			showTransfers();
		}
		QStringList args = parser->positionalArguments();
		if(args.count() > 0) {
			QString curpath = QDir::currentPath();
			QDir::setCurrent(workingDirectory);
			openURL(QUrl::fromUserInput(args.at(0)));
			QDir::setCurrent(curpath);
		}
		args.clear();
	}
	setWindowState((windowState() & ~Qt::WindowMinimized) | Qt::WindowActive);
	show();
	activateWindow();
}

void Eqonomize::saveOptions() {
	KConfigGroup config = KSharedConfig::openConfig()->group("General Options");
	ActionOpenRecent->saveEntries(KSharedConfig::openConfig()->group("RecentFiles"));
	config.writeEntry("lastURL", current_url.url());
	config.writeEntry("currentEditExpenseFromItem", expensesWidget->currentEditFromItem());
	config.writeEntry("currentEditExpenseToItem", expensesWidget->currentEditToItem());
	config.writeEntry("currentEditIncomeFromItem", incomesWidget->currentEditFromItem());
	config.writeEntry("currentEditIncomeToItem", incomesWidget->currentEditToItem());
	config.writeEntry("currentEditTransferFromItem", transfersWidget->currentEditFromItem());
	config.writeEntry("currentEditTransferToItem", transfersWidget->currentEditToItem());
	config.writeEntry("useExtraProperties", b_extra);
	config.writeEntry("firstRun", false);
	config.writeEntry("size", size());

	InitialPeriod initial_period = (InitialPeriod) ActionSelectInitialPeriod->currentItem();
	config.writeEntry("initialPeriod", int(initial_period));
	if(initial_period == INITIAL_PERIOD_LAST) {
		config.writeEntry("initialFromDate", from_date.toString(Qt::ISODate));
		config.writeEntry("initialFromDateEnabled", accountsPeriodFromButton->isChecked());
		config.writeEntry("initialToDate", to_date.toString(Qt::ISODate));
	}
	emit timeToSaveConfig();
}


void Eqonomize::readFileDependentOptions() {
	KConfigGroup config = KSharedConfig::openConfig()->group("General Options");
	expensesWidget->setCurrentEditFromItem(config.readEntry("currentEditExpenseFromItem", int(0)));
	expensesWidget->setCurrentEditToItem(config.readEntry("currentEditExpenseToItem", int(0)));
	incomesWidget->setCurrentEditFromItem(config.readEntry("currentEditIncomeFromItem", int(0)));
	incomesWidget->setCurrentEditToItem(config.readEntry("currentEditIncomeToItem", int(0)));
	transfersWidget->setCurrentEditFromItem(config.readEntry("currentEditTransferFromItem", int(0)));
	transfersWidget->setCurrentEditToItem(config.readEntry("currentEditTransferToItem", int(0)));
}
void Eqonomize::readOptions() {
	KConfigGroup config = KSharedConfig::openConfig()->group("General Options");
	ActionOpenRecent->loadEntries(KSharedConfig::openConfig()->group("RecentFiles"));
	first_run = config.readEntry("firstRun", true);
	config.writeEntry("firstRun", false);
	QSize window_size = config.readEntry("size", QSize());
	if(window_size.isValid()) resize(window_size);
}

bool Eqonomize::queryClose() {
	if(askSave(true)) {
		saveOptions();
		return true;
	}
	return false;
}

void Eqonomize::dragEnterEvent(QDragEnterEvent *event) {
	event->setAccepted(event->mimeData()->hasUrls());
}

void Eqonomize::dropEvent(QDropEvent *event) {
	QList<QUrl> urls =event->mimeData()->urls();
	if(!urls.isEmpty()) {
		const QUrl &url = urls.first();
		if(!askSave()) return;
		openURL(url);
	}
}

void Eqonomize::fileNew() {
	if(!askSave()) return;
	budget->clear();
	reloadBudget();
	setCaption(QString::null, false);
	current_url = "";
	ActionFileReload->setEnabled(false);
	KConfigGroup config = KSharedConfig::openConfig()->group("General Options");
	config.writeEntry("lastURL", current_url.url());
	if(cr_tmp_file) {
		cr_tmp_file->releaseLock();
		delete cr_tmp_file;
		cr_tmp_file = NULL;
	}
	config.sync();
	setModified(false);
	emit accountsModified();
	emit transactionsModified();
	emit budgetUpdated();
}

void Eqonomize::fileOpen() {
	if(!askSave()) return;
	QMimeDatabase db;
	QUrl url = QFileDialog::getOpenFileUrl(this, QString(), QUrl(), db.mimeTypeForName("application/x-eqonomize").filterString());
	if(!url.isEmpty()) openURL(url);
}
void Eqonomize::fileOpenRecent(const QUrl &url) {
	if(!askSave()) return;
	openURL(url);
}

void Eqonomize::fileReload() {
	openURL(current_url);
}
void Eqonomize::fileSave() {
	if(!current_url.isValid()) {
		QMimeDatabase db;
		QUrl file_url = QFileDialog::getSaveFileUrl(this, QString(), QUrl::fromLocalFile("budget.eqz"), db.mimeTypeForName("application/x-eqonomize").filterString());
		if (!file_url.isEmpty() && file_url.isValid()) {
			QString spath = file_url.path();
			if(!spath.endsWith(".eqz", Qt::CaseInsensitive)) {
				spath += ".eqz";
				file_url.setPath(spath);
			}
			saveURL(file_url);
		}
	} else {
		saveURL(current_url);
	}
}

void Eqonomize::fileSaveAs() {
	QMimeDatabase db;
	QUrl file_url = QFileDialog::getSaveFileUrl(this, QString(), current_url.adjusted(QUrl::RemoveFilename), db.mimeTypeForName("application/x-eqonomize").filterString());
	if (!file_url.isEmpty() && file_url.isValid()) {
		QString spath = file_url.path();
		if(!spath.endsWith(".eqz", Qt::CaseInsensitive)) {
			spath += ".eqz";
			file_url.setPath(spath);
		}
		saveURL(file_url);
	}
}

bool Eqonomize::askSave(bool before_exit) {
	if(!modified) return true;
	int b_save = 0;
	if(before_exit && current_url.isValid()) b_save = KMessageBox::warningYesNoCancel(this, i18n("The current file has been modified. Do you want to save it?"), QString(), KStandardGuiItem::yes(), KStandardGuiItem::no(), KStandardGuiItem::cancel(), "saveOnExit");
	else b_save = KMessageBox::warningYesNoCancel(this, i18n("The current file has been modified. Do you want to save it?"));
	if(b_save == KMessageBox::Yes) {
		if(!current_url.isValid()) {
			QMimeDatabase db;
			QUrl file_url = QFileDialog::getSaveFileUrl(this, QString(), QUrl::fromLocalFile("budget.eqz"), db.mimeTypeForName("application/x-eqonomize").filterString());
			if (!file_url.isEmpty() && file_url.isValid()) {
				QString spath = file_url.path();
				if(!spath.endsWith(".eqz", Qt::CaseInsensitive)) {
					spath += ".eqz";
					file_url.setPath(spath);
				}
				return saveURL(file_url);
			} else {
				return false;
			}
		} else {
			return saveURL(current_url);
		}
	}
	if(b_save == KMessageBox::No) {
		if(cr_tmp_file) {
			cr_tmp_file->releaseLock();
			delete cr_tmp_file;
			cr_tmp_file = NULL;
		}
		return true;
	}
	return false;
}

void Eqonomize::optionsPreferences() {
}

void Eqonomize::checkSchedule() {
	checkSchedule(true);
}
bool Eqonomize::checkSchedule(bool update_display) {
	bool b = false;
	ScheduledTransaction *strans = budget->scheduledTransactions.first();
	while(strans) {
		if(strans->firstOccurrence() < QDate::currentDate() || (QTime::currentTime().hour() >= 18 && strans->firstOccurrence() == QDate::currentDate())) {
			b = true;
			ConfirmScheduleDialog *dialog = new ConfirmScheduleDialog(b_extra, budget, this, i18n("Confirm Schedule"));
			updateScheduledTransactions();
			dialog->exec();
			Transaction *trans = dialog->firstTransaction();
			while(trans) {
				budget->addTransaction(trans);
				trans = dialog->nextTransaction();
			}
			dialog->deleteLater();
			break;
		}
		strans = budget->scheduledTransactions.next();
	}
	if(b && update_display) {
		expensesWidget->transactionsReset();
		incomesWidget->transactionsReset();
		transfersWidget->transactionsReset();
		filterAccounts();
		updateScheduledTransactions();
		updateSecurities();
		emit transactionsModified();
		setModified(true);
	}
	return b;
}

void Eqonomize::updateScheduledTransactions() {
	scheduleView->clear();
	ScheduledTransaction *strans = budget->scheduledTransactions.first();
	QList<QTreeWidgetItem *> items;
	while(strans) {
		items.append(new ScheduleListViewItem(strans, strans->firstOccurrence()));
		strans = budget->scheduledTransactions.next();
	}
	scheduleView->addTopLevelItems(items);
	scheduleView->setSortingEnabled(true);
}
void Eqonomize::appendScheduledTransaction(ScheduledTransaction *strans) {
	scheduleView->insertTopLevelItem(scheduleView->topLevelItemCount(), new ScheduleListViewItem(strans, strans->firstOccurrence()));
	scheduleView->setSortingEnabled(true);
}

void Eqonomize::addAccount() {
	QTreeWidgetItem *i = selectedItem(accountsView);
	if(i == NULL) return;
	if(i == assetsItem || (account_items.contains(i) && (account_items[i]->type() == ACCOUNT_TYPE_ASSETS))) {
		newAssetsAccount();
	} else if(i == incomesItem || (account_items.contains(i) && (account_items[i]->type() == ACCOUNT_TYPE_INCOMES))) {
		newIncomesAccount();
	} else {
		newExpensesAccount();
	}
}
void Eqonomize::newAssetsAccount() {
	EditAssetsAccountDialog *dialog = new EditAssetsAccountDialog(budget, this, i18n("New Account"));
	if(dialog->exec() == QDialog::Accepted) {
		AssetsAccount *account = dialog->newAccount();
		budget->addAccount(account);
		appendAssetsAccount(account);
		filterAccounts();
		expensesWidget->updateFromAccounts();
		incomesWidget->updateToAccounts();
		transfersWidget->updateAccounts();
		emit accountsModified();
		setModified(true);
	}
	dialog->deleteLater();
}
void Eqonomize::newIncomesAccount() {
	EditIncomesAccountDialog *dialog = new EditIncomesAccountDialog(budget, this, i18n("New Income Category"));
	if(dialog->exec() == QDialog::Accepted) {
		IncomesAccount *account = dialog->newAccount();
		budget->addAccount(account);
		appendIncomesAccount(account);
		filterAccounts();
		incomesWidget->updateFromAccounts();
		emit accountsModified();
		setModified(true);
	}
	dialog->deleteLater();
}
void Eqonomize::newExpensesAccount() {
	EditExpensesAccountDialog *dialog = new EditExpensesAccountDialog(budget, this, i18n("New Expense Category"));
	if(dialog->exec() == QDialog::Accepted) {
		ExpensesAccount *account = dialog->newAccount();
		budget->addAccount(account);
		appendExpensesAccount(account);
		filterAccounts();
		expensesWidget->updateToAccounts();
		emit accountsModified();
		setModified(true);
	}
	dialog->deleteLater();
}
void Eqonomize::accountExecuted(QTreeWidgetItem*) {
	showAccountTransactions();
}
void Eqonomize::accountExecuted(QTreeWidgetItem *i, int c) {
	if(i == NULL) return;
	switch(c) {
		case 0: {
			if(account_items.contains(i)) {
				editAccount(account_items[i]);
			}
			break;
		}
		case 1: {
			if(account_items.contains(i)) {
				if(account_items[i]->type() == ACCOUNT_TYPE_ASSETS) {
					showAccountTransactions();
				} else {
					accountsTabs->setCurrentIndex(1);
					if(budgetEdit->isEnabled()) {
						budgetEdit->setFocus();
						budgetEdit->lineEdit()->selectAll();
					} else {
						budgetButton->setFocus();
					}
				}
			}
			break;
		}
		case 2: {
			showAccountTransactions();
			break;
		}
		case 3: {
			showAccountTransactions(true);
			break;
		}
	}
}
void Eqonomize::balanceAccount() {
	QTreeWidgetItem *i = selectedItem(accountsView);
	if(!account_items.contains(i)) return;
	Account *i_account = account_items[i];
	balanceAccount(i_account);
}
void Eqonomize::balanceAccount(Account *i_account) {
	if(!i_account) return;
	if(i_account->type() != ACCOUNT_TYPE_ASSETS || ((AssetsAccount*) i_account)->accountType() == ASSETS_TYPE_SECURITIES) return;
	AssetsAccount *account = (AssetsAccount*) i_account;
	double book_value = account->initialBalance();
	double current_balancing = 0.0;
	Transaction *trans = budget->transactions.first();
	while(trans) {
		if(trans->fromAccount() == account) {
			book_value -= trans->value();
			if(trans->toAccount() == budget->balancingAccount) current_balancing -= trans->value();
		}
		if(trans->toAccount() == account) {
			book_value += trans->value();
			if(trans->fromAccount() == budget->balancingAccount) current_balancing += trans->value();
		}
		trans = budget->transactions.next();
	}
	QDialog *dialog = new QDialog(this, 0);
	dialog->setWindowTitle(i18n("Balance Account"));
	dialog->setModal(true);
	QVBoxLayout *box1 = new QVBoxLayout(dialog);
	QGridLayout *grid = new QGridLayout();
	box1->addLayout(grid);
	grid->addWidget(new QLabel(i18n("Book value:"), dialog), 0, 0);
	QLabel *label = new QLabel(QLocale().toCurrencyString(book_value), dialog);
	label->setAlignment(Qt::AlignRight);
	grid->addWidget(label, 0, 1);
	label = new QLabel(QString("of which %1 is balanced").arg(QLocale().toCurrencyString(current_balancing)), dialog);
	label->setAlignment(Qt::AlignRight);
	grid->addWidget(label, 1, 1);
	grid->addWidget(new QLabel(i18n("Real value:"), dialog), 2, 0);
	EqonomizeValueEdit *realEdit = new EqonomizeValueEdit(book_value, true, true, dialog);
	grid->addWidget(realEdit, 2, 1);
	QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
	buttonBox->button(QDialogButtonBox::Ok)->setShortcut(Qt::CTRL | Qt::Key_Return);
	buttonBox->button(QDialogButtonBox::Ok)->setDefault(true);
	connect(buttonBox->button(QDialogButtonBox::Cancel), SIGNAL(clicked()), dialog, SLOT(reject()));
	connect(buttonBox->button(QDialogButtonBox::Ok), SIGNAL(clicked()), dialog, SLOT(accept()));
	box1->addWidget(buttonBox);
	if(dialog->exec() == QDialog::Accepted && realEdit->value() != book_value) {
		trans = new Balancing(budget, realEdit->value() - book_value, QDate::currentDate(), account);
		budget->addTransaction(trans);
		transactionAdded(trans);
	}
	dialog->deleteLater();
}
void Eqonomize::editAccount() {
	QTreeWidgetItem *i = selectedItem(accountsView);
	if(!account_items.contains(i)) return;
	Account *i_account = account_items[i];
	editAccount(i_account);
}
bool Eqonomize::editAccount(Account *i_account) {
	return editAccount(i_account, this);
}
bool Eqonomize::editAccount(Account *i_account, QWidget *parent) {
	QTreeWidgetItem *i = item_accounts[i_account];
	switch(i_account->type()) {
		case ACCOUNT_TYPE_ASSETS: {
			EditAssetsAccountDialog *dialog = new EditAssetsAccountDialog(budget, parent, i18n("Edit Account"));
			AssetsAccount *account = (AssetsAccount*) i_account;
			dialog->setAccount(account);
			double prev_ib = account->initialBalance();
			bool was_budget_account = account->isBudgetAccount();
			if(dialog->exec() == QDialog::Accepted) {
				dialog->modifyAccount(account);
				if(was_budget_account != account->isBudgetAccount()) {
					filterAccounts();
				} else {
					account_value[account] += (account->initialBalance() - prev_ib);
					assets_accounts_value += (account->initialBalance() - prev_ib);
					if(to_date > QDate::currentDate()) i->setText(0, account->name() + "*");
					else  i->setText(0, account->name());
					i->setText(VALUE_COLUMN, QLocale().toString(account_value[account], 'f', MONETARY_DECIMAL_PLACES) + " ");
					assetsItem->setText(VALUE_COLUMN, QLocale().toString(assets_accounts_value, 'f', MONETARY_DECIMAL_PLACES) + " ");
				}
				emit accountsModified();
				setModified(true);
				expensesWidget->updateFromAccounts();
				incomesWidget->updateToAccounts();
				transfersWidget->updateAccounts();
				assetsItem->sortChildren(0, Qt::AscendingOrder);
				expensesWidget->filterTransactions();
				incomesWidget->filterTransactions();
				transfersWidget->filterTransactions();
				dialog->deleteLater();
				return true;
			}
			dialog->deleteLater();
			break;
		}
		case ACCOUNT_TYPE_INCOMES: {
			EditIncomesAccountDialog *dialog = new EditIncomesAccountDialog(budget, parent, i18n("Edit Income Category"));
			IncomesAccount *account = (IncomesAccount*) i_account;
			dialog->setAccount(account);
			if(dialog->exec() == QDialog::Accepted) {
				dialog->modifyAccount(account);
				i->setText(0, account->name());
				emit accountsModified();
				setModified(true);
				incomesWidget->updateFromAccounts();
				incomesItem->sortChildren(0, Qt::AscendingOrder);
				incomesWidget->filterTransactions();
				dialog->deleteLater();
				return true;
			}
			dialog->deleteLater();
			break;
		}
		case ACCOUNT_TYPE_EXPENSES: {
			EditExpensesAccountDialog *dialog = new EditExpensesAccountDialog(budget, parent, i18n("Edit Expense Category"));
			ExpensesAccount *account = (ExpensesAccount*) i_account;
			dialog->setAccount(account);
			if(dialog->exec() == QDialog::Accepted) {
				dialog->modifyAccount(account);
				i->setText(0, account->name());
				emit accountsModified();
				setModified(true);
				expensesWidget->updateToAccounts();
				expensesItem->sortChildren(0, Qt::AscendingOrder);
				expensesWidget->filterTransactions();
				dialog->deleteLater();
				return true;
			}
			dialog->deleteLater();
			break;
		}
	}
	return false;
}
void Eqonomize::deleteAccount() {
	QTreeWidgetItem *i = selectedItem(accountsView);
	if(!account_items.contains(i)) return;
	Account *account = account_items[i];
	if(!budget->accountHasTransactions(account)) {
		item_accounts.remove(account);
		account_items.remove(i);
		delete i;
		account_change.remove(account);
		account_value.remove(account);
		budget->removeAccount(account);
		expensesWidget->updateAccounts();
		transfersWidget->updateAccounts();
		incomesWidget->updateAccounts();
		filterAccounts();
		emit accountsModified();
		setModified(true);
	} else {
		QRadioButton *deleteButton = NULL, *moveToButton = NULL;
		QComboBox *moveToCombo = NULL;
		QButtonGroup *group = NULL;
		QDialog *dialog = NULL;
		bool accounts_left = false;
		QVector<Account*> moveto_accounts;
		switch(account->type()) {
			case ACCOUNT_TYPE_EXPENSES: {accounts_left = budget->expensesAccounts.count() > 1; break;}
			case ACCOUNT_TYPE_INCOMES: {accounts_left = budget->incomesAccounts.count() > 1; break;}
			case ACCOUNT_TYPE_ASSETS: {
				AssetsAccount *aaccount = budget->assetsAccounts.first();
				while(aaccount) {
					if(aaccount != budget->balancingAccount && aaccount != account && ((((AssetsAccount*) account)->accountType() == ASSETS_TYPE_SECURITIES) == (aaccount->accountType() == ASSETS_TYPE_SECURITIES))) {
						accounts_left = true;
						break;
					}
					aaccount = budget->assetsAccounts.next();
				}
				break;
			}
		}
		if(accounts_left) {
			dialog = new QDialog(this, 0);
			dialog->setWindowTitle(i18n("Move transactions?"));
			dialog->setModal(true);
			QVBoxLayout *box1 = new QVBoxLayout(dialog);
			QGridLayout *grid = new QGridLayout();
			box1->addLayout(grid);
			group = new QButtonGroup(dialog);
			QLabel *label = NULL;
			deleteButton = new QRadioButton(KStandardGuiItem::remove().text(), dialog);
			group->addButton(deleteButton);
			moveToButton = new QRadioButton(i18n("Move to:"), dialog);
			group->addButton(moveToButton);
			moveToButton->setChecked(true);
			moveToCombo = new QComboBox(dialog);
			moveToCombo->setEditable(false);
			switch(account->type()) {
				case ACCOUNT_TYPE_EXPENSES: {
					label = new QLabel(i18n("The category contains some expenses.\nWhat do you want to do with them?"), dialog);
					ExpensesAccount *eaccount = budget->expensesAccounts.first();
					while(eaccount) {
						if(eaccount != account) {
							moveToCombo->addItem(eaccount->name());
							moveto_accounts.push_back(eaccount);
						}
						eaccount = budget->expensesAccounts.next();
					}
					break;
				}
				case ACCOUNT_TYPE_INCOMES: {
					label = new QLabel(i18n("The category contains some incomes.\nWhat do you want to do with them?"), dialog);
					IncomesAccount *iaccount = budget->incomesAccounts.first();
					while(iaccount) {
						if(iaccount != account) {
							moveToCombo->addItem(iaccount->name());
							moveto_accounts.push_back(iaccount);
						}
						iaccount = budget->incomesAccounts.next();
					}
					break;
				}
				case ACCOUNT_TYPE_ASSETS: {
					label = new QLabel(i18n("The account contains some transactions.\nWhat do you want to do with them?"), dialog);
					AssetsAccount *aaccount = budget->assetsAccounts.first();
					while(aaccount) {
						if(aaccount != budget->balancingAccount && aaccount != account && ((((AssetsAccount*) account)->accountType() == ASSETS_TYPE_SECURITIES) == (aaccount->accountType() == ASSETS_TYPE_SECURITIES))) {
							moveToCombo->addItem(aaccount->name());
							moveto_accounts.push_back(aaccount);
						}
						aaccount = budget->assetsAccounts.next();
					}
					break;
				}
			}
			grid->addWidget(label, 0, 0, 1, 2);
			grid->addWidget(deleteButton, 1, 0);
			grid->addWidget(moveToButton, 2, 0);
			grid->addWidget(moveToCombo, 2, 1);
			QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
			buttonBox->button(QDialogButtonBox::Cancel)->setShortcut(Qt::CTRL | Qt::Key_Return);
			buttonBox->button(QDialogButtonBox::Cancel)->setDefault(true);
			connect(buttonBox->button(QDialogButtonBox::Cancel), SIGNAL(clicked()), dialog, SLOT(reject()));
			connect(buttonBox->button(QDialogButtonBox::Ok), SIGNAL(clicked()), dialog, SLOT(accept()));
			box1->addWidget(buttonBox);
		}
		bool do_delete = false;
		if(accounts_left) {
			do_delete = (dialog->exec() == QDialog::Accepted);
		} else {
			switch(account->type()) {
				case ACCOUNT_TYPE_EXPENSES: {do_delete = (KMessageBox::warningYesNo(this, i18n("The category contains some expenses that will be removed. Do you still want to remove the category?"), i18n("Remove Category?")) == KMessageBox::Yes); break;}
				case ACCOUNT_TYPE_INCOMES: {do_delete = (KMessageBox::warningYesNo(this, i18n("The category contains some incomes that will be removed. Do you still want to remove the category?"), i18n("Remove Category?")) == KMessageBox::Yes); break;}
				case ACCOUNT_TYPE_ASSETS: {do_delete = (KMessageBox::warningYesNo(this, i18n("The account contains some transactions that will be removed. Do you still want to remove the account?"), i18n("Remove Account?")) == KMessageBox::Yes); break;}
			}
		}
		if(do_delete) {
			if(accounts_left && moveToButton->isChecked()) {
				budget->moveTransactions(account, moveto_accounts[moveToCombo->currentIndex()]);
			}
			item_accounts.remove(account);
			account_items.remove(i);
			delete i;
			account_change.remove(account);
			account_value.remove(account);
			AccountType type = account->type();
			budget->removeAccount(account);
			filterAccounts();
			updateSecurities();
			updateScheduledTransactions();
			switch(type) {
				case ACCOUNT_TYPE_EXPENSES: {expensesWidget->filterTransactions(); break;}
				case ACCOUNT_TYPE_INCOMES: {incomesWidget->filterTransactions(); break;}
				case ACCOUNT_TYPE_ASSETS: {incomesWidget->filterTransactions(); expensesWidget->filterTransactions(); transfersWidget->filterTransactions(); break;}
			}
			expensesWidget->updateAccounts();
			transfersWidget->updateAccounts();
			incomesWidget->updateAccounts();
			emit accountsModified();
			emit transactionsModified();
			setModified(true);
		}
		if(dialog) dialog->deleteLater();
	}
}

void Eqonomize::transactionAdded(Transaction *trans) {
	addTransactionValue(trans, trans->date(), true);
	emit transactionsModified();
	setModified(true);
	expensesWidget->onTransactionAdded(trans);
	incomesWidget->onTransactionAdded(trans);
	transfersWidget->onTransactionAdded(trans);
	if(trans->type() == TRANSACTION_TYPE_SECURITY_BUY || trans->type() == TRANSACTION_TYPE_SECURITY_SELL) {
		updateSecurity(((SecurityTransaction*) trans)->security());
	} else if(trans->type() == TRANSACTION_TYPE_INCOME && ((Income*) trans)->security()) {
		updateSecurity(((Income*) trans)->security());
	}
}
void Eqonomize::transactionModified(Transaction *trans, Transaction *oldtrans) {
	subtractTransactionValue(oldtrans, true);
	addTransactionValue(trans, trans->date(), true);
	emit transactionsModified();
	setModified(true);
	expensesWidget->onTransactionModified(trans, oldtrans);
	incomesWidget->onTransactionModified(trans, oldtrans);
	transfersWidget->onTransactionModified(trans, oldtrans);
	if(trans->type() == TRANSACTION_TYPE_SECURITY_BUY || trans->type() == TRANSACTION_TYPE_SECURITY_SELL) {
		updateSecurity(((SecurityTransaction*) trans)->security());
		if(((SecurityTransaction*) trans)->security() != ((SecurityTransaction*) oldtrans)->security()) {
			updateSecurity(((SecurityTransaction*) oldtrans)->security());
		}
	} else if(trans->type() == TRANSACTION_TYPE_INCOME && ((Income*) trans)->security()) {
		updateSecurity(((Income*) trans)->security());
		if(((Income*) trans)->security() != ((Income*) oldtrans)->security()) {
			updateSecurity(((Income*) oldtrans)->security());
		}
	}
}
void Eqonomize::transactionRemoved(Transaction *trans) {
	subtractTransactionValue(trans, true);
	emit transactionsModified();
	setModified(true);
	expensesWidget->onTransactionRemoved(trans);
	incomesWidget->onTransactionRemoved(trans);
	transfersWidget->onTransactionRemoved(trans);
	if(trans->type() == TRANSACTION_TYPE_SECURITY_BUY || trans->type() == TRANSACTION_TYPE_SECURITY_SELL) {
		updateSecurity(((SecurityTransaction*) trans)->security());
	} else if(trans->type() == TRANSACTION_TYPE_INCOME && ((Income*) trans)->security()) {
		updateSecurity(((Income*) trans)->security());
	}
}

void Eqonomize::scheduledTransactionAdded(ScheduledTransaction *strans) {
	appendScheduledTransaction(strans);
	addScheduledTransactionValue(strans, true);
	emit transactionsModified();
	setModified(true);
	expensesWidget->onScheduledTransactionAdded(strans);
	incomesWidget->onScheduledTransactionAdded(strans);
	transfersWidget->onScheduledTransactionAdded(strans);
	if(strans->transaction()->type() == TRANSACTION_TYPE_SECURITY_BUY || strans->transaction()->type() == TRANSACTION_TYPE_SECURITY_SELL) {
		updateSecurity(((SecurityTransaction*) strans->transaction())->security());
	} else if(strans->transaction()->type() == TRANSACTION_TYPE_INCOME && ((Income*) strans->transaction())->security()) {
		updateSecurity(((Income*) strans->transaction())->security());
	}
}
void Eqonomize::scheduledTransactionModified(ScheduledTransaction *strans, ScheduledTransaction *oldstrans) {
	QTreeWidgetItemIterator it(scheduleView);
	ScheduleListViewItem *i = (ScheduleListViewItem*) *it;
	while(i) {
		if(i->scheduledTransaction() == strans) {
			i->setScheduledTransaction(strans);
			i->setDate(strans->firstOccurrence());
			break;
		}
		++it;
		i = (ScheduleListViewItem*) *it;
	}
	subtractScheduledTransactionValue(oldstrans, true);
	addScheduledTransactionValue(strans, true);
	emit transactionsModified();
	setModified(true);
	expensesWidget->onScheduledTransactionModified(strans, oldstrans);
	incomesWidget->onScheduledTransactionModified(strans, oldstrans);
	transfersWidget->onScheduledTransactionModified(strans, oldstrans);
	if(strans->transaction()->type() == TRANSACTION_TYPE_SECURITY_BUY || strans->transaction()->type() == TRANSACTION_TYPE_SECURITY_SELL) {
		updateSecurity(((SecurityTransaction*) strans->transaction())->security());
		if(((SecurityTransaction*) strans->transaction())->security() != ((SecurityTransaction*) oldstrans->transaction())->security()) {
			updateSecurity(((SecurityTransaction*) oldstrans->transaction())->security());
		}
	} else if(strans->transaction()->type() == TRANSACTION_TYPE_INCOME && ((Income*) strans->transaction())->security()) {
		updateSecurity(((Income*) strans->transaction())->security());
		if(((Income*) strans->transaction())->security() != ((Income*) oldstrans->transaction())->security()) {
			updateSecurity(((Income*) oldstrans->transaction())->security());
		}
	}
}
void Eqonomize::scheduledTransactionRemoved(ScheduledTransaction *strans) {
	scheduledTransactionRemoved(strans, strans);
}
void Eqonomize::scheduledTransactionRemoved(ScheduledTransaction *strans, ScheduledTransaction *old_strans) {
	QTreeWidgetItemIterator it(scheduleView);
	ScheduleListViewItem *i = (ScheduleListViewItem*) *it;
	while(i) {
		if(i->scheduledTransaction() == strans) {
			delete i;
			break;
		}
		++it;
		i = (ScheduleListViewItem*) *it;
	}
	subtractScheduledTransactionValue(old_strans, true);
	emit transactionsModified();
	setModified(true);
	expensesWidget->onScheduledTransactionRemoved(strans);
	incomesWidget->onScheduledTransactionRemoved(strans);
	transfersWidget->onScheduledTransactionRemoved(strans);
	if(strans->transaction()->type() == TRANSACTION_TYPE_SECURITY_BUY || strans->transaction()->type() == TRANSACTION_TYPE_SECURITY_SELL) {
		updateSecurity(((SecurityTransaction*) strans->transaction())->security());
	} else if(strans->transaction()->type() == TRANSACTION_TYPE_INCOME && ((Income*) strans->transaction())->security()) {
		updateSecurity(((Income*) strans->transaction())->security());
	}
}
void Eqonomize::splitTransactionAdded(SplitTransaction *split) {
	blockSignals(true);
	QVector<Transaction*>::size_type c = split->splits.count();
	for(QVector<Transaction*>::size_type i = 0; i < c; i++) {
		transactionAdded(split->splits[i]);
	}
	blockSignals(false);
	emit transactionsModified();
}
void Eqonomize::splitTransactionRemoved(SplitTransaction *split) {
	blockSignals(true);
	QVector<Transaction*>::size_type c = split->splits.count();
	for(QVector<Transaction*>::size_type i = 0; i < c; i++) {
		transactionRemoved(split->splits[i]);
	}
	blockSignals(false);
	emit transactionsModified();
}

#define NEW_ACCOUNT_TREE_WIDGET_ITEM(i, parent, s1, s2, s3, s4) QTreeWidgetItem *i = new QTreeWidgetItem(parent); i->setText(0, s1); i->setText(1, s2); i->setText(2, s3); i->setText(3, s4); i->setTextAlignment(BUDGET_COLUMN, Qt::AlignRight); i->setTextAlignment(CHANGE_COLUMN, Qt::AlignRight); i->setTextAlignment(VALUE_COLUMN, Qt::AlignRight);

void Eqonomize::appendExpensesAccount(ExpensesAccount *account) {
	NEW_ACCOUNT_TREE_WIDGET_ITEM(i, expensesItem, account->name(), "-", QLocale().toString(0.0, 'f', MONETARY_DECIMAL_PLACES), QLocale().toString(0.0, 'f', MONETARY_DECIMAL_PLACES) + " ");
	account_items[i] = account;
	item_accounts[account] = i;
	account_value[account] = 0.0;
	account_change[account] = 0.0;
	expensesItem->sortChildren(0, Qt::AscendingOrder);
}
void Eqonomize::appendIncomesAccount(IncomesAccount *account) {
	NEW_ACCOUNT_TREE_WIDGET_ITEM(i, incomesItem, account->name(), "-", QLocale().toString(0.0, 'f', MONETARY_DECIMAL_PLACES), QLocale().toString(0.0, 'f', MONETARY_DECIMAL_PLACES) + " ");
	account_items[i] = account;
	item_accounts[account] = i;
	account_value[account] = 0.0;
	account_change[account] = 0.0;
	incomesItem->sortChildren(0, Qt::AscendingOrder);
}
void Eqonomize::appendAssetsAccount(AssetsAccount *account) {
	NEW_ACCOUNT_TREE_WIDGET_ITEM(i, assetsItem, account->name(), QString::null, QLocale().toString(0.0, 'f', MONETARY_DECIMAL_PLACES), QLocale().toString(account->initialBalance(), 'f', MONETARY_DECIMAL_PLACES) + " ");
	if(account->isBudgetAccount() && to_date > QDate::currentDate()) i->setText(0, account->name() + "*");
	account_items[i] = account;
	item_accounts[account] = i;
	account_value[account] = account->initialBalance();
	assets_accounts_value += account->initialBalance();
	account_change[account] = 0.0;
	assetsItem->setText(VALUE_COLUMN, QLocale().toString(assets_accounts_value, 'f', MONETARY_DECIMAL_PLACES) + " ");
	assetsItem->sortChildren(0, Qt::AscendingOrder);
}

bool Eqonomize::filterTransaction(Transaction *trans) {
	if(accountsPeriodFromButton->isChecked() && trans->date() < from_date) return true;
	if(trans->date() > to_date) return true;
	return false;
}
void Eqonomize::subtractScheduledTransactionValue(ScheduledTransaction *strans, bool update_value_display) {
	addScheduledTransactionValue(strans, update_value_display, true);
}
void Eqonomize::addScheduledTransactionValue(ScheduledTransaction *strans, bool update_value_display, bool subtract) {
	if(!strans->recurrence()) return addTransactionValue(strans->transaction(), strans->transaction()->date(), update_value_display, subtract, -1, -1, NULL);
	Recurrence *rec = strans->recurrence();
	QDate curdate = rec->firstOccurrence();
	int b_future = 1;
	if(to_date <= QDate::currentDate()) b_future = 0;
	else if(strans->transaction()->date() <= QDate::currentDate()) b_future = -1;
	while(!curdate.isNull() && curdate <= to_date) {
		addTransactionValue(strans->transaction(), curdate, update_value_display, subtract, 1, b_future, NULL);
		curdate = rec->nextOccurrence(curdate);
	}
}
void Eqonomize::subtractTransactionValue(Transaction *trans, bool update_value_display) {
	addTransactionValue(trans, trans->date(), update_value_display, true);
}
void Eqonomize::addTransactionValue(Transaction *trans, const QDate &transdate, bool update_value_display, bool subtract, int n, int b_future, const QDate *monthdate) {
	if(n == 0) return;
	bool b_filter_to = n < 0 && transdate > to_date;
	bool b_from = accountsPeriodFromButton->isChecked();
	bool b_lastmonth = false;
	QDate date;
	if(b_filter_to) {
		if(!monthdate) {			
			if(transdate.year() != to_date.year() || transdate.month() != to_date.month()) return;
		} else {
			if(transdate > *monthdate) return;
		}
		b_lastmonth = true;
	}
	bool b_filter =  !b_lastmonth && b_from && transdate < from_date;
	bool b_curmonth = false;
	bool b_firstmonth = false;	
	if(!b_lastmonth && b_future < 0) {
		if(to_date <= QDate::currentDate()) {
			b_future = 0;
		} else {
			b_future = (transdate > QDate::currentDate());
			if(!b_future) {
				QDate curdate = QDate::currentDate();
				if(curdate.year() == transdate.year() && curdate.month() == transdate.month()) {
					b_curmonth = true;
					b_future = true;
				}
			}
		}
	} else if(!b_lastmonth && b_future) {
		b_curmonth = (transdate <= QDate::currentDate());
	}
	if(b_from && !b_lastmonth && b_filter && !frommonth_begin.isNull()) {
		if(transdate >= frommonth_begin) b_firstmonth = true;
	}
	bool balfrom = false;
	bool balto = false;
	double value = subtract ? -trans->value() : trans->value();
	if(!monthdate) {
		date.setDate(transdate.year(), transdate.month(), transdate.daysInMonth());
		monthdate = &date;
	}
	if(n > 1) value *= n;
	switch(trans->fromAccount()->type()) {
		case ACCOUNT_TYPE_EXPENSES: {
			if(b_lastmonth) {
				account_month_endlast[trans->fromAccount()] -= value;
				account_month[trans->fromAccount()][*monthdate] -= value;
				if(update_value_display) {
					updateMonthlyBudget(trans->fromAccount());
					updateTotalMonthlyExpensesBudget();
				}
				break;
			}
			bool update_month_display = false;
			if(b_firstmonth) {
				account_month_beginfirst[trans->fromAccount()] -= value;
				update_month_display = true;
			}
			if(b_curmonth) {
				account_month_begincur[trans->fromAccount()] -= value;
				update_month_display = true;
			}
			if(b_future || (!frommonth_begin.isNull() && transdate >= frommonth_begin) || (!prevmonth_begin.isNull() && transdate >= prevmonth_begin)) {
				account_month[trans->fromAccount()][*monthdate] -= value;
				update_month_display = true;
			}
			if(update_value_display && update_month_display) {
				updateMonthlyBudget(trans->fromAccount());
				updateTotalMonthlyExpensesBudget();
			}
			account_value[trans->fromAccount()] -= value;
			expenses_accounts_value -= value;
			if(!b_filter) {
				account_change[trans->fromAccount()] -= value;
				expenses_accounts_change -= value;
				if(update_value_display) {
					expensesItem->setText(CHANGE_COLUMN, QLocale().toString(expenses_accounts_change, 'f', MONETARY_DECIMAL_PLACES));
				}
			}
			if(update_value_display) {
				expensesItem->setText(VALUE_COLUMN, QLocale().toString(expenses_accounts_value, 'f', MONETARY_DECIMAL_PLACES) + " ");
			}
			break;
		}
		case ACCOUNT_TYPE_INCOMES: {
			if(b_lastmonth) {
				account_month_endlast[trans->fromAccount()] += value;
				account_month[trans->fromAccount()][*monthdate] += value;
				if(update_value_display) {
					updateMonthlyBudget(trans->fromAccount());
					updateTotalMonthlyIncomesBudget();
				}
				break;
			}
			bool update_month_display = false;
			if(b_firstmonth) {
				account_month_beginfirst[trans->fromAccount()] += value;
				update_month_display = true;
			}
			if(b_curmonth) {
				account_month_begincur[trans->fromAccount()] += value;
				update_month_display = true;
			}
			if(b_future || (!frommonth_begin.isNull() && transdate >= frommonth_begin) || (!prevmonth_begin.isNull() && transdate >= prevmonth_begin)) {
				account_month[trans->fromAccount()][*monthdate] += value;
				update_month_display = true;
			}
			if(update_value_display && update_month_display) {
				updateMonthlyBudget(trans->fromAccount());
				updateTotalMonthlyIncomesBudget();
			}
			account_value[trans->fromAccount()] += value;
			incomes_accounts_value += value;
			if(!b_filter) {
				account_change[trans->fromAccount()] += value;
				incomes_accounts_change += value;
				if(update_value_display) {
					incomesItem->setText(CHANGE_COLUMN, QLocale().toString(incomes_accounts_change, 'f', MONETARY_DECIMAL_PLACES));
				}
			}
			if(update_value_display) {
				incomesItem->setText(VALUE_COLUMN, QLocale().toString(incomes_accounts_value, 'f', MONETARY_DECIMAL_PLACES) + " ");
			}
			break;
		}
		case ACCOUNT_TYPE_ASSETS: {
			if(b_lastmonth) break;
			if(((AssetsAccount*) trans->fromAccount())->accountType() == ASSETS_TYPE_SECURITIES) {
				if(update_value_display) {
					updateSecurityAccount((AssetsAccount*) trans->fromAccount(), false);
					assetsItem->setText(VALUE_COLUMN, QLocale().toString(assets_accounts_value, 'f', MONETARY_DECIMAL_PLACES) + " ");
					assetsItem->setText(CHANGE_COLUMN, QLocale().toString(assets_accounts_change, 'f', MONETARY_DECIMAL_PLACES));
					item_accounts[trans->fromAccount()]->setText(CHANGE_COLUMN, QLocale().toString(account_change[trans->fromAccount()], 'f', MONETARY_DECIMAL_PLACES));
				}
				break;
			}
			balfrom = (trans->fromAccount() == budget->balancingAccount);
			if(!balfrom) {
				account_value[trans->fromAccount()] -= value;
				assets_accounts_value -= value;
				if(!b_filter) {
					account_change[trans->fromAccount()] -= value;
					assets_accounts_change -= value;
					if(update_value_display) {
						assetsItem->setText(CHANGE_COLUMN, QLocale().toString(assets_accounts_change, 'f', MONETARY_DECIMAL_PLACES));
						item_accounts[trans->fromAccount()]->setText(CHANGE_COLUMN, QLocale().toString(account_change[trans->fromAccount()], 'f', MONETARY_DECIMAL_PLACES));
					}
				}
				if(update_value_display) assetsItem->setText(VALUE_COLUMN, QLocale().toString(assets_accounts_value, 'f', MONETARY_DECIMAL_PLACES) + " ");
			}
			break;
		}
	}
	switch(trans->toAccount()->type()) {
		case ACCOUNT_TYPE_EXPENSES: {
			if(b_lastmonth) {
				account_month_endlast[trans->toAccount()] += value;
				account_month[trans->toAccount()][*monthdate] += value;
				if(update_value_display) {
					updateMonthlyBudget(trans->toAccount());
					updateTotalMonthlyExpensesBudget();
				}
				break;
			}
			bool update_month_display = false;
			if(b_firstmonth) {
				account_month_beginfirst[trans->toAccount()] += value;
				update_month_display = true;
			}
			if(b_curmonth) {
				account_month_begincur[trans->toAccount()] += value;
				update_month_display = true;
			}
			if(b_future || (!frommonth_begin.isNull() && transdate >= frommonth_begin) || (!prevmonth_begin.isNull() && transdate >= prevmonth_begin)) {
				account_month[trans->toAccount()][*monthdate] += value;
				update_month_display = true;
			}
			if(update_value_display && update_month_display) {
				updateMonthlyBudget(trans->toAccount());
				updateTotalMonthlyExpensesBudget();
			}
			account_value[trans->toAccount()] += value;
			expenses_accounts_value += value;
			if(!b_filter) {
				account_change[trans->toAccount()] += value;
				expenses_accounts_change += value;
				if(update_value_display) {
					expensesItem->setText(CHANGE_COLUMN, QLocale().toString(expenses_accounts_change, 'f', MONETARY_DECIMAL_PLACES));
				}
			}
			if(update_value_display) {
				expensesItem->setText(VALUE_COLUMN, QLocale().toString(expenses_accounts_value, 'f', MONETARY_DECIMAL_PLACES) + " ");
			}
			break;
		}
		case ACCOUNT_TYPE_INCOMES: {
			if(b_lastmonth) {
				account_month_endlast[trans->toAccount()] -= value;
				account_month[trans->toAccount()][*monthdate] -= value;
				if(update_value_display) {
					updateMonthlyBudget(trans->toAccount());
					updateTotalMonthlyIncomesBudget();
				}
				break;
			}
			bool update_month_display = false;
			if(b_firstmonth) {
				account_month_beginfirst[trans->toAccount()] -= value;
				update_month_display = true;
			}
			if(b_curmonth) {
				account_month_begincur[trans->toAccount()] -= value;
				update_month_display = true;
			}
			if(b_future || (!frommonth_begin.isNull() && transdate >= frommonth_begin) || (!prevmonth_begin.isNull() && transdate >= prevmonth_begin)) {
				account_month[trans->toAccount()][*monthdate] -= value;
				update_month_display = true;
			}
			if(update_value_display && update_month_display) {
				updateMonthlyBudget(trans->toAccount());
				updateTotalMonthlyIncomesBudget();
			}
			account_value[trans->toAccount()] -= value;
			incomes_accounts_value -= value;
			if(!b_filter) {
				account_change[trans->toAccount()] -= value;
				incomes_accounts_change -= value;
				if(update_value_display) {
					incomesItem->setText(CHANGE_COLUMN, QLocale().toString(incomes_accounts_change, 'f', MONETARY_DECIMAL_PLACES));
				}
			}
			if(update_value_display) {
				incomesItem->setText(VALUE_COLUMN, QLocale().toString(incomes_accounts_value, 'f', MONETARY_DECIMAL_PLACES) + " ");
			}
			break;
		}
		case ACCOUNT_TYPE_ASSETS: {
			if(b_lastmonth) break;
			if(((AssetsAccount*) trans->toAccount())->accountType() == ASSETS_TYPE_SECURITIES) {
				if(update_value_display) {
					updateSecurityAccount((AssetsAccount*) trans->toAccount(), false);
					assetsItem->setText(VALUE_COLUMN, QLocale().toString(assets_accounts_value, 'f', MONETARY_DECIMAL_PLACES) + " ");
					assetsItem->setText(CHANGE_COLUMN, QLocale().toString(assets_accounts_change, 'f', MONETARY_DECIMAL_PLACES));
					item_accounts[trans->toAccount()]->setText(CHANGE_COLUMN, QLocale().toString(account_change[trans->toAccount()], 'f', MONETARY_DECIMAL_PLACES));
				}
				break;
			}
			balto = (trans->toAccount() == budget->balancingAccount);
			if(!balto) {
				account_value[trans->toAccount()] += value;
				assets_accounts_value += value;
				if(!b_filter) {
					account_change[trans->toAccount()] += value;
					assets_accounts_change += value;
					if(update_value_display) {
						assetsItem->setText(CHANGE_COLUMN, QLocale().toString(assets_accounts_change, 'f', MONETARY_DECIMAL_PLACES));
						item_accounts[trans->toAccount()]->setText(CHANGE_COLUMN, QLocale().toString(account_change[trans->toAccount()], 'f', MONETARY_DECIMAL_PLACES));
					}
				}
				if(update_value_display) assetsItem->setText(VALUE_COLUMN, QLocale().toString(assets_accounts_value, 'f', MONETARY_DECIMAL_PLACES) + " ");
			}
			break;
		}
	}
	if(update_value_display && !b_lastmonth) {
		if(!balfrom) {
			item_accounts[trans->fromAccount()]->setText(VALUE_COLUMN, QLocale().toString(account_value[trans->fromAccount()], 'f', MONETARY_DECIMAL_PLACES) + " ");
			item_accounts[trans->fromAccount()]->setText(CHANGE_COLUMN, QLocale().toString(account_change[trans->fromAccount()], 'f', MONETARY_DECIMAL_PLACES));
		}
		if(!balto) {
			item_accounts[trans->toAccount()]->setText(VALUE_COLUMN, QLocale().toString(account_value[trans->toAccount()], 'f', MONETARY_DECIMAL_PLACES) + " ");
			item_accounts[trans->toAccount()]->setText(CHANGE_COLUMN, QLocale().toString(account_change[trans->toAccount()], 'f', MONETARY_DECIMAL_PLACES));
		}
	}
}
void Eqonomize::updateTotalMonthlyExpensesBudget() {
	if(budget->expensesAccounts.count() > 0) {
		expenses_budget = 0.0;
		expenses_budget_diff = 0.0;
		ExpensesAccount *eaccount = budget->expensesAccounts.first();
		bool b_budget = false;
		while(eaccount) {
			double d = account_budget[eaccount];
			if(d >= 0.0) {
				expenses_budget += d;
				expenses_budget_diff += account_budget_diff[eaccount];
				b_budget = true;
			}
			eaccount = budget->expensesAccounts.next();
		}
		if(b_budget) {
			expensesItem->setText(BUDGET_COLUMN, i18nc("%1: budget; %2: remaining budget", "%2 of %1", QLocale().toString(expenses_budget, 'f', MONETARY_DECIMAL_PLACES), QLocale().toString(expenses_budget_diff, 'f', MONETARY_DECIMAL_PLACES)));
			return;
		} else {
			expenses_budget = -1.0;
		}
	}
	expensesItem->setText(BUDGET_COLUMN, "-");
}
void Eqonomize::updateTotalMonthlyIncomesBudget() {
	if(budget->incomesAccounts.count() > 0) {
		incomes_budget = 0.0;
		incomes_budget_diff = 0.0;
		IncomesAccount *iaccount = budget->incomesAccounts.first();
		bool b_budget = false;
		while(iaccount) {
			double d = account_budget[iaccount];
			if(d >= 0.0) {
				incomes_budget += d;
				incomes_budget_diff += account_budget_diff[iaccount];
				b_budget = true;
			}
			iaccount = budget->incomesAccounts.next();
		}
		if(b_budget) {
			incomesItem->setText(BUDGET_COLUMN, i18nc("%1: budget; %2: remaining budget", "%2 of %1", QLocale().toString(incomes_budget, 'f', MONETARY_DECIMAL_PLACES), QLocale().toString(incomes_budget_diff, 'f', MONETARY_DECIMAL_PLACES)));
			return;
		} else {
			incomes_budget = -1.0;
		}
	}
	incomesItem->setText(BUDGET_COLUMN, "-");
}
void Eqonomize::updateMonthlyBudget(Account *account) {

	if(account->type() == ACCOUNT_TYPE_ASSETS) return;
	CategoryAccount *ca = (CategoryAccount*) account;
	double mbudget = 0.0;
	QTreeWidgetItem *i = item_accounts[account];
	QMap<QDate, double>::const_iterator it = ca->mbudgets.begin();
	QMap<QDate, double>::const_iterator it_e = ca->mbudgets.end();
	while(it != it_e && it.value() < 0.0) ++it;
	if(it == it_e || it.key() > to_date) {

		i->setText(BUDGET_COLUMN, "-");
		account_budget[account] = -1.0;

	} else {
		
		bool after_from = false;
		QDate monthdate, monthend, curdate = QDate::currentDate(), curmonth, frommonth;

		frommonth = frommonth_begin;
		if(!accountsPeriodFromButton->isChecked() || frommonth <= it.key()) {
			after_from = true;
		} else {
			it = ca->mbudgets.find(frommonth);
			if(it == it_e) {
				QMap<QDate, double>::const_iterator it_b = ca->mbudgets.begin();
				--it;
				while(it != it_b) {
					if(frommonth > it.key()) break;
					--it;
				}
			}
		}

		double diff = 0.0, future_diff = 0.0, future_change_diff = 0.0, m = 0.0, v = 0.0;
		QDate monthlast;
		monthlast.setDate(to_date.year(), to_date.month(), 1);

		QMap<QDate, double>::const_iterator it_n = it;
		++it_n;
		bool has_budget = false;
		bool b_firstmonth = !after_from && (monthdate != frommonth);
		monthdate = frommonth;
		do {
			m = it.value();
			if(m >= 0.0) {
				monthend = monthdate.addDays(monthdate.daysInMonth() - 1);
				has_budget = true;
				bool b_lastmonth = (monthlast == monthdate && to_date != monthend);
				v = account_month[account][monthend];
				if(partial_budget && (b_firstmonth || b_lastmonth)) {
					int day = 1;
					if(b_firstmonth) day = from_date.day();
					int dim = monthend.day();
					int day2 = dim;
					if(b_lastmonth) day2 = to_date.day();
					m = (m * (day2 - day + 1)) / dim;
					if(b_firstmonth) v -= account_month_beginfirst[account];
					if(b_lastmonth) v -= account_month_endlast[account];
				}
				mbudget += m;
				diff += m - v;
				b_firstmonth = false;
			}
			monthdate = monthdate.addMonths(1);
			if(it_n != it_e && monthdate == it_n.key()) {
				it = it_n;
				++it_n;
			}
		} while(monthdate <= monthlast);

		bool b_future = (curdate < to_date);
		curdate = curdate.addDays(1);
		if(b_future) {
			bool after_cur = false;
			it = ca->mbudgets.begin();
			while(it != it_e && it.value() < 0.0) ++it;
			if(curdate < it.key()) {
				curmonth = it.key();
				after_cur = true;
			} else {
				curmonth.setDate(curdate.year(), curdate.month(), 1);
				it = ca->mbudgets.find(curmonth);
				if(it == it_e) {
					QMap<QDate, double>::const_iterator it_b = ca->mbudgets.begin();
					--it;
					while(it != it_b) {
						if(curmonth > it.key()) break;
						--it;
					}
				}
			}
			it_n = it;
			++it_n;
			bool had_from = after_from || from_date <= curdate;
			bool b_curmonth = !after_cur && (curmonth != curdate);
			do {
				m = it.value();
				if(m >= 0.0) {
					monthend = curmonth.addDays(curmonth.daysInMonth() - 1);
					v = account_month[account][monthend];
					bool b_lastmonth = (monthlast == curmonth && to_date != monthend);
					bool b_frommonth = !had_from && frommonth == curmonth;
					if(partial_budget && (b_curmonth || b_lastmonth || b_frommonth)) {
						int day = 1;
						if(b_curmonth) {
							v -= account_month_begincur[account];
							day = curdate.day();
						}
						int dim = monthend.day();
						int day2;
						if(b_lastmonth) {
							v -= account_month_endlast[account];
							day2 = to_date.day();
						} else {
							day2 = dim;
						}
						m = (m * (day2 - day + 1)) / dim;
						if(b_frommonth) {
							int day3 = from_date.day();
							double v3 = b_curmonth ? (account_month_beginfirst[account] - account_month_begincur[account]) : account_month_beginfirst[account];
							double m3 = ((m - v) * (day3 - day + 1)) / (day2 - day + 1);
							if(v3 > m3) m3 = v3;
							double m2 = m - m3;
							double v2 = v - v3;
							if(m2 > v2) future_change_diff += m2 - v2;
						}
					} else if(b_frommonth) {
						int dim = monthend.day();
						int day = dim;
						if(b_lastmonth) {
							v -= account_month_endlast[account];
							int day = to_date.day();
							m = (m * day) / dim;
						}
						int day2 = 1;
						if(b_curmonth) day2 = curdate.day();
						int day3 = from_date.day();
						double v3 = b_curmonth ? (account_month_beginfirst[account] - account_month_begincur[account]) : account_month_beginfirst[account];
						double m3 = ((m - v) * (day3 - day2 + 1)) / (day - day2 + 1);
						if(v3 > m3) m3 = v3;
						double m2 = m - v - m3;
						double v2 = v - account_month_beginfirst[account];
						if(m2 > v2) future_change_diff += m2 - v2;
					} else if(b_lastmonth) {
						v -= account_month_endlast[account];
						int dim = monthend.day();
						int day = to_date.day();
						m = (m * day) / dim;
					}
					if(m > v) {
						future_diff += m - v;
						if(had_from) future_change_diff += m - v;
					}
					if(b_frommonth) had_from = true;
					b_curmonth = false;
				}
				curmonth = curmonth.addMonths(1);
				if(it_n != it_e && curmonth == it_n.key()) {
					it = it_n;
					++it_n;
				}
			} while(curmonth <= monthlast);
		}

		if(has_budget) {
			i->setText(BUDGET_COLUMN, i18nc("%1: budget; %2: remaining budget", "%2 of %1", QLocale().toString(mbudget, 'f', MONETARY_DECIMAL_PLACES), QLocale().toString(diff, 'f', MONETARY_DECIMAL_PLACES)));
			account_budget[account] = mbudget;
			account_budget_diff[account] = diff;
		} else {
			i->setText(BUDGET_COLUMN, "-");
			account_budget[account] = -1.0;
		}
		double future_diff_bak = future_diff, future_diff_change_bak = future_change_diff;
		future_diff -= account_future_diff[account];
		future_change_diff -= account_future_diff_change[account];
		account_future_diff[account] = future_diff_bak;
		account_future_diff_change[account] = future_diff_change_bak;
		if(future_diff == 0.0 && future_change_diff == 0.0) return;
		if(budget->budgetAccount) {
			if(account->type() == ACCOUNT_TYPE_EXPENSES) {
				account_value[budget->budgetAccount] -= future_diff;
				account_change[budget->budgetAccount] -= future_change_diff;
			} else {
				account_value[budget->budgetAccount] += future_diff;
				account_change[budget->budgetAccount] += future_change_diff;
			}
			item_accounts[budget->budgetAccount]->setText(CHANGE_COLUMN, QLocale().toString(account_change[budget->budgetAccount], 'f', MONETARY_DECIMAL_PLACES));
			item_accounts[budget->budgetAccount]->setText(VALUE_COLUMN, QLocale().toString(account_value[budget->budgetAccount], 'f', MONETARY_DECIMAL_PLACES) + " ");
		}
		if(account->type() == ACCOUNT_TYPE_EXPENSES) {
			assets_accounts_value -= future_diff;
			assets_accounts_change -= future_change_diff;
		} else {
			assets_accounts_value += future_diff;
			assets_accounts_change += future_change_diff;
		}
		assetsItem->setText(VALUE_COLUMN, QLocale().toString(assets_accounts_value, 'f', MONETARY_DECIMAL_PLACES) + " ");
		assetsItem->setText(CHANGE_COLUMN, QLocale().toString(assets_accounts_change, 'f', MONETARY_DECIMAL_PLACES));
	}

	if(item_accounts[account] == selectedItem(accountsView)) updateBudgetEdit();

}
void Eqonomize::updateBudgetEdit() {
	QTreeWidgetItem *i = selectedItem(accountsView);
	budgetEdit->blockSignals(true);
	budgetButton->blockSignals(true);
	if(i == NULL || i == assetsItem || i == incomesItem || i == expensesItem || account_items[i]->type() == ACCOUNT_TYPE_ASSETS) {
		budgetEdit->setValue(0.0);
		budgetEdit->setEnabled(false);
		budgetButton->setEnabled(false);
		if(i == incomesItem || i == expensesItem) {
			QDate tomonth, prevmonth_end;
			tomonth.setDate(to_date.year(), to_date.month(), 1);
			prevmonth_end = prevmonth_begin.addDays(prevmonth_begin.daysInMonth() - 1);
			double d_to = 0.0, d_prev = 0.0, v_prev = 0.0;
			CategoryAccount *ca;
			bool b_budget = false, b_budget_prev = false;
			if(i == incomesItem) ca = budget->incomesAccounts.first();
			else ca = budget->expensesAccounts.first();
			while(ca) {
				double d = ca->monthlyBudget(tomonth);
				if(d >= 0.0) {
					d_to += d;
					b_budget = true;
				}
				d = ca->monthlyBudget(prevmonth_begin);
				if(d >= 0.0) {
					d_prev += d;
					b_budget_prev = true;
					v_prev += account_month[ca][prevmonth_end];
				}
				if(i == incomesItem) ca = budget->incomesAccounts.next();
				else ca = budget->expensesAccounts.next();
			}
			if(!b_budget_prev) {
				if(i == incomesItem) ca = budget->incomesAccounts.first();
				else ca = budget->expensesAccounts.first();
				while(ca) {
					v_prev += account_month[ca][prevmonth_end];
					if(i == incomesItem) ca = budget->incomesAccounts.next();
					else ca = budget->expensesAccounts.next();
				}
			}
			if(!b_budget_prev) prevMonthBudgetLabel->setText(i18n("%1 (with no budget)", QLocale().toCurrencyString(v_prev)));
			else prevMonthBudgetLabel->setText(i18n("%1 (with budget %2)", QLocale().toCurrencyString(v_prev), QLocale().toCurrencyString(d_prev)));
			budgetButton->setChecked(b_budget);
			budgetEdit->setValue(d_to);
		} else {
			budgetButton->setChecked(false);
			prevMonthBudgetLabel->setText("-");
		}
	} else {
		CategoryAccount *ca = (CategoryAccount*) account_items[i];
		QDate tomonth;
		tomonth.setDate(to_date.year(), to_date.month(), 1);
		double d = ca->monthlyBudget(tomonth);
		if(d < 0.0) {
			budgetEdit->setValue(0.0);
			budgetButton->setChecked(false);
			budgetEdit->setEnabled(false);
		} else {
			budgetEdit->setValue(d);
			budgetButton->setChecked(true);
			budgetEdit->setEnabled(true);
		}
		budgetButton->setEnabled(true);
		d = ca->monthlyBudget(prevmonth_begin);
		if(d < 0.0) prevMonthBudgetLabel->setText(i18n("%1 (with no budget)", QLocale().toCurrencyString(account_month[ca][prevmonth_begin.addDays(prevmonth_begin.daysInMonth() - 1)])));
		else prevMonthBudgetLabel->setText(i18n("%1 (with budget %2)", QLocale().toCurrencyString(account_month[ca][prevmonth_begin.addDays(prevmonth_begin.daysInMonth() - 1)]), QLocale().toCurrencyString(d)));
 	}
	budgetEdit->blockSignals(false);
	budgetButton->blockSignals(false);
}
void Eqonomize::accountsSelectionChanged() {
	QTreeWidgetItem *i = selectedItem(accountsView);
	if(i == NULL || i == assetsItem || i == incomesItem || i == expensesItem) {
		ActionDeleteAccount->setEnabled(false);
		ActionEditAccount->setEnabled(false);
		ActionBalanceAccount->setEnabled(false);
		budgetEdit->setEnabled(false);
	} else {
		ActionDeleteAccount->setEnabled(true);
		ActionEditAccount->setEnabled(true);
		ActionBalanceAccount->setEnabled(account_items[i]->type() == ACCOUNT_TYPE_ASSETS && ((AssetsAccount*) account_items[i])->accountType() != ASSETS_TYPE_SECURITIES);
	}
	ActionShowAccountTransactions->setEnabled(i != NULL && i != assetsItem);
	updateBudgetEdit();
}
void Eqonomize::updateSecurityAccount(AssetsAccount *account, bool update_display) {
	double value = 0.0, value_from = 0.0;
	bool b_from = accountsPeriodFromButton->isChecked();
	Security *security = budget->securities.first();
	while(security) {
		if(security->account() == account) {
			value += security->value(to_date, true);
			if(b_from) value_from += security->value(from_date, true);
		}
		security = budget->securities.next();
	}
	if(!b_from) {
		value_from = account->initialBalance();
	}
	assets_accounts_value -= account_value[account];
	assets_accounts_value += value;
	assets_accounts_change -= account_change[account];
	assets_accounts_change += (value - value_from);
	account_value[account] = value;
	account_change[account] = value - value_from;
	if(update_display) {
		assetsItem->setText(VALUE_COLUMN, QLocale().toString(assets_accounts_value, 'f', MONETARY_DECIMAL_PLACES) + " ");
		assetsItem->setText(CHANGE_COLUMN, QLocale().toString(assets_accounts_change, 'f', MONETARY_DECIMAL_PLACES));
		item_accounts[account]->setText(CHANGE_COLUMN, QLocale().toString(value - value_from, 'f', MONETARY_DECIMAL_PLACES));
		item_accounts[account]->setText(VALUE_COLUMN, QLocale().toString(value, 'f', MONETARY_DECIMAL_PLACES) + " ");
	}
}
void Eqonomize::filterAccounts() {
	expenses_accounts_value = 0.0;
	expenses_accounts_change = 0.0;
	incomes_accounts_value = 0.0;
	incomes_accounts_change = 0.0;
	assets_accounts_value = 0.0;
	assets_accounts_change = 0.0;
	incomes_budget = 0.0;
	incomes_budget_diff = 0.0;
	expenses_budget = 0.0;
	expenses_budget_diff = 0.0;
	AssetsAccount *aaccount = budget->assetsAccounts.first();
	while(aaccount) {
		if(aaccount->accountType() == ASSETS_TYPE_SECURITIES) {
			account_value[aaccount] = 0.0;
			account_change[aaccount] = 0.0;
			updateSecurityAccount(aaccount, false);
		} else {
			account_value[aaccount] = aaccount->initialBalance();
			account_change[aaccount] = 0.0;
			assets_accounts_value += aaccount->initialBalance();
		}
		aaccount = budget->assetsAccounts.next();
	}	
	Transaction *trans = budget->transactions.first();
	QDate monthdate, monthdate_begin;
	bool b_from = accountsPeriodFromButton->isChecked();
	QDate lastmonth;
	lastmonth.setDate(to_date.year(), to_date.month(), to_date.daysInMonth());
	QDate curdate = QDate::currentDate(), curmonth, curmonth_begin;
	curmonth_begin.setDate(curdate.year(), curdate.month(), 1);
	prevmonth_begin = to_date.addMonths(-1);
	prevmonth_begin = prevmonth_begin.addDays(-(prevmonth_begin.day() - 1));
	curmonth.setDate(curdate.year(), curdate.month(), curdate.daysInMonth());
	frommonth_begin = QDate();
	IncomesAccount *iaccount = budget->incomesAccounts.first();
	while(iaccount) {
		account_value[iaccount] = 0.0;
		account_change[iaccount] = 0.0;
		account_month[iaccount] = QMap<QDate, double>();
		account_month[iaccount][monthdate] = 0.0;
		account_budget[iaccount] = -1.0;
		account_budget_diff[iaccount] = 0.0;
		account_month_beginfirst[iaccount] = 0.0;
		account_month_begincur[iaccount] = 0.0;
		account_month_endlast[iaccount] = 0.0;
		account_future_diff[iaccount] = 0.0;
		account_future_diff_change[iaccount] = 0.0;
		if(!iaccount->mbudgets.isEmpty() && iaccount->mbudgets.begin().value() >= 0.0 && (frommonth_begin.isNull() || iaccount->mbudgets.begin().key() < frommonth_begin)) {
			frommonth_begin = iaccount->mbudgets.begin().key();
		}
		iaccount = budget->incomesAccounts.next();
	}
	ExpensesAccount *eaccount = budget->expensesAccounts.first();
	while(eaccount) {
		account_value[eaccount] = 0.0;
		account_change[eaccount] = 0.0;
		account_month[eaccount] = QMap<QDate, double>();
		account_month[eaccount][monthdate] = 0.0;
		account_budget[eaccount] = -1.0;
		account_budget_diff[eaccount] = 0.0;
		account_month_beginfirst[eaccount] = 0.0;
		account_month_begincur[eaccount] = 0.0;
		account_month_endlast[eaccount] = 0.0;
		account_future_diff[eaccount] = 0.0;
		account_future_diff_change[eaccount] = 0.0;
		if(!eaccount->mbudgets.isEmpty() && eaccount->mbudgets.begin().value() >= 0.0 && (frommonth_begin.isNull() || eaccount->mbudgets.begin().key() < frommonth_begin)) {
			frommonth_begin = eaccount->mbudgets.begin().key();
		}
		eaccount = budget->expensesAccounts.next();
	}
	if(frommonth_begin.isNull() || (b_from && frommonth_begin < from_date)) {
		if(b_from) frommonth_begin.setDate(from_date.year(), from_date.month(), 1);
		else frommonth_begin = curmonth_begin;
	}
	if(frommonth_begin.isNull() || frommonth_begin > prevmonth_begin) {
		monthdate_begin = prevmonth_begin;
	} else {
		monthdate_begin = frommonth_begin;
	}
	if(monthdate_begin > curmonth_begin) monthdate_begin = curmonth_begin;
	monthdate = monthdate_begin.addDays(monthdate_begin.daysInMonth() - 1);
	bool b_future = false;
	bool b_past = (curdate >= to_date);
	if(b_past) {
		footer1->hide();
		assetsItem->setText(0, i18n("Accounts"));
		if(budget->budgetAccount) {
			item_accounts[budget->budgetAccount]->setText(0, budget->budgetAccount->name());
		}
	} else {
		footer1->show();
		assetsItem->setText(0, i18n("Accounts") + "*");
		if(budget->budgetAccount) {
			item_accounts[budget->budgetAccount]->setText(0, budget->budgetAccount->name() + "*");
		}
	}
	while(trans && trans->date() <= lastmonth) {
		if(!b_past && !b_future && trans->date() >= curmonth_begin) b_future = true;
		if(!b_from || b_future || trans->date() >= frommonth_begin || trans->date() >= prevmonth_begin) {
			while(trans->date() > monthdate) {
				monthdate_begin = monthdate_begin.addMonths(1);
				monthdate = monthdate_begin.addDays(monthdate_begin.daysInMonth() - 1);
				iaccount = budget->incomesAccounts.first();
				while(iaccount) {
					account_month[iaccount][monthdate] = 0.0;
					iaccount = budget->incomesAccounts.next();
				}
				eaccount = budget->expensesAccounts.first();
				while(eaccount) {
					account_month[eaccount][monthdate] = 0.0;
					eaccount = budget->expensesAccounts.next();
				}
			}
			addTransactionValue(trans, trans->date(), false, false, -1, b_future, &monthdate);
		} else {
			addTransactionValue(trans, trans->date(), false, false, -1, b_future, NULL);
		}
		trans = budget->transactions.next();
	}
	while(lastmonth >= monthdate) {
		monthdate_begin = monthdate_begin.addMonths(1);
		monthdate = monthdate_begin.addDays(monthdate_begin.daysInMonth() - 1);
		iaccount = budget->incomesAccounts.first();
		while(iaccount) {
			account_month[iaccount][monthdate] = 0.0;
			iaccount = budget->incomesAccounts.next();
		}
		eaccount = budget->expensesAccounts.first();
		while(eaccount) {
			account_month[eaccount][monthdate] = 0.0;
			eaccount = budget->expensesAccounts.next();
		}
	}
	ScheduledTransaction *strans = budget->scheduledTransactions.first();
	while(strans && strans->transaction()->date() <= lastmonth) {
		addScheduledTransactionValue(strans, false, false);
		strans = budget->scheduledTransactions.next();
	}
	for(QMap<QTreeWidgetItem*, Account*>::Iterator it = account_items.begin(); it != account_items.end(); ++it) {
		switch(it.value()->type()) {
			case ACCOUNT_TYPE_INCOMES: {}
			case ACCOUNT_TYPE_EXPENSES: {
				updateMonthlyBudget(it.value());
				break;
			}
			default: {}
		}
	}
	updateTotalMonthlyIncomesBudget();
	updateTotalMonthlyExpensesBudget();
	for(QMap<QTreeWidgetItem*, Account*>::Iterator it = account_items.begin(); it != account_items.end(); ++it) {
		it.key()->setText(CHANGE_COLUMN, QLocale().toString(account_change[it.value()], 'f', MONETARY_DECIMAL_PLACES));
		it.key()->setText(VALUE_COLUMN, QLocale().toString(account_value[it.value()], 'f', MONETARY_DECIMAL_PLACES) + " ");
	}
	incomesItem->setText(VALUE_COLUMN, QLocale().toString(incomes_accounts_value, 'f', MONETARY_DECIMAL_PLACES) + " ");
	incomesItem->setText(CHANGE_COLUMN, QLocale().toString(incomes_accounts_change, 'f', MONETARY_DECIMAL_PLACES));
	expensesItem->setText(VALUE_COLUMN, QLocale().toString(expenses_accounts_value, 'f', MONETARY_DECIMAL_PLACES) + " ");
	expensesItem->setText(CHANGE_COLUMN, QLocale().toString(expenses_accounts_change, 'f', MONETARY_DECIMAL_PLACES));
	assetsItem->setText(VALUE_COLUMN, QLocale().toString(assets_accounts_value, 'f', MONETARY_DECIMAL_PLACES) + " ");
	assetsItem->setText(CHANGE_COLUMN, QLocale().toString(assets_accounts_change, 'f', MONETARY_DECIMAL_PLACES));
	budgetMonthEdit->blockSignals(true);
	budgetMonthEdit->setDate(to_date);
	budgetMonthEdit->blockSignals(false);
	updateBudgetEdit();
}

EqonomizeTreeWidget::EqonomizeTreeWidget(QWidget *parent) : QTreeWidget(parent) {
	connect(this, SIGNAL(doubleClicked(const QModelIndex&)), this, SLOT(onDoubleClick(const QModelIndex&)));
}
EqonomizeTreeWidget::EqonomizeTreeWidget() : QTreeWidget() {}
void EqonomizeTreeWidget::keyPressEvent(QKeyEvent *e) {
	if((e->key() == Qt::Key_Return || e->key() == Qt::Key_Enter) && currentItem()) {
		emit returnPressed(currentItem());
		QTreeWidget::keyPressEvent(e);
		e->accept();
		return;
	}
	QTreeWidget::keyPressEvent(e);
}
void EqonomizeTreeWidget::onDoubleClick(const QModelIndex &mi) {
	QModelIndex mip = mi.parent();
	QTreeWidgetItem *i = NULL;
	if(mip.isValid()) {
		i = topLevelItem(mip.row())->child(mi.row());
	} else {
		i = topLevelItem(mi.row());
	}
	if(i) emit doubleClicked(i, mi.column());
}

QIFWizardPage::QIFWizardPage() : QWizardPage() {
	is_complete = false;
}
bool QIFWizardPage::isComplete() const {return is_complete;}
void QIFWizardPage::setComplete(bool b) {is_complete = b; emit completeChanged();}


#include "eqonomize.moc"
