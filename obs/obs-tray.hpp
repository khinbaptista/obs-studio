/******************************************************************************
    Copyright (C) 2013 by Hugh Bailey <obs.jim@gmail.com>

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
******************************************************************************/

#pragma once

#include <QtGui>
#include <QAction>
#include <QComboBox>
#include <QDialog>
#include <QGroupBox>
#include <QSystemTrayIcon>
#include <QWidget>

#include <QtWebSockets\QtWebSockets>
#include <QtWebSockets\qwebsocket.h>
#include <QtNetwork\QtNetwork>
#include <QtNetwork\qhostaddress.h>
#include <QtWebSockets\qwebsocketserver.h>

#include "window-main.hpp"
//#include "window-basic-main.hpp"

#define balloonDuration 1000

#define ptbr QString::fromLatin1
#define STR_INFO ptbr("Informações")
#define STR_TOGGLE ptbr("OBS")
#define STR_CONFIG ptbr("Configurações")
#define STR_EXIT ptbr("Sair")

class Message;

class OBSTray : public QSystemTrayIcon {
	Q_OBJECT
public:
	OBSTray();
	~OBSTray();
	void setVisible(bool visible);

protected:
	void SendStartStreamingSignal(Message configs);
	void SendStopStreamingSignal(bool showBalloon = true);
	void SendCloseSignal();

private slots:
	void setTrayIcon(int index);
	void onClientConnected();
	void onMessageReceived(QString str);
	void onClientDisconnected();
	void onActivated(QSystemTrayIcon::ActivationReason reason);
	void onTrayConfig(int displayid, bool captureMouse);
	
public slots:
	void ShowInfo();
	void ToggleVisibility();
	void ShowConfigWindow();
	void Close();

signals:
	void signal_toggleVisibility();
	void signal_close();
	void signal_startStreaming(QString url, QString path, int width,
		int height, int swidth, int sheight, int fps, int bitrate);
	void signal_stopStreaming();
	void signal_trayConfigInit(int *displayid, bool *captureMouse);
	void signal_trayConfigChanged(int displayid, bool captureMouse);

private:
	void CreateActions();
	void CreateTrayIcon();

	QPointer<QWebSocketServer>	wsServer;
	QPointer<QWebSocket>		wsClient;

	QGroupBox *iconGroupBox;
	QComboBox *iconComboBox;

	QAction *infoAction;
	QAction *toggleVisibilityAction;
	QAction *configAction;
	QAction *stopAction;
	QAction *quitAction;

	QIcon defaultIcon;
	QIcon playingIcon;
	QMenu *trayIconMenu;

	bool isConnected;
	bool isStreaming;
	QString streamURL;
	QString streamPath;
};

class Message{
public:
	Message();
	Message(QString json_data);
	
	bool isValid;
	QString RawData;
	int MessageID;

	QString Type;

	QString StreamPath;
	QString StreamName;
	
	int DisplayID;
	int Width;
	int Height;
	int FPS;
	int Bitrate;

	void ReadFrom(std::string message);
};
