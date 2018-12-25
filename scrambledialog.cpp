/*
	Copyright (C) 2009-2013 jakago

	This file is part of CaptureStream, the flv downloader for NHK radio
	language courses.

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "scrambledialog.h"
#include "ui_scrambledialog.h"

#define OPTIONAL1 "english/basic1"
#define OPTIONAL2 "english/basic2"
#define OPTIONAL3 "english/basic3"
#define OPTIONAL4 "english/enjoy"
#define OPT_TITLE1 "任意講座１"
#define OPT_TITLE2 "任意講座２"
#define OPT_TITLE3 "任意講座３"
#define OPT_TITLE4 "任意講座４"

QString ScrambleDialog::optional1;
QString ScrambleDialog::optional2;
QString ScrambleDialog::optional3;
QString ScrambleDialog::optional4;
QString ScrambleDialog::opt_title1;
QString ScrambleDialog::opt_title2;
QString ScrambleDialog::opt_title3;
QString ScrambleDialog::opt_title4;

//QString optional1 = MainWindow::optional1;
//QString optional2 = MainWindow::optional2;
//QString optional3 = MainWindow::optional3;
//QString optional4 = MainWindow::optional4;
//QString opt_title1 = MainWindow::opt_title1;
//QString opt_title2 = MainWindow::opt_title2;
//QString opt_title3 = MainWindow::opt_title3;
//QString opt_title4 = MainWindow::opt_title4;


ScrambleDialog::ScrambleDialog( QString opt_title1, QString opt_title2, QString opt_title3, QString opt_title4, QString optional1, QString optional2 , QString optional3, QString optional4, QWidget *parent )
		: QDialog(parent), ui(new Ui::ScrambleDialog) {
    ui->setupUi(this);
	ui->opt_title1->setText( opt_title1 ),
	ui->opt_title2->setText( opt_title2 ),
	ui->opt_title3->setText( opt_title3 ),
	ui->opt_title4->setText( opt_title4 ),
	ui->optional1->setText( optional1 ),
	ui->optional2->setText( optional2 ),
	ui->optional3->setText( optional3 ),
	ui->optional4->setText( optional4 );
}

ScrambleDialog::~ScrambleDialog() {
    delete ui;
}

QString ScrambleDialog::scramble() {
	return ui->opt_title1->text();
}

QString ScrambleDialog::scramble2() {
	return ui->opt_title2->text();
}

QString ScrambleDialog::scramble3() {
	return ui->opt_title3->text();
}

QString ScrambleDialog::scramble4() {
	return ui->opt_title4->text();
}

QString ScrambleDialog::scramble5() {
	return ui->optional1->text();
}

QString ScrambleDialog::scramble6() {
	return ui->optional2->text();
}

QString ScrambleDialog::scramble7() {
	return ui->optional3->text();
}

QString ScrambleDialog::scramble8() {
	return ui->optional4->text();
}

