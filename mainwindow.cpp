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

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "downloadthread.h"
#include "customizedialog.h"
#include "scrambledialog.h"
#include "utility.h"
#include "qt4qt5.h"

#include <QMessageBox>
#include <QByteArray>
#include <QXmlQuery>
#include <QStringList>
#include <QProcess>
#include <QCoreApplication>
#include <QFile>
#include <QDir>
#include <QFileInfo>
#include <QThread>
#include <QSettings>
#include <QDesktopWidget>
#include <QInputDialog>
#include <QFileDialog>
#include <QTextStream>

#define SETTING_GROUP "MainWindow"
#define SETTING_GEOMETRY "geometry"
#define SETTING_SAVE_FOLDER "save_folder"
#define SETTING_SCRAMBLE "scramble"
#define SETTING_SCRAMBLE_URL1 "scramble_url1"
#define SETTING_SCRAMBLE_URL2 "scramble_url2"
#define SETTING_OPTIONAL1 "optional1"
#define SETTING_OPTIONAL2 "optional2"
#define SETTING_OPTIONAL3 "optional3"
#define SETTING_OPTIONAL4 "optional4"
#define SETTING_OPT_TITLE1 "opt_title1"
#define SETTING_OPT_TITLE2 "opt_title2"
#define SETTING_OPT_TITLE3 "opt_title3"
#define SETTING_OPT_TITLE4 "opt_title4"
#define SCRAMBLE_URL1 "http://www47.atwiki.jp/jakago/pub/scramble.xml"
#define SCRAMBLE_URL2 "http://cdn47.atwikiimg.com/jakago/pub/scramble.xml"
#define OPTIONAL1 "english/basic1"
#define OPTIONAL2 "english/basic2"
#define OPTIONAL3 "english/basic3"
#define OPTIONAL4 "english/enjoy"
#define OPT_TITLE1 "任意講座１"
#define OPT_TITLE2 "任意講座２"
#define OPT_TITLE3 "任意講座３"
#define OPT_TITLE4 "任意講座４"

#define X11_WINDOW_VERTICAL_INCREMENT 5

#ifdef QT4_QT5_WIN
#define STYLE_SHEET "stylesheet-win.qss"
#else
#ifdef QT4_QT5_MAC
#define STYLE_SHEET "stylesheet-mac.qss"
#else
#define STYLE_SHEET "stylesheet-ubu.qss"
#endif
#endif

namespace {
	bool outputDirSpecified = false;
	QString version() {
		QString result;
		// 日本語ロケールではQDate::fromStringで曜日なしは動作しないのでQRegExpを使う
		// __DATE__の形式： "Jul  8 2011"
		static QRegExp regexp( "([a-zA-Z]{3})\\s+(\\d{1,2})\\s+(\\d{4})" );
		static QStringList months = QStringList()
				<< "Jan" << "Feb" << "Mar" << "Apr" << "May" << "Jun"
				<< "Jul" << "Aug" << "Sep" << "Oct" << "Nov" << "Dec";
		if ( regexp.indexIn( __DATE__ ) != -1 ) {
			int month = months.indexOf( regexp.cap( 1 ) ) + 1;
			int day = regexp.cap( 2 ).toInt();
			result = QString( " (%1/%2/%3)" ).arg( regexp.cap( 3 ) )
					.arg( month, 2, 10, QLatin1Char( '0' ) ).arg( day, 2, 10, QLatin1Char( '0' ) );
		}
		return result;
	}
}

QString MainWindow::outputDir;
QString MainWindow::scramble;
QString MainWindow::scrambleUrl1;
QString MainWindow::scrambleUrl2;
QString MainWindow::optional1;
QString MainWindow::optional2;
QString MainWindow::optional3;
QString MainWindow::optional4;
QString MainWindow::opt_title1;
QString MainWindow::opt_title2;
QString MainWindow::opt_title3;
QString MainWindow::opt_title4;

MainWindow::MainWindow( QWidget *parent )
		: QMainWindow( parent ), ui( new Ui::MainWindowClass ), downloadThread( NULL ) {
	ui->setupUi( this );
	settings( ReadMode );
	this->setWindowTitle( this->windowTitle() + version() );

#ifdef QT4_QT5_MAC		// Macのウィンドウにはメニューが出ないので縦方向に縮める
	setMaximumHeight( maximumHeight() - menuBar()->height() );
	setMinimumHeight( maximumHeight() - menuBar()->height() );
	QRect rect = geometry();
	rect.setHeight( rect.height() - menuBar()->height() );
	rect.moveTop( rect.top() + menuBar()->height() );	// 4.6.3だとこれがないとウィンドウタイトルがメニューバーに隠れる
	setGeometry( rect );
#endif
#ifdef Q_OS_LINUX		// Linuxでは高さが足りなくなるので縦方向に伸ばしておく
	setMaximumHeight( maximumHeight() + X11_WINDOW_VERTICAL_INCREMENT );
	setMinimumHeight( maximumHeight() + X11_WINDOW_VERTICAL_INCREMENT );
	QRect rect = geometry();
	rect.setHeight( rect.height() + X11_WINDOW_VERTICAL_INCREMENT );
	setGeometry( rect );
#endif

#if !defined( QT4_QT5_MAC ) && !defined( QT4_QT5_WIN )
	QPoint bottomLeft = geometry().bottomLeft();
	bottomLeft += QPoint( 0, menuBar()->height() + statusBar()->height() + 3 );
	messagewindow.move( bottomLeft );
#endif

	// 「カスタマイズ」メニューの構築
	customizeMenu = menuBar()->addMenu( QString::fromUtf8( "カスタマイズ" ) );

	QAction* action = new QAction( QString::fromUtf8( "ファイル名設定..." ), this );
	connect( action, SIGNAL( triggered() ), this, SLOT( customizeFileName() ) );
	customizeMenu->addAction( action );

	action = new QAction( QString::fromUtf8( "タイトルタグ設定..." ), this );
	connect( action, SIGNAL( triggered() ), this, SLOT( customizeTitle() ) );
	customizeMenu->addAction( action );

	action = new QAction( QString::fromUtf8( "保存フォルダ..." ), this );
	connect( action, SIGNAL( triggered() ), this, SLOT( customizeSaveFolder() ) );
	customizeMenu->addAction( action );

	action = new QAction( QString::fromUtf8( "任意講座設定" ), this );
	connect( action, SIGNAL( triggered() ), this, SLOT( customizeScramble() ) );
	customizeMenu->addAction( action );

	//action = new QAction( QString::fromUtf8( "スクランブル文字列..." ), this );
	//connect( action, SIGNAL( triggered() ), this, SLOT( customizeScramble() ) );
	//customizeMenu->addAction( action );

	QString styleSheet;
	QFile real( Utility::applicationBundlePath() + STYLE_SHEET );
	if ( real.exists() ) {
		real.open( QFile::ReadOnly );
		styleSheet = QLatin1String( real.readAll() );
	} else {
		QFile res( QString( ":/" ) + STYLE_SHEET );
		res.open( QFile::ReadOnly );
		styleSheet = QLatin1String( res.readAll() );
	}
	qApp->setStyleSheet( styleSheet );
}

MainWindow::~MainWindow() {
	if ( downloadThread ) {
		downloadThread->terminate();
		delete downloadThread;
	}
	if ( !Utility::nogui() )
		settings( WriteMode );
	delete ui;
}

void MainWindow::closeEvent( QCloseEvent *event ) {
	Q_UNUSED( event )
	if ( downloadThread ) {
		messagewindow.appendParagraph( QString::fromUtf8( "ダウンロードをキャンセル中..." ) );
		download();
	}
	messagewindow.close();
	QCoreApplication::exit();
}

void MainWindow::settings( enum ReadWriteMode mode ) {
	typedef struct CheckBox {
		QAbstractButton* checkBox;
		QString key;
		QVariant defaultValue;
		QString titleKey;
		QVariant titleFormat;
		QString fileNameKey;
		QVariant fileNameFormat;
	} CheckBox;
#define DefaultTitle "%k_%Y_%M_%D"
#define DefaultFileName "%k_%Y_%M_%D.m4a"
	CheckBox checkBoxes[] = {
		{ ui->toolButton_basic0, "basic0", false, "basic0_title", DefaultTitle, "basic0_file_name", DefaultFileName },
		{ ui->toolButton_basic1, "basic1", false, "basic1_title", DefaultTitle, "basic1_file_name", DefaultFileName },
		{ ui->toolButton_basic2, "basic2", false, "basic2_title", DefaultTitle, "basic2_file_name", DefaultFileName },
		{ ui->toolButton_basic3, "basic3", false, "basic3_title", DefaultTitle, "basic3_file_name", DefaultFileName },
		{ ui->toolButton_timetrial, "timetrial", false, "timetrial_title", DefaultTitle, "timetrial_file_name", DefaultFileName },
		{ ui->toolButton_kaiwa, "kaiwa", false, "kaiwa_title", DefaultTitle, "kaiwa_file_name", DefaultFileName },
		{ ui->toolButton_business1, "business1", false, "business1_title", DefaultTitle, "business1_file_name", DefaultFileName },
		{ ui->toolButton_business2, "business2", false, "business2_title", DefaultTitle, "business2_file_name", DefaultFileName },
		{ ui->toolButton_chinese, "chinese", false, "chinese_title", DefaultTitle, "chinese_file_name", DefaultFileName },
		{ ui->toolButton_french, "french", false, "french_title", DefaultTitle, "french_file_name", DefaultFileName },
		{ ui->toolButton_french2, "french2", false, "french_title", DefaultTitle, "french_file_name", DefaultFileName },
		{ ui->toolButton_italian, "italian", false, "italian_title", DefaultTitle, "italian_file_name", DefaultFileName },
		{ ui->toolButton_italian2, "italian2", false, "italian_title", DefaultTitle, "italian_file_name", DefaultFileName },
		{ ui->toolButton_hangeul, "hangeul", false, "hangeul_title", DefaultTitle, "hangeul_file_name", DefaultFileName },
		{ ui->toolButton_german, "german", false, "german_title", DefaultTitle, "german_file_name", DefaultFileName },
		{ ui->toolButton_german2, "german2", false, "german_title", DefaultTitle, "german_file_name", DefaultFileName },
		{ ui->toolButton_spanish, "spanish", false, "spanish_title", DefaultTitle, "spanish_file_name", DefaultFileName },
		{ ui->toolButton_spanish2, "spanish2", false, "spanish_title", DefaultTitle, "spanish_file_name", DefaultFileName },
		{ ui->checkBox_13, "charo", false, "charo_title", DefaultTitle, "charo_file_name", DefaultFileName },
		{ ui->checkBox_14, "e-news", false, "e-news_title", DefaultTitle, "e-news_file_name", DefaultFileName },
		{ ui->checkBox_15, "e-news-reread", false, "e-news-reread_title", DefaultTitle, "e-news-reread_file_name", DefaultFileName },
		{ ui->toolButton_levelup_chinese, "levelup-chinese", false, "levelup-chinese_title", DefaultTitle, "levelup-chinese_file_name", DefaultFileName },
		{ ui->toolButton_omotenashino_hangeul, "omotenashino-hangeul", false, "omotenashino-hangeul_title", DefaultTitle, "omotenashino-hangeul_file_name", DefaultFileName },
		{ ui->toolButton_omotenashino_chinese, "omotenashino-chinese", false, "omotenashino-chinese_title", DefaultTitle, "omotenashino-chinese_file_name", DefaultFileName },
		{ ui->toolButton_levelup_hangeul, "levelup-hangeul", false, "levelup-hangeul_title", DefaultTitle, "levelup-hangeul_file_name", DefaultFileName },
		{ ui->toolButton_russian, "russian", false, "russian_title", DefaultTitle, "russian_file_name", DefaultFileName },
		{ ui->toolButton_russian2, "russian2", false, "russian_title", DefaultTitle, "russian_file_name", DefaultFileName },
		{ ui->toolButton_vrradio, "vr-radio", false, "vr-radio_title", DefaultTitle, "vr-radio_file_name", DefaultFileName },
		{ ui->toolButton_optional1, "optional_1", false, "optional1_title", DefaultTitle, "optional1_file_name", DefaultFileName },
		{ ui->toolButton_optional2, "optional_2", false, "optional2_title", DefaultTitle, "optional2_file_name", DefaultFileName },
		{ ui->toolButton_optional3, "optional_3", false, "optional3_title", DefaultTitle, "optional3_file_name", DefaultFileName },
		{ ui->toolButton_optional4, "optional_4", false, "optional4_title", DefaultTitle, "optional4_file_name", DefaultFileName },
		{ ui->toolButton_vrradio1, "vr-radio_all", false,  "", "", "", "" },
		{ ui->toolButton_vrradio2, "vr-radio_year", false,  "", "", "", "" },
		{ ui->checkBox_shower, "shower", false, "shower_title", DefaultTitle, "shower_file_name", DefaultFileName },
		{ ui->toolButton_gendai, "gendai", false, "gendai_title", DefaultTitle, "gendai_file_name", DefaultFileName },
		{ ui->toolButton_gakusyu, "gakusyu", false, "gakusyu_title", DefaultTitle, "gakusyu_file_name", DefaultFileName },
		{ ui->toolButton_enjoy, "enjoy", false, "enjoy_title", DefaultTitle, "enjoy_file_name", DefaultFileName },
		{ ui->toolButton_skip, "skip", true, "", "", "", "" },
		{ ui->checkBox_keep_on_error, "keep_on_error", false, "", "", "", "" },
		{ ui->checkBox_this_week, "this_week", true, "", "", "", "" },
		{ ui->checkBox_next_week, "next_week", false, "", "", "", "" },
		{ ui->checkBox_past_week, "past_week", false, "", "", "", "" },
		{ ui->toolButton_detailed_message, "detailed_message", false, "", "", "", "" },
		{ NULL, NULL, false, "", "", "", "" }
	};
	typedef struct ComboBox {
		QComboBox* comboBox;
		QString key;
		QVariant defaultValue;
	} ComboBox;
	ComboBox comboBoxes[] = {
		{ ui->comboBox_enews, "e-news-index", ENewsSaveBoth },
		{ ui->comboBox_shower, "shower_index", ENewsSaveBoth },
		{ NULL, NULL, false }
	};
	ComboBox textComboBoxes[] = {
		{ ui->comboBox_extension, "audio_extension", "m4a" },
		{ NULL, NULL, false }
	};

	QSettings settings( Utility::applicationBundlePath() + INI_FILE, QSettings::IniFormat );
	settings.beginGroup( SETTING_GROUP );

	if ( mode == ReadMode ) {	// 設定読み込み
		QVariant saved;
#if defined( QT4_QT5_MAC ) || defined( QT4_QT5_WIN )	// X11では正しく憶えられないので位置をリストアしない
		saved = settings.value( SETTING_GEOMETRY );
		if ( saved.type() == QVariant::Invalid )
			move( 70, 22 );
		else {
			// ウィンドウサイズはバージョン毎に変わる可能性があるのでウィンドウ位置だけリストアする
			QSize windowSize = size();
			restoreGeometry( saved.toByteArray() );
			resize( windowSize );
		}
#endif

		saved = settings.value( SETTING_SAVE_FOLDER );
		outputDir = saved.type() == QVariant::Invalid ? Utility::applicationBundlePath() : saved.toString();

		saved = settings.value( SETTING_SCRAMBLE );
		scramble = saved.type() == QVariant::Invalid ? "" : saved.toString();

		saved = settings.value( SETTING_SCRAMBLE_URL1 );
		scrambleUrl1 = saved.type() == QVariant::Invalid ? SCRAMBLE_URL1 : saved.toString();
		saved = settings.value( SETTING_SCRAMBLE_URL2 );
		scrambleUrl2 = saved.type() == QVariant::Invalid ? SCRAMBLE_URL2 : saved.toString();

		saved = settings.value( SETTING_OPTIONAL1 );
		optional1 = saved.type() == QVariant::Invalid ? OPTIONAL1 : saved.toString();
		saved = settings.value( SETTING_OPTIONAL2 );
		optional2 = saved.type() == QVariant::Invalid ? OPTIONAL2 : saved.toString();
		saved = settings.value( SETTING_OPTIONAL3 );
		optional3 = saved.type() == QVariant::Invalid ? OPTIONAL3 : saved.toString();
		saved = settings.value( SETTING_OPTIONAL4 );
		optional4 = saved.type() == QVariant::Invalid ? OPTIONAL4 : saved.toString();

		saved = settings.value( SETTING_OPT_TITLE1 );
		opt_title1 = saved.type() == QVariant::Invalid ? QString::fromUtf8( OPT_TITLE1 ) : saved.toString();
		saved = settings.value( SETTING_OPT_TITLE2 );
		opt_title2 = saved.type() == QVariant::Invalid ? QString::fromUtf8( OPT_TITLE2 ) : saved.toString();
		saved = settings.value( SETTING_OPT_TITLE3 );
		opt_title3 = saved.type() == QVariant::Invalid ? QString::fromUtf8( OPT_TITLE3 ) : saved.toString();
		saved = settings.value( SETTING_OPT_TITLE4 );
		opt_title4 = saved.type() == QVariant::Invalid ? QString::fromUtf8( OPT_TITLE4 ) : saved.toString();

		ui->toolButton_optional1->setText( QString( opt_title1 ) );
		ui->toolButton_optional2->setText( QString( opt_title2 ) );
		ui->toolButton_optional3->setText( QString( opt_title3 ) );
		ui->toolButton_optional4->setText( QString( opt_title4 ) );

		for ( int i = 0; checkBoxes[i].checkBox != NULL; i++ ) {
			checkBoxes[i].checkBox->setChecked( settings.value( checkBoxes[i].key, checkBoxes[i].defaultValue ).toBool() );
		}
		for ( int i = 0; comboBoxes[i].comboBox != NULL; i++ )
			comboBoxes[i].comboBox->setCurrentIndex( settings.value( comboBoxes[i].key, comboBoxes[i].defaultValue ).toInt() );
		for ( int i = 0; textComboBoxes[i].comboBox != NULL; i++ ) {
			QString extension = settings.value( textComboBoxes[i].key, textComboBoxes[i].defaultValue ).toString();
			textComboBoxes[i].comboBox->setCurrentIndex( textComboBoxes[i].comboBox->findText( extension ) );
		}
	} else {	// 設定書き出し
#if defined( QT4_QT5_MAC ) || defined( QT4_QT5_WIN )
		settings.setValue( SETTING_GEOMETRY, saveGeometry() );
#endif
		if ( outputDirSpecified )
			settings.setValue( SETTING_SAVE_FOLDER, outputDir );
		settings.setValue( SETTING_SCRAMBLE, scramble );
		settings.setValue( SETTING_SCRAMBLE_URL1, scrambleUrl1 );
		settings.setValue( SETTING_SCRAMBLE_URL2, scrambleUrl2 );
		settings.setValue( SETTING_OPTIONAL1, optional1 );
		settings.setValue( SETTING_OPTIONAL2, optional2 );
		settings.setValue( SETTING_OPTIONAL3, optional3 );
		settings.setValue( SETTING_OPTIONAL4, optional4 );
		settings.setValue( SETTING_OPT_TITLE1, opt_title1 );
		settings.setValue( SETTING_OPT_TITLE2, opt_title2 );
		settings.setValue( SETTING_OPT_TITLE3, opt_title3 );
		settings.setValue( SETTING_OPT_TITLE4, opt_title4 );

		for ( int i = 0; checkBoxes[i].checkBox != NULL; i++ ) {
			settings.setValue( checkBoxes[i].key, checkBoxes[i].checkBox->isChecked() );
		}
		for ( int i = 0; comboBoxes[i].comboBox != NULL; i++ )
			settings.setValue( comboBoxes[i].key, comboBoxes[i].comboBox->currentIndex() );
		for ( int i = 0; textComboBoxes[i].comboBox != NULL; i++ )
			settings.setValue( textComboBoxes[i].key, textComboBoxes[i].comboBox->currentText() );
	}

	settings.endGroup();
}

void MainWindow::customizeTitle() {
	CustomizeDialog dialog( Ui::TitleMode );
	dialog.exec();
}

void MainWindow::customizeFileName() {
	CustomizeDialog dialog( Ui::FileNameMode );
	dialog.exec();
}

void MainWindow::customizeSaveFolder() {
	QString dir = QFileDialog::getExistingDirectory( 0, QString::fromUtf8( "保存フォルダを指定してください" ),
									   outputDir, QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks );
	if ( dir.length() ) {
		outputDir = dir + QDir::separator();
		outputDirSpecified = true;
	}
}

void MainWindow::customizeScramble() {
	ScrambleDialog dialog( opt_title1, opt_title2, opt_title3, opt_title4, optional1, optional2 , optional3, optional4 );
	dialog.exec();
	opt_title1 = dialog.scramble();
	opt_title2 = dialog.scramble2();
	opt_title3 = dialog.scramble3();
	opt_title4 = dialog.scramble4();
	optional1 = dialog.scramble5();
	optional2 = dialog.scramble6();
	optional3 = dialog.scramble7();
	optional4 = dialog.scramble8();

//	ui->toolButton_optional1->setText( QString( opt_title1 ) );
//	ui->toolButton_optional2->setText( QString( opt_title2 ) );
//	ui->toolButton_optional3->setText( QString( opt_title3 ) );
//	ui->toolButton_optional4->setText( QString( opt_title4 ) );


//	if ( QString::compare( opt_title1, "" ) == true ) opt_title1 = OPT_TITLE1;
//	if ( QString::compare( opt_title2, "" ) == true ) opt_title2 = OPT_TITLE2;
//	if ( QString::compare( opt_title3, "" ) == true ) opt_title3 = OPT_TITLE3;
//	if ( QString::compare( opt_title4, "" ) == true ) opt_title4 = OPT_TITLE4;

//	if ( optional1.isNull() == true ) opt_title1 = OPTIONAL1;
//	if ( optional2.isNull() == true ) opt_title2 = OPTIONAL2;
//	if ( optional3.isNull() == true ) opt_title3 = OPTIONAL3;
//	if ( optional4.isNull() == true ) opt_title4 = OPTIONAL4;
}

void MainWindow::download() {	//「ダウンロード」または「キャンセル」ボタンが押されると呼び出される
	if ( !downloadThread ) {	//ダウンロード実行
		if ( messagewindow.text().length() > 0 )
			messagewindow.appendParagraph( "\n----------------------------------------" );
		ui->downloadButton->setEnabled( false );
		downloadThread = new DownloadThread( ui );
		connect( downloadThread, SIGNAL( finished() ), this, SLOT( finished() ) );
		connect( downloadThread, SIGNAL( critical( QString ) ), &messagewindow, SLOT( appendParagraph( QString ) ), Qt::BlockingQueuedConnection );
		connect( downloadThread, SIGNAL( information( QString ) ), &messagewindow, SLOT( appendParagraph( QString ) ), Qt::BlockingQueuedConnection );
		connect( downloadThread, SIGNAL( current( QString ) ), &messagewindow, SLOT( appendParagraph( QString ) ) );
		connect( downloadThread, SIGNAL( messageWithoutBreak( QString ) ), &messagewindow, SLOT( append( QString ) ) );
		downloadThread->start();
		ui->downloadButton->setText( QString::fromUtf8( "キャンセル" ) );
		ui->downloadButton->setEnabled( true );
	} else {	//キャンセル
		downloadThread->disconnect();	//wait中にSIGNALが発生するとデッドロックするためすべてdisconnect
		finished();
	}
}

void MainWindow::toggled( bool checked ) {
	QObject* sender = this->sender();
	if ( sender ) {
		QToolButton* button = (QToolButton*)sender;
		QString text = button->text();
		if ( checked )
			text.insert( 0, QString::fromUtf8( "✓ " ) );
		else
			text.remove( 0, 2 );
		button->setText( text );
	}
}

void MainWindow::finished() {
	if ( downloadThread ) {
		ui->downloadButton->setEnabled( false );
		if ( downloadThread->isRunning() ) {	//キャンセルでMainWindow::downloadから呼ばれた場合
			downloadThread->cancel();
			downloadThread->wait();
			messagewindow.appendParagraph( QString::fromUtf8( "ダウンロードをキャンセルしました。" ) );
		}
		delete downloadThread;
		downloadThread = NULL;
		ui->downloadButton->setText( QString::fromUtf8( "ダウンロード" ) );
		ui->downloadButton->setEnabled( true );
	}
	//ui->label->setText( "" );
	if ( Utility::nogui() )
		QCoreApplication::exit();
}
