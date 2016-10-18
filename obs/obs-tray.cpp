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

#include <QtWidgets/QAction>
#include <QtWidgets/QWidget>
#include <QIcon>
#include <QMenu>
#include <QMessageBox>
#include <QDesktopWidget>
#include <QJsonDocument>
#include <QJsonObject>

#include "obs-tray.hpp"
#include "obs-app.hpp"
#include "window-main.hpp"
#include "window-basic-main.hpp"

#include <iostream>

OBSTray::OBSTray(){
	//cria websocket que recebera comandos do mconf
	wsServer = new QWebSocketServer(QStringLiteral(""),
		QWebSocketServer::NonSecureMode, this);

	// Prevents the application from exiting when there are no windows open
	qApp->setQuitOnLastWindowClosed(false);
}

void OBSTray::open() {
	if (wsServer->listen(QHostAddress("127.0.0.1"), 2900)) {
		connect(wsServer, SIGNAL(newConnection()), this, SLOT(onClientConnected()));
		std::cout << "OBS: Server listening" << std::endl;
	}
}

void OBSTray::onClientConnected(){
	wsClient = wsServer->nextPendingConnection();

	connect(wsClient, SIGNAL(textMessageReceived(QString)), this, SLOT(onMessageReceived(QString)));
	connect(wsClient, SIGNAL(disconnected()), this, SLOT(onClientDisconnected()));
}

void OBSTray::onMessageReceived(QString str){
	Message m(str);

	if (!m.isValid) {
		return;
	}

	if (m.Type == "Toggle")
		ToggleVisibility();

	else if (m.Type == "TrayConfig")
		onTrayConfig(m.DisplayID, m.CaptureMouse);

	else if (m.Type == "StartStreaming")
		SendStartStreamingSignal(m);

	else if (m.Type == "StopStreaming")
		SendStopStreamingSignal();

	else if (m.Type == "Close")
		SendCloseSignal();
}

void OBSTray::onClientDisconnected(){
	SendCloseSignal();
}

void OBSTray::ToggleVisibility(){
	emit signal_toggleVisibility();
}

void OBSTray::onTrayConfig(int displayid, bool captureMouse){
	emit signal_trayConfigChanged(displayid, captureMouse);
}

void OBSTray::SendStartStreamingSignal(Message c){
	if (!c.isValid) return;

	/* sample
{"type": "StartStreaming", "streamPath": "path", "streamName": "name", "displayId": 0,
"bitrate": 1000, "fps": 15, "width":800, "height": 600, "messageid":"4"}
	*/
	QDesktopWidget desktop;

	int width	= desktop.screenGeometry(c.DisplayID).width();
	int height	= desktop.screenGeometry(c.DisplayID).height();

	emit signal_startStreaming(c.StreamName, c.StreamPath,
		width, height, c.Width, c.Height, c.FPS, c.Bitrate);
}

void OBSTray::SendStopStreamingSignal(){
	emit signal_stopStreaming();
}

void OBSTray::SendCloseSignal(){
	SendStopStreamingSignal();

	emit signal_close();
}

void OBSTray::on_signal_StreamStarted() {
	SendMessageToTray("{ \"type\": \"StreamStarted\" }");
}

void OBSTray::on_signal_StreamStopped() {
	SendMessageToTray("{ \"type\": \"StreamStopped\" }");
}

void OBSTray::SendMessageToTray(QString msg) {
	if (!wsClient.isNull() && wsClient->isValid()) {
		wsClient->sendTextMessage(msg);
	}
}

Message::Message() {
	isValid		= true;
	RawData		= "";		MessageID	= 0;

	Type		= "";		StreamPath	= "";
	StreamName	= "";		DisplayID	= 0;
	Width		= 0;		Height		= 0;
	FPS			= 0;		Bitrate		= 0;
	CaptureMouse = false;
}

Message::Message(QString str) : Message() {
	RawData = str;
	ReadFrom(str);
}

void Message::ReadFrom(QString data){
	bool debug = false;

	QJsonDocument d = QJsonDocument::fromJson(data.toUtf8());
	QJsonObject o = d.object();

	if (!o.contains("type")){
		isValid = false;
		return;
	}

	Type = o["type"].toString();

	if (o.contains("messageid")){
		MessageID = std::stoi(o["messageid"].toString().toStdString());
	}

	if (o.contains("debug")){
		debug = o["debug"].toBool();
	}

	if (debug){
		QMessageBox::information(nullptr, "WebsocketMessage",
			data, QMessageBox::StandardButton::Ok);
	}

	if (Type == "StartStreaming"){
		try{
			StreamPath	=	o["streamPath"].toString();
			StreamName	=	o["streamName"].toString();

			DisplayID	=	o["displayId"].toInt();
			Width		=	o["width"].toInt();
			Height		=	o["height"].toInt();
			FPS		=	o["fps"].toInt();
			Bitrate		=	o["bitrate"].toInt();
		}
		catch (std::exception e){
			isValid = false;
		}

	}
	else if (Type == "TrayConfig"){
		if (o.contains("Display"))
			DisplayID = o["Display"].toInt();
		else
			isValid = false;

		if (o.contains("CaptureMouse"))
			CaptureMouse = o["CaptureMouse"].toBool();
		else
			isValid = false;
	}

	if (debug && !isValid){
		QMessageBox::information(nullptr, "WebsocketMessage",
			"Message was invalid", QMessageBox::StandardButton::Ok);
	}
}
