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

#ifndef SCRAMBLEDIALOG_H
#define SCRAMBLEDIALOG_H

#include <QDialog>

namespace Ui {
    class ScrambleDialog;
}

class ScrambleDialog : public QDialog {
    Q_OBJECT

public:
	explicit ScrambleDialog( QString opt_title1, QString opt_title2, QString opt_title3, QString opt_title4, QString optional1, QString optional2 , QString optional3, QString optional4, QWidget *parent = 0 );
    ~ScrambleDialog();
	QString scramble();
	QString scramble2();
	QString scramble3();
	QString scramble4();
	QString scramble5();
	QString scramble6();
	QString scramble7();
	QString scramble8();

	static QString optional1;
	static QString optional2;
	static QString optional3;
	static QString optional4;
	static QString opt_title1;
	static QString opt_title2;
	static QString opt_title3;
	static QString opt_title4;


private:
    Ui::ScrambleDialog *ui;
};

#endif // SCRAMBLEDIALOG_H
