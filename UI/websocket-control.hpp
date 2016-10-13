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
#include <QObject>
#include <QWidget>

#include <QtWebSockets/QtWebSockets>
#include <QtWebSockets/qwebsocket.h>
#include <QtNetwork/QtNetwork>
#include <QtNetwork/qhostaddress.h>
#include <QtWebSockets/qwebsocketserver.h>

#include <obs-frontend-internal.hpp>
//#include "window-basic-main.hpp"

#ifndef WEBSOCKET_PORT
#define WEBSOCKET_PORT 2900
#endif

#define MSG_TYPE_TRAYCONFIG "TrayConfig"
#define MSG_TYPE_STARTSTREAMING "StartStreaming"
#define MSG_TYPE_STOPSTREAMING "StopStreaming"
#define MSG_TYPE_CLOSE "Close"
#define MSG_TYPE_TOGGLE "Toggle"

struct WebsocketMessage{
	WebsocketMessage();
	WebsocketMessage(QString json_data);

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
	bool CaptureMouse;
};

class OBSBasic;

class WebsocketControl : public QObject {
	Q_OBJECT
public:
	WebsocketControl(obs_frontend_callbacks*, bool start_listening = true);
	void Open();

protected:
	void StartStreaming(WebsocketMessage configs);
	void StopStreaming();
	void Close();
	void SendMessage(QString msg);

private slots:
	void onClientConnected();
	void onMessageReceived(QString str);
	void onClientDisconnected();
	void onTrayConfig(int displayid, bool captureMouse);

public slots:
	void ToggleVisibility();
	void onStreamStarted();
	void onStreamStopped();

private:
	QPointer<QWebSocketServer>	wsServer;
	QPointer<QWebSocket>		wsClient;

	obs_frontend_callbacks *api = nullptr;
	OBSBasic *main_window = nullptr;
};
